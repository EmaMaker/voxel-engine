#include "debugwindow.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>

namespace debug{
    namespace window{


	void init(GLFWwindow* window){
	    // Setup Dear ImGui context
	    IMGUI_CHECKVERSION();
	    ImGui::CreateContext();
	    ImGuiIO& io = ImGui::GetIO();
	    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	    // Setup Platform/Renderer backends
	    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	    ImGui_ImplOpenGL3_Init();
	}

	void prerender(){
	    // Start the Dear ImGui frame
	    ImGui_ImplOpenGL3_NewFrame();
	    ImGui_ImplGlfw_NewFrame();
	    ImGui::NewFrame();
	    ImGui::ShowDemoWindow(); // Show demo window! :)
	}

	void render(){
	    // (Your code clears your framebuffer, renders your other stuff etc.)
	    ImGui::Render();
	    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	    // (Your code calls glfwSwapBuffers() etc.)
	}

	void destroy(){
	    ImGui_ImplOpenGL3_Shutdown();
	    ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();
	    ImGui_ImplOpenGL3_Shutdown();
	    ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();
	}
    }
}
