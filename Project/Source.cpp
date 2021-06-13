#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Light.h"

typedef unsigned char byte;
#define SCR_WIDTH 1920
#define SCR_HEIGHT 1080

struct ModelTransform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	void setScale(float s)
	{
		scale.x = s;
		scale.y = s;
		scale.z = s;
	}
};

struct Material
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
};

Camera camera(glm::vec3(-1.5f, 0.2f, 0.8f), glm::vec3(0.f, 1.0f, 0.f), -30, 0);

Light* flashLight, * sunLight;
bool wireframeMode = false, boxMode = false, cameraRotationMode = false;
bool mouseLeftPress, mouseRightPress, mouseMiddlePress;
bool meteorAlarm = false, meteorCollide = false, ISScolapse = false;
float cameraAngleX, cameraAngleY;
double mouseX = SCR_WIDTH / 2, mouseY = SCR_HEIGHT / 2, mouseXtmp = 0, mouseYtmp = 0;

void UpdatePolygoneMode();
unsigned int loadCubemap(vector<std::string> faces);
void processInput(GLFWwindow* win, double dt);
void OnKeyAction(GLFWwindow* win, int key, int scancode, int action, int mods);
void OnMouseKeyAction(GLFWwindow* win, int button, int action, int mods);
void OnMouseMoutionAction(GLFWwindow* win, double x, double y);
void OnScroll(GLFWwindow* win, double x, double y);
void OnResize(GLFWwindow* win, int width, int height);
unsigned int loadTexture(char const* path, bool gammaCorrection);
void renderCube();
void renderQuad();



ModelTransform meteorTrans;
ModelTransform earthTrans;
ModelTransform moonTrans;

int main()
{
#pragma region WINDOW INITIALIZATION
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* win = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Window", NULL, NULL);
	if (win == NULL)
	{
		std::cout << "Error. Couldn't create window!" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(win);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Error. Couldn't load GLAD!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetFramebufferSizeCallback(win, OnResize);
	glfwSetScrollCallback(win, OnScroll);
	glfwSetKeyCallback(win, OnKeyAction);
	glfwSetMouseButtonCallback(win, OnMouseKeyAction);
	glfwSetCursorPosCallback(win, OnMouseMoutionAction);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	UpdatePolygoneMode();
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#pragma endregion

#pragma region BUFFERS INITIALIZATION

	// Конфигурирование фреймбуферов (типа с плавающей точкой)
	unsigned int hdrFBO;
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

	// Создаем 2 цветовых фреймбуфера типа с плавающей точкой (первый - для обычного рендеринга, другой - для граничных значений яркости)
	unsigned int colorBuffers[2];
	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  //используем режим GL_CLAMP_TO_EDGE, т.к. в противном случае фильтр размытия производил бы выборку повторяющихся значений текстуры!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Прикрепляем текстуру к фреймбуферу
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
	}

	// Создаем и прикрепляем буфер глубины (рендербуфер)
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// Сообщаем OpenGL, какой прикрепленный цветовой буфер мы будем использовать для рендеринга
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	// Проверяем готовность фреймбуфера
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ping-pong-фреймбуфер для размытия
	unsigned int pingpongFBO[2];
	unsigned int pingpongColorbuffers[2];
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongColorbuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // используем режим GL_CLAMP_TO_EDGE, т.к. в противном случае фильтр размытия производил бы выборку повторяющихся значений текстуры!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

		// Также проверяем, готовы ли фреймбуферы (буфер глубины нам не нужен)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
	const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	// Создаем текстуру кубической карты глубины
	unsigned int depthCubemap;
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for (unsigned int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Прикрепляем текстуру глубины в качестве буфера глубины для FBO
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion

#pragma region LIGHT INITIALIZATION

	vector<Light*> lights;
	int active_lights = 0;

	sunLight = new Light("Sun", true);
	sunLight->initLikePointLight(
		glm::vec3(-10.f, 4.90f, -4.90f),	//position
		glm::vec3(0.001f, 0.001f, 0.001f),	//ambient
		glm::vec3(0.9f, 0.9f, .8f),			//diffuse
		glm::vec3(0.0f, 0.0f, 0.0f),		//specular
		1.0f, 0.f, 0.0f);
	lights.push_back(sunLight);

	flashLight = new Light("FlashLight", true);
	flashLight->initLikeSpotLight(
		glm::vec3(0.0f, 0.0f, 0.0f),	//position
		glm::vec3(0.0f, 0.0f, 0.0f),	//direction
		glm::radians(5.f),
		glm::vec3(0.f, 0.f, 0.f),		//ambient
		glm::vec3(0.3f, 0.3f, 0.1f),	//diffuse
		glm::vec3(0.8f, 0.8f, 0.6f),	//specular
		1.0f, 0.1f, 0.09f);
	//lights.push_back(flashLight);

	ModelTransform lightTrans = {
	glm::vec3(0.f, 0.f, 0.f),			// position
	glm::vec3(0.f, 0.f, 0.f),			// rotation
	glm::vec3(.5f, .5f, .5f) };			// scale
#pragma endregion

#pragma region OBJECTS INITIALIZATION

#pragma region CUBES
	int cubeMat = 0;
	Material cubeMaterials[3] = {
		{
			glm::vec3(0.25, 0.20725, 0.20725),
			glm::vec3(1, 0.829, 0.829),
			glm::vec3(0.296648,	0.296648, 0.296648),
			12.f
		}, // pearl
		{
			glm::vec3(0.25, 0.25, 0.25),
			glm::vec3(0.4, 0.4, 0.4),
			glm::vec3(0.774597,	0.774597, 0.774597),
			77.f
		}, // chrome
		{
			glm::vec3(0.1745, 0.01175, 0.01175),
			glm::vec3(0.61424, 0.04136, 0.04136),
			glm::vec3(0.727811, 0.626959, 0.626959),
			77.f
		} // ruby
	};

	ModelTransform cubeTrans = {
	glm::vec3(0.f, 0.f, 0.f),				// position
	glm::vec3(0.f, 0.f, 0.f),				// rowatation
	glm::vec3(0.001f, 0.001f, 0.001f) };	// scale

	unsigned int box_texture = loadTexture("res\\images\\box.png", true);
#pragma endregion
	Model ISS("res/models/ISS/ISS.obj", true);

	ModelTransform ISSTrans = {
	glm::vec3(-0.45f, 0.3f, 0.f),		// position
	glm::vec3(0.f, 0.f, 90.f),			// rotation
	glm::vec3(0.01f, 0.01f, 0.01f) };	// scale

	Model moon("res/models/moon/moon.obj", true);

	moonTrans = {
	glm::vec3(0.f, 0.2f, 0.f),			// position
	glm::vec3(0.f, 0.f, 0.f),			// rotation
	glm::vec3(0.2f, 0.2f, 0.2f) };		// scale

	//earth
	Model earth("res/models/earth/earth.obj", true);

	earthTrans = {
	glm::vec3(0.f, 0.f, 0.f),			// position
	glm::vec3(0.f, 0.f, -10.f),			// rotation
	glm::vec3(0.1f, 0.1f, 0.1f) };		// scale

	Model meteor("res/models/meteorite/meteoriteobj.obj", true);

	meteorTrans = {
	glm::vec3(-0.5f, 0.f, -0.5f),		// position
	glm::vec3(0.f, 0.f, 0.f),			// rotation
	glm::vec3(0.01f, 0.01f, 0.01f) };	// scale

	glm::vec3 direction = glm::vec3(0.f, 0.f, 0.f);
	//skybox
	vector<std::string> skyboxTexFaces
	{
		"res\\skyboxes\\space\\right.jpg",
		"res\\skyboxes\\space\\left.jpg",
		"res\\skyboxes\\space\\top.jpg",
		"res\\skyboxes\\space\\bottom.jpg",
		"res\\skyboxes\\space\\front.jpg",
		"res\\skyboxes\\space\\back.jpg"
	};
	unsigned int cubemapTexture = loadCubemap(skyboxTexFaces);
#pragma endregion

#pragma region SHADERS INITIALIZATION
	Shader* basic_shader = new Shader("shaders/basic.vert", "shaders/basic.frag");
	Shader* light_shader = new Shader("shaders/light.vert", "shaders/light.frag");
	Shader* model_shader = new Shader("shaders/model.vert", "shaders/model.frag");
	Shader* model_exp_shader = new Shader("shaders/model.vert", "shaders/model_exp.frag", "shaders/explode.geom");
	Shader* skybox_shader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");
	Shader* shaderBlur = new Shader("shaders/blur.vert", "shaders/blur.frag");
	Shader* shaderBloomFinal = new Shader("shaders/bloom_final.vert", "shaders/bloom_final.frag");
	Shader* simpleDepthShader = new Shader("shaders/point_shadows_depth.vert", "shaders/point_shadows_depth.frag", "shaders/point_shadows_depth.geom");

	basic_shader->use();
	basic_shader->setInt("ourTexture", 0);
	basic_shader->setInt("depthMap", 1);
	shaderBlur->use();
	shaderBlur->setInt("image", 0);
	shaderBloomFinal->use();
	shaderBloomFinal->setInt("scene", 0);
	shaderBloomFinal->setInt("bloomBlur", 1);
#pragma endregion


	double oldTime = glfwGetTime(), newTime, deltaTime;
	while (!glfwWindowShouldClose(win))
	{
#pragma region LOGIC
		newTime = glfwGetTime();
		deltaTime = newTime - oldTime;
		oldTime = newTime;

		processInput(win, deltaTime);

		flashLight->position = camera.Position - camera.Up * 0.01f;
		flashLight->direction = camera.Front;

		ISSTrans.rotation.x >= 360 ? ISSTrans.rotation.x -= 360 - 0.05f : ISSTrans.rotation.x += 0.05f;

		moonTrans.position.x = 1.f * cosf(float(newTime));
		moonTrans.position.z = 1.f * sinf(float(newTime));
		moonTrans.rotation.y >= 360 ? moonTrans.rotation.y -= 360 - 0.05f : moonTrans.rotation.y += 0.05f;


		earthTrans.rotation.y >= 360 ? earthTrans.rotation.y -= 360 - 0.1f : earthTrans.rotation.y += 0.1f;

		if (meteorAlarm)
		{
			static glm::vec3 tmpPosition;
			if (!meteorCollide)
			{
				if (glm::length(meteorTrans.position - earthTrans.position) < 0.05f + 0.42f || glm::length(meteorTrans.position - moonTrans.position) < 0.22f + 0.05f)
				{
					cout << glm::length(meteorTrans.position - earthTrans.position) << " " << glm::length(meteorTrans.position - moonTrans.position) << endl;
					tmpPosition = meteorTrans.position;
					meteorCollide = true;
				}
				if (glm::length(meteorTrans.position - ISSTrans.position) < 0.05f + 0.1f)
				{
					ISScolapse = true;
					direction = (meteorTrans.position - earthTrans.position);
					meteorTrans.position -= direction * (float)deltaTime * 0.1f;
				}
				else
				{

					direction = (meteorTrans.position - ISSTrans.position);
					meteorTrans.position -= direction * (float)deltaTime * 0.5f;
				}
				meteorTrans.rotation.x >= 360 ? meteorTrans.rotation.x -= 360 - 0.05f : meteorTrans.rotation.x += 0.05f;
				meteorTrans.rotation.y >= 360 ? meteorTrans.rotation.y -= 360 - 0.01f : meteorTrans.rotation.y += 0.01f;
				meteorTrans.rotation.z >= 360 ? meteorTrans.rotation.z -= 360 - 0.10f : meteorTrans.rotation.z += 0.10f;
			}
			else
			{
				meteorTrans.position.x = 0.42f * sinf(earthTrans.rotation.y / 180 * glm::pi<float>());
				meteorTrans.position.z = 0.42f * cosf(earthTrans.rotation.y / 180 * glm::pi<float>());
				cout << meteorTrans.position.x << " " << meteorTrans.position.z << endl;
				meteorTrans.rotation = earthTrans.rotation;
			}


		}

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#pragma endregion

#pragma region DEPTH MAP RENDERING
		float near_plane = 1.0f;
		float far_plane = 100.0f;
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
		std::vector<glm::mat4> shadowTransforms;
		for (int i = 0; i < lights.size(); i++)
		{
			shadowTransforms.push_back(shadowProj * glm::lookAt(lights[i]->position, lights[i]->position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lights[i]->position, lights[i]->position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lights[i]->position, lights[i]->position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lights[i]->position, lights[i]->position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lights[i]->position, lights[i]->position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lights[i]->position, lights[i]->position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
		}

		// Рендерим сцену в кубическую карту глубины
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		simpleDepthShader->use();
		for (unsigned int i = 0; i < 6; ++i)
			simpleDepthShader->setMatrix4F("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
		simpleDepthShader->setFloat("far_plane", far_plane);
		for (int i = 0; i < lights.size(); i++)
			simpleDepthShader->setVec3("lightPos", lights[i]->position);
		glm::mat4 model;

		// DRAWING MOON
		model = glm::mat4(1.0f);
		model = glm::translate(model, moonTrans.position);
		model = glm::rotate(model, glm::radians(moonTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(moonTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(moonTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, moonTrans.scale);

		simpleDepthShader->setMatrix4F("model", model);
		moon.Draw(simpleDepthShader);

		//model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));
		//simpleDepthShader->setMatrix4F("model", model);
		//renderCube();

		// DRAWING ISS
		model = glm::mat4(1.0f);
		model = glm::translate(model, ISSTrans.position);
		model = glm::rotate(model, glm::radians(ISSTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(ISSTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(ISSTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, ISSTrans.scale);

		simpleDepthShader->setMatrix4F("model", model);
		ISS.Draw(simpleDepthShader);


		// DRAWING EARTH
		model = glm::mat4(1.0f);
		model = glm::translate(model, earthTrans.position);
		model = glm::rotate(model, glm::radians(earthTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(earthTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(earthTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, earthTrans.scale);

		if (!boxMode)
		{
			model = glm::scale(model, glm::vec3(3.f, 3.f, 3.f));
			simpleDepthShader->setMatrix4F("model", model);
			earth.Draw(simpleDepthShader);
		}
		else {
			model = glm::scale(model, glm::vec3(3.f, 3.f, 3.f));
			simpleDepthShader->setMatrix4F("model", model);
			renderCube();
		}
		if (meteorAlarm)
		{
			// DRAWING METEOR
			model = glm::mat4(1.0f);
			model = glm::translate(model, meteorTrans.position);
			model = glm::rotate(model, glm::radians(meteorTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
			model = glm::rotate(model, glm::radians(meteorTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
			model = glm::rotate(model, glm::radians(meteorTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
			model = glm::scale(model, meteorTrans.scale);

			simpleDepthShader->setMatrix4F("model", model);
			meteor.Draw(simpleDepthShader);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region NORMAL RENDERING 
		// Рендерим сцену как обычно
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 p = camera.GetProjectionMatrix();
		glm::mat4 v = camera.GetViewMatrix();
		if (cameraRotationMode) {
			v = glm::rotate(v, glm::radians(cameraAngleX), glm::vec3(0.f, 1.f, 0.f));
			v = glm::rotate(v, glm::radians(cameraAngleY), glm::vec3(0.f, 1.f, 0.f));
		}
		glm::mat4 pv = p * v;

		// DRAWING MOON
		model = glm::mat4(1.0f);
		model = glm::translate(model, moonTrans.position);
		model = glm::rotate(model, glm::radians(moonTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(moonTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(moonTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, moonTrans.scale);

		if (!boxMode)
		{
			model_shader->use();
			model_shader->setMatrix4F("pv", pv);
			model_shader->setMatrix4F("model", model);
			model_shader->setVec3("viewPos", camera.Position);

			active_lights = 0;
			for (int i = 0; i < lights.size(); i++)
				active_lights += lights[i]->putInShader(model_shader, active_lights);
			model_shader->setInt("lights_count", active_lights);

			model_shader->setFloat("far_plane", far_plane);
			model_shader->setBool("shadows", true);
			model_shader->setBool("blur", false);


			moon.Draw(model_shader);
		}
		else
		{
			// DRAWING MOON-BOX
			basic_shader->use();
			basic_shader->setMatrix4F("pv", pv);
			basic_shader->setVec3("viewPos", camera.Position);
			active_lights = 0;
			for (int i = 0; i < lights.size(); i++)
				active_lights += lights[i]->putInShader(basic_shader, active_lights);
			model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));
			basic_shader->setInt("lights_count", active_lights);
			basic_shader->setMatrix4F("model", model);
			basic_shader->setFloat("far_plane", far_plane);
			basic_shader->setVec3("material.ambient", cubeMaterials[cubeMat].ambient);
			basic_shader->setVec3("material.diffuse", cubeMaterials[cubeMat].diffuse);
			basic_shader->setVec3("material.specular", cubeMaterials[cubeMat].specular);
			basic_shader->setFloat("material.shininess", cubeMaterials[cubeMat].shininess);
			basic_shader->setBool("shadows", true);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, box_texture);
			renderCube();
		}

		// DRAWING ISS
		model = glm::mat4(1.0f);
		model = glm::translate(model, ISSTrans.position);
		model = glm::rotate(model, glm::radians(ISSTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(ISSTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(ISSTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, ISSTrans.scale);

		model_exp_shader->use();
		model_exp_shader->setMatrix4F("pv", pv);
		model_exp_shader->setMatrix4F("model", model);
		model_exp_shader->setVec3("viewPos", camera.Position);

		active_lights = 0;
		for (int i = 0; i < lights.size(); i++)
			active_lights += lights[i]->putInShader(model_exp_shader, active_lights);
		model_exp_shader->setInt("lights_count", active_lights);

		model_exp_shader->setFloat("far_plane", far_plane);
		model_exp_shader->setBool("shadows", true);
		model_exp_shader->setBool("blur", false);
		model_exp_shader->setBool("collapse", ISScolapse);
		static float blow = 0;
		if (ISScolapse)
		{
			if (blow < glm::pi<float>() / 2)
				blow += 0.005f;
			model_exp_shader->setFloat("blow", blow);
		}
		else
			blow = 0;

		ISS.Draw(model_exp_shader);

		// DRAWING EARTH
		model = glm::mat4(1.0f);
		model = glm::translate(model, earthTrans.position);
		model = glm::rotate(model, glm::radians(earthTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		model = glm::rotate(model, glm::radians(earthTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		model = glm::rotate(model, glm::radians(earthTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::scale(model, earthTrans.scale);

		if (boxMode)
		{
			// DRAWING EARTH-BOX
			basic_shader->use();
			basic_shader->setMatrix4F("pv", pv);
			basic_shader->setVec3("viewPos", camera.Position);
			active_lights = 0;
			for (int i = 0; i < lights.size(); i++)
				active_lights += lights[i]->putInShader(basic_shader, active_lights);
			model = glm::scale(model, glm::vec3(3.f, 3.f, 3.f));
			basic_shader->setInt("lights_count", active_lights);
			basic_shader->setMatrix4F("model", model);
			basic_shader->setFloat("far_plane", far_plane);
			basic_shader->setVec3("material.ambient", cubeMaterials[cubeMat].ambient);
			basic_shader->setVec3("material.diffuse", cubeMaterials[cubeMat].diffuse);
			basic_shader->setVec3("material.specular", cubeMaterials[cubeMat].specular);
			basic_shader->setFloat("material.shininess", cubeMaterials[cubeMat].shininess);
			basic_shader->setBool("shadows", true);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, box_texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
			renderCube();
		}

		if (!boxMode || meteorAlarm)
		{
			model_shader->use();
			model_shader->setMatrix4F("pv", pv);
			model_shader->setMatrix4F("model", model);
			model_shader->setVec3("viewPos", camera.Position);

			active_lights = 0;
			for (int i = 0; i < lights.size(); i++)
				active_lights += lights[i]->putInShader(model_shader, active_lights);

			model_shader->setInt("lights_count", active_lights);

			model_shader->setFloat("far_plane", far_plane);
			model_shader->setBool("shadows", true);
			model_shader->setBool("blur", true);

			if (!boxMode)
				earth.Draw(model_shader);
			if (meteorAlarm)
			{
				model = glm::mat4(1.0f);
				model = glm::translate(model, meteorTrans.position);
				model = glm::rotate(model, glm::radians(meteorTrans.rotation.x), glm::vec3(1.f, 0.f, 0.f));
				model = glm::rotate(model, glm::radians(meteorTrans.rotation.y), glm::vec3(0.f, 1.f, 0.f));
				model = glm::rotate(model, glm::radians(meteorTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
				model = glm::scale(model, meteorTrans.scale);

				model_shader->setMatrix4F("model", model);
				meteor.Draw(model_shader);
			}
		}
#pragma region BACKGROUND
		// DRAWING SKYBOX (as last)
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		glCullFace(GL_FRONT);

		v = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
		if (cameraRotationMode) {
			v = glm::rotate(v, glm::radians(cameraAngleX), glm::vec3(0.f, 1.f, 0.f));
			v = glm::rotate(v, glm::radians(cameraAngleY), glm::vec3(0.f, 1.f, 0.f));
		}
		pv = p * v;
		skybox_shader->use();
		skybox_shader->setMatrix4F("pv", pv);

		// skybox cube
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		renderCube();
		glCullFace(GL_BACK);

		if (boxMode)
		{
			// DRAWING LAMPS
			light_shader->use();
			light_shader->setMatrix4F("pv", pv);

			// Sun
			lightTrans.position = sunLight->position;
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightTrans.position);
			//model = glm::rotate(model, glm::radians(lightTrans.rotation.y >= 360 ? lightTrans.rotation.y -= 360.f - 10.f : lightTrans.rotation.y += 10.f), glm::vec3(0.f, 1.f, 0.f));

			model = glm::scale(model, lightTrans.scale);
			light_shader->setMatrix4F("model", model);
			light_shader->setVec3("lightColor", glm::vec3(1.f, 1.f, 1.f));
			renderCube();
		}

		glDepthFunc(GL_LESS); // set depth function back to default
#pragma endregion
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#pragma endregion

#pragma region FINAL RENDERING
		// Размываем яркие фрагменты с помощью двухпроходного размытия по Гауссу
		bool horizontal = true, first_iteration = true;
		unsigned int amount = 40;
		shaderBlur->use();
		for (unsigned int i = 0; i < amount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			shaderBlur->setInt("horizontal", horizontal);
			glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // привязка текстуры другого фреймбуфера (или сцены, если это - первая итерация)
			renderQuad();
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.5f, 0.5f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Теперь рендерим цветовой буфер (типа с плавающей точкой) на 2D-прямоугольник и сужаем диапазон значений HDR-цветов к цветовому диапазону значений заданного по умолчанию фреймбуфера
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderBloomFinal->use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
		renderQuad();
#pragma endregion

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	delete basic_shader;
	delete light_shader;
	delete model_shader;
	delete model_exp_shader;
	delete skybox_shader;
	delete shaderBlur;
	delete shaderBloomFinal;
	delete simpleDepthShader;
}





void OnResize(GLFWwindow* win, int width, int height)
{
	glViewport(0, 0, width, height);
}

void UpdatePolygoneMode()
{
	if (wireframeMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void processInput(GLFWwindow* win, double dt)
{
	if (glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS)
	{
		cout << camera.Position.x << " " << camera.Position.y << " " << camera.Position.z << endl;
		cout << camera.Yaw << " " << camera.Pitch << endl;
	}

	uint32_t dir = 0;

	if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		dir |= CAM_UP;
	if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		dir |= CAM_DOWN;
	if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
		dir |= CAM_FORWARD;
	if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
		dir |= CAM_BACKWARD;
	if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
		dir |= CAM_LEFT;
	if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
		dir |= CAM_RIGHT;

	camera.Move(dir, float(dt));

}

void OnScroll(GLFWwindow* win, double x, double y)
{
	camera.ChangeFOV(y);
	std::cout << "Scrolled x: " << x << ", y: " << y << ". FOV = " << camera.Fov << std::endl;
}

void OnKeyAction(GLFWwindow* win, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(win, true);
			break;
		case GLFW_KEY_TAB:
			wireframeMode = !wireframeMode;
			UpdatePolygoneMode();
			break;
		case GLFW_KEY_LEFT_ALT:
			boxMode = !boxMode;
			break;
		case GLFW_KEY_C:
			ISScolapse = !ISScolapse;
			break;
		case GLFW_KEY_SPACE:
			do
			{
				meteorTrans.position = glm::vec3((rand() % 20 - 10) / 10.f, 0.f, (rand() % 20 - 10) / 10.f);
			} while (glm::length(meteorTrans.position - earthTrans.position) < 0.05f + 0.45f + .5f
				|| glm::length(meteorTrans.position - moonTrans.position) < 0.05f + 0.22f + .5f);
			meteorAlarm = true;
			meteorCollide = false;
			ISScolapse = false;
			break;
		}
	}
}

void OnMouseKeyAction(GLFWwindow* win, int button, int action, int mods)
{

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			cameraRotationMode = !cameraRotationMode;
			glfwGetCursorPos(win, &mouseXtmp, &mouseYtmp);
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPos(win, SCR_HEIGHT / 2, SCR_WIDTH / 2);
			mouseLeftPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			cameraRotationMode = !cameraRotationMode;
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPos(win, mouseXtmp, mouseYtmp);
			mouseX = SCR_HEIGHT / 2;
			mouseY = SCR_WIDTH / 2;
			mouseLeftPress = false;
		}
	}

	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(win, &mouseXtmp, &mouseYtmp);
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPos(win, SCR_HEIGHT / 2, SCR_WIDTH / 2);
			mouseRightPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPos(win, mouseXtmp, mouseYtmp);
			mouseX = SCR_HEIGHT / 2;
			mouseY = SCR_WIDTH / 2;
			mouseRightPress = false;
		}
	}

	else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS)
		{
			mouseMiddlePress = true;
		}
		else if (action == GLFW_RELEASE)
			mouseMiddlePress = false;
	}
}

void OnMouseMoutionAction(GLFWwindow* win, double x, double y)
{
	if (mouseLeftPress)
	{
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		double newx = 0.f, newy = 0.f;
		glfwGetCursorPos(win, &newx, &newy);
		cameraAngleX += float(newx - mouseX);
		cameraAngleY += float(newy - mouseY);
		mouseX = newx;
		mouseY = newy;
	}
	if (mouseRightPress)
	{
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		double newx = 0.f, newy = 0.f;
		glfwGetCursorPos(win, &newx, &newy);
		float xoffset = float(newx - mouseX);
		float yoffset = float(newy - mouseY);
		mouseX = newx;
		mouseY = newy;

		camera.Rotate(xoffset, -yoffset);
	}
}

unsigned int loadCubemap(vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int loadTexture(char const* path, bool gammaCorrection)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum internalFormat = 0;
		GLenum dataFormat = 0;
		if (nrComponents == 1)
		{
			internalFormat = dataFormat = GL_RED;
		}
		else if (nrComponents == 3)
		{
			internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		}
		else if (nrComponents == 4)
		{
			internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
			dataFormat = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void renderCube()
{
	static unsigned int cubeVAO = 0;
	static unsigned int cubeVBO = 0;
	// Инициализация (если необходимо)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// задняя грань
		   -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // нижняя-левая
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // верхняя-правая
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // нижняя-правая         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // верхняя-правая
		   -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // нижняя-левая
		   -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // верхняя-левая

			// передняя грань
		   -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // нижняя-левая
			1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // нижняя-правая
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // верхняя-правая
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // верхняя-правая
		   -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // верхняя-левая
		   -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // нижняя-левая

			// грань слева
		   -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // верхняя-правая
		   -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // верхняя-левая
		   -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // нижняя-левая
		   -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // нижняя-левая
		   -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // нижняя-правая
		   -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // верхняя-правая

			// грань справа
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // верхняя-левая
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // нижняя-правая
			1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // верхняя-правая         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // нижняя-правая
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // верхняя-левая
			1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // нижняя-левая     

			// нижняя грань
		   -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // верхняя-правая
			1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // верхняя-левая
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // нижняя-левая
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // нижняя-левая
		   -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // нижняя-правая
		   -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // верхняя-правая

			// верхняя грань
		   -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // верхняя-левая
			1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // нижняя-правая
			1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // верхняя-правая     
			1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // нижняя-правая
		   -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // верхняя-левая
		   -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // нижняя-левая        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);

		// Заполняем буфер
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Связываем вершинные атрибуты
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	// Рендер ящика
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void renderQuad()
{
	static unsigned int quadVAO = 0;
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	static unsigned int quadVBO;
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// координаты      // текстурные коодинаты
		   -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		   -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		// Установка VAO плоскости
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	if (wireframeMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}