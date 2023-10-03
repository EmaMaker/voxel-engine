#include "renderer.hpp"

#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <oneapi/tbb/concurrent_hash_map.h>

#include "chunkmanager.hpp"
#include "chunkmesher.hpp"
#include "debugwindow.hpp"
#include "globals.hpp"
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace renderer{
    typedef oneapi::tbb::concurrent_hash_map<int32_t, RenderInfo*> RenderTable;

    RenderTable ChunksToRender;
    ChunkMeshDataQueue MeshDataQueue;

    Shader* theShader, *quadShader;
    GLuint chunkTexture;

    GLuint renderTexFrameBuffer, renderTex, renderTexDepthBuffer, quadVAO, quadVBO;
    int screenWidth, screenHeight;

    int crosshair_type{0};
    bool wireframe{false};

    Shader* getRenderShader() { return theShader; }
    ChunkMeshDataQueue& getMeshDataQueue(){ return MeshDataQueue; }

    void init(GLFWwindow* window){
	// Setup rendering
	// We will render the image to a texture, then display the texture on a quad that fills the
	// entire screen.
	// This makes it easy to capture screenshots or apply filters to the final image (e.g.
	// over-impress HUD elements like a crosshair)
	glfwGetWindowSize(window, &screenWidth, &screenHeight);

	glGenFramebuffers(1, &renderTexFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTexFrameBuffer);

	// Depth buffer
	glGenRenderbuffers(1, &renderTexDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderTexDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight); //Support up to
										//full-hd for now
	// Attach it to the frame buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
		renderTexDepthBuffer);
	// Create texture to render to
	// The texture we're going to render to
	glGenTextures(1, &renderTex);
	glBindTexture(GL_TEXTURE_2D, renderTex);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, screenWidth, screenHeight, 0,GL_RGB, GL_UNSIGNED_BYTE, 0); // Support
											 // up to
											 // full-hd
											 // for now
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// Set the texture as a render attachment for the framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

	// Create the quad to render the texture to
	float vertices[] = {
	    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	    1.0f, 1.0f, 0.0f, 1.0f, 1.0f
	};
	glGenBuffers(1, &quadVBO);
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// Rendering of the world
	// Create Shader
	theShader = new Shader{"shaders/shader-texture.gs", "shaders/shader-texture.vs", "shaders/shader-texture.fs"};
	quadShader = new Shader{nullptr, "shaders/shader-quad.vs", "shaders/shader-quad.fs"};

	// Block textures
	// Create 3d array texture
	constexpr int layerCount = 5;
	glGenTextures(1, &chunkTexture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, chunkTexture);
	
	int width, height, nrChannels;
	unsigned char *texels = stbi_load("textures/cobblestone.png", &width, &height, &nrChannels, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels);
	unsigned char *texels1 = stbi_load("textures/dirt.png", &width, &height, &nrChannels, 0);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels1);
	unsigned char *texels2 = stbi_load("textures/grass_top.png", &width, &height, &nrChannels, 0);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 2, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, texels2);
	unsigned char *texels3 = stbi_load("textures/wood.png", &width, &height, &nrChannels, 0);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 3, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels3);
	unsigned char *texels4 = stbi_load("textures/leaves.png", &width, &height, &nrChannels, 0);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 4, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texels4);

	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);

	debug::window::set_parameter("crosshair_type_return", &crosshair_type);
	debug::window::set_parameter("wireframe_return", &wireframe);
    }


    void render(){
	// Bind the frame buffer to render to the texture
	glBindFramebuffer(GL_FRAMEBUFFER, renderTexFrameBuffer);
	glViewport(0, 0, screenWidth, screenHeight);
	glEnable(GL_DEPTH_TEST);
	if(wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Clear the screen
        glClearColor(0.431f, 0.694f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* UPDATE IMGUI */
	debug::window::prerender();

	/* RENDER THE WORLD TO TEXTURE */
	int total{0}, toGpu{0}, oof{0}, vertices{0};
	glm::vec4 frustumPlanes[6];
	theCamera.getFrustumPlanes(frustumPlanes, true);
	glm::vec3 cameraPos = theCamera.getPos();	
	glm::vec3 cameraChunkPos = cameraPos / static_cast<float>(CHUNK_SIZE);

	theShader->use();
	theShader->setVec3("viewPos", cameraPos);

	// TODO: works but some stuff is rendered wrong (trees floating or inside the terrain,
	// missing or malformed chunks)
	ChunkMeshData* m;
	while(MeshDataQueue.try_pop(m)){
	    RenderTable::accessor a;
	    RenderInfo* render_info;

	    if(ChunksToRender.find(a, m->index)){
		render_info = a->second;
		render_info->position = m->position;
		render_info->num_vertices = m->num_vertices;
		std::cout << "index collision on " << render_info->index << std::endl;
	    }else{
		render_info = new RenderInfo();
		render_info->index = m->index;
		render_info->position = m->position;
		render_info->num_vertices = m->num_vertices;

		ChunksToRender.emplace(a, std::make_pair(render_info->index, render_info));
	    }

	    send_chunk_to_gpu(m, render_info);
	    chunkmesher::getMeshDataQueue().push(m);
	}

	// TODO: implement removal of chunks from rendering
	std::cout << "chunks to render: " << ChunksToRender.size();


	// Render the chunks
	// parallel_for cannot be used since all the rendering needs to happen in a single thread
	for(RenderTable::iterator i = ChunksToRender.begin(); i != ChunksToRender.end(); i++){
	    RenderInfo* render_info = i->second;
	    
	    total++;
	    if(render_info->num_vertices > 0)
	    {

		// Increase total vertex count
		vertices += render_info->num_vertices;

		// Perform frustum culling and eventually render
		glm::vec3 chunk = render_info->position;
		glm::vec4 chunkW = glm::vec4(chunk.x*static_cast<float>(CHUNK_SIZE), chunk.y*static_cast<float>(CHUNK_SIZE), chunk.z*static_cast<float>(CHUNK_SIZE),1.0);
		glm::mat4 model = glm::translate(glm::mat4(1.0), ((float)CHUNK_SIZE) * chunk);

		// Check if all the corners of the chunk are outside any of the planes
		// TODO (?) implement frustum culling as per (Inigo Quilez)[https://iquilezles.org/articles/frustumcorrect/], and check each
		// plane against each corner of the chunk 
		bool out=false;
		int a{0};
		for(int p = 0; p < 6; p++){
		    a = 0;
		    for(int i = 0; i < 8; i++)  a += glm::dot(frustumPlanes[p], glm::vec4(chunkW.x + ((float)(i & 1))*CHUNK_SIZE, chunkW.y
				    + ((float)((i & 2) >> 1))*CHUNK_SIZE, chunkW.z + ((float)((i & 4) >> 2))*CHUNK_SIZE, 1.0)) < 0.0;

		    if(a==8){
			out=true;
			break;
		    }
		}

		if (!out)
		{
		    theShader->setMat4("model", model);
		    theShader->setMat4("view", theCamera.getView());
		    theShader->setMat4("projection", theCamera.getProjection());

		    glBindVertexArray(render_info->VAO);
		    glDrawArrays(GL_POINTS, 0, render_info->num_vertices);
		    glBindVertexArray(0);

		    toGpu++;
		}
	    }
	}

	debug::window::set_parameter("render_chunks_total", total);
	debug::window::set_parameter("render_chunks_rendered", toGpu);
	debug::window::set_parameter("render_chunks_culled", total-toGpu);
	debug::window::set_parameter("render_chunks_vertices", vertices);

	/* DISPLAY TEXTURE ON A QUAD THAT FILLS THE SCREEN */
	// Now to render the quad, with the texture on top
	// Switch to the default frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.431f, 0.694f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindVertexArray(quadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, renderTex);
	quadShader->use();
	quadShader->setInt("screenWidth", screenWidth);
	quadShader->setInt("screenHeight", screenHeight);
	quadShader->setInt("crosshairType", crosshair_type);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	debug::window::render();
    }

    void send_chunk_to_gpu(ChunkMeshData* mesh_data, RenderInfo* render_info)
    {
	if (render_info->num_vertices > 0)
	{
	    if(!render_info->buffers_allocated) render_info->allocateBuffers();

	    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	    glBindVertexArray(render_info->VAO);

	    // TODO: change GL_STATIC_DRAW to the one that means "few redraws and further in between"

	    // position attribute
	    glBindBuffer(GL_ARRAY_BUFFER, render_info->VBO);
	    glBufferData(GL_ARRAY_BUFFER, mesh_data->vertices.size() * sizeof(GLfloat), &(mesh_data->vertices[0]), GL_STATIC_DRAW);
	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	    glEnableVertexAttribArray(0);

	    // normal attribute
	    glBindBuffer(GL_ARRAY_BUFFER, render_info->extentsBuffer);
	    glBufferData(GL_ARRAY_BUFFER, mesh_data->extents.size() * sizeof(GLfloat), &(mesh_data->extents[0]), GL_STATIC_DRAW);
	    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)(0));
	    glEnableVertexAttribArray(1);

	    // texcoords attribute
	    glBindBuffer(GL_ARRAY_BUFFER, render_info->texinfoBuffer);
	    glBufferData(GL_ARRAY_BUFFER, mesh_data->texinfo.size() * sizeof(GLfloat), &(mesh_data->texinfo[0]), GL_STATIC_DRAW);
	    glEnableVertexAttribArray(2);
	    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

	    glBindVertexArray(0);
	}
    }

    void framebuffer_size_callback(GLFWwindow *window, int width, int height){
	resize_framebuffer(width, height);
    }

    void resize_framebuffer(int width, int height){
	screenWidth = width;
	screenHeight = height;

	theCamera.viewPortCallBack(nullptr, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, renderTexFrameBuffer);
	glBindTexture(GL_TEXTURE_2D, renderTex);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0); // Support
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, renderTexDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height); //Support up to
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
		renderTexDepthBuffer);
    }

    void saveScreenshot(bool forceFullHD){
	int old_screenWidth = screenWidth;
	int old_screenHeight = screenHeight;

	if(forceFullHD){
	    resize_framebuffer(1920, 1080);
	    // Do a render pass
	    render();
	}

	// Bind the render frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, renderTexFrameBuffer);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	// Save the framebuffer in a byte array
	GLubyte data[screenWidth*screenHeight*3];
	glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, data);
	// Save the byte array onto a texture
	stbi_flip_vertically_on_write(1);
	stbi_write_png(forceFullHD ? "screenshot_fullhd.png" : "screenshot.png", screenWidth,
		screenHeight, 3, data, screenWidth*3);

	if(forceFullHD) resize_framebuffer(old_screenWidth, old_screenHeight);
    }

    void destroy(){
	delete theShader;
	delete quadShader;
    }


};
