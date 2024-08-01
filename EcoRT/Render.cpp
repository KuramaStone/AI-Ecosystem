#include "Render.h"


Render::Render(b2Vec2 worldSize) {
	world_size = worldSize;
}

void Render::init() {
	currentWorldViewCenter.Set(0, 0);
	nextWorldViewCenter.Set(0, 0);
	currentViewScale = 0.000001f;
	nextViewScale = 1.0f; 

	cameraHeight = world_size.x;

	lightPosition = glm::vec3(1, 1, 1);
	lightDirection = glm::normalize(lightPosition);
	buildScreen();
	initText();

	loadShaders();

	brainRenderer.build();
	// pass text data over so that it can render text as well
	brainRenderer.textShader = textShader;
	brainRenderer.textVAO = textVAO;
	brainRenderer.textVBO = textVBO;

	treeRenderer.build();
	treeRenderer.textShader = textShader;
	treeRenderer.textVAO = textVAO;
	treeRenderer.textVBO = textVBO;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);

	glDisable(GL_CULL_FACE);

	glfwSwapInterval(0);  // Toggle VSync

	int frameWidth, frameHeight;
	glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
	viewportSize.x = frameWidth;
	viewportSize.y = frameHeight;

}

void Render::initModelShadows(Border& border) {
	// create buffer with mesh data
	std::vector<float>& vertices = border.caveWalls->triangles;
	std::vector<unsigned int>& indices = border.caveWalls->indices;

	//std::vector<float> vertices {
	//	// Vertex positions
	//	-0.5f, -0.5f, -0.5f, // 0
	//	 0.5f, -0.5f, -0.5f, // 1
	//	 0.5f,  0.5f, -0.5f, // 2
	//	-0.5f,  0.5f, -0.5f, // 3
	//	-0.5f, -0.5f,  0.5f, // 4
	//	 0.5f, -0.5f,  0.5f, // 5
	//	 0.5f,  0.5f,  0.5f, // 6
	//	-0.5f,  0.5f,  0.5f  // 7
	//};
	//std::vector<int> indices {
	//	// Indices for front face
	//	0, 1, 2, // Triangle 1
	//	2, 3, 0, // Triangle 2

	//	// Indices for back face
	//	4, 5, 6, // Triangle 3
	//	6, 7, 4, // Triangle 4

	//	// Indices for left face
	//	0, 3, 7, // Triangle 5
	//	7, 4, 0, // Triangle 6

	//	// Indices for right face
	//	1, 5, 6, // Triangle 7
	//	6, 2, 1, // Triangle 8

	//	// Indices for top face
	//	3, 2, 6, // Triangle 9
	//	6, 7, 3, // Triangle 10

	//	// Indices for bottom face
	//	0, 1, 5, // Triangle 11
	//	5, 4, 0  // Triangle 12
	//};


	glGenTextures(1, &depthMapTexture);
	glGenTextures(1, &shadowMapTexture);

	glGenVertexArrays(1, &caveMeshVAO); 
	glBindVertexArray(caveMeshVAO);

	// Create and bind vertex buffer object (VBO)
	glGenBuffers(1, &caveMeshVBO);
	glBindBuffer(GL_ARRAY_BUFFER, caveMeshVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Create and bind element buffer object (EBO)
	glGenBuffers(1, &caveMeshEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, caveMeshEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

	// Set vertex attribute pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);


	// Unbind VAO, VBO, and EBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// prebake the shadow map to use for rendering terrain
	glGenFramebuffers(1, &depthMapFBO);
	glGenFramebuffers(1, &shadowMapFBO);

	int swidth = int(border.size.x * border.caveWalls->shadowMapScale);
	int sheight = int(border.size.y * border.caveWalls->shadowMapScale);

	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, swidth, sheight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer incomplete. Status: " << status << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//

	glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, swidth, sheight, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTexture, 0);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer incomplete. Status: " << status << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	checkError(1234);

	// empty to save memory as they are now in gpu
	vertices.clear();
	indices.clear();
}

void Render::bakeModelShadows(Border& border) {
	// doesnt update mesh data, just shader values atm
	std::vector<float>& vertices = border.caveWalls->triangles;
	std::vector<unsigned int>& indices = border.caveWalls->indices;

	int swidth = int(border.size.x * border.caveWalls->shadowMapScale);
	int sheight = int(border.size.y * border.caveWalls->shadowMapScale);


	glViewport(0, 0, swidth, sheight);


	float cornerToCorner = 2 * glm::length(glm::vec3(border.size.x, border.size.y, border.caveWalls->renderHeightScale));
	glm::vec3 modLightPosition = glm::vec3(lightPosition.x * border.size.x / 2, lightPosition.y * border.size.y / 2, lightPosition.z * border.caveWalls->renderHeightScale);
	glm::vec3 lightDir = glm::normalize(modLightPosition);
	glm::vec3 lightLookAt(0.0f, 0.0f, 0.0f);
	float near_plane = -cornerToCorner * 0.1;
	float far_plane = cornerToCorner;

	float higherSide = std::max(world_size.x, world_size.y);
	glm::mat4 lightIdentity = glm::mat4(1.0f);
	lightIdentity = glm::translate(lightIdentity, glm::vec3(0, 0, 0.0f));
	lightIdentity = glm::scale(lightIdentity, glm::vec3(-2.15, -2.15, 2.15));
	glm::mat4 lightProjection = glm::ortho(-higherSide, higherSide, -higherSide, higherSide, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(modLightPosition, lightLookAt, glm::vec3(0.0f, 0.0f, -1.0f));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView * lightIdentity;

	// render shadows from light perspective
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glUseProgram(shadowShader);
	glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
	glBindVertexArray(caveMeshVAO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDrawElements(GL_TRIANGLES, border.caveWalls->indicesCount, GL_UNSIGNED_INT, 0);
	glDisable(GL_CULL_FACE);

	checkError(1234);

	glm::mat4 mvp = calculateMVPMatrix(0, 0, 1, false);

	// redraw into texture from world perspective
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, border.caveWalls->noiseTexture);
	glUseProgram(shadowPerShader);
	glUniformMatrix4fv(glGetUniformLocation(shadowPerShader, "mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shadowPerShader, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
	glUniform1i(glGetUniformLocation(shadowPerShader, "depthMap"), 0);
	glUniform1i(glGetUniformLocation(shadowPerShader, "heightMap"), 1);
	glUniform2f(glGetUniformLocation(shadowPerShader, "size"), border.size.x, border.size.y);
	glUniform1f(glGetUniformLocation(shadowPerShader, "step"), 1.0 / swidth);
	glUniform1f(glGetUniformLocation(shadowPerShader, "biasValue"), border.caveWalls->meshShadowsBiasValue);
	glUniform1f(glGetUniformLocation(shadowPerShader, "minBiasValue"), border.caveWalls->minMeshShadowsBiasValue);
	glUniform3f(glGetUniformLocation(shadowPerShader, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	glBindVertexArray(caveMeshVAO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glDrawElements(GL_TRIANGLES, border.caveWalls->indicesCount, GL_UNSIGNED_INT, 0);


	glBindTexture(GL_TEXTURE_2D, shadowMapTexture); 
	int width, height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	std::cout << "Depth map: " << width << "x" << height << std::endl;
	std::cout << "Viewport dimensions: " << swidth << "x" << sheight << std::endl;
	std::cout << "Near plane: " << near_plane << ", Far plane: " << far_plane << std::endl;

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkError(1234);

	checkError(1234);
}

void Render::setCavesTexture(std::vector<float>& data, Border& border) {

	// update texture data
	glBindTexture(GL_TEXTURE_2D, border.caveWalls->noiseTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, border.caveWalls->stepWidth, border.caveWalls->stepHeight, GL_RED, GL_FLOAT, data.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glGenerateTextureMipmap(border.caveWalls->noiseTexture);
	glBindTexture(GL_TEXTURE_2D, 0);

}

void Render::buildCavesTexture(int seed, Border& border) {

	// render noise shader onto cave noise texture
	int w = int(border.caveWalls->stepWidth);
	int h = int(border.caveWalls->stepHeight);

	float scaling = 3.0f;
	float sx = scaling;
	float sy = scaling;
	float aspectRatio = float(w) / float(h);

	// scale according to map aspect ratio to avoid stretching.
	if (w > h) {
		sx = sx * aspectRatio;
	}
	else if (w < h) {
		sy = sy / aspectRatio;
	}

	b2Vec2 scale(sx, sy); // scale.
	float time = (seed % 20000) + 0.0; // used as a seed
	int layers = 4;
	float exponent = 2.3;
	float persistance = 0.3;

	GLuint frameBuffer;
	GLuint& noiseTexture = border.caveWalls->noiseTexture;
	glGenFramebuffers(1, &frameBuffer);


	checkError(2000);

	// Vertex data for a quad
	GLfloat quadVertices[] = {
		-1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f,  1.0f,
		 1.0f, -1.0f
	};

	// Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	checkError(2001);
	// bind border noise texture
	glGenFramebuffers(1, &frameBuffer);
	glGenTextures(1, &noiseTexture);

	// setup texture
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.5f);

	// Attach the texture to the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, noiseTexture, 0);

	// Check for framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		// Handle framebuffer incomplete error
		std::cerr << "Framebuffer is not complete!" << std::endl;
	}


	// generate noise onto one channel framebuffer
	renderNoiseOnFrameBuffer(frameBuffer, noiseTexture, w, h, scale, time, layers, exponent, persistance, border.caveWalls->threshold);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	float* pixels = new float[w * h];
	glReadPixels(0, 0, w, h, GL_RED, GL_FLOAT, pixels);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// move pixels to textureData
	std::vector<float>& texData = border.caveWalls->textureData;
	std::vector<float>& originalTextureData = border.caveWalls->originalTextureData;
	texData.resize(w * h);
	originalTextureData.resize(w * h);
	for (int i = 0; i < w * h; ++i) {
		texData[i] = pixels[i];
		originalTextureData[i] = pixels[i];
	}

	// after storing data in pixels, no need to keep the frame buffer and array around.
	glDeleteFramebuffers(1, &frameBuffer);
	delete[] pixels;

	checkError(2004);

}

void Render::updateCaveModel(Border& border) {

	std::vector<float> lineVertices;
	borderLineColors.clear();
	bool withNormals = true;
	borderLineCount = 0;
	for (std::vector<CaveLine*>& list : border.caveWalls->chains) {
		int e = list.size() - 1;
		b2Vec3 color;
		bool loops = abs(list[0]->x1 - list[e]->x2) < 0.001 && abs(list[0]->y1 - list[e]->y2) < 0.001;
		
		for (int index = 0; index < list.size(); index++) {
			CaveLine* line = list[index];

			if (index == 0) {
				color = b2Vec3(1, 0, 0);
			}
			else if (index == list.size() - 1) {
				color = b2Vec3(0, 0, 1);
			}
			else {
				color = loops ? b2Vec3(1, 1, 1) : b2Vec3(0, 0, 0);
			}

			float x1 = line->x1;
			float y1 = line->y1;
			float x2 = line->x2;
			float y2 = line->y2;


			lineVertices.push_back(x1);
			lineVertices.push_back(y1);
			lineVertices.push_back(x2);
			lineVertices.push_back(y2);

			borderLineColors.push_back(color);

			borderLineCount++;

			// add normal line
			if (withNormals) {
				float midx = (x1 + x2) / 2;
				float midy = (y1 + y2) / 2;

				float angle = atan2f(y2 - y1, x2 - x1);
				float nex = cosf(angle - M_PI / 2) * 1 + midx;
				float ney = sinf(angle - M_PI / 2) * 1 + midy;


				lineVertices.push_back(midx);
				lineVertices.push_back(midy);
				lineVertices.push_back(nex);
				lineVertices.push_back(ney);
				borderLineColors.push_back(b2Vec3(0, 1, 0));
				borderLineCount++;
			}
			
		}
	}
	
	glBindVertexArray(borderLineVao);
	glBindBuffer(GL_ARRAY_BUFFER, borderLineVbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
}

void Render::buildCavesModel(Border& border) {

	std::vector<float> lineVertices;
	std::vector<float> lineColors;
	bool withNormals = true;
	borderLineCount = 0;
	for (std::vector<CaveLine*>& list : border.caveWalls->chains) {
		int e = list.size() - 1;
		b2Vec3 color;
		bool loops = abs(list[0]->x1 - list[e]->x2) < 0.001 && abs(list[0]->y1 - list[e]->y2) < 0.001;
		int index = 0;
		for (CaveLine* line : list) {
			color = loops ? b2Vec3(1, 1, 1) : b2Vec3(0, 0, 0);

			if (index == 0) {
				color = b2Vec3(1, 0, 0);
			}
			else if (index == list.size() - 1) {
				color = b2Vec3(0, 0, 1);
			}

			float x1 = line->x1;
			float y1 = line->y1;
			float x2 = line->x2;
			float y2 = line->y2;


			lineVertices.push_back(x1);
			lineVertices.push_back(y1);
			lineVertices.push_back(x2);
			lineVertices.push_back(y2);

			borderLineColors.push_back(color);

			borderLineCount++;

			// add normal line
			if (withNormals) {
				float midx = (x1 + x2) / 2;
				float midy = (y1 + y2) / 2;

				float angle = atan2f(y2 - y1, x2 - x1);
				float nex = cosf(angle - M_PI / 2) * 1 + midx;
				float ney = sinf(angle - M_PI / 2) * 1 + midy;


				lineVertices.push_back(midx);
				lineVertices.push_back(midy);
				lineVertices.push_back(nex);
				lineVertices.push_back(ney);
				borderLineColors.push_back(b2Vec3(0, 1, 0));
				borderLineCount++;
			}

			index++;
		}
	}

	// store line data
	glGenVertexArrays(1, &borderLineVao);
	glGenBuffers(1, &borderLineVbo);


	glBindVertexArray(borderLineVao);
	glBindBuffer(GL_ARRAY_BUFFER, borderLineVbo);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);


	//
	glGenVertexArrays(1, &caveVAO);
	glGenBuffers(1, &caveVBO);
	glGenBuffers(1, &caveEBO);

	glBindVertexArray(caveVAO);

	std::vector<float>& triangles = border.caveWalls->flatTriangles;
	std::vector<unsigned int>& indices = border.caveWalls->flatIndices;
	
	// bind vertices
	glBindBuffer(GL_ARRAY_BUFFER, caveVBO);
	glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(float), triangles.data(), GL_STATIC_DRAW);

	// bind indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, caveEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	// bind vertex attribute pointers
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
}

std::string Render::LoadShaderSource(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::string filepath = std::filesystem::absolute(filename).string();
		std::cerr << filepath << std::endl;
		std::cerr << "Failed to open shader file: " << filename << std::endl;
		throw std::runtime_error("Couldn't open shader file.");
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}


// Function to read shader source code from a file
std::string Render::readShaderSource(const std::string& filePath) {

	std::ifstream fileStream(filePath);
	if (!fileStream.is_open()) {
		std::cerr << "Failed to open file: " << filePath << std::endl;

		std::string exists = std::filesystem::exists(filePath) ? "true" : "false";
		std::string absolute = std::filesystem::absolute(filePath).string();
		std::cerr << "Exists: " << exists << std::endl;
		std::cerr << "Absolute: " << absolute << std::endl;

		throw std::runtime_error("Couldn't open shader.");
	}

	std::stringstream shaderSource;
	shaderSource << fileStream.rdbuf();
	fileStream.close();

	return shaderSource.str();
}

// Function to create and compile a shader
unsigned int Render::createShaderData(const std::string& shaderSource, GLenum shaderType) {
	unsigned int shader = glCreateShader(shaderType);
	const char* src = shaderSource.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	// Check for shader compilation errors
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "Shader compilation error: " << infoLog << std::endl;
		return 0;
	}

	return shader;
}

unsigned int Render::createShader(std::string vertex, std::string frag) {
	std::string vertexShaderSource = readShaderSource(vertex);
	std::string fragmentShaderSource = readShaderSource(frag);

	// Create and compile shaders
	unsigned int vertexShader = createShaderData(vertexShaderSource, GL_VERTEX_SHADER);
	unsigned int fragmentShader = createShaderData(fragmentShaderSource, GL_FRAGMENT_SHADER);

	// Create a shader program, attach shaders, and link them
	unsigned int shaderProgram = glCreateProgram();
	checkError(30000);
	glAttachShader(shaderProgram, vertexShader);
	checkError(30000);
	glAttachShader(shaderProgram, fragmentShader);
	checkError(30001);
	glLinkProgram(shaderProgram);
	checkError(30002);

	// Check for shader program linking errors
	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "Vertex path: " << vertex << std::endl;
		std::cerr << "Frag path: " << frag << std::endl;
		std::cerr << "Shader program linking error: " << infoLog << std::endl;
		throw std::runtime_error("Couldnt load shader.");
	}
	checkError(30003);

	// Delete the individual shaders as they are now linked into the program
	glDeleteShader(vertexShader);
	checkError(30004);
	glDeleteShader(fragmentShader);

	checkError(30005);

	return shaderProgram;
}

void Render::loadShaders() {

	if (haveShadersLoaded) {
		//std::cout << "Reloading noise shader: " << noiseShader << std::endl;
		//// delete old shaders
		//glDeleteProgram(noiseShader);
		//glDeleteProgram(shadowShader);
		//glDeleteProgram(shadowPerShader);
		//glDeleteProgram(caveShader);
		//glDeleteProgram(defShader);
		//glDeleteProgram(entityShader);
		//glDeleteProgram(borderShader);
		//glDeleteProgram(textShader);
		//checkError(29999);

		//noiseShader = 0;
		//shadowShader = 0;
		//shadowPerShader = 0;
		//caveShader = 0;
		//defShader = 0;
		//entityShader = 0;
		//borderShader = 0;
		//textShader = 0;
	}

	noiseShader = createShader("shaders/noiseVertexShader.glsl", "shaders/noiseFragmentShader.glsl");
	shadowShader = createShader("shaders/shadowVertexShader.glsl", "shaders/shadowFragmentShader.glsl");
	shadowPerShader = createShader("shaders/shadowPerspectiveVertexShader.glsl", "shaders/shadowPerspectiveFragmentShader.glsl");
	caveShader = createShader("shaders/caveVertexShader.glsl", "shaders/caveFragmentShader.glsl");
	defShader = createShader("shaders/defVertexShader.glsl", "shaders/defFragmentShader.glsl");
	entityShader = createShader("shaders/entityVertexShader.glsl", "shaders/entityFragmentShader.glsl");
	borderShader = createShader("shaders/borderVertexShader.glsl", "shaders/borderFragmentShader.glsl");
	textShader = createShader("shaders/textVertexShader.glsl", "shaders/textFragmentShader.glsl");

	haveShadersLoaded = true;
}

void Render::generateEntityVertices(int numVertices, float radius, float length) {

	if (numVertices < 3) {
		return; // Not enough vertices to form a shape
	}

	// Define the angle increment 
	float mouthSize = M_PI * 2 * 0.25;
	float angleIncrement = (2 * M_PI - mouthSize) / (numVertices - 4);

	// Center point (0, 0)
	entityVertices.push_back(0.0f);
	entityVertices.push_back(0.0f);

	// Create the curved part of the teardrop
	for (int i = 0; i <= numVertices - 4; ++i) {
		float angle = angleIncrement * i;
		float x = radius * cos(angle + mouthSize / 2);
		float y = radius * sin(angle + mouthSize / 2);
		entityVertices.push_back(x);
		entityVertices.push_back(y);
	}

	// Create the pointy part of the teardrop
	entityVertices.push_back(radius + length);
	entityVertices.push_back(0.0f);

	float x = radius * cos(0 + mouthSize / 2);
	float y = radius * sin(0 + mouthSize / 2);
	entityVertices.push_back(x);
	entityVertices.push_back(y);

}

void Render::buildScreen() {
	// Initialize GLFW
	if (!glfwInit()) {
		// Handle initialization failure
		throw std::runtime_error("failed to use glfwInit()");
	}

	// Create a GLFW window and OpenGL context
	window = glfwCreateWindow(int(nextWindowSize.x), int(nextWindowSize.y), "OpenGL Window", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		throw std::runtime_error("failed to create window");
	}
	glfwSetWindowPos(window, int(nextWindowPosition.x), int(nextWindowPosition.y));
	glfwMakeContextCurrent(window);

	glewInit();
	checkError(0);


	// Create the OpenGL vertex array and buffer
	checkError(1);


	// Define vertices for an entity
	generateEntityVertices(numSegments, 1.0f, 0.5f);

	// Define vertices for a circle
	float centerX = 0.0f;
	float centerY = 0.0f;
	float radius = 1.0f;
	for (int i = 0; i <= numSegments;i++) {
		b2Vec2 vec(radius, 0);

		b2Rot rotMatrix((i * 2 * M_PI / numSegments) - M_PI);
		vec = b2Mul(rotMatrix, vec);

		circleVertices.push_back(centerX + vec.x);
		circleVertices.push_back(centerY + vec.y);
	}

	entityVerticesVBO.dataSize = entityVertices.size() * sizeof(float);
	entityVerticesVBO.size = entityVertices.size();

	// load entity vertices vao
	glGenVertexArrays(1, &entityVerticesVAO);
	glBindVertexArray(entityVerticesVAO);
	glGenBuffers(1, &entityVerticesVBO.vboID);

	// bind data to a buffer
	glBindBuffer(GL_ARRAY_BUFFER, entityVerticesVBO.vboID);
	glBufferData(GL_ARRAY_BUFFER, entityVerticesVBO.dataSize, entityVertices.data(), GL_STATIC_DRAW);

	// store at location 0
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	circleVerticesVBO.dataSize = circleVertices.size() * sizeof(float);
	circleVerticesVBO.size = circleVertices.size();

	glGenBuffers(1, &circleVerticesVBO.vboID);

	glBindBuffer(GL_ARRAY_BUFFER, circleVerticesVBO.vboID);
	glBufferData(GL_ARRAY_BUFFER, circleVerticesVBO.dataSize, circleVertices.data(), GL_STATIC_DRAW);

	// store at location 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &cellDataVBO.vboID);
	glBindBuffer(GL_ARRAY_BUFFER, cellDataVBO.vboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cellData), cellData, GL_DYNAMIC_DRAW);

	// Configure the attribute pointer for entity positions as an instance attribute
	for (int i = 0; i < 10; ++i) {
		glEnableVertexAttribArray(2 + i);
		glVertexAttribPointer(2 + i, 1, GL_FLOAT, GL_FALSE, dataPerCell * sizeof(float), reinterpret_cast<void*>(i * sizeof(float)));
		glVertexAttribDivisor(2 + i, 1); // Set as an instance attribute
	}
	glEnableVertexAttribArray(0);

	checkError(50);

	buildBorder();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void Render::buildBorder() {

	// lines for rendering 

	checkError(1000);

	// Define vertices for a rectangle
	std::vector<float> rectVertices = {
		-0.5f, -0.5f,
		 0.5f, -0.5f,
		 0.5f,  0.5f,
		-0.5f,  0.5f
	};

	// In the buildBorderGL function:
	glGenVertexArrays(1, &borderVAO);
	glBindVertexArray(borderVAO);

	// Create and bind a VBO to store rectangle data
	glGenBuffers(1, &borderVBO.vboID);
	borderVBO.dataSize = rectVertices.size() * sizeof(float);
	borderVBO.size = rectVertices.size();
	glBindBuffer(GL_ARRAY_BUFFER, borderVBO.vboID);
	glBufferData(GL_ARRAY_BUFFER, borderVBO.dataSize, rectVertices.data(), GL_STATIC_DRAW);

	// Configure the attribute pointers for the rectangle vertices
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glVertexAttribDivisor(0, 0);
}

glm::mat4 Render::calculateMVPMatrix(float offsetX, float offsetY, float scale, bool useAspectRatio) {

	b2Vec2 scaling = b2Vec2(viewportSize.x / world_size.x, viewportSize.y / world_size.y);

	float viewWidth;
	float viewHeight;

	if (useAspectRatio) {
		float aspectRatio = viewportSize.x / viewportSize.y;
		if (aspectRatio > 1.0f) {
			viewWidth = world_size.x;
			viewHeight = world_size.y * (world_size.x / world_size.y) / aspectRatio;
		}
		else if(aspectRatio < 1.0f) {
			viewWidth = world_size.x;
			viewHeight = world_size.y * (world_size.x / world_size.y) / aspectRatio;
		}
		else {
			viewWidth = world_size.x;
			viewHeight = world_size.y * (world_size.x / world_size.y);
		}
	}
	else {
		viewWidth = std::max(world_size.x, world_size.y);
		viewHeight = viewWidth;
	}

	float left = -(viewWidth / 2) / scale;    // Left coordinate of the view
	float right = (viewWidth / 2) / scale;     // Right coordinate of the view
	float bottom = -(viewHeight / 2) / scale;  // Bottom coordinate of the view
	float top = (viewHeight / 2) / scale;       // Top coordinate of the view
	//std::cout << "MVP_LRBT: " << left << " " << right << " " << bottom << " " << top << std::endl;

	// Create the Identity Matrix
	glm::mat4 identityMatrix = glm::mat4(1.0f);
	identityMatrix = glm::translate(identityMatrix, glm::vec3(-offsetX, offsetY, 0.0f));
	identityMatrix = glm::scale(identityMatrix, glm::vec3(1, -1, 1));

	// Create the View Matrix (2D view at [0, 0] looking along -Z)
	glm::mat4 viewMatrix = glm::lookAt(
		glm::vec3(0, 0, cameraHeight),  // Camera position (eye)
		glm::vec3(0, 0, 0.0f),  // Target position (center)
		glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
	);

	// Create the Projection Matrix (Orthographic 2D view)
	glm::mat4 projectionMatrix = glm::ortho(left, right, bottom, top, 0.0f, viewWidth);

	// Combine the matrices to create the MVP matrix

	glm::mat4 modelViewProjectionMatrix = projectionMatrix * viewMatrix * identityMatrix;

	return modelViewProjectionMatrix;
}
// Your OpenGL rendering function
void Render::RenderImGui(glm::mat4 mvp, ImDrawData* draw_data) {
}

void Render::buildGUI() {

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup ImGui for OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");  // Use your OpenGL version

}

// interpolate between current view center and next view center.
void Render::updateViewCenter(float deltaTime, float offsetX, float offsetY, float scale) {

	currentWorldViewCenter.Set(offsetX, offsetY);

	// scale
	if (abs(scale - nextViewScale) > 0.000001) {
		startViewScale = currentViewScale;
		nextViewScale = scale;
		// dont reset  unless already finished
		if (currentViewScaleVelocity >= 0.1f)
			currentViewScaleVelocity = 0;
	}

	if (abs(currentViewScale - scale) > 0.000001) {

		float vss = viewScaleSpeed;// / std::max(1.0f, abs(startViewScale - nextViewScale) / viewScaleRate);
		currentViewScaleVelocity = std::min(1.0f, currentViewScaleVelocity + (vss / deltaTime));
		float newScale = MyMath::cosine_interp(startViewScale, nextViewScale, currentViewScaleVelocity);

		currentViewScale = newScale;
	}
}

void Render::render(float deltaTime, double worldTime, std::vector<Entity*>& entities, std::vector<PlantEntity*>& plants, Border& border, float offsetX, float offsetY, float scale,
	EnvironmentBody* focusedEntity, std::vector<TextData>& textData, float seasonTime) {

	updateViewCenter(deltaTime, offsetX, offsetY, scale);

	modelViewProjectionMatrix = calculateMVPMatrix(currentWorldViewCenter.x, currentWorldViewCenter.y, currentViewScale, true);


	double loopedTime = fmod(worldTime, 50000.0);
	worldTime = loopedTime;

	glViewport(0, 0, viewportSize.x, viewportSize.y);

	// clear buffer
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	renderBorder(worldTime, modelViewProjectionMatrix, border, seasonTime);
	renderEntities(scale, worldTime, modelViewProjectionMatrix, entities, plants, offsetX, offsetY);
	renderText(textData);

	glViewport(0, 0, viewportSize.x, viewportSize.y);
	ImGui::Render();
	// draw gui
	ImDrawData* data = ImGui::GetDrawData();
	//data->DisplaySize = ImVec2(viewportSize.x, viewportSize.y);
	//data->DisplayPos = ImVec2(-viewportSize.x / 2, -viewportSize.y / 2);
	ImGui_ImplOpenGL3_RenderDrawData(data);


	glfwSwapBuffers(window);
}

b2Vec3 Render::mixColor(b2Vec3 a, b2Vec3 b, float perc) {
	float x = a.x * (1 - perc) + b.x * perc;
	float y = a.y * (1 - perc) + b.y * perc;
	float z = a.z * (1 - perc) + b.z * perc;

	return b2Vec3(x, y, z);
}

void Render::renderBorder(double worldTime, glm::mat4& modelViewProjectionMatrix, Border& border, float seasonTime) {


	b2Vec3 seasonalColors[4];
	seasonalColors[0] = b2Vec3(78, 135, 105);
	seasonalColors[1] = b2Vec3(121, 145, 57);
	seasonalColors[2] = b2Vec3(153, 108, 62);
	seasonalColors[3] = b2Vec3(115, 119, 168);

	b2Vec3 colorA = seasonalColors[int(seasonTime)];
	b2Vec3 colorB = seasonalColors[int(seasonTime + 1.) % 4];
	float perc = fmod(seasonTime, 1.0f);
	b2Vec3 seasonColor = mixColor(colorA, colorB, perc);
	seasonColor *= 1.0 / 255.0;

	
	b2Vec2 borderSize = border.bounds[1] - border.bounds[0];
	// Pass the modelViewProjectionMatrix to the shader
	// 	glUseProgram(borderShader);
	//  glBindVertexArray(borderVAO);
	//glUniformMatrix4fv(glGetUniformLocation(borderShader, "modelViewProjectionMatrix"), 1, GL_FALSE, &modelViewProjectionMatrix[0][0]);
	//glUniform3f(glGetUniformLocation(borderShader, "seasonColor"), seasonColor.x, seasonColor.y, seasonColor.z);
	//glUniform2f(glGetUniformLocation(borderShader, "borderSize"), borderSize.x, borderSize.y);
	//glUniform1f(glGetUniformLocation(borderShader, "worldTime"), worldTime);
	//glUniform1f(glGetUniformLocation(borderShader, "seasonTime"), seasonTime);
	//glUniform1f(glGetUniformLocation(borderShader, "backgroundTexScale"), backgroundTexScale);
	//glUniform1f(glGetUniformLocation(borderShader, "backgroundTexSize"), backgroundTexSize);

	//// Render the rectangle
	//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);



	// draw caves
	glUseProgram(caveShader);
	glBindVertexArray(caveVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, border.caveWalls->noiseTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

	float w = border.caveWalls->stepWidth;
	float h = border.caveWalls->stepHeight;
	float scale = 8.0f * std::min(border.size.x, border.size.y) / 1000.0f; // scale relative to world size
	float seed = (border.seed % 20000) + 0.0; // used as a seed
	int layers = 2;
	float exponent = 2.0;
	float persistance = 0.3;
	glUniform3f(glGetUniformLocation(caveShader, "lightDirection"), lightDirection.x, lightDirection.y, lightDirection.z);
	glUniform3f(glGetUniformLocation(caveShader, "seasonColor"), seasonColor.x, seasonColor.y, seasonColor.z);
	glUniformMatrix4fv(glGetUniformLocation(caveShader, "modelViewProjectionMatrix"), 1, GL_FALSE, &modelViewProjectionMatrix[0][0]);
	glUniform2f(glGetUniformLocation(caveShader, "borderSize"), border.size.x, border.size.y);
	glUniform1f(glGetUniformLocation(caveShader, "threshold"), border.caveWalls->threshold);
	glUniform2f(glGetUniformLocation(caveShader, "step"), 1.0 / border.caveWalls->stepWidth, 1.0 / border.caveWalls->stepHeight);
	glUniform1f(glGetUniformLocation(caveShader, "seasonTime"), seasonTime);
	glUniform1i(glGetUniformLocation(caveShader, "noiseTexture"), 0);
	glUniform1i(glGetUniformLocation(caveShader, "shadowTexture"), 1);
	glUniform1f(glGetUniformLocation(caveShader, "heightScale"), border.caveWalls->renderHeightScale);
	glUniform1f(glGetUniformLocation(caveShader, "worldTime"), worldTime);

	int count = border.caveWalls->flatIndices.size();
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);


	// draw lines
	if(border.shouldRenderBorders)
		drawBorderLines(modelViewProjectionMatrix);

}

void Render::drawBorderLines(glm::mat4& modelViewProjectionMatrix) {

	glUseProgram(defShader);
	glBindVertexArray(borderLineVao);


	glUniformMatrix4fv(glGetUniformLocation(defShader, "modelViewProjectionMatrix"), 1, GL_FALSE, &modelViewProjectionMatrix[0][0]);
	for (int i = 0; i < borderLineCount; i++) {
		b2Vec3& color = borderLineColors[i];
		glUniform3f(glGetUniformLocation(defShader, "color"), color.x, color.y, color.z);
		glLineWidth(1.0f);
		glDrawArrays(GL_LINES, i * 2, 2);
	}
}

void Render::renderEntities(float worldScale, double worldTime, glm::mat4& modelViewProjectionMatrix, std::vector<Entity*>& entities,
	std::vector<PlantEntity*>& plants, float offsetX, float offsetY) {
	b2Vec3 cameraPosition(offsetX, offsetY, cameraHeight);
	
	glUseProgram(entityShader);

	int toUpdate = updateData(entities, plants);
	glBindBuffer(GL_ARRAY_BUFFER, cellDataVBO.vboID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, toUpdate * dataPerCell * sizeof(float), cellData);

	int entCount = entities.size();
	int plantCount = plants.size();
	glBindVertexArray(entityVerticesVAO);


	glUniformMatrix4fv(glGetUniformLocation(entityShader, "modelViewProjectionMatrix"), 1, GL_FALSE, &modelViewProjectionMatrix[0][0]);
	glUniform1f(glGetUniformLocation(entityShader, "worldTime"), worldTime);
	glUniform1f(glGetUniformLocation(entityShader, "worldScale"), worldScale);
	glUniform2f(glGetUniformLocation(entityShader, "worldSize"), world_size.x, world_size.y);
	glUniform2f(glGetUniformLocation(entityShader, "viewportSize"), viewportSize.x, viewportSize.y);
	glUniform3f(glGetUniformLocation(entityShader, "cameraPosition"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
	glUniform1f(glGetUniformLocation(entityShader, "waterHeight"), waterRenderHeight);
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, numSegments, toUpdate);
}

void Render::initText() {


	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		throw std::runtime_error("Failed to load opengl text");
	}

	if (FT_New_Face(ft, "C:\\Windows\\Fonts\\Minecraft.ttf", 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		throw std::runtime_error("Failed to load opengl text");
	}

	FT_Set_Pixel_Sizes(face, 0, 16);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

	for (unsigned char c = 0; c < 128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// generate texture
		glGenTextures(1, &textTexture);
		glBindTexture(GL_TEXTURE_2D, textTexture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		Character character = {
			textTexture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		characters.insert(std::pair<char, Character>(c, character));

	}
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Render::renderText(std::vector<TextData>& textData) {
	glUseProgram(textShader);

	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(viewportSize.x), 0.0f, static_cast<float>(viewportSize.y));

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, &projection[0][0]);
	float x;
	float y;
	for (TextData td : textData) {
		b2Vec3& color = td.color;
		b2Vec2& position = td.position;
		x = position.x;
		// flip y so that 0,0 is top left
		y = viewportSize.y - position.y - td.size;
		float textScale = td.size / 16.0f;

		glUniform3f(glGetUniformLocation(textShader, "textColor"), color.x, color.y, color.z);
		// iterate through all characters
		std::string::const_iterator c;
		for (c = td.string.begin(); c != td.string.end(); c++)
		{
			Character ch = characters[*c];

			float w = ch.Size.x * textScale;
			float h = ch.Size.y * textScale;

			float xpos = x + ch.Bearing.x * textScale;
			float ypos = y - (ch.Size.y - ch.Bearing.y) * textScale;

			// update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
			};
			// render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, textVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.Advance >> 6) * textScale; // bitshift by 6 to get value in pixels (2^6 = 64)
		}
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

// renders red-only noise onto a 1 channel frame buffer.
void Render::renderNoiseOnFrameBuffer(GLuint& frameBuffer, GLuint& renderedTexture, int textureWidth, int textureHeight,
	b2Vec2 scale, float time, int layers, float exponent, float persistance, float threshold) {

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glUseProgram(noiseShader);
	glBindVertexArray(quadVAO);
	glViewport(0, 0, textureWidth, textureHeight);

	// render
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	glUniform2f(glGetUniformLocation(noiseShader, "scale"), scale.x, scale.y);
	glUniform1f(glGetUniformLocation(noiseShader, "time"), time);
	glUniform1i(glGetUniformLocation(noiseShader, "layers"), layers);
	glUniform1f(glGetUniformLocation(noiseShader, "exponent"), exponent);
	glUniform1f(glGetUniformLocation(noiseShader, "persistance"), persistance);
	glUniform1f(glGetUniformLocation(noiseShader, "threshold"), threshold);

	// draw noise
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// disable frame buffer for now.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLFWwindow* Render::getWindow() {
	return window;
}

int Render::updateData(std::vector<Entity*>& entities, std::vector<PlantEntity*>& plants) {

	float pixelsPerWC = 1;
	if (plants.size() > 2) {
		pixelsPerWC = (plants[0]->screenPosition - plants[1]->screenPosition).Length() / plants[0]->distanceTo(plants[1]);
	}

	int toUpdate = 0;
	int index = 0;
	for (Entity* entity : entities) {
		b2Vec2& pixel = entity->screenPosition;
		float radiusInPixels = entity->radius * pixelsPerWC;

		// check if they would render on screen considering their radius.
		if (pixel.x >= -radiusInPixels && pixel.x < viewportSize.x+ radiusInPixels && 
			pixel.y >= -radiusInPixels && pixel.y < viewportSize.y+ radiusInPixels) {
			toUpdate++;

			int rgb = (int(entity->displayColor.x * 255) << 16) | (int(entity->displayColor.y * 255) << 8) | int(entity->displayColor.z * 255);
			cellData[index++] = entity->position.x;
			cellData[index++] = entity->position.y;
			cellData[index++] = float(rgb);
			cellData[index++] = entity->radius;
			cellData[index++] = entity->angle;
			cellData[index++] = 0.0f; // not plant
			cellData[index++] = entity->attackingAmount;
			cellData[index++] = entity->uid;
			cellData[index++] = entity->gradualVelocity.x;
			cellData[index++] = entity->gradualVelocity.y;
		}

	}
	for (PlantEntity* entity : plants) {
		b2Vec2& pixel = entity->screenPosition;
		float radiusInPixels = entity->radius * pixelsPerWC;

		// check if they would render on screen
		if (pixel.x >= -radiusInPixels && pixel.x < viewportSize.x + radiusInPixels &&
			pixel.y >= -radiusInPixels && pixel.y < viewportSize.y + radiusInPixels) {
			toUpdate++;

			int rgb = (int(entity->displayColor.x * 255) << 16) | (int(entity->displayColor.y * 255) << 8) | int(entity->displayColor.z * 255);
			cellData[index++] = entity->position.x;
			cellData[index++] = entity->position.y;
			cellData[index++] = float(rgb);
			cellData[index++] = entity->radius;
			cellData[index++] = entity->angle;
			cellData[index++] = 1.0f; // is plant
			cellData[index++] = 0.0f; // attacking value
			cellData[index++] = entity->uid;
			cellData[index++] = entity->gradualVelocity.x;
			cellData[index++] = entity->gradualVelocity.y;
		}
	}

	return toUpdate;
}

void Render::checkError(int id) {
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << id << " OpenGL Error: " << error << std::endl;
		throw std::runtime_error("uwu");
	}
}


TextData Render::createTextData(std::string text, int x, int y, int size, b2Vec3 color) {
	TextData td = {
		color,
		b2Vec2(x, y),
		size,
		text
	};

	return td;
}

void Render::cleanUp() {
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

// BrainRender

BrainRender::BrainRender(std::map<char, Character>& chars) : characters(chars) {

}

void BrainRender::build() {
	glGenVertexArrays(1, &circleVao);
	glGenBuffers(1, &circleVbo);
	glGenVertexArrays(1, &lineVao);
	glGenBuffers(1, &lineVbo);

	glGenFramebuffers(1, &frameBuffer);
	glGenTextures(1, &renderedTexture);
	createdBuffer = true;

	brainShader = Render::createShader("shaders/brainVertexShader.glsl", "shaders/brainFragmentShader.glsl");
}

// Converts a brain into a texture visualization
void BrainRender::render(int uid, CellBrain& brain, bool forceRender) {
	if (uidOfLastEntity == uid && !forceRender) {
		// already built brain
		return;
	}


	uidOfLastEntity = uid;
	std::cout << "Rendering new brain" << std::endl;

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// setup texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

	// set render mode to frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glViewport(0, 0, textureWidth, textureHeight);

	// render
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	drawBrain(brain, textureWidth, textureHeight);

	// disable frame buffer for now.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BrainRender::drawAllCircles(std::vector<Circle>& circles) {

	std::vector<float> circleVertices;
	for (Circle& circle : circles) {
		float x = (circle.x / textureWidth) * 2 - 1;
		float y = (circle.y / textureHeight) * 2 - 1;
		float radius = circle.radius;
		b2Vec3 color = circle.color;

		const float twicePi = 2.0f * M_PI;

		circleVertices.push_back(x);
		circleVertices.push_back(y);
		for (int i = 0; i <= triangleAmountInCircle; ++i) {
			float x1 = (radius * cos(i * twicePi / triangleAmountInCircle));
			float y1 = (radius * sin(i * twicePi / triangleAmountInCircle));

			x1 = x + (x1 / textureWidth) * 2;
			y1 = y + (y1 / textureHeight) * 2;
			
			circleVertices.push_back(x1);
			circleVertices.push_back(y1);
		}
	}

	glBindVertexArray(circleVao);
	glBindBuffer(GL_ARRAY_BUFFER, circleVbo);
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glUseProgram(brainShader);

	for (size_t i = 0; i < circles.size(); ++i) {
		b2Vec3 color = circles[i].color;
		glUniform3f(glGetUniformLocation(brainShader, "color"), color.x, color.y, color.z);
		glDrawArrays(GL_TRIANGLE_FAN, i * (triangleAmountInCircle + 2), triangleAmountInCircle + 2);
	}
}

void BrainRender::drawAllLines(std::vector<Line>& lines) {
	std::vector<float> lineVertices;
	for (const auto& line : lines) {
		float x1 = line.x1;
		float y1 = line.y1;
		float x2 = line.x2;
		float y2 = line.y2;
		x1 = (x1 / textureWidth) * 2 - 1;
		y1 = (y1 / textureWidth) * 2 - 1;
		x2 = (x2 / textureWidth) * 2 - 1;
		y2 = (y2 / textureWidth) * 2 - 1;


		lineVertices.push_back(x1);
		lineVertices.push_back(y1);
		lineVertices.push_back(x2);
		lineVertices.push_back(y2);
	}

	glBindVertexArray(lineVao);
	glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glUseProgram(brainShader);

	float min = std::min(textureWidth, textureHeight);
	for (size_t i = 0; i < lines.size(); ++i) {
		b2Vec3 color = lines[i].color;
		glUniform3f(glGetUniformLocation(brainShader, "color"), color.x, color.y, color.z);
		glLineWidth(lines[i].size / min);
		glDrawArrays(GL_LINES, i * 2, 2);
	}
}

// slightly different from Render::renderText as it has a different viewport dynamic
void BrainRender::renderText(std::vector<TextData>& textData) {

	glUseProgram(textShader);

	// flipped to let it render
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(textureWidth), static_cast<float>(textureHeight), 0.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, &projection[0][0]);
	float x;
	float y;
	for (TextData td : textData) {
		b2Vec3& color = td.color;
		b2Vec2& position = td.position;
		x = position.x;
		y = textureHeight - position.y - td.size;
		float textScale = td.size / 16.0f;

		glUniform3f(glGetUniformLocation(textShader, "textColor"), color.x, color.y, color.z);
		// iterate through all characters
		std::string::const_iterator c;
		for (c = td.string.begin(); c != td.string.end(); c++)
		{
			Character ch = characters[*c];

			float w = ch.Size.x * textScale;
			float h = ch.Size.y * textScale;

			float xpos = x + ch.Bearing.x * textScale;
			float ypos = y - (ch.Size.y - ch.Bearing.y) * textScale;

			// update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
			};
			// render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, textVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.Advance >> 6) * textScale; // bitshift by 6 to get value in pixels (2^6 = 64)
		}
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

void BrainRender::drawBrain(CellBrain& brain, int textureWidth, int textureHeight) {
	std::vector<Circle> circles;
	std::vector<Line> lines;
	std::vector<TextData> textData;

	int inputSize = brain.input_size + 1; // add bias for visuals
	int hiddenSize = brain.h1_size + 1;
	int outputSize = brain.output_size;



	// these cause gaps between neurons.
	int gaps[] = {
		1, // bias
		3, // protein/sugar
		5, // stomach
		7, // attacked
		10, // bind
		11,  // avg rgb

		14, 
		25, 
		36,
		47
	};
	static b2Vec3 colorPerInput[] = {
		b2Vec3(0, 1, 0), // bias
		b2Vec3(1, 1, 0), b2Vec3(1, 1, 0), // age, heartbeat
		b2Vec3(1, 0.2, 0.2), b2Vec3(0.1, 1, 0.9), // protein, sugar
		b2Vec3(0.5, 0.2, 0.2), b2Vec3(0.1, 1, 0.45), // stomach protein, stomach sugar
		b2Vec3(1, 1, 0), b2Vec3(1, 1, 0), b2Vec3(1, 1, 0), // attackedValue, angleC, angleS
		b2Vec3(0.1, 0.1, 0.1), // bind
		b2Vec3(1, 0, 0), b2Vec3(0, 1, 0), b2Vec3(0, 0, 1), // avg rgb

		// eyes
		b2Vec3(1, 0, 0), b2Vec3(0, 1, 0), b2Vec3(0, 0, 1), b2Vec3(1, 1, 0), b2Vec3(1, 1, 0), b2Vec3(1.0, 1.0, 1.0), b2Vec3(1.0, 1.0, 1.0), b2Vec3(0.75, 0.75, 0), b2Vec3(0.75, 0.75, 0), b2Vec3(1, 0, 0), b2Vec3(0, 0, 1), // rgb, dist, radius, angle
		b2Vec3(1, 0, 0), b2Vec3(0, 1, 0), b2Vec3(0, 0, 1), b2Vec3(1, 1, 0), b2Vec3(1, 1, 0), b2Vec3(1.0, 1.0, 1.0), b2Vec3(1.0, 1.0, 1.0), b2Vec3(0.75, 0.75, 0), b2Vec3(0.75, 0.75, 0), b2Vec3(1, 0, 0), b2Vec3(0, 0, 1), // rgb, dist, radius, angle
		b2Vec3(1, 0, 0), b2Vec3(0, 1, 0), b2Vec3(0, 0, 1), b2Vec3(1, 1, 0), b2Vec3(1, 1, 0), b2Vec3(1.0, 1.0, 1.0), b2Vec3(1.0, 1.0, 1.0), b2Vec3(0.75, 0.75, 0), b2Vec3(0.75, 0.75, 0), b2Vec3(1, 0, 0), b2Vec3(0, 0, 1), // rgb, dist, radius, angle
		b2Vec3(0, 0, 0), b2Vec3(0, 0, 0), b2Vec3(0, 0, 0), b2Vec3(0, 0, 0) 
	};

	int inputX = (textureWidth / (3 + 1)) * 1;
	int hiddenX = (textureWidth / (3 + 1)) * 2;
	int outputX = (textureWidth / (3 + 1)) * 3;
	int yPerInput = (float(textureHeight) / (inputSize + 1 + std::size(gaps)));
	int yPerHidden = (float(textureHeight) / (hiddenSize + 1));
	int yPerOutput = (float(textureHeight) / (outputSize + 1));
	int inputRadius = int((textureHeight / float(inputSize + std::size(gaps))) * 0.5 * 0.85);
	int hiddenRadius = int((textureHeight / float(hiddenSize)) * 0.5 * 0.85);
	int outputRadius = int((textureHeight / float(outputSize)) * 0.5 * 0.85);

	// draw each neuron
	int gapsPassed = 0;
	for (int i = 0; i < inputSize; i++) {

		bool gap = false;
		for (int g : gaps) {
			if (i == g) {
				gap = true;
				break;
			}
		}
		if (gap) {
			gapsPassed++;
		}

		int inputY = yPerInput * (i + 1 + gapsPassed);

		if (i == 0) {
			// bias is green i guess
			circles.push_back(Circle{ float(inputX), float(inputY), float(inputRadius), colorPerInput[i] });
		}
		else {
			circles.push_back(Circle{ float(inputX), float(inputY), float(inputRadius), colorPerInput[i] });
		}
	}

	for (int i = 0; i < hiddenSize; i++) {
		int hiddenY = yPerHidden * (i + 1);
		if (i == 0) {
			// bias is green i guess
			circles.push_back(Circle{ float(hiddenX), float(hiddenY), float(hiddenRadius), b2Vec3(0, 1, 0) });
		}
		else {
			circles.push_back(Circle{ float(hiddenX), float(hiddenY), float(hiddenRadius), b2Vec3(0.25, 0.25, 0.25) });
		}
	}

	for (int i = 0; i < outputSize; i++) {
		int outputY = yPerOutput * (i + 1);

		circles.push_back(Circle{ float(outputX), float(outputY), float(outputRadius), b2Vec3(0.25, 0.5, 1) });
	}

	// draw weights
	Eigen::MatrixXf& inputWeights = brain.actual_inputLayerWeights.toDense(); // convert to MatrixXf
	Eigen::MatrixXf& outputWeights = brain.actual_outputLayerWeights.toDense();


	// check which neurons have connections worth rendering
	std::vector<int> enabledHiddenNeurons;

	// output - hidden
	for (int o = 0; o < outputWeights.rows(); o++) {
		for (int h = 0; h < outputWeights.cols(); h++) {
			if (brain.outputLayerWeightsMask(o, h) > 0.5) {

				bool hasChildren = false;

				// check hidden-inputs to see if it has any connections
				for (int i = 0; i < inputWeights.cols(); i++) {
					float v = brain.inputLayerWeightsMask(h, i);
					if (v > 0.5) {
						hasChildren = true;
						break;
					}
				}

				if(hasChildren)
					enabledHiddenNeurons.push_back(h);
			}
		}


	}

	// render lines

	for (int i = 0; i < outputWeights.rows(); i++) {
		for (int j = 0; j < outputWeights.cols(); j++) {
			// i = output neuron
			// j = hidden neuron
			int x1 = hiddenX;
			int y1 = yPerHidden * (j + 1 + 1); // add an extra +1 for bias

			int x2 = outputX;
			int y2 = yPerOutput * (i + 1);

			float p = outputWeights(i, j);
			if (abs(outputWeights(i, j)) < 1.0e-6) {
				continue;
			}

			if (std::find(enabledHiddenNeurons.begin(), enabledHiddenNeurons.end(), j) == enabledHiddenNeurons.end() && trimConnections) {
				continue;
			}
			

			b2Vec3 mixColor = p > 0 ? b2Vec3(1, 1 - p, 1 - p) : b2Vec3(0, 0, -p);
			lines.push_back(Line{ float(x1), float(y1), float(x2), float(y2), 5 + 5 * abs(outputWeights(i, j)), mixColor });
		}

	}

	for (int i = 0; i < inputWeights.rows(); i++) {
		gapsPassed = 0;
		for (int j = 0; j < inputWeights.cols(); j++) {
			bool gap = false;
			for (int g : gaps) {
				if ((j+1) == g) { // add one to compensate for bias
					gap = true;
					break;
				}
			}
			if (gap) {
				gapsPassed++;
			}

			// i = hidden neuron
			// j = input neuron
			int x1 = inputX;
			int y1 = yPerInput * (j + 2 + gapsPassed); // add 2 instead of 1 due to bias neuron being first 

			int x2 = hiddenX;
			int y2 = yPerHidden * (i + 1 + 1); // add an extra +1 for bias

			float p = inputWeights(i, j);
			if (abs(inputWeights(i, j)) < 1.0e-6) {
				continue;
			}

			if (std::find(enabledHiddenNeurons.begin(), enabledHiddenNeurons.end(), i) == enabledHiddenNeurons.end() && trimConnections) {
				continue;
			}

			b2Vec3 mixColor = p > 0 ? b2Vec3(1, 1 - p, 1 - p) : b2Vec3(0, 0, -p);
			lines.push_back(Line{ float(x1), float(y1), float(x2), float(y2), 5 + 5 * abs(inputWeights(i, j)), mixColor });
		}
	}

	// draw biases
	Eigen::VectorXf& inputBias = brain.actual_inputLayerBias.toDense();
	Eigen::VectorXf& outputBias = brain.actual_outputLayerBias.toDense();
	int biasX = inputX;
	int biasY = yPerInput * (1);
	for (int i = 0; i < inputBias.rows(); i++) {
		// i = hidden neuron
		int x2 = hiddenX;
		int y2 = yPerHidden * (i + 1 + 1); // +1 for bias


		std::string type = brain.activationName(brain.activationPerHidden[i]);
		textData.push_back(Render::createTextData(type, x2, y2 + hiddenRadius / 2.0, hiddenRadius / 2.0, b2Vec3(1, 1, 0)));

		if (abs(inputBias(i)) < 1.0e-6) {
			continue;
		}

		// if hidden neuron isn't connected to outputs, dont render.
		if (std::find(enabledHiddenNeurons.begin(), enabledHiddenNeurons.end(), i) == enabledHiddenNeurons.end() && trimConnections) {
			continue;
		}

		float p = inputBias(i);
		b2Vec3 mixColor = p > 0 ? b2Vec3(1, 1 - p, 1 - p) : b2Vec3(0, 0, -p);
		lines.push_back(Line{ float(biasX), float(biasY), float(x2), float(y2), 5 + 5 * abs(inputBias(i)), mixColor });
	}

	// change bias location to hidden layer column
	biasX = hiddenX;
	biasY = yPerHidden * 1;
	for (int i = 0; i < outputBias.rows(); i++) {
		// i = output neuron
		int x2 = outputX;
		int y2 = yPerOutput * (i + 1);


		std::string outputName = brain.outputName(i);
		textData.push_back(Render::createTextData(outputName, x2, y2 + outputRadius / 2.0, outputRadius / 2.0, b2Vec3(1, 1, 0)));

		if (abs(outputBias(i)) < 1.0e-6) {
			continue;
		}
		// guaranteed to be connected to output
		/*if (std::find(enabledNeurons.begin(), enabledNeurons.end(), 1000 + i) == enabledNeurons.end()) {
			continue;
		}*/

		float p = outputBias(i);
		b2Vec3 mixColor = p > 0 ? b2Vec3(1, 1 - p, 1 - p) : b2Vec3(0, 0, -p);
		lines.push_back(Line{ float(biasX), float(biasY), float(x2), float(y2), 5 + 5 * abs(outputBias(i)), mixColor });
	}


	drawAllCircles(circles);
	drawAllLines(lines);
	renderText(textData);

}

// TreeOfLifeRenderer

TreeOfLifeRenderer::TreeOfLifeRenderer(std::map<char, Character>& chars) : characters(chars) {

}

void TreeOfLifeRenderer::build() {
	glGenVertexArrays(1, &circleVao);
	glGenBuffers(1, &circleVbo);
	glGenVertexArrays(1, &lineVao);
	glGenBuffers(1, &lineVbo);

	glGenFramebuffers(1, &frameBuffer);
	glGenTextures(1, &renderedTexture);
	createdBuffer = true;

	brainShader = Render::createShader("shaders/brainVertexShader.glsl", "shaders/brainFragmentShader.glsl");
}

// Converts a brain into a texture visualization
void TreeOfLifeRenderer::render(TreeOfLife& tree, std::map<int, b2Vec3>& extantBranches) {
	// don't call rapidly, may be expensive

	std::cout << "Rendering new Tree of Life" << std::endl;

	// bind to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// setup texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

	// set render mode to frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glViewport(0, 0, textureWidth, textureHeight);

	// render
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	drawTree(tree, extantBranches, textureWidth, textureHeight);

	// disable frame buffer for now.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TreeOfLifeRenderer::drawAllCircles(std::vector<Circle>& circles) {

	std::vector<float> circleVertices;
	for (Circle circle : circles) {
		float x = (circle.x / textureWidth) * 2 - 1;
		float y = (circle.y / textureHeight) * 2 - 1;
		float radius = circle.radius;
		b2Vec3 color = circle.color;

		const float twicePi = 2.0f * M_PI;

		circleVertices.push_back(x);
		circleVertices.push_back(y);
		for (int i = 0; i <= triangleAmountInCircle; ++i) {
			float x1 = (radius * cos(i * twicePi / triangleAmountInCircle));
			float y1 = (radius * sin(i * twicePi / triangleAmountInCircle));

			x1 = x + (x1 / textureWidth) * 2;
			y1 = y + (y1 / textureHeight) * 2;

			circleVertices.push_back(x1);
			circleVertices.push_back(y1);
		}
	}

	glBindVertexArray(circleVao);
	glBindBuffer(GL_ARRAY_BUFFER, circleVbo);
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glUseProgram(brainShader);

	for (size_t i = 0; i < circles.size(); ++i) {
		b2Vec3 color = circles[i].color;
		glUniform3f(glGetUniformLocation(brainShader, "color"), color.x, color.y, color.z);
		glDrawArrays(GL_TRIANGLE_FAN, i * (triangleAmountInCircle + 2), triangleAmountInCircle + 2);
	}
}

void TreeOfLifeRenderer::drawAllLines(std::vector<Line>& lines) {
	std::vector<float> lineVertices;
	for (const auto& line : lines) {
		float x1 = line.x1;
		float y1 = line.y1;
		float x2 = line.x2;
		float y2 = line.y2;
		x1 = (x1 / textureWidth) * 2 - 1;
		y1 = (y1 / textureWidth) * 2 - 1;
		x2 = (x2 / textureWidth) * 2 - 1;
		y2 = (y2 / textureWidth) * 2 - 1;


		lineVertices.push_back(x1);
		lineVertices.push_back(y1);
		lineVertices.push_back(x2);
		lineVertices.push_back(y2);
	}

	glBindVertexArray(lineVao);
	glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glUseProgram(brainShader);

	float min = std::min(textureWidth, textureHeight);
	for (size_t i = 0; i < lines.size(); ++i) {
		b2Vec3 color = lines[i].color;
		glUniform3f(glGetUniformLocation(brainShader, "color"), color.x, color.y, color.z);
		glLineWidth(lines[i].size / min);
		glDrawArrays(GL_LINES, i * 2, 2);
	}
}

// slightly different from Render::renderText as it has a different viewport dynamic
void TreeOfLifeRenderer::renderText(std::vector<TextData>& textData) {

	glUseProgram(textShader);

	// flipped to let it render
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(textureWidth), static_cast<float>(textureHeight), 0.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, &projection[0][0]);
	float x;
	float y;
	for (TextData td : textData) {
		b2Vec3& color = td.color;
		b2Vec2& position = td.position;
		x = position.x;
		y = textureHeight - position.y - td.size;
		float textScale = td.size / 16.0f;

		glUniform3f(glGetUniformLocation(textShader, "textColor"), color.x, color.y, color.z);
		// iterate through all characters
		std::string::const_iterator c;
		for (c = td.string.begin(); c != td.string.end(); c++)
		{
			Character ch = characters[*c];

			float w = ch.Size.x * textScale;
			float h = ch.Size.y * textScale;

			float xpos = x + ch.Bearing.x * textScale;
			float ypos = y - (ch.Size.y - ch.Bearing.y) * textScale;

			// update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
			};
			// render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, textVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.Advance >> 6) * textScale; // bitshift by 6 to get value in pixels (2^6 = 64)
		}
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

void TreeOfLifeRenderer::drawTree(TreeOfLife& tree, std::map<int, b2Vec3>& extantBranches, int textureWidth, int textureHeight) {
	std::vector<Circle> circles;
	std::vector<Line> lines;
	std::vector<TextData> textData;

	// decrease radius as the depth increases to maintain max radius
	float maxRadius = (textureWidth / 2.0);
	float step = maxRadius / (1 + tree.deepestChain);
	BranchRenderData renderData = { textureWidth / 2, textureHeight / 2, -M_PI/2, 0,
		circles, lines, textData, step, 0, 0, M_PI * 2, extantBranches };
	drawBranch(tree.trunk, renderData);

	drawAllLines(lines);
	drawAllCircles(circles);
	renderText(textData);

}

bool TreeOfLifeRenderer::drawBranch(TreeBranch* branch, BranchRenderData& renderData) {
	if (branch == nullptr) {
		return false;
	}
	float size = 5;

	float x = renderData.x;
	float y = renderData.y;

	int thisBranch = branch->branchData.branchIndex;
	bool hasParent = branch->parent != nullptr;

	// render children recursively
	// render by default if it is extant
	bool doesLineageSurvive = renderData.extantBranches.find(thisBranch) != renderData.extantBranches.end();
	if (!branch->children.empty()) {
		int count = branch->children.size();

		// but isnt it very expensive to check all branches to see which ones are extinct? yes.
		int childrenToRender = 0;
		for (int j = 0; j < count; j++) {

		}

		double angleIncrement = (renderData.thetaSpan) / (count + 1);
		if (!hasParent) {
			angleIncrement = M_PI * 2 / (count);
		}

		for (int j = 0; j < count; j++) {
			float brdangle = -renderData.thetaSpan / 2 + renderData.angle + (j +1) * angleIncrement;
			float brdradius = renderData.radius + renderData.step;
			float brdx = renderData.x + renderData.step * cos(brdangle);
			float brdy = renderData.y + renderData.step * sin(brdangle);
			float brdThetaSpawn = renderData.thetaSpan / (count);

			BranchRenderData newData = { brdx, brdy, brdangle, brdradius,
				renderData.circles, renderData.lines, renderData.textData, renderData.step, x, y, brdThetaSpawn, renderData.extantBranches };


			drawBranch(branch->children[j], newData);
		}
	}


	// draw branch node
	Circle node = { x, y, size, branch->branchData.color };

	if (renderData.extantBranches.find(thisBranch) != renderData.extantBranches.end()) {
		// if circle is to be rendered, update color to new average
		b2Vec3 newAvgColor = renderData.extantBranches[thisBranch];
		node.color = newAvgColor;
	}
	else {
		// if extinct, render as black
		node.color = b2Vec3(0, 0, 0);
	}
	renderData.circles.push_back(node);

	// draw line connecting to parent node if possible
	if (hasParent) {
		Line line = { x, y, renderData.px, renderData.py,
			1.0f, b2Vec3(0, 0, 0) };

		renderData.lines.push_back(line);
	}

	return doesLineageSurvive;
}