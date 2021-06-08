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

struct Color {
	float r, g, b, a;
};

Color background = { 0.f, 0.f, 0.f, 1.f };

struct Material
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
};

Camera camera(glm::vec3(0.240, 0.187, 0.502), glm::vec3(0.f, 1.0f, 0.f), 243.051, -20.7);

Light* flashLight, * redLamp, * sunLight;
bool wireframeMode = false;

void UpdatePolygoneMode();
unsigned int loadCubemap(vector<std::string> faces);

void processInput(GLFWwindow* win, double dt);
void OnKeyAction(GLFWwindow* win, int key, int scancode, int action, int mods);
void OnMouseKeyAction(GLFWwindow* win, int button, int action, int mods);
void OnMouseMoutionAction(GLFWwindow* win, double x, double y);
void OnScroll(GLFWwindow* win, double x, double y);
void OnResize(GLFWwindow* win, int width, int height);

struct 
{
	int h;
	int w;
} resolution = { 1280, 720 };

int main()
{
#pragma region WINDOW INITIALIZATION
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* win = glfwCreateWindow(resolution.h, resolution.w, "OpenGL Window", NULL, NULL);
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
	glViewport(0, 0, resolution.h, resolution.w);
	glEnable(GL_DEPTH_TEST);
	glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	UpdatePolygoneMode();
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	
	//glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

#pragma endregion

	Model earth("res/models/earth/earth.obj", true);

	ModelTransform earthTrans = {
	glm::vec3(0.f, 0.f, 0.f),		// position
	glm::vec3(0.f, 0.f, 0.f),		// rotation
	glm::vec3(0.001f, 0.001f, 0.001f) };	// scale


	int box_width, box_height, channels;
	byte* data = stbi_load("res\\images\\box.png", &box_width, &box_height, &channels, 0);

	const int verts = 36;
#pragma region CUBES
	float cube[] = {
		//position			normal					texture				color			
	-1.0f,-1.0f,-1.0f,	-1.0f,  0.0f,  0.0f,	0.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	-1.0f,-1.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	1.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	1.0f, 1.0f,		0.0f, 1.0f, 0.0f,
	-1.0f,-1.0f,-1.0f,	-1.0f,  0.0f,  0.0f,	0.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	1.0f, 1.0f,		0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f,-1.0f,	-1.0f,  0.0f,  0.0f,	0.0f, 1.0f,		0.0f, 1.0f, 0.0f,

	1.0f, 1.0f,-1.0f,	0.0f,  0.0f, -1.0f, 	0.0f, 1.0f,		1.0f, 0.0f, 0.0f,
	-1.0f,-1.0f,-1.0f,	0.0f,  0.0f, -1.0f, 	1.0f, 0.0f,		1.0f, 0.0f, 0.0f,
	-1.0f, 1.0f,-1.0f,	0.0f,  0.0f, -1.0f, 	1.0f, 1.0f,		1.0f, 0.0f, 0.0f,
	1.0f, 1.0f,-1.0f,	0.0f,  0.0f, -1.0f,		0.0f, 1.0f,		1.0f, 0.0f, 0.0f,
	1.0f,-1.0f,-1.0f,	0.0f,  0.0f, -1.0f,		0.0f, 0.0f,		1.0f, 0.0f, 0.0f,
	-1.0f,-1.0f,-1.0f,	0.0f,  0.0f, -1.0f,		1.0f, 0.0f,		1.0f, 0.0f, 0.0f,

	1.0f,-1.0f, 1.0f,	0.0f, -1.0f,  0.0f,		0.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,	0.0f, -1.0f,  0.0f,		1.0f, 1.0f,		0.0f, 0.0f, 1.0f,
	1.0f,-1.0f,-1.0f,	0.0f, -1.0f,  0.0f,		0.0f, 1.0f,		0.0f, 0.0f, 1.0f,
	1.0f,-1.0f, 1.0f,	0.0f, -1.0f,  0.0f,		0.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,	0.0f, -1.0f,  0.0f,		1.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,	0.0f, -1.0f,  0.0f,		1.0f, 1.0f,		0.0f, 0.0f, 1.0f,

	-1.0f, 1.0f, 1.0f,	0.0f,  0.0f, 1.0f,		0.0f, 1.0f,		0.0f, 0.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,	0.0f,  0.0f, 1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	1.0f,-1.0f, 1.0f,	0.0f,  0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f,	0.0f,  0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,	0.0f,  0.0f, 1.0f,		0.0f, 1.0f,		0.0f, 0.0f, 1.0f,
	1.0f,-1.0f, 1.0f,	0.0f,  0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 1.0f,

	1.0f, 1.0f, 1.0f,	1.0f,  0.0f,  0.0f,		0.0f, 1.0f,		1.0f, 0.0f, 0.0f,
	1.0f,-1.0f,-1.0f,	1.0f,  0.0f,  0.0f,		1.0f, 0.0f,		1.0f, 0.0f, 0.0f,
	1.0f, 1.0f,-1.0f,	1.0f,  0.0f,  0.0f,		1.0f, 1.0f,		1.0f, 0.0f, 0.0f,
	1.0f,-1.0f,-1.0f,	1.0f,  0.0f,  0.0f,		1.0f, 0.0f,		1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 1.0f,	1.0f,  0.0f,  0.0f,		0.0f, 1.0f,		1.0f, 0.0f, 0.0f,
	1.0f,-1.0f, 1.0f,	1.0f,  0.0f,  0.0f,		0.0f, 0.0f,		1.0f, 0.0f, 0.0f,

	1.0f, 1.0f, 1.0f,	0.0f,  1.0f,  0.0f,		1.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	1.0f, 1.0f,-1.0f,	0.0f,  1.0f,  0.0f,		1.0f, 1.0f,		0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f,-1.0f,	0.0f,  1.0f,  0.0f,		0.0f, 1.0f,		0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f,	0.0f,  1.0f,  0.0f,		1.0f, 0.0f,		0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f,-1.0f,	0.0f,  1.0f,  0.0f,		0.0f, 1.0f,		0.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 1.0f,	0.0f,  1.0f,  0.0f,		0.0f, 0.0f,		0.0f, 1.0f, 0.0f
	};

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

	const int cube_count = 1;

	ModelTransform cubeTrans[cube_count];
	int cubeMat[cube_count];
	for (int i = 0; i < cube_count; i++)
	{
		float scale = (rand() % 6 + 1) / 20.0f;
		cubeTrans[i] = {
			glm::vec3((rand() % 201 - 100) / 50.0f, (rand() % 201 - 100) / 50.0f, (rand() % 201 - 100) / 50.0f),
			glm::vec3(rand() / 100.0f, rand() / 100.0f, rand() / 100.0f),
			glm::vec3(scale, scale, scale)
		};
		cubeMat[i] = rand() % 3;

		if ((glm::vec3(0, 0, 0) - cubeTrans[i].position).length() < 0.7f)
			i--;
	}
#pragma endregion

#pragma region BUFFERS INITIALIZATION

	vector<std::string> faces
	{
		"res\\skyboxes\\space\\right.jpg",
		"res\\skyboxes\\space\\left.jpg",
		"res\\skyboxes\\space\\top.jpg",
		"res\\skyboxes\\space\\bottom.jpg",
		"res\\skyboxes\\space\\front.jpg",
		"res\\skyboxes\\space\\back.jpg"
	};
	unsigned int cubemapTexture = loadCubemap(faces);

	unsigned int box_texture;
	glGenTextures(1, &box_texture);

	glBindTexture(GL_TEXTURE_2D, box_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (channels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, box_width, box_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, box_width, box_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);

	unsigned int VBO_polygon, VAO_polygon;

	glGenBuffers(1, &VBO_polygon);
	glGenVertexArrays(1, &VAO_polygon);

	glBindVertexArray(VAO_polygon);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_polygon);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// texture coords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// color
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

#pragma endregion

	Shader* polygon_shader = new Shader("shaders\\basic.vert", "shaders\\basic.frag");
	Shader* light_shader = new Shader("shaders\\light.vert", "shaders\\light.frag");
	Shader* earth_shader = new Shader("shaders\\earth.vert", "shaders\\earth.frag");
	Shader* skybox_shader = new Shader("shaders\\skybox.vert", "shaders\\skybox.frag");

#pragma region LIGHT INITIALIZATION

	vector<Light*> lights;
	int total_lights = 3;
	int active_lights = 0;

	redLamp = new Light("LampRed", true);
	redLamp->initLikePointLight(
		glm::vec3(-0.9f, 0.445f, -0.44f),	//position
		glm::vec3(0.f, 0.f, 0.f),		//ambient
		glm::vec3(0.9f, 0.9f, 0.9f),		//diffuse
		glm::vec3(0.0f, 0.0f, 0.0f),		//specular
		1.0f, 0.f, 0.0f);
	lights.push_back(redLamp);

	sunLight = new Light("Sun", true);
	sunLight->initLikeDirectionalLight(
		glm::vec3(-1.0f, -1.0f, -1.0f),	//position
		glm::vec3(0.f, 0.f, 0.f),		//ambient
		glm::vec3(0.f, 0.f, 0.f),		//diffuse
		glm::vec3(0.0f, 0.0f, 0.0f));	//specular
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
	lights.push_back(flashLight);

#pragma endregion

	ModelTransform lightTrans = {
	glm::vec3(0.f, 0.f, 0.f),			// position
	glm::vec3(0.f, 0.f, 0.f),			// rotation
	glm::vec3(0.05f, 0.05f, 0.05f) };		// scale

	double oldTime = glfwGetTime(), newTime, deltaTime;

	while (!glfwWindowShouldClose(win))
	{
		newTime = glfwGetTime();
		deltaTime = newTime - oldTime;
		oldTime = newTime;

		processInput(win, deltaTime);

		glClearColor(background.r, background.g, background.b, background.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 p = camera.GetProjectionMatrix();
		glm::mat4 v = camera.GetViewMatrix();
		//float camX = sin(glfwGetTime());
		//float camZ = cos(glfwGetTime());
		//glm::mat4 v = glm::lookAt(glm::vec3(camera.Position.x, camera.Position.y, camera.Position.z), camera.Position + camera.Front, camera.Up);
		glm::mat4 pv = p * v;
		glm::mat4 model;

		flashLight->position = camera.Position - camera.Up * 0.5f;
		flashLight->direction = camera.Front;

		// DRAWING EARTH
		model = glm::mat4(1.0f);
		model = glm::translate(model, earthTrans.position);
		model = glm::rotate(model, glm::radians(earthTrans.rotation.y > 360 ? earthTrans.rotation.y -= 360 : earthTrans.rotation.y += 0.f), glm::vec3(0.f, 1.f, 0.f));
		model = glm::scale(model, earthTrans.scale);
		earth_shader->use();
		earth_shader->setMatrix4F("pv", pv);
		earth_shader->setMatrix4F("model", model);
		earth_shader->setVec3("viewPos", camera.Position);

		active_lights = 0;
		for (int i = 0; i < lights.size(); i++)
		{
			active_lights += lights[i]->putInShader(earth_shader, active_lights);
		}
		earth_shader->setInt("lights_count", active_lights);
		
		glEnable(GL_BLEND);
		earth.Draw(earth_shader);
		glDisable(GL_BLEND);

		/*polygon_shader->use();
		polygon_shader->setMatrix4F("pv", pv);
		polygon_shader->setBool("wireframeMode", wireframeMode);
		polygon_shader->setVec3("viewPos", camera.Position);
		
		model = glm::scale(model, glm::vec3(0.55f, 0.55f, 0.55f));
		
		polygon_shader->setMatrix4F("model", model);
		
		polygon_shader->setVec3("material.ambient", cubeMaterials[cubeMat[0]].ambient);
		polygon_shader->setVec3("material.diffuse", cubeMaterials[cubeMat[0]].diffuse);
		polygon_shader->setVec3("material.specular", cubeMaterials[cubeMat[0]].specular);
		polygon_shader->setFloat("material.shininess", cubeMaterials[cubeMat[0]].shininess);
		
		glBindTexture(GL_TEXTURE_2D, box_texture);
		glBindVertexArray(VAO_polygon);
		glDrawArrays(GL_TRIANGLES, 0, verts);*/

#pragma region skybox 
		// draw skybox as last
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		glCullFace(GL_FRONT);

		v = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
		pv = p * v;
		skybox_shader->use();
		skybox_shader->setMatrix4F("pv", pv);

		// skybox cube
		glBindVertexArray(VAO_polygon);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// DRAWING LAMPS
		light_shader->use();
		light_shader->setMatrix4F("pv", pv);
		glBindVertexArray(VAO_polygon);

		// Red Lamp
		lightTrans.position = redLamp->position;
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightTrans.position);
		model = glm::rotate(model, glm::radians(lightTrans.rotation.z == 360 ? lightTrans.rotation.z -= 360 : lightTrans.rotation.z += 10.f), glm::vec3(0.f, 1.f, 0.f));
				
		model = glm::scale(model, lightTrans.scale);
		light_shader->setMatrix4F("model", model);
		light_shader->setVec3("lightColor", glm::vec3(1.0f, 1.f, 1.f));
		glDrawArrays(GL_TRIANGLES, 0, verts);


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
	if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS)
		background = { 1.0f, 0.0f, 0.0f, 1.0f };
	if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS)
		background = { 0.0f, 1.0f, 0.0f, 1.0f };
	if (glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS)
		background = { 0.0f, 0.0f, 1.0f, 1.0f };
	if (glfwGetKey(win, GLFW_KEY_4) == GLFW_PRESS)
		background = { 0.55f, 0.8f, 0.85f, 1.0f };
	if (glfwGetKey(win, GLFW_KEY_5) == GLFW_PRESS)
		background = { 0.0f, 0.0f, 0.0f, 1.0f };

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

void UpdatePolygoneMode()
{
	if (wireframeMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

bool mouseLeftPress, mouseRightPress, mouseMiddlePress;
double mouseX = resolution.h / 2, mouseY = resolution.w / 2, mouseXtmp = 0, mouseYtmp = 0;
void OnMouseKeyAction(GLFWwindow* win, int button, int action, int mods)
{
	
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			mouseLeftPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			mouseLeftPress = false;
		}
	}

	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(win, &mouseXtmp, &mouseYtmp);
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwSetCursorPos(win, resolution.h / 2, resolution.w / 2);
			mouseRightPress = true;
		}
		else if (action == GLFW_RELEASE)
		{
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPos(win, mouseXtmp, mouseYtmp);
			mouseX = resolution.h / 2;
			mouseY = resolution.w / 2;
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