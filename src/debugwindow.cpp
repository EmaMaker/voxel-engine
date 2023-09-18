#include "debugwindow.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui_stdlib.h>

#include <iostream>
#include <string>
#include <unordered_map>

namespace debug{
    namespace window{

	void show_debug_window();
	constexpr int frametimes_array_size = 20;
	float frametimes_array[frametimes_array_size]{};

	std::unordered_map<std::string, std::any> parameters;

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
	    //ImGui::ShowDemoWindow(); // Show demo window! :)
	    show_debug_window();
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

	void set_parameter(std::string key, std::any value){
	    parameters[key] = value;
	}

	int ch_type{0};
	int block_type{0};
	void show_debug_window(){
	    ImGui::Begin("Debug Window");

	    ImGui::PushItemWidth(ImGui::GetFontSize() * -12);

	    try{
		if (ImGui::CollapsingHeader("Frametimes")){
		    ImGui::Text("FPS: %d", std::any_cast<int>(parameters.at("fps")));
		    ImGui::Text("Frametime (ms): %f",
			    std::any_cast<float>(parameters.at("frametime"))*1000);
		    //ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr);
		}

		if(ImGui::CollapsingHeader("Player")){
		    ImGui::Text("X: %f, Y: %f, Z: %f",
			    std::any_cast<float>(parameters.at("px")),std::any_cast<float>(parameters.at("py")),std::any_cast<float>(parameters.at("pz"))  );
		    ImGui::Text("X: %d, Y: %d, Z: %d (chunk)", std::any_cast<int>(parameters.at("cx")),std::any_cast<int>(parameters.at("cy")),std::any_cast<int>(parameters.at("cz"))  );
		    ImGui::Text("Pointing in direction: %f, %f, %f", 
			    std::any_cast<float>(parameters.at("lx")),std::any_cast<float>(parameters.at("ly")),std::any_cast<float>(parameters.at("lz"))  );

		    if(parameters.find("block_last_action") != parameters.end()){
			ImGui::Text("Last Block action: %s",
			    std::any_cast<bool>(parameters.at("block_last_action")) ? "place" : "destroy");
			ImGui::Text("Last Block action block type: %d",
			    std::any_cast<int>(parameters.at("block_last_action_block_type")));
			ImGui::Text("Last Block action position: X: %d, Y: %d, Z: %d",
			    std::any_cast<int>(parameters.at("block_last_action_x")),std::any_cast<int>(parameters.at("block_last_action_y")),std::any_cast<int>(parameters.at("block_last_action_z"))  );
		    }

		    ImGui::SliderInt("Crosshair type",
			    &ch_type, 0, 1);
		    ImGui::SliderInt("Block to place",
			    &block_type, 2, 6);
		}

		if(ImGui::CollapsingHeader("Mesh")){
		    ImGui::Text("Total chunks updated: %d",
			    std::any_cast<int>(parameters.at("render_chunks_total")));
		    ImGui::Text("Chunks rendered: %d",
			std::any_cast<int>(parameters.at("render_chunks_rendered")));
		    ImGui::Text("Frustum culled: %d",
			std::any_cast<int>(parameters.at("render_chunks_culled")));
		    ImGui::Text("Chunks out of view: %d",
			std::any_cast<int>(parameters.at("render_chunks_oof")));
		    if(parameters.find("render_chunks_deleted") != parameters.end())
			ImGui::Text("Chunks deleted: %d",
			    std::any_cast<int>(parameters.at("render_chunks_deleted")));
		    ImGui::Text("Vertices in the scene: %d",
			std::any_cast<int>(parameters.at("render_chunks_vertices")));
		}

		if(ImGui::CollapsingHeader("Chunks")){
		    ImGui::Text("Total chunks present: %d",
			std::any_cast<int>(parameters.at("update_chunks_total")));
		    /*ImGui::Text("Chunks freed from memory: %d",
			std::any_cast<int>(parameters.at("update_chunks_delete")));*/
		    ImGui::Text("Bucket size: %d",
			std::any_cast<int>(parameters.at("update_chunks_bucket")));
		}
	    }catch(const std::bad_any_cast& e){
		std::cout << e.what();
	    }

	    ImGui::End();
	}
    }
}
