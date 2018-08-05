// ImGui - standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "draw.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <ctime>
#include <chrono>
#include <ratio>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void checkKeys(MyDrawController& mdc, ImGuiIO& io)
{
		const float dt = ImGui::GetIO().DeltaTime;
		const bool haste = io.KeyShift;
		if (io.KeysDown[GLFW_KEY_W])
			mdc.OnKeyW(dt, haste);
		if (io.KeysDown[GLFW_KEY_S])
			mdc.OnKeyS(dt, haste);
		if (io.KeysDown[GLFW_KEY_A])
			mdc.OnKeyA(dt, haste);
		if (io.KeysDown[GLFW_KEY_D])
			mdc.OnKeyD(dt, haste);
		if (io.KeysDown[GLFW_KEY_UP])
			mdc.OnKeyUp(dt);
		if (io.KeysDown[GLFW_KEY_DOWN])
			mdc.OnKeyDown(dt);
		if (io.KeysDown[GLFW_KEY_LEFT])
			mdc.OnKeyLeft(dt);
		if (io.KeysDown[GLFW_KEY_RIGHT])
			mdc.OnKeyRight(dt);
		if (io.KeysDown[GLFW_KEY_SPACE])
			mdc.OnKeySpace(dt);
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
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "eduRen", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

		gl3wInit();
		MyDrawController mdc;

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

    ImGuiIO& io = ImGui::GetIO();

		mdc.Init();
		
    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(115.0f/255.0f, 140.0f/255.0f, 153.0f/255.0f, 1.00f);


		aiLogStream stream;
	//	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
	//	aiAttachLogStream(&stream);

		stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
		aiAttachLogStream(&stream);

		const size_t kFPScnt = 100;
		std::vector<float> fpss;
		fpss.reserve(kFPScnt);

		duration<float> timeSpan;
		bool clamp60FPS = true;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

				//std::cout << "WCK: " << io.WantCaptureKeyboard << std::endl;
				high_resolution_clock::time_point start = high_resolution_clock::now();

        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
				checkKeys(mdc, io);

        static int sFOV = 60;
        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug".
        {
						const glm::vec3& camPos = mdc.GetCam().Position;
            ImGui::Text("Cam dir %.2f %.2f %.2f", camPos[0], camPos[1], camPos[2]);
						
						unsigned int meshN = 0;
						unsigned int lightsN = 0;
						unsigned int materialsN = 0;
						if (mdc.GetScene())
						{
							meshN = mdc.GetScene()->mNumMeshes;
							lightsN = mdc.GetScene()->mNumLights;
							materialsN = mdc.GetScene()->mNumMaterials;
						}
            ImGui::Text("meshes:%d, lights:%d, materials:%d", meshN, lightsN, materialsN);

						ImGui::Checkbox("ambient", &MyDrawController::isAmbient);	ImGui::SameLine(100);
						ImGui::Checkbox("diffuse", &MyDrawController::isDiffuse);	ImGui::SameLine(200);
						ImGui::Checkbox("specular", &MyDrawController::isSpecular);	
						ImGui::Checkbox("wire mode", &MyDrawController::isWireMode);	
						ImGui::SliderInt("fov", &sFOV, 10, 90);

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
        }

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
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
				glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

				glEnable(GL_CULL_FACE); // cull face
				glCullFace(GL_BACK); // cull back face

				mdc.Render(display_w, display_h, sFOV);

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
