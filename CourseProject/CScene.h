#pragma once
//#include "../Project/glfw-3.3.4.bin.WIN64/include/glm/trigonometric.hpp"
#include <glew.h>
#include <math.h>

class CScene
{
public:

	enum {
		step = 8,
		numberOfVertices = ((180 / step) + 1) * ((360 / step) + 1),
		numberOfIndices = 2 * (numberOfVertices - (360 / step) - 1)
	}; // enum

	CScene(void)
	{
		/// вычисляем вершины сферы и заносим их в массив
		const GLfloat fRadius = 1.0;

		for (int alpha = 90, index = 0; alpha <= 270; alpha += step)
		{
			const int angleOfVertical = alpha % 360;

			for (int phi = 0; phi <= 360; phi += step, ++index)
			{
				const int angleOfHorizontal = phi % 360;

				/// вычисляем координаты точки
				m_vertices[index].x = fRadius * cos(angleOfVertical) * cos(angleOfHorizontal);
				m_vertices[index].y = fRadius * sin(angleOfVertical);
				m_vertices[index].z = fRadius * cos(angleOfVertical) * sin(angleOfHorizontal);
			} // for
		} // for

		/// вычисляем индексы вершин и заносим их в массив
		for (int index = 0; index < numberOfIndices; index += 2)
		{
			m_indices[index] = index >> 1;
			m_indices[index + 1] = m_indices[index] + (360 / step) + 1;
		} // for
	} // constructor CScene

	void Init(const int _nWidth, const int _nHeight)
	{
		glViewport(0, 0, _nWidth, _nHeight);	// устанавливаем порт отображения

		//// устанавливаем перспективную проекцию
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		//gluPerspective(80.0, (GLdouble)_nWidth / (GLdouble)_nHeight, 0.1, 3.0);
		
		// устанавливаем камеру
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		//gluLookAt(0.0, 1.5, 1.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

		/// разрешаем отсечение невидимых граней
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);

		glEnable(GL_DEPTH_TEST);					// рзрешаем тест буфера глубины
		glEnable(GL_NORMALIZE);						// разрешаем нормализацию
		glShadeModel(GL_SMOOTH);					// устанавливаем плавное закрашивание
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// устанавливаем метод отображения - закрашивание

		/// устанавливаем источники света
		this->InitLights();
		glEnable(GL_LIGHTING);
		glClearColor(0.0, 0.0, 0.0, 1.0);	// задаем цвет фона - черный

		/// задаем свойства материала объекта
		const GLfloat clr[] = { 1.0f, 0.0f, 1.0f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, clr);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);

		this->InitArrays();	// инициализируем вершинные массивы
	} // Init

	inline void DrawEarth(void)
	{
		glDrawElements(GL_QUAD_STRIP, numberOfIndices, GL_UNSIGNED_INT, m_indices);
	} // DrawEarth

	void Redraw(void)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		this->DrawEarth();
		glFinish();
	} // Redraw

private:
	inline void InitArrays(void)
	{
		const GLsizei stride = 3 * sizeof(GLfloat);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, stride, m_vertices);

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, stride, m_vertices);
	} // InitArrays

	inline void InitLights(void)
	{
		const GLfloat pos[] = { 1.0, 1.0, 1.0, 0.0 };
		const GLfloat clr[] = { 1.0, 1.0, 1.0, 1.0 };

		glEnable(GL_LIGHT0);

		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, clr);
		glLightfv(GL_LIGHT0, GL_SPECULAR, clr);
	} // InitLights

    struct 
    {
        GLfloat x, y, z; // координаты точки
    } m_vertices[numberOfVertices]; // массив вершин

    GLuint m_indices[numberOfIndices]; // массив индексов

};

