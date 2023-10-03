cmake_minimum_required(VERSION 3.2)
project(OpenGLTest)

set(SOURCE_FILES main.cpp controls.cpp chunk.cpp chunkmanager.cpp chunkmesher.cpp chunkgenerator.cpp
	debugwindow.cpp renderer.cpp spacefilling.cpp stb_image.cpp utils.cpp OpenSimplexNoise.cpp)

add_executable(OpenGLTest ${SOURCE_FILES})

target_link_libraries(OpenGLTest glfw tbb glad glm imgui)
install(TARGETS OpenGLTest DESTINATION ${DIVISIBLE_INSTALL_BIN_DIR})
