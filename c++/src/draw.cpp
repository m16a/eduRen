#include "draw.h"

#include <iostream>
#include <GL/gl3w.h>
//#include "vgl.h"
#include "LoadShaders.h"



using namespace std;

enum VAO_IDs { Verts, NumVAOs};
enum EAO_IDs { Indices, NumEAOs};
enum Buffer_IDs { ArrayBuffer, IndicesBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0 };

GLuint VAOs[NumVAOs];
GLuint EAOs[NumEAOs];
GLuint Buffers[NumBuffers];

const GLuint NumVertices = 8;
GLuint gMVP_Location = 0;
GLuint g_program = 0;

void MyDrawController::Init(void)
{
	glGenVertexArrays(NumVAOs, VAOs);
	glBindVertexArray(VAOs[Verts]);

	GLfloat CubeVertices[NumVertices][3] = {
				{ -0.5, -0.5, -0.5 },  // Vertex 0
				{  0.5, -0.5, -0.5 },  // Vertex 1
				{  0.5,  0.5, -0.5 },  // Vertex 2
				{ -0.5,  0.5, -0.5 },  // Vertex 3
				{ -0.5, -0.5,  0.5 },  // Vertex 4
				{  0.5, -0.5,  0.5 },  // Vertex 5
				{  0.5,  0.5,  0.5 },  // Vertex 6
				{ -0.5,  0.5,  0.5 }	 // Vertex 7
	};

	//12 triangles * 3 verticies
	GLint CubeIndices[36] = 
	{
		0, 1, 2,
		0, 2, 3,
		5, 4, 6,
		4, 7, 6,
		7, 3, 2,
		6, 7, 2,
		4, 1, 0,
		4, 5, 1,
		6, 2, 1,
		5, 6, 1,
		4, 0, 7,
		3, 7, 0
	};

	glGenBuffers(NumBuffers, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[ArrayBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices),
				CubeVertices, GL_STATIC_DRAW);


	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "shaders/main.vert" },
		{ GL_FRAGMENT_SHADER, "shaders/main.frag" },
		{ GL_NONE, NULL }
	};

	g_program = LoadShaders(shaders);
	glUseProgram(g_program);

	glVertexAttribPointer(vPosition, 3, GL_FLOAT,
							GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[IndicesBuffer]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CubeIndices),
				CubeIndices, GL_STATIC_DRAW);

	gMVP_Location = glGetUniformLocation(g_program, "MVP");
	m_cameraTransform = vmath::translate(0.0f, 0.f, -2.5f);
	m_camPos = vmath::vec3(0, 0, -1); 
	m_camDir = vmath::vec3(1, 0, 0); 
}


void MyDrawController::Render(int w, int h)
{
	//glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(VAOs[Verts]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[IndicesBuffer]);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	vmath::mat4 mvp_matrix = vmath::perspective(90.f, float(w) / h, 0.1f, 100.f) * vmath::scale(0.1f, 0.1f, 0.1f) * m_cameraTransform* vmath::translate(0.f, 0.f, 0.f) * vmath::rotate(30.f, 0.f, 1.f, 0.f) * vmath::rotate(30.f, 1.f, 0.f, 0.f);
	glUniformMatrix4fv(gMVP_Location, 1, GL_FALSE, mvp_matrix);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	/*
	mvp_matrix = vmath::perspective(90.f,1.0f,0.1f,100.f) * vmath::scale(0.1f, 0.1f, 0.1f) * vmath::translate(0.0f, 0.f, -2.5f) *  vmath::translate(1.f, 0.f, 0.f) * vmath::rotate(30.f, 0.f, 1.f, 0.f) * vmath::rotate(30.f, 1.f, 0.f, 0.f);
	glUniformMatrix4fv(gMVP_Location, 1, GL_FALSE, mvp_matrix);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	*/
	glFlush();
}

static float s_transSpeed = 0.01;

void MyDrawController::OnKeyW()
{
	m_cameraTransform *= vmath::translate(0.0f, 0.f, 0.01f);
}

void MyDrawController::OnKeyS()
{
	m_cameraTransform *= vmath::translate(0.0f, 0.f, -0.01f);
}

void MyDrawController::OnKeyA()
{
	m_cameraTransform *= vmath::translate(-0.01f, 0.f, 0.f);
}

void MyDrawController::OnKeyD()
{
	m_cameraTransform *= vmath::translate(0.01f, 0.f, 0.f);
}

static float s_rotSpeed = 3;

void MyDrawController::OnKeyUp()
{
	vmath::vec3 camRight = cross(m_camDir, vmath::vec3(0,0,1));
	
	vmath::vec4 tmp = vmath::vec4(m_camDir, 0.0f) * vmath::rotate(s_rotSpeed, camRight) ;
	m_camDir = vmath::vec3(tmp[0], tmp[1], tmp[2]);
	std::cout << m_camDir[0] << " " << m_camDir[1] << " " << m_camDir[2] << std::endl;

	camRight = cross(m_camDir, vmath::vec3(0,0,1));
	m_cameraTransform *= vmath::rotate(s_rotSpeed, camRight);
}

void MyDrawController::OnKeyDown()
{
	m_cameraTransform *= vmath::rotate(-s_rotSpeed, 1.0f, 0.f, 0.0f);
}

void MyDrawController::OnKeyRight()
{
	m_cameraTransform *= vmath::rotate(s_rotSpeed, 0.0f, 1.f, 0.0f);
}

void MyDrawController::OnKeyLeft()
{
	m_cameraTransform *= vmath::rotate(-s_rotSpeed, 0.0f, 1.f, 0.0f);
}

void MyDrawController::OnKeySpace()
{
	m_cameraTransform = vmath::translate(0.0f, 0.f, -2.5f);
}
