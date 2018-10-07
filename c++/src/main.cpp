#include "draw.h"
#include "shader.h"

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <vector>
#include <ctime>
#include <chrono>
#include <ratio>

static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 800;
static int sWinWidth = WINDOW_WIDTH;
static int sWinHeight = WINDOW_HEIGHT;


struct SOffscreenRenderIDs
{
		GLuint FB {0};//framebuffer
		GLuint textID {0};
    GLuint rbo {0};//render buffer object

		GLuint intermediateFB {0};
		GLuint screenTextID {0};
};

static SOffscreenRenderIDs offscreen;
static bool sNeedUpdateOffscreenIds = true;

float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void checkKeys(MyDrawController& mdc, ImGuiIO& io)
{
		const float dt = ImGui::GetIO().DeltaTime;
		const bool haste = io.KeyShift;
		if (io.KeysDown[GLFW_KEY_W])
			mdc.GetInputHandler().OnKeyW(dt, haste);
		if (io.KeysDown[GLFW_KEY_S])
			mdc.GetInputHandler().OnKeyS(dt, haste);
		if (io.KeysDown[GLFW_KEY_A])
			mdc.GetInputHandler().OnKeyA(dt, haste);
		if (io.KeysDown[GLFW_KEY_D])
			mdc.GetInputHandler().OnKeyD(dt, haste);
		if (io.KeysDown[GLFW_KEY_UP])
			mdc.GetInputHandler().OnKeyUp(dt);
		if (io.KeysDown[GLFW_KEY_DOWN])
			mdc.GetInputHandler().OnKeyDown(dt);
		if (io.KeysDown[GLFW_KEY_LEFT])
			mdc.GetInputHandler().OnKeyLeft(dt);
		if (io.KeysDown[GLFW_KEY_RIGHT])
			mdc.GetInputHandler().OnKeyRight(dt);
		if (io.KeysDown[GLFW_KEY_SPACE])
			mdc.GetInputHandler().OnKeySpace(dt);
}

static void windowSizeChanged(GLFWwindow* window, int width, int height)
{
	sWinWidth = width;
	sWinHeight = height;
	sNeedUpdateOffscreenIds = true;
}

static void UpdateOffscreenRenderIDs(SOffscreenRenderIDs& offscreen, int w, int h)
{
	if (!sNeedUpdateOffscreenIds)
		return;

	glDeleteFramebuffers(1, &offscreen.FB);
	glGenFramebuffers(1, &offscreen.FB);

	glBindFramebuffer(GL_FRAMEBUFFER, offscreen.FB);

	glDeleteTextures(1, &offscreen.textID);
	glDeleteRenderbuffers(1, &offscreen.rbo);//bad :(

	glGenTextures(1, &offscreen.textID);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, offscreen.textID);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, w, h, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, offscreen.textID, 0);

	glGenRenderbuffers(1, &offscreen.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, offscreen.rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, w, h); // use a single renderbuffer object for both a depth AND stencil buffer.
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, offscreen.rbo); // now actually attach it

	// configure second post-processing framebuffer
	glDeleteFramebuffers(1, &offscreen.intermediateFB);
	glGenFramebuffers(1, &offscreen.intermediateFB);

	glBindFramebuffer(GL_FRAMEBUFFER, offscreen.intermediateFB);

	glDeleteTextures(1, &offscreen.screenTextID);

	glGenTextures(1, &offscreen.screenTextID);
	glBindTexture(GL_TEXTURE_2D, offscreen.screenTextID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, offscreen.screenTextID, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	sNeedUpdateOffscreenIds = false;
}

int main(int, char**)
{
		using namespace std::chrono;

    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(sWinWidth, sWinHeight, "eduRen", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
		glfwSetWindowSizeCallback(window, windowSizeChanged);

		gl3wInit();
		MyDrawController mdc;

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);
    ImGuiIO& io = ImGui::GetIO();
		mdc.Load();
		
    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(10.0f/255.0f, 10.0f/255.0f, 10.0f/255.0f, 1.00f);
    //ImVec4 clear_color = ImVec4(115.0f/255.0f, 140.0f/255.0f, 153.0f/255.0f, 1.00f);

		aiLogStream stream;
		stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
		aiAttachLogStream(&stream);

		const size_t kFPScnt = 100;
		std::vector<float> fpss;
		fpss.reserve(kFPScnt);

		duration<float> timeSpan;
		bool clamp60FPS = true;


		CShader offToBackShader("shaders/rect2d.vert", "shaders/rect2d.frag");
		
    // screen quad VAO
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

				high_resolution_clock::time_point start = high_resolution_clock::now();

        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
				checkKeys(mdc, io);


				Camera& cam = mdc.GetCam();

				const glm::vec3& camPos = cam.Position;
				ImGui::Text("Cam pos: %.2f %.2f %.2f", camPos[0], camPos[1], camPos[2]);
				
				unsigned int meshN = 0;
				unsigned int lightsN = 0;
				unsigned int materialsN = 0;
				unsigned int texturesN = 0;
				if (mdc.GetScene())
				{
					meshN = mdc.GetScene()->mNumMeshes;
					lightsN = mdc.GetScene()->mNumLights;
					materialsN = mdc.GetScene()->mNumMaterials;
					texturesN = mdc.GetScene()->mNumTextures;
				}
				ImGui::Text("meshes:%d, lights:%d, materials:%d, embededTextures:%d", meshN, lightsN, materialsN, texturesN);

				ImGui::Checkbox("ambient", &MyDrawController::isAmbient);	ImGui::SameLine(100);
				ImGui::Checkbox("diffuse", &MyDrawController::isDiffuse);	ImGui::SameLine(200);
				ImGui::Checkbox("specular", &MyDrawController::isSpecular);	
				ImGui::Checkbox("wire mode", &MyDrawController::isWireMode); ImGui::SameLine(100);
				ImGui::Checkbox("skybox", &MyDrawController::drawSkybox);	
				ImGui::Checkbox("normals", &MyDrawController::drawNormals);	
				ImGui::Checkbox("MSAA", &MyDrawController::isMSAA);
				ImGui::Checkbox("gamma correction", &MyDrawController::isGammaCorrection);	ImGui::SameLine(200);
				ImGui::Checkbox("draw gradient", &MyDrawController::drawGradientReference);	
				ImGui::Checkbox("draw shadows", &MyDrawController::drawShadows);	

				ImGui::SliderInt("fov", &cam.FOV, 10, 90);

				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::Text("Real time %.2f/%.2f/%.2f ms",
										*(std::min_element(fpss.begin(), fpss.end())) * 1000.0f,
										 std::accumulate(fpss.begin(), fpss.end(), 0.0f) / fpss.size() * 1000.0f,
										*(std::max_element(fpss.begin(), fpss.end())) * 1000.0f);

				if (fpss.size() > kFPScnt)
					fpss.erase(fpss.begin());

				//fpss.push_back(ImGui::GetIO().DeltaTime);
				fpss.push_back(timeSpan.count());

				ImGui::PlotLines("Frame ms", fpss.data(), fpss.size(), 0, nullptr, 0.0f, 0.010, ImVec2(0, 80));
				ImGui::Checkbox("Clamp 60 FPS", &clamp60FPS);	

        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name the window.
        if (show_another_window)
        {
					ImGui::Begin("Another Window", &show_another_window);
					ImGui::Text("Hello from another window!");
					ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow().
        if (show_test_window)
        {
					ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
					ImGui::ShowTestWindow(&show_test_window);
        }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
				UpdateOffscreenRenderIDs(offscreen, display_w, display_h);

				if (MyDrawController::isMSAA)
					glBindFramebuffer(GL_FRAMEBUFFER, offscreen.FB);
				else
					glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, display_w, display_h);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
				glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				cam.Width = sWinWidth;
				cam.Height = sWinHeight;

				if (MyDrawController::isGammaCorrection && !MyDrawController::isMSAA)
					glEnable(GL_FRAMEBUFFER_SRGB);

				mdc.Render(cam);

				//draw offscreen to screen
				if (MyDrawController::isMSAA)
				{
					glBindFramebuffer(GL_READ_FRAMEBUFFER, offscreen.FB);
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, offscreen.intermediateFB);
					glBlitFramebuffer(0, 0, sWinWidth, sWinHeight, 0, 0, sWinWidth, sWinHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

					//now render quad with scene's visuals as its texture image
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT);
					glDisable(GL_DEPTH_TEST);

					mdc.DrawRect2d(0, 0, sWinWidth, sWinHeight, offscreen.screenTextID, MyDrawController::isGammaCorrection, false);
				}

				if (MyDrawController::isGammaCorrection && !MyDrawController::isMSAA)
					glDisable(GL_FRAMEBUFFER_SRGB);

        ImGui::Render();
        glfwSwapBuffers(window);


				high_resolution_clock::time_point end = high_resolution_clock::now();
				timeSpan = duration_cast<duration<float>>(end - start);
				if (clamp60FPS)
				{
					float secLeft = 1.0f / 60 - timeSpan.count();
					if (secLeft > 0)
					{
						struct timespec t, empty;
						t.tv_sec = 0;
						t.tv_nsec = secLeft * 1e9f; 
						nanosleep(&t, &empty);
					}
				}
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

		aiDetachAllLogStreams();

    return 0;
}
