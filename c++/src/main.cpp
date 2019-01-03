#include "draw.h"
#include "shader.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <stdio.h>
#include "imgui_impl_glfw_gl3.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>
#include <vector>

static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 800;
static int sWinWidth = WINDOW_WIDTH;
static int sWinHeight = WINDOW_HEIGHT;
static bool sNeedUpdateOffscreenIds = true;

struct SOffscreenRenderIDs {
  GLuint FB{0};  // framebuffer
  GLuint textID{0};
  GLuint rbo{0};  // render buffer object

  GLuint intermediateFB{0};
  GLuint screenTextID{0};
};

static SOffscreenRenderIDs offscreen;

float quadVertices[] = {  // vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

static void error_callback(int error, const char* description) {
  fprintf(stderr, "Error %d: %s\n", error, description);
}

void checkKeys(MyDrawController& mdc, ImGuiIO& io) {
  const float dt = ImGui::GetIO().DeltaTime;
  const bool haste = io.KeyShift;
  if (io.KeysDown[GLFW_KEY_W]) mdc.GetInputHandler().OnKeyW(dt, haste);
  if (io.KeysDown[GLFW_KEY_S]) mdc.GetInputHandler().OnKeyS(dt, haste);
  if (io.KeysDown[GLFW_KEY_A]) mdc.GetInputHandler().OnKeyA(dt, haste);
  if (io.KeysDown[GLFW_KEY_D]) mdc.GetInputHandler().OnKeyD(dt, haste);
  if (io.KeysDown[GLFW_KEY_UP]) mdc.GetInputHandler().OnKeyUp(dt);
  if (io.KeysDown[GLFW_KEY_DOWN]) mdc.GetInputHandler().OnKeyDown(dt);
  if (io.KeysDown[GLFW_KEY_LEFT]) mdc.GetInputHandler().OnKeyLeft(dt);
  if (io.KeysDown[GLFW_KEY_RIGHT]) mdc.GetInputHandler().OnKeyRight(dt);
  if (io.KeysDown[GLFW_KEY_SPACE]) mdc.GetInputHandler().OnKeySpace(dt);
}

static void windowSizeChanged(GLFWwindow* window, int width, int height) {
  sWinWidth = width;
  sWinHeight = height;
  sNeedUpdateOffscreenIds = true;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

void MessageCallback(unsigned int source, unsigned int type, unsigned int id,
                     unsigned int severity, int length, const char* message,
                     void* userParam) {
  char* _source;
  char* _type;
  char* _severity;

  switch (source) {
    case GL_DEBUG_SOURCE_API:
      _source = "API";
      break;

    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      _source = "WINDOW SYSTEM";
      break;

    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      _source = "SHADER COMPILER";
      break;

    case GL_DEBUG_SOURCE_THIRD_PARTY:
      _source = "THIRD PARTY";
      break;

    case GL_DEBUG_SOURCE_APPLICATION:
      _source = "APPLICATION";
      break;

    case GL_DEBUG_SOURCE_OTHER:
      _source = "UNKNOWN";
      break;

    default:
      _source = "UNKNOWN";
      break;
  }

  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      _type = "ERROR";
      break;

    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      _type = "DEPRECATED BEHAVIOR";
      break;

    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      _type = "UDEFINED BEHAVIOR";
      break;

    case GL_DEBUG_TYPE_PORTABILITY:
      _type = "PORTABILITY";
      break;

    case GL_DEBUG_TYPE_PERFORMANCE:
      _type = "PERFORMANCE";
      break;

    case GL_DEBUG_TYPE_OTHER:
      _type = "OTHER";
      break;

    case GL_DEBUG_TYPE_MARKER:
      _type = "MARKER";
      break;

    default:
      _type = "UNKNOWN";
      break;
  }

  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      _severity = "HIGH";
      break;

    case GL_DEBUG_SEVERITY_MEDIUM:
      _severity = "MEDIUM";
      break;

    case GL_DEBUG_SEVERITY_LOW:
      _severity = "LOW";
      break;

    case GL_DEBUG_SEVERITY_NOTIFICATION:
      _severity = "NOTIFICATION";
      return;
      break;

    default:
      _severity = "UNKNOWN";
      break;
  }

  fprintf(stderr, "%d: %s of %s severity, raised from %s: %s\n", id, _type,
          _severity, _source, message);
}
#pragma GCC diagnostic pop

static void UpdateOffscreenRenderIDs(SOffscreenRenderIDs& offscreen, int w,
                                     int h) {
  if (!sNeedUpdateOffscreenIds) return;

  glDeleteFramebuffers(1, &offscreen.FB);
  glGenFramebuffers(1, &offscreen.FB);

  glDeleteTextures(1, &offscreen.textID);
  glGenTextures(1, &offscreen.textID);

  glDeleteRenderbuffers(1, &offscreen.rbo);  // bad :( , can be reused
  glGenRenderbuffers(1, &offscreen.rbo);

  glBindFramebuffer(GL_FRAMEBUFFER, offscreen.FB);
  if (MyDrawController::isMSAA) {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, offscreen.textID);
    if (MyDrawController::HDR)
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F, w, h,
                              GL_TRUE);
    else
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, w, h,
                              GL_TRUE);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D_MULTISAMPLE, offscreen.textID, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, offscreen.rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, w,
                                     h);  // use a single renderbuffer object
                                          // for both a depth AND stencil
                                          // buffer.
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              offscreen.rbo);  // now actually attach it
  } else {
    glBindTexture(GL_TEXTURE_2D, offscreen.textID);
    if (MyDrawController::HDR)
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGB, GL_FLOAT,
                   NULL);
    else
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
                   NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           offscreen.textID, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, offscreen.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w,
                          h);  // use a single renderbuffer object for both a
                               // depth AND stencil buffer.
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER,
                              offscreen.rbo);  // now actually attach it
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // configure second post-processing framebuffer
  glDeleteFramebuffers(1, &offscreen.intermediateFB);
  glGenFramebuffers(1, &offscreen.intermediateFB);
  glBindFramebuffer(GL_FRAMEBUFFER, offscreen.intermediateFB);

  glDeleteTextures(1, &offscreen.screenTextID);
  glGenTextures(1, &offscreen.screenTextID);
  glBindTexture(GL_TEXTURE_2D, offscreen.screenTextID);

  if (MyDrawController::HDR)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         offscreen.screenTextID, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  sNeedUpdateOffscreenIds = false;
}

void DrawUI(MyDrawController& mdc, const std::vector<float>& fpss) {
  Camera& cam = mdc.GetCam();

  static bool oldMSAA = MyDrawController::isMSAA;
  static bool oldHDR = MyDrawController::HDR;

  const glm::vec3& camPos = cam.Position;
  ImGui::Text("Cam pos: %.2f %.2f %.2f", camPos[0], camPos[1], camPos[2]);

  unsigned int meshN = 0;
  unsigned int lightsN = 0;
  unsigned int materialsN = 0;
  unsigned int texturesN = 0;
  if (mdc.GetScene()) {
    meshN = mdc.GetScene()->mNumMeshes;
    lightsN = mdc.GetScene()->mNumLights;
    materialsN = mdc.GetScene()->mNumMaterials;
    texturesN = mdc.GetScene()->mNumTextures;
  }
  ImGui::Text("meshes:%d, lights:%d, materials:%d, embededTextures:%d", meshN,
              lightsN, materialsN, texturesN);

  ImGui::Checkbox("ambient", &MyDrawController::isAmbient);
  ImGui::SameLine(100);
  ImGui::Checkbox("diffuse", &MyDrawController::isDiffuse);
  ImGui::SameLine(200);
  ImGui::Checkbox("specular", &MyDrawController::isSpecular);
  ImGui::Checkbox("wire mode", &MyDrawController::isWireMode);
  ImGui::SameLine(100);
  ImGui::Checkbox("normals", &MyDrawController::drawNormals);
  ImGui::Checkbox("skybox", &MyDrawController::drawSkybox);

  ImGui::Checkbox("MSAA", &MyDrawController::isMSAA);
  if (oldMSAA != MyDrawController::isMSAA) {
    oldMSAA = MyDrawController::isMSAA;
    sNeedUpdateOffscreenIds = true;
  }

  ImGui::Checkbox("gamma correction", &MyDrawController::isGammaCorrection);
  ImGui::SameLine(200);
  ImGui::Checkbox("draw gradient", &MyDrawController::drawGradientReference);
  ImGui::Checkbox("shadows", &MyDrawController::drawShadows);

  if (!MyDrawController::drawShadows) {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  ImGui::Checkbox("debugShadowMaps", &MyDrawController::debugShadowMaps);

  static int item = -1;

  if (MyDrawController::pointLightNames.size()) {
    ImGui::SameLine(150);
    ImGui::PushItemWidth(150);
    const char** arr = nullptr;
    if (!MyDrawController::pointLightNames.empty()) {
      arr = new const char*[MyDrawController::pointLightNames
                                .size()];  // TODO::ugly
      for (int i = 0; i < MyDrawController::pointLightNames.size(); ++i) {
        arr[i] = MyDrawController::pointLightNames[i].c_str();
      }

      ImGui::Combo("point light", &item, arr,
                   MyDrawController::pointLightNames.size());

      if (item > -1) MyDrawController::debugOnmiShadowLightName = arr[item];
    }

    if (arr) delete[] arr;

    ImGui::PopItemWidth();
  }

  if (!MyDrawController::drawShadows) {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }

  {
    ImGui::Checkbox("bump maping", &MyDrawController::bumpMapping);

    if (!MyDrawController::bumpMapping) {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    ImGui::SameLine(150);
    ImGui::PushItemWidth(150);

    const char* types[] = {"Normal", "Height"};

    static int bumpType = 0;
    ImGui::Combo("method", &bumpType, types, 2);

    if (bumpType == 0)
      MyDrawController::bumpMappingType = Normal;
    else if (bumpType == 1)
      MyDrawController::bumpMappingType = Height;
    else
      assert(0);

    ImGui::PopItemWidth();

    if (!MyDrawController::bumpMapping) {
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }

  ImGui::Checkbox("HDR", &MyDrawController::HDR);
  if (!MyDrawController::HDR) {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }
  static float tmp = 1.0f;
  ImGui::SliderFloat("Exposure", &tmp, 0.05, 10.0f);
  MyDrawController::HDR_exposure = MyDrawController::HDR ? tmp : -1.0f;
  if (oldHDR != MyDrawController::HDR) {
    oldHDR = MyDrawController::HDR;
    sNeedUpdateOffscreenIds = true;
  }

  if (!MyDrawController::HDR) {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }

  ImGui::Checkbox("deferred shading", &MyDrawController::deferredShading);
  ImGui::SameLine(200);

  if (!MyDrawController::deferredShading) {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  ImGui::Checkbox("debug GBUffer", &MyDrawController::debugGBuffer);
  if (!MyDrawController::deferredShading) {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }

  ImGui::SliderInt("fov", &cam.FOV, 10, 90);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::Text(
      "Real time %.2f/%.2f/%.2f ms",
      *(std::min_element(fpss.begin(), fpss.end())) * 1000.0f,
      std::accumulate(fpss.begin(), fpss.end(), 0.0f) / fpss.size() * 1000.0f,
      *(std::max_element(fpss.begin(), fpss.end())) * 1000.0f);

  ImGui::PlotLines("Frame ms", fpss.data(), fpss.size(), 0, nullptr, 0.0f,
                   0.010, ImVec2(0, 80));
  ImGui::Checkbox("Clamp 60 FPS", &MyDrawController::clamp60FPS);

  // 2. Show another simple window. In most cases you will use an explicit
  // Begin/End pair to name the window.
  static bool show_test_window = true;
  static bool show_another_window = false;

  if (show_another_window) {
    ImGui::Begin("Another Window", &show_another_window);
    ImGui::Text("Hello from another window!");
    ImGui::End();
  }

  // 3. Show the ImGui test window. Most of the sample code is in
  // ImGui::ShowTestWindow().
  if (show_test_window) {
    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
    ImGui::ShowTestWindow(&show_test_window);
  }
}

inline void ClampFPS(float currFrameMS) {
  if (MyDrawController::clamp60FPS) {
    float secLeft = 1.0f / 60.0f - currFrameMS;
    if (secLeft > 0.0f) {
      struct timespec t, empty;
      t.tv_sec = 0.0f;
      t.tv_nsec = secLeft * 1e9f;
      nanosleep(&t, &empty);
    }
  }
}

inline void Render(MyDrawController& mdc) {
  UpdateOffscreenRenderIDs(offscreen, sWinWidth, sWinHeight);

  glBindFramebuffer(GL_FRAMEBUFFER, offscreen.FB);
  glViewport(0, 0, sWinWidth, sWinHeight);

  auto& c = MyDrawController::clearColor;

  glClearColor(c[0], c[1], c[2], c[3]);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  Camera& cam = mdc.GetCam();
  cam.Width = sWinWidth;
  cam.Height = sWinHeight;

  mdc.Render(cam);

  // draw offscreen to screen
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, offscreen.FB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, offscreen.intermediateFB);
    glBlitFramebuffer(0, 0, sWinWidth, sWinHeight, 0, 0, sWinWidth, sWinHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // now render quad with scene's visuals as its texture image
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    mdc.DrawRect2d(0, 0, sWinWidth, sWinHeight, offscreen.screenTextID,
                   MyDrawController::isGammaCorrection, false,
                   MyDrawController::HDR_exposure);
  }
}

int main(int, char**) {
  using namespace std::chrono;

  // Setup window
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) return 1;

  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window =
      glfwCreateWindow(sWinWidth, sWinHeight, "eduRen", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync
  glfwSetWindowSizeCallback(window, windowSizeChanged);
  gl3wInit();

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(MessageCallback, 0);

  MyDrawController* mdc = new MyDrawController();

  // Setup ImGui binding
  ImGui_ImplGlfwGL3_Init(window, true);
  ImGuiIO& io = ImGui::GetIO();
  mdc->Load();

  aiLogStream stream;
  stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, nullptr);
  aiAttachLogStream(&stream);

  stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "assimp_log.txt");
  aiAttachLogStream(&stream);

  const size_t kFPScnt = 100;
  std::vector<float> fpss;
  fpss.reserve(kFPScnt);

  while (!glfwWindowShouldClose(window)) {
    high_resolution_clock::time_point start = high_resolution_clock::now();

    glfwPollEvents();
    ImGui_ImplGlfwGL3_NewFrame();
    checkKeys(*mdc, io);

    DrawUI(*mdc, fpss);

    Render(*mdc);

    ImGui::Render();
    glfwSwapBuffers(window);

    high_resolution_clock::time_point end = high_resolution_clock::now();
    duration<float> timeSpan = duration_cast<duration<float>>(end - start);

    if (fpss.size() > kFPScnt) fpss.erase(fpss.begin());

    fpss.push_back(timeSpan.count());

    ClampFPS(timeSpan.count());
  }

  delete mdc;

  // Cleanup
  ImGui_ImplGlfwGL3_Shutdown();
  glfwTerminate();

  aiDetachAllLogStreams();
  return 0;
}
