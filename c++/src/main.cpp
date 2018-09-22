#include "draw.h"

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
static const int WINDOW_HEIGHT = 1280;

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
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "eduRen", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

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

		GLuint offscreenFB;
		glGenFramebuffers(1, &offscreenFB);
		glBindFramebuffer(GL_FRAMEBUFFER, offscreenFB);

		GLuint offscreenRBO;
		glGenRenderbuffers(1, &offscreenRBO);

		glBindFramebuffer(GL_RENDERBUFFER, offscreenRBO);

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
        glViewport(0, 0, display_w, display_h);

				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, display_w, display_h);
				glBindRenderbuffer(GL_RENDERBUFFER, 0);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, offscreenRBO);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
				glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				cam.Width = display_w;
				cam.Height = display_h;
				mdc.Render(cam);

				//copy offscreen to back
				glBindFramebuffer(GL_READ_FRAMEBUFFER, offscreenFB);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				glBlitFramebuffer(0, 0, display_w, display_h, 0, 0, display_w, display_h, GL_COLOR_BUFFER_BIT, GL_NEAREST); 

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
