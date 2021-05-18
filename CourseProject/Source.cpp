//http://docs.gl/

#include <glew.h>
#include <glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Renderer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "CScene.h"

using std::cout;
using std::endl;
using std::string;

struct color
{
	float R;
	float G;
	float B;
	float A;
};

int main()
{
#pragma region Init and window creation

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK)
		return -1;

	cout << glGetString(GL_VERSION) << endl;
#pragma endregion

	float positions[] = {
		-0.5f, -0.5f,	//0
		 0.5f, -0.5f,	//1
		 0.5f,  0.5f,	//2
		-0.5f,  0.5f,	//3
	};

	unsigned int indeces[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexArray va;
	VertexBuffer vb(positions, 4 * 2 * sizeof(float));

	VertexBufferLayout layout;
	layout.Push<float>(2);
	va.AddBuffer(vb, layout);

	IndexBuffer ib(indeces, 6);

	Shader shader("res/shaders/Basic.shader");
	shader.Bind();
	shader.SetUniform4f("u_Color", 0.2f, 0.3f, 0.8f, 1.0f);

	va.Unbind();
	shader.Unbind();
	vb.Unbind();
	ib.Unbind();

	Renderer renderer;

	
	color col = { 0.5f, 0.8f, 0.3f, 1.f };
	float increment = 0.005f;
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		renderer.Clear();
		shader.Bind();
		if (col.R > 1 || col.R < 0) increment *= -1;
		shader.SetUniform4f("u_Color", col.R += increment, col.G, col.B, col.A);

		renderer.Draw(va, ib, shader);

		glEnd();
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}
	shader.Unbind();


	glfwTerminate();
	return 0;
}