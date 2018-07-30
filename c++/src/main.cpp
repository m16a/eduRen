// ImGui - standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <iostream>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include "draw.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void checkKeys(MyDrawController& mdc, ImGuiIO& io)
{
		if (io.KeysDown[GLFW_KEY_W])
			mdc.OnKeyW();
		if (io.KeysDown[GLFW_KEY_S])
			mdc.OnKeyS();
		if (io.KeysDown[GLFW_KEY_A])
			mdc.OnKeyA();
		if (io.KeysDown[GLFW_KEY_D])
			mdc.OnKeyD();
		if (io.KeysDown[GLFW_KEY_UP])
			mdc.OnKeyUp();
		if (io.KeysDown[GLFW_KEY_DOWN])
			mdc.OnKeyDown();
		if (io.KeysDown[GLFW_KEY_LEFT])
			mdc.OnKeyLeft();
		if (io.KeysDown[GLFW_KEY_RIGHT])
			mdc.OnKeyRight();
		if (io.KeysDown[GLFW_KEY_SPACE])
			mdc.OnKeySpace();
}

int main(int, char**)
{
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
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


		aiLogStream stream;
	//	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
	//	aiAttachLogStream(&stream);

		stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
		aiAttachLogStream(&stream);


    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

				//std::cout << "WCK: " << io.WantCaptureKeyboard << std::endl;

        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
				checkKeys(mdc, io);

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug".
        {
            static float f = 0.0f;
						const vmath::vec3& cd = mdc.GetCamDir();
            ImGui::Text("Cam dir %.2f %.2f %.2f", cd[0], cd[1], cd[2]);
						
						unsigned int meshN = 0;
						if (mdc.GetScene())
							meshN = mdc.GetScene()->mNumMeshes;
            ImGui::Text("meshes:  %d", meshN);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
        glClear(GL_COLOR_BUFFER_BIT);

				mdc.Render(display_w, display_h);
        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

		aiDetachAllLogStreams();

    return 0;
}
