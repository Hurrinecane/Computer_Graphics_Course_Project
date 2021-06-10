#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>

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

Camera camera(glm::vec3(-1.f, 0.f, 0.5f), glm::vec3(0.f, 1.0f, 0.f), -30, 0);

Light* flashLight, * sunLight;
bool wireframeMode = false;

double mouseX = SCR_WIDTH / 2, mouseY = SCR_HEIGHT / 2, mouseXtmp = 0, mouseYtmp = 0;
bool mouseLeftPress, mouseRightPress, mouseMiddlePress;

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

bool mode = 0;
float cameraAngleX, cameraAngleY;


ModelTransform meteorTrans;

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

	// ���������������� ������������ (���� � ��������� ������)
	unsigned int hdrFBO;
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

	// ������� 2 �������� ����������� ���� � ��������� ������ (������ - ��� �������� ����������, ������ - ��� ��������� �������� �������)
	unsigned int colorBuffers[2];
	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  //���������� ����� GL_CLAMP_TO_EDGE, �.�. � ��������� ������ ������ �������� ���������� �� ������� ������������� �������� ��������!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// ����������� �������� � �����������
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
	}

	// ������� � ����������� ����� ������� (�����������)
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// �������� OpenGL, ����� ������������� �������� ����� �� ����� ������������ ��� ����������
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	// ��������� ���������� �����������
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ping-pong-���������� ��� ��������
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // ���������� ����� GL_CLAMP_TO_EDGE, �.�. � ��������� ������ ������ �������� ���������� �� ������� ������������� �������� ��������!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

		// ����� ���������, ������ �� ����������� (����� ������� ��� �� �����)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
#pragma endregion

#pragma region LIGHT INITIALIZATION

	vector<Light*> lights;
	int active_lights = 0;

	sunLight = new Light("Sun", true);
	sunLight->initLikePointLight(
		glm::vec3(-0.9f, 0.445f, -0.44f),	//position
		glm::vec3(0.001f, 0.001f, 0.001f),			//ambient
		glm::vec3(1.f, 1.f, .8f),			//diffuse
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
	glm::vec3(0.05f, 0.05f, 0.05f) };	// scale
#pragma endregion

#pragma region OBJECTS INITIALIZATION

#pragma region CUBES
	int cubeMat = 2;
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

	//Model meteor("res/models/meteorite/meteoriteobj.obj", true);
	//
	//meteorTrans = {
	//glm::vec3(-0.5f, 0.f, -0.5f),		// position
	//glm::vec3(0.f, 0.f, 0.f),			// rotation
	//glm::vec3(0.01f, 0.01f, 0.01f) };	// scale
	//
	//glm::vec3 direction = glm::vec3(0.f, 0.f, 0.f);

	//earth
	Model earth("res/models/earth/earth.obj", true);

	ModelTransform earthTrans = {
	glm::vec3(0.f, 0.f, 0.f),		// position
	glm::vec3(0.f, 0.f, -10.f),		// rotation
	glm::vec3(0.1f, 0.1f, 0.1f) };	// scale

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
	Shader* polygon_shader = new Shader("shaders/basic.vert", "shaders/basic.frag");
	Shader* light_shader = new Shader("shaders/light.vert", "shaders/light.frag");
	Shader* earth_shader = new Shader("shaders/earth.vert", "shaders/earth.frag");
	Shader* skybox_shader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");
	Shader* shaderBlur = new Shader("shaders/7.blur.vert", "shaders/7.blur.frag");
	Shader* shaderBloomFinal = new Shader("shaders/7.bloom_final.vert", "shaders/7.bloom_final.frag");

	shaderBlur->use();
	shaderBlur->setInt("image", 0);
	shaderBloomFinal->use();
	shaderBloomFinal->setInt("scene", 0);
	shaderBloomFinal->setInt("bloomBlur", 1);
#pragma endregion





	double oldTime = glfwGetTime(), newTime, deltaTime;

	while (!glfwWindowShouldClose(win))
	{
		newTime = glfwGetTime();
		deltaTime = newTime - oldTime;
		oldTime = newTime;

		processInput(win, deltaTime);

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 p = camera.GetProjectionMatrix();
		glm::mat4 v = camera.GetViewMatrix();
		if (mode) {
			v = glm::rotate(v, glm::radians(cameraAngleX), glm::vec3(0.f, 1.f, 0.f));
			v = glm::rotate(v, glm::radians(cameraAngleY), glm::vec3(0.f, 1.f, 0.f));
		}
		glm::mat4 pv = p * v;
		glm::mat4 model;

		flashLight->position = camera.Position - camera.Up * 0.01f;
		flashLight->direction = camera.Front;

		// DRAWING EARTH
		model = glm::mat4(1.0f);
		model = glm::translate(model, earthTrans.position);
		model = glm::rotate(model, glm::radians(earthTrans.rotation.z), glm::vec3(0.f, 0.f, 1.f));
		model = glm::rotate(model, glm::radians(earthTrans.rotation.y >= 360 ? earthTrans.rotation.y -= 360 - 0.1f : earthTrans.rotation.y += 0.1f), glm::vec3(0.f, 1.f, 0.f));
		model = glm::scale(model, earthTrans.scale);
		earth_shader->use();
		earth_shader->setMatrix4F("pv", pv);
		earth_shader->setMatrix4F("model", model);
		earth_shader->setVec3("viewPos", camera.Position);

		active_lights = 0;
		for (int i = 0; i < lights.size(); i++)
			active_lights += lights[i]->putInShader(earth_shader, active_lights);

		earth_shader->setInt("lights_count", active_lights);
		earth.Draw(earth_shader);

		//model = glm::mat4(1.0f);
		//direction = (meteorTrans.position - earthTrans.position);
		//model = glm::translate(model, meteorTrans.position -= direction * (float)deltaTime * 0.5f);
		//model = glm::rotate(model, glm::radians(meteorTrans.rotation.y >= 360 ? meteorTrans.rotation.x -= 360 - 0.1f : meteorTrans.rotation.x += 0.05f), glm::vec3(1.f, 0.f, 0.f));
		//model = glm::rotate(model, glm::radians(meteorTrans.rotation.y >= 360 ? meteorTrans.rotation.y -= 360 - 0.1f : meteorTrans.rotation.y += 0.01f), glm::vec3(0.f, 1.f, 0.f));
		//model = glm::rotate(model, glm::radians(meteorTrans.rotation.z >= 360 ? meteorTrans.rotation.z -= 360 - 0.1f : meteorTrans.rotation.z += 0.1f), glm::vec3(0.f, 0.f, 1.f));
		//model = glm::scale(model, meteorTrans.scale);
		//
		//earth_shader->setMatrix4F("model", model);
		//meteor.Draw(earth_shader);

		// DRAWING BOX
		//polygon_shader->use();
		//polygon_shader->setMatrix4F("pv", pv);
		//polygon_shader->setVec3("viewPos", camera.Position);
		//
		//active_lights = 0;
		//for (int i = 0; i < lights.size(); i++)
		//	active_lights += lights[i]->putInShader(polygon_shader, active_lights);
		//polygon_shader->setInt("lights_count", active_lights);
		//
		//model = glm::scale(model, glm::vec3(3.f, 3.f, 3.f));
		//
		//polygon_shader->setMatrix4F("model", model);
		//
		//polygon_shader->setVec3("material.ambient", cubeMaterials[cubeMat].ambient);
		//polygon_shader->setVec3("material.diffuse", cubeMaterials[cubeMat].diffuse);
		//polygon_shader->setVec3("material.specular", cubeMaterials[cubeMat].specular);
		//polygon_shader->setFloat("material.shininess", cubeMaterials[cubeMat].shininess);
		//
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, box_texture);
		//renderCube();

#pragma region background
		// draw skybox as last
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		glCullFace(GL_FRONT);

		v = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
		if (mode) {
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

		// DRAWING LAMPS
		light_shader->use();
		light_shader->setMatrix4F("pv", pv);

		// Sun
		lightTrans.position = sunLight->position;
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightTrans.position);
		model = glm::rotate(model, glm::radians(lightTrans.rotation.z >= 360 ? lightTrans.rotation.z -= 360.f + 20.f : lightTrans.rotation.z += 20.f), glm::vec3(0.f, 1.f, 0.f));

		model = glm::scale(model, lightTrans.scale);
		//model = glm::scale(model, glm::vec3(5.f, 5.f, 5.f));
		light_shader->setMatrix4F("model", model);
		light_shader->setVec3("lightColor", glm::vec3(1.f, 1.f, 1.f));
		renderCube();

		glCullFace(GL_BACK);
		glDepthFunc(GL_LESS); // set depth function back to default
#pragma endregion

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. ��������� ����� ��������� � ������� �������������� �������� �� ������
		bool horizontal = true, first_iteration = true;
		unsigned int amount = 40;
		shaderBlur->use();
		for (unsigned int i = 0; i < amount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			shaderBlur->setInt("horizontal", horizontal);
			glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // �������� �������� ������� ����������� (��� �����, ���� ��� - ������ ��������)
			renderQuad();
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 3. ������ �������� �������� ����� (���� � ��������� ������) �� 2D-������������� � ������ �������� �������� HDR-������ � ��������� ��������� �������� ��������� �� ��������� �����������
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderBloomFinal->use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
		shaderBloomFinal->setInt("bloom", true);
		shaderBloomFinal->setFloat("exposure", 10.f);
		renderQuad();

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	delete polygon_shader;
	delete earth_shader;

	glfwTerminate();
	return 0;
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
	if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(win, true);

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

	camera.Move(dir, dt);

	if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		meteorTrans.position = glm::vec3(-0.5f, 0.f, -0.5f);
	}
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
		case GLFW_KEY_TAB:
			wireframeMode = !wireframeMode;
			UpdatePolygoneMode();
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
			mode = !mode;
			glfwGetCursorPos(win, &mouseXtmp, &mouseYtmp);
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPos(win, SCR_HEIGHT / 2, SCR_WIDTH / 2);
			mouseLeftPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			mode = !mode;
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
		cameraAngleX += newx - mouseX;
		cameraAngleY += newy - mouseY;
		mouseX = newx;
		mouseY = newy;
	}
	if (mouseRightPress)
	{
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		double newx = 0.f, newy = 0.f;
		glfwGetCursorPos(win, &newx, &newy);
		double xoffset = newx - mouseX;
		double yoffset = newy - mouseY;
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
		GLenum internalFormat;
		GLenum dataFormat;
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
	// ������������� (���� ����������)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// ������ �����
		   -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // ������-�����
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // �������-������
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // ������-������         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // �������-������
		   -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // ������-�����
		   -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // �������-�����

			// �������� �����
		   -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // ������-�����
			1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // ������-������
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // �������-������
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // �������-������
		   -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // �������-�����
		   -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // ������-�����

			// ����� �����
		   -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // �������-������
		   -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // �������-�����
		   -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // ������-�����
		   -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // ������-�����
		   -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // ������-������
		   -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // �������-������

			// ����� ������
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // �������-�����
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // ������-������
			1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // �������-������         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // ������-������
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // �������-�����
			1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // ������-�����     

			// ������ �����
		   -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // �������-������
			1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // �������-�����
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // ������-�����
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // ������-�����
		   -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // ������-������
		   -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // �������-������

			// ������� �����
		   -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // �������-�����
			1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // ������-������
			1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // �������-������     
			1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // ������-������
		   -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // �������-�����
		   -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // ������-�����        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);

		// ��������� �����
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// ��������� ��������� ��������
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

	// ������ �����
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void renderQuad()
{
	static unsigned int quadVAO = 0;
	static unsigned int quadVBO;
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// ����������      // ���������� ���������
		   -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		   -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		// ��������� VAO ���������
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
}
