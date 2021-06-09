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

Camera camera(glm::vec3(-0.9f, 0.f, -0.44f), glm::vec3(0.f, 1.0f, 0.f), 20, 0);

Light* flashLight, * sunLight;
bool wireframeMode = false;
bool mouseLeftPress, mouseRightPress, mouseMiddlePress;

struct
{
	int HEIGHT;
	int WIDTH;
} RESOLUTION = { 1280, 720 };

double mouseX = RESOLUTION.HEIGHT / 2, mouseY = RESOLUTION.WIDTH / 2, mouseXtmp = 0, mouseYtmp = 0;

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

int main()
{
#pragma region WINDOW INITIALIZATION
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* win = glfwCreateWindow(RESOLUTION.HEIGHT, RESOLUTION.WIDTH, "OpenGL Window", NULL, NULL);
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
	glViewport(0, 0, RESOLUTION.HEIGHT, RESOLUTION.WIDTH);
	glEnable(GL_DEPTH_TEST);
	glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	UpdatePolygoneMode();
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#pragma endregion

	Shader* shader			= new Shader("shaders/7.bloom.vert", "shaders/7.bloom.frag");
	Shader* shaderLight		= new Shader("shaders/7.bloom.vert", "shaders/7.light_box.frag");
	Shader* shaderBlur		= new Shader("shaders/7.blur.vert", "shaders/7.blur.frag");
	Shader* shaderBloomFinal= new Shader("shaders/7.bloom_final.vert", "shaders/7.bloom_final.frag");

	Shader* polygon_shader	= new Shader("shaders\\basic.vert", "shaders\\basic.frag");
	Shader* light_shader	= new Shader("shaders\\light.vert", "shaders\\light.frag");
	Shader* earth_shader	= new Shader("shaders\\earth.vert", "shaders\\earth.frag");
	Shader* skybox_shader	= new Shader("shaders\\skybox.vert", "shaders\\skybox.frag");

#pragma region LIGHT INITIALIZATION

	vector<Light*> lights;
	int active_lights = 0;

	sunLight = new Light("Sun", true);
	sunLight->initLikePointLight(
		glm::vec3(-0.9f, -0.445f, -0.44f),	//position
		glm::vec3(0.3f, 0.3f, 0.3f),		//ambient
		glm::vec3(1.f, 1.f, .8f),			//diffuse
		glm::vec3(0.0f, 0.0f, 0.0f),		//specular
		1.0f, 0.f, 0.0f);
	lights.push_back(sunLight);

	flashLight = new Light("FlashLight", true);
	flashLight->initLikeSpotLight(
		glm::vec3(0.0f, 0.0f, 0.0f),	//position
		glm::vec3(0.0f, 0.0f, 0.0f),	//direction
		glm::radians(10.f),
		glm::vec3(0.2f, 0.2f, 0.2f),	//ambient
		glm::vec3(0.7f, 0.7f, 0.6f),	//diffuse
		glm::vec3(0.8f, 0.8f, 0.6f),	//specular
		1.0f, 0.1f, 0.09f);
	//lights.push_back(flashLight);


	ModelTransform lightTrans = {
	glm::vec3(0.f, 0.f, 0.f),			// position
	glm::vec3(0.f, 0.f, 0.f),			// rotation
	glm::vec3(0.05f, 0.05f, 0.05f) };	// scale
#pragma endregion


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
	;

#pragma endregion

	unsigned int box_texture = loadTexture("res\\images\\box.png", true);

	Model earth("res/models/earth/earth.obj", true);

	ModelTransform earthTrans = {
	glm::vec3(0.f, 0.f, 0.f),		// position
	glm::vec3(0.f, 0.f, 0.f),		// rotation
	glm::vec3(0.1f, 0.1f, 0.1f) };	// scale


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

	double oldTime = glfwGetTime(), newTime, deltaTime;



	shaderBlur->use();
	shaderBlur->setInt("image", 0);
	shaderBloomFinal->use();
	shaderBloomFinal->setInt("scene", 0);
	shaderBloomFinal->setInt("bloomBlur", 1);

	while (!glfwWindowShouldClose(win))
	{
		newTime = glfwGetTime();
		deltaTime = newTime - oldTime;
		oldTime = newTime;

		processInput(win, deltaTime);

		glClearColor(0.1f, .1f, .1f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 p = camera.GetProjectionMatrix();
		glm::mat4 v;
		switch (mode)
		{
		case 0:
			v = camera.GetViewMatrix();
			break;
		case 1:
			v = camera.GetViewMatrix();
			v = glm::rotate(v, glm::radians(cameraAngleX), glm::vec3(0.f, 1.f, 0.f));
			v = glm::rotate(v, glm::radians(cameraAngleY), glm::vec3(0.f, 1.f, 0.f));
			break;
		}
		glm::mat4 pv = p * v;
		glm::mat4 model;

		flashLight->position = camera.Position - camera.Up * 0.01f;
		flashLight->direction = camera.Front;

		// DRAWING EARTH
		model = glm::mat4(1.0f);
		model = glm::translate(model, earthTrans.position);
		model = glm::rotate(model, glm::radians(earthTrans.rotation.y > 360 ? earthTrans.rotation.y -= 360 : earthTrans.rotation.y += 0.1f), glm::vec3(0.f, 1.f, 0.f));
		model = glm::scale(model, earthTrans.scale);
		earth_shader->use();
		earth_shader->setMatrix4F("pv", pv);
		earth_shader->setMatrix4F("model", model);
		earth_shader->setVec3("viewPos", camera.Position);

		active_lights = 0;
		for (int i = 0; i < lights.size(); i++)
			active_lights += lights[i]->putInShader(earth_shader, active_lights);

		earth_shader->setInt("lights_count", active_lights);

		glEnable(GL_BLEND);
		earth.Draw(earth_shader);
		glDisable(GL_BLEND);

		// DRAWING BOX
		polygon_shader->use();
		polygon_shader->setMatrix4F("pv", pv);
		polygon_shader->setBool("wireframeMode", wireframeMode);
		polygon_shader->setVec3("viewPos", camera.Position);
		
		active_lights = 0;
		for (int i = 0; i < lights.size(); i++)
			active_lights += lights[i]->putInShader(polygon_shader, active_lights);
		polygon_shader->setInt("lights_count", active_lights);
		
		model = glm::scale(model, glm::vec3(3.f, 3.f, 3.f));
		
		polygon_shader->setMatrix4F("model", model);
		
		polygon_shader->setVec3("material.ambient", cubeMaterials[cubeMat].ambient);
		polygon_shader->setVec3("material.diffuse", cubeMaterials[cubeMat].diffuse);
		polygon_shader->setVec3("material.specular", cubeMaterials[cubeMat].specular);
		polygon_shader->setFloat("material.shininess", cubeMaterials[cubeMat].shininess);
		
		glBindTexture(GL_TEXTURE_2D, box_texture);
		renderCube();

#pragma region skybox 
		// draw skybox as last
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		glCullFace(GL_FRONT);

		v = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
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
		//model = glm::rotate(model, glm::radians(lightTrans.rotation.z == 360 ? lightTrans.rotation.z -= 360 : lightTrans.rotation.z += 10.f), glm::vec3(0.f, 1.f, 0.f));

		model = glm::scale(model, lightTrans.scale);
		model = glm::scale(model, glm::vec3(5.f, 5.f, 5.f));
		light_shader->setMatrix4F("model", model);
		light_shader->setVec3("lightColor", glm::vec3(1.0f, 1.f, 1.f));
		renderCube();
		
		glCullFace(GL_BACK);
		glDepthFunc(GL_LESS); // set depth function back to default
#pragma endregion

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	delete polygon_shader;

	glfwTerminate();
	return 0;
}





void OnResize(GLFWwindow* win, int width, int height)
{
	glViewport(0, 0, width, height);
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
			glfwSetCursorPos(win, RESOLUTION.HEIGHT / 2, RESOLUTION.WIDTH / 2);
			mouseLeftPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			mode = !mode;
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPos(win, mouseXtmp, mouseYtmp);
			mouseX = RESOLUTION.HEIGHT / 2;
			mouseY = RESOLUTION.WIDTH / 2;
			mouseLeftPress = false;
		}
	}

	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(win, &mouseXtmp, &mouseYtmp);
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPos(win, RESOLUTION.HEIGHT / 2, RESOLUTION.WIDTH / 2);
			mouseRightPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPos(win, mouseXtmp, mouseYtmp);
			mouseX = RESOLUTION.HEIGHT / 2;
			mouseY = RESOLUTION.WIDTH / 2;
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

void UpdatePolygoneMode()
{
	if (wireframeMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ��������������� ������� �������� 2D-������� �� �����
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

// renderCube() �������� 1x1 3D-���� � NDC
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

// renderQuad() �������� 1x1 XY-������������� � NDC
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