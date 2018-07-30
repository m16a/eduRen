#include "draw.h"

#include <iostream>
//#include "vgl.h"
#include "LoadShaders.h"

using namespace std;

#define MAX_MESHES_COUNT 100

enum Buffer_IDs { ArrayBuffer, IndicesBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0 };

GLuint VAOs[MAX_MESHES_COUNT];
GLuint EAOs[MAX_MESHES_COUNT];

GLuint Buffers[2 * MAX_MESHES_COUNT];

GLuint gMVP_Location = 0;
GLuint g_program = 0;

void MergeElements(const aiMesh& mesh, std::vector<unsigned int>& res)
{
	for (int i = 0; i < mesh.mNumFaces; ++i)
	{
		assert(mesh.mFaces[i].mNumIndices == 3);
		res.push_back(mesh.mFaces[i].mIndices[0]);
		res.push_back(mesh.mFaces[i].mIndices[1]);
		res.push_back(mesh.mFaces[i].mIndices[2]);
	}
}

void MyDrawController::Init(void)
{
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/dragon_recon/dragon_vrip_res4.ply");
	bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/bunny/reconstruction/bun_zipper_res4.ply");
	assert(res);

	const unsigned int meshN = GetScene()->mNumMeshes;

	glGenVertexArrays(meshN, VAOs);
	glGenBuffers(2 * meshN, Buffers);

	for (int i = 0; i < meshN; ++i)
	{
		const aiMesh* pMesh = GetScene()->mMeshes[i];
		assert(pMesh);

		glBindVertexArray(VAOs[i]);

		glBindBuffer(GL_ARRAY_BUFFER, Buffers[i*2]);
		glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mVertices, GL_STATIC_DRAW);

	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "shaders/main.vert" },
		{ GL_FRAGMENT_SHADER, "shaders/main.frag" },
		{ GL_NONE, NULL }
	};

		g_program = LoadShaders(shaders);
		glUseProgram(g_program);

		glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vPosition);

		printf("NumFaces: %d", pMesh->mNumFaces);

		std::vector<unsigned int> elms;

		MergeElements(*pMesh, elms);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[i*2+1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,  elms.size() * sizeof(unsigned int), elms.data(), GL_STATIC_DRAW);
		
		gMVP_Location = glGetUniformLocation(g_program, "MVP");
		m_cameraTransform = vmath::translate(0.0f, 0.f, -2.5f);
		m_camPos = vmath::vec3(0, 0, -3); 
		m_camDir = vmath::vec3(1, 0, 0); 
	}


	/*
	GLfloat CubeVertices[NumVertices][3];

	//12 triangles * 3 verticies
	GLint CubeIndices[36];

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
	*/
}

void MyDrawController::RecursiveRender(const aiScene& scene, const aiNode* nd, int w, int h)
{
	aiMatrix4x4 m = nd->mTransformation;

	aiTransposeMatrix4(&m);

	for (int i=0; i < nd->mNumMeshes; ++i) 
	{
		const aiMesh* mesh = scene.mMeshes[nd->mMeshes[i]];
		assert(mesh);
		//apply_material(scene.mMaterials[mesh->mMaterialIndex]);
		glBindVertexArray(VAOs[nd->mMeshes[i]]);
		
		vmath::mat4 mvp_matrix = vmath::perspective(60.f, float(w) / h, 0.001f, 10.f) * vmath::scale(1.0f, 1.0f, 1.0f) * m_cameraTransform;
		glUniformMatrix4fv(gMVP_Location, 1, GL_FALSE, mvp_matrix);
		GLint size = 0;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	}

	for (int i = 0; i < nd->mNumChildren; ++i) {
		RecursiveRender(scene, nd->mChildren[i], w, h);
	}
}

void MyDrawController::Render(int w, int h)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, w, h);

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

void get_bounding_box_for_node(const aiScene& scene, const aiNode* nd, aiVector3D* min,	aiVector3D* max, aiMatrix4x4* trafo)
{
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo,&nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		aiMesh* mesh = scene.mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp, trafo);

			min->x = std::min(min->x, tmp.x);
			min->y = std::min(min->y, tmp.y);
			min->z = std::min(min->z, tmp.z);

			max->x = std::max(max->x, tmp.x);
			max->y = std::max(max->y, tmp.y);
			max->z = std::max(max->z, tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(scene, nd->mChildren[n], min, max,trafo);
	}
	*trafo = prev;
}

void get_bounding_box(const aiScene& scene, aiVector3D* min, aiVector3D* max)
{
	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z =  1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene, scene.mRootNode, min, max, &trafo);
}

bool MyDrawController::LoadScene(const char* path)
{
	bool res = false;
	if (const aiScene* p = aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality))
	{
		get_bounding_box(*p, &m_scene_min, &m_scene_max);
		m_scene_center.x = (m_scene_min.x + m_scene_max.x) / 2.0f;
		m_scene_center.y = (m_scene_min.y + m_scene_max.y) / 2.0f;
		m_scene_center.z = (m_scene_min.z + m_scene_max.z) / 2.0f;

		m_pScene.reset(p);

		res = true;

		
	
	}

	return res;

}
