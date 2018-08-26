#include "draw.h"

#include "shader.h"
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MAX_MESHES_COUNT 100

bool MyDrawController::isWireMode = false;
bool MyDrawController::isAmbient = true;
bool MyDrawController::isDiffuse = true;
bool MyDrawController::isSpecular = true;
bool MyDrawController::drawSkybox = false;
bool MyDrawController::drawNormals = false;

enum Buffer_IDs { ArrayBuffer, IndicesBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0, vNormals = 1, uvTextCoords = 2};

GLuint VAOs[MAX_MESHES_COUNT];
GLuint EAOs[MAX_MESHES_COUNT];

GLuint cubeVAO[1];
GLuint cubeBuffers[2];

GLuint Buffers[2 * MAX_MESHES_COUNT]; // vertices + elements
GLuint NormalBuffers[MAX_MESHES_COUNT];
GLuint TextCoordBuffers[MAX_MESHES_COUNT];

Shader* mainTextShader = nullptr;
Shader* mainColShader = nullptr;
Shader* lightModelShader = nullptr;
Shader* skyboxShader = nullptr;
Shader* envMapColorShader = nullptr;
Shader* normalShader = nullptr;

Shader* currShader = nullptr;

GLuint skyboxID;

std::vector<GLuint> texturesID;

unsigned int LoadCubemap();

inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
    glm::mat4 to;

    to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
    to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
    to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
    to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

    return to;
}

unsigned int TextureFromFile(const char *path, const std::string &directory)
{
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
			GLenum format;
			if (nrComponents == 1)
					format = GL_RED;
			else if (nrComponents == 3)
					format = GL_RGB;
			else if (nrComponents == 4)
					format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
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

void MergeUV(const aiMesh& mesh, std::vector<glm::vec2>& res)
{
	if (mesh.mTextureCoords == nullptr || mesh.mNumUVComponents[0] == 0)
		return;

	assert(mesh.mNumUVComponents[0] == 2);
	glm::vec2 tmp;
	for (int i = 0; i < mesh.mNumVertices; ++i)
	{
		tmp[0] = mesh.mTextureCoords[0][i][0];
		tmp[1] = mesh.mTextureCoords[0][i][1];
		res.push_back(tmp);
	}
}

void MyDrawController::InitLightModel()
{
	glGenVertexArrays(1, cubeVAO);
	glGenBuffers(2, cubeBuffers);

	glBindVertexArray(cubeVAO[0]);

	glBindBuffer(GL_ARRAY_BUFFER, cubeBuffers[0]);		
	size_t numVertices = 8;

	GLfloat cubeVertices[numVertices][3] = {	
			{ -1.0, -1.0, -1.0 },  // Vertex 0	
			{  1.0, -1.0, -1.0 },  // Vertex 1	
			{  1.0,  1.0, -1.0 },  // Vertex 2	
			{ -1.0,  1.0, -1.0 },  // Vertex 3	
			{ -1.0, -1.0,  1.0 },  // Vertex 4	
			{  1.0, -1.0,  1.0 },  // Vertex 5	
			{  1.0,  1.0,  1.0 },  // Vertex 6	
			{ -1.0,  1.0,  1.0 }	 // Vertex 7
	};

	glBufferData(GL_ARRAY_BUFFER,	3 * numVertices * sizeof(GLfloat), cubeVertices, GL_STATIC_DRAW);

	lightModelShader = new Shader("shaders/light.vert", "shaders/light.frag");

	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	//12 triangles * 3 verticies
	GLint cubeIndices[36] = 
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

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeBuffers[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,  36 * sizeof(GLint), cubeIndices, GL_STATIC_DRAW);
		
}

void MyDrawController::InitTextures(const aiScene& scene)
{
	if (!scene.mNumTextures)
		return;
	
	assert(false && "embeded textures are not handled");

	texturesID.resize(scene.mNumTextures);
	glGenTextures(scene.mNumTextures, texturesID.data());

	for (int i=0; i<scene.mNumTextures; ++i)
	{
		const aiTexture& texture = *scene.mTextures[i];
		glBindTexture(GL_TEXTURE_2D, texturesID[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.mWidth, texture.mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<void*>(texture.pcData));
    glGenerateMipmap(GL_TEXTURE_2D);				
	}
}

void MyDrawController::LoadTextureForMaterial(const aiMaterial& mat)
{

}
	
void MyDrawController::Init(void)
{
	bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/nanosuit/untitled.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/untitled.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/sponza.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/sponza_cry.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/sponza/sponza.obj");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/dragon_recon/dragon_vrip_res4.ply");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/bunny/reconstruction/bun_zipper_res4.ply");
	assert(res);

	const unsigned int meshN = GetScene()->mNumMeshes;

	glGenVertexArrays(meshN, VAOs);
	glGenBuffers(2 * meshN, Buffers);
	glGenBuffers(meshN, NormalBuffers);
	glGenBuffers(meshN, TextCoordBuffers);

	InitTextures(*GetScene());

	mainTextShader = new Shader("shaders/main_textured.vert", "shaders/main_textured.frag");
	mainColShader = new Shader("shaders/main_col.vert", "shaders/main_col.frag");
	skyboxShader  = new Shader("shaders/skybox.vert", "shaders/skybox.frag");
	envMapColorShader = new Shader("shaders/env_map_color.vert", "shaders/env_map_color.frag");
	normalShader = new Shader("shaders/normal.vert", "shaders/normal.frag", "shaders/normal.geom");

	for (int i = 0; i < meshN; ++i)
	{
		const aiMesh* pMesh = GetScene()->mMeshes[i];
		assert(pMesh);

		glBindVertexArray(VAOs[i]);

		glBindBuffer(GL_ARRAY_BUFFER, Buffers[i*2]);
		glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vPosition);


		glBindBuffer(GL_ARRAY_BUFFER, NormalBuffers[i]);
		glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mNormals, GL_STATIC_DRAW);
		glVertexAttribPointer(vNormals, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vNormals);

		//TextCoordBuffers
		std::vector<glm::vec2> uvTmp;
		MergeUV(*pMesh, uvTmp);
		if (!uvTmp.empty())
		{
			glBindBuffer(GL_ARRAY_BUFFER, TextCoordBuffers[i]);
			glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(glm::vec2), uvTmp.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(uvTextCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(uvTextCoords);
		}

		std::vector<unsigned int> elms;
		MergeElements(*pMesh, elms);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[i*2+1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,  elms.size() * sizeof(unsigned int), elms.data(), GL_STATIC_DRAW);
	}

	InitLightModel();
	skyboxID = LoadCubemap();
}

void MyDrawController::RecursiveRender(const aiScene& scene, const aiNode* nd, int w, int h, int fov, bool _drawNormals)
{
	aiMatrix4x4 m = nd->mTransformation;
	glm::mat4 model = aiMatrix4x4ToGlm(&m);

	for (int i=0; i < nd->mNumMeshes; ++i) 
	{
		const aiMesh* mesh = scene.mMeshes[nd->mMeshes[i]];
		assert(mesh);

		//material setup
		if (m_pScene->mNumMaterials)
		{
			unsigned int matIndx = mesh->mMaterialIndex;
			//std::cout << "matIndx: " << matIndx << std::endl;
			const aiMaterial& mat = *m_pScene->mMaterials[matIndx];

			if (_drawNormals)
			{
				currShader = normalShader;
			}
			else if (drawSkybox)
			{
				currShader = envMapColorShader;
			}
			else
			{
				if (mat.GetTextureCount(aiTextureType_DIFFUSE))
					currShader = mainTextShader;
				else
					currShader = mainColShader;
			}
			
			currShader->use();

			if (mat.GetTextureCount(aiTextureType_DIFFUSE))
			{
				BindTexture(mat, aiTextureType_DIFFUSE, 0);
				BindTexture(mat, aiTextureType_SPECULAR, 1);
				//BindTexture(mat, aiTextureType_NORMALS, 2);
			}
			else
			{
				aiColor3D col;

				if (!mat.Get(AI_MATKEY_COLOR_AMBIENT, col))
					currShader->setVec3("material.ambient", col[0], col[1], col[2]);

				if (!mat.Get(AI_MATKEY_COLOR_DIFFUSE, col))
					currShader->setVec3("material.diffuse", col[0], col[1], col[2]);

				if (!mat.Get(AI_MATKEY_COLOR_SPECULAR, col))
					currShader->setVec3("material.specular", col[0], col[1], col[2]);

				float shininess; 
				if (!mat.Get(AI_MATKEY_SHININESS, shininess))
					currShader->setFloat("material.shininess", shininess);
			}
		}
		else
			assert(false && "no materials");
		
		//apply_material(scene.mMaterials[mesh->mMaterialIndex]);
		glBindVertexArray(VAOs[nd->mMeshes[i]]);
		
		glm::mat4 proj = glm::perspective(glm::radians(float(fov)), float(w) / h, 0.001f, 100.f);
		glm::mat4 view = m_cam.GetViewMatrix();

		currShader->setMat4("model", model);
		currShader->setMat4("view", view);
		currShader->setMat4("proj", proj);
		currShader->setVec3("camPos", m_cam.Position);

		if (drawSkybox)
		{
				glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1, 0, 0));
				currShader->setMat4("rotfix", rot);
		}

		SetupLights(currShader);

		GLint size = 0;

		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	}

	for (int i = 0; i < nd->mNumChildren; ++i) {
		RecursiveRender(scene, nd->mChildren[i], w, h, fov, _drawNormals);
	}
}

void MyDrawController::SetupLights(Shader* currShader)
{
	if (drawSkybox)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

		return;
	}

	int pointLightIndx = 0;
	int dirLightIndx = 0;
	//light setup
	for (int i =0; i < m_pScene->mNumLights; ++i)
	{
		const aiLight& light= *m_pScene->mLights[i];

		assert(light.mType == aiLightSource_POINT || light.mType == aiLightSource_DIRECTIONAL);
		aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName);
		assert(pLightNode);

		aiMatrix4x4 m = pLightNode->mTransformation;
		glm::mat4 t = aiMatrix4x4ToGlm(&m);

		char buff[100];
		std::string lightI;
		if (light.mType == aiLightSource_POINT)
		{
			snprintf(buff, sizeof(buff), "pointLights[%d].", pointLightIndx++);
			lightI = buff; 
			currShader->setVec3(lightI + "pos", glm::vec3(t[3]));
			currShader->setFloat(lightI + "constant", light.mAttenuationConstant);
			currShader->setFloat(lightI + "linear", light.mAttenuationLinear);
			currShader->setFloat(lightI + "quadratic", light.mAttenuationQuadratic);
		}
		else if (light.mType == aiLightSource_DIRECTIONAL)
		{
			snprintf(buff, sizeof(buff), "dirLights[%d].", dirLightIndx++);
			lightI = buff; 
			currShader->setVec3(lightI + "dir", glm::vec3(t[2]));
		}

		aiColor3D tmp = light.mColorAmbient;

		if (isAmbient)
			//mainShader->setVec3(lightI + "ambient", tmp[0], tmp[1], tmp[2]);
			currShader->setVec3(lightI + "ambient", 0.2, 0.2, 0.2);
		else
			currShader->setVec3(lightI + "ambient", glm::vec3());
		
		tmp = light.mColorDiffuse;	
		if (isDiffuse)
			currShader->setVec3(lightI + "diffuse", tmp[0], tmp[1], tmp[2]);
		else
			currShader->setVec3(lightI + "diffuse", glm::vec3());
		//printf("%.1f %.1f %.1f\n", diffCol[0], diffCol[1], diffCol[2] );

		tmp = light.mColorSpecular;
		if (isSpecular)
			currShader->setVec3(lightI + "specular", tmp[0], tmp[1], tmp[2]);
		else
			currShader->setVec3(lightI + "specular", glm::vec3());
		//printf("%.1f %.1f %.1f\n", diffCol[0], diffCol[1], diffCol[2] );
		
	}

	currShader->setInt("nPointLights", pointLightIndx);
	currShader->setInt("nDirLights", dirLightIndx);
}

std::string GetUniformTextureName(aiTextureType type)
{
	switch (type)
	{
		case aiTextureType_DIFFUSE:
			return "inTexture.diff";
			break;
		case aiTextureType_SPECULAR:
			return "inTexture.spec";
			break;
		case aiTextureType_NORMALS:
			return "inTexture.norm";
			break;
	}
	
	assert(false && "provide texture unoform name");
	return "none";
}

void MyDrawController::BindTexture(const aiMaterial& mat, aiTextureType type, int startIndx)
{
	//lazy loading textures
	unsigned int texCnt = mat.GetTextureCount(type);
	assert(texCnt <= 1);

	for (int i=0; i<texCnt; ++i)
	{
		aiString path;
		if (!mat.GetTexture(type, i, &path))
		{
			auto it = m_texturePathToID.find(std::string(path.C_Str()));
			GLuint id = 0;

			if (it == m_texturePathToID.end())
			{
				id = TextureFromFile(path.C_Str(), m_dirPath);
				m_texturePathToID[std::string(path.C_Str())] = id;
			}
			else
				id = it->second;

			glActiveTexture(GL_TEXTURE0 + startIndx);
			currShader->setInt(GetUniformTextureName(type).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, id);

		}
		else
		{
			std::cout << "Texture reading fail\n";
		}
	}
}

void MyDrawController::Render(int w, int h, int fov)
{
	glPolygonMode(GL_FRONT_AND_BACK, isWireMode ? GL_LINE : GL_FILL);

	RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, w, h, fov, false);

	if (drawNormals)
		RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, w, h, fov, true);
	
	if (!drawSkybox)
		RenderLights(w, h, fov);

	if (drawSkybox)
		RenderSkyBox(w, h, fov);
}

void MyDrawController::RenderLights(int w, int h, int fov)
{
	glBindVertexArray(cubeVAO[0]);
	lightModelShader->use();

	for (int i =0; i < m_pScene->mNumLights; ++i)
	{
		const aiLight& light= *m_pScene->mLights[i];

		aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName);
		assert(pLightNode);

		aiMatrix4x4 m = pLightNode->mTransformation;
		glm::mat4 t = aiMatrix4x4ToGlm(&m);

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat4 mvp_matrix = glm::perspective(glm::radians(float(fov)), float(w) / h, 0.001f, 100.f) * m_cam.GetViewMatrix() * t * scale;
		
		aiColor3D tmp = light.mColorDiffuse;	
		lightModelShader->setVec3("lightColor", tmp[0], tmp[1], tmp[2]);
		//printf("%.1f %.1f %.1f\n", tmp[0], tmp[1], tmp[2] );

		lightModelShader->setMat4("MVP", mvp_matrix);

		GLint size = 0;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	}
}

constexpr float kRotSpeed = 1000.0f;

void MyDrawController::OnKeyW(float dt, bool haste)
{
	m_cam.ProcessKeyboard(Camera_Movement::FORWARD, dt, haste);
}

void MyDrawController::OnKeyS(float dt, bool haste)
{
	m_cam.ProcessKeyboard(Camera_Movement::BACKWARD, dt, haste);
}

void MyDrawController::OnKeyA(float dt, bool haste)
{
	m_cam.ProcessKeyboard(Camera_Movement::LEFT, dt, haste);
}

void MyDrawController::OnKeyD(float dt, bool haste)
{
	m_cam.ProcessKeyboard(Camera_Movement::RIGHT, dt, haste);
}

void MyDrawController::OnKeyUp(float dt)
{
	m_cam.ProcessMouseMovement(0.0f, kRotSpeed * dt);
}

void MyDrawController::OnKeyDown(float dt)
{
	m_cam.ProcessMouseMovement(0.0f, -kRotSpeed * dt);
}

void MyDrawController::OnKeyRight(float dt)
{
	m_cam.ProcessMouseMovement(kRotSpeed * dt, 0.0f);
}

void MyDrawController::OnKeyLeft(float dt)
{
	m_cam.ProcessMouseMovement(-kRotSpeed * dt, 0.0f);
}

void MyDrawController::OnKeySpace(float dt)
{
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

bool MyDrawController::LoadScene(const std::string& path)
{
	bool res = false;
	if (const aiScene* p = aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality))
	{
		get_bounding_box(*p, &m_scene_min, &m_scene_max);
		m_scene_center.x = (m_scene_min.x + m_scene_max.x) / 2.0f;
		m_scene_center.y = (m_scene_min.y + m_scene_max.y) / 2.0f;
		m_scene_center.z = (m_scene_min.z + m_scene_max.z) / 2.0f;

		m_pScene.reset(p);

		res = true;
	}
	else
		std::cout << "[assimp error]" << aiGetErrorString() << std::endl;

	m_dirPath = path.substr(0, path.find_last_of('/'));


	return res;
}


unsigned int LoadCubemap()
{
	static std::string pathToSkyboxFolder = "/home/m16a/Documents/github/eduRen/models/skybox/skybox/";
	static std::vector<std::string> faces = 
	{
			"right.jpg",
			"left.jpg",
			"bottom.jpg",
			"top.jpg",
			"front.jpg",
			"back.jpg"
	};

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load((pathToSkyboxFolder + faces[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
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


void MyDrawController::RenderSkyBox(int w, int h, int fov)
{
	glDepthFunc(GL_LEQUAL);
	skyboxShader->use();
	glBindVertexArray(cubeVAO[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(-1, 0, 0));
	
  glm::mat4 view = glm::mat4(glm::mat3(m_cam.GetViewMatrix())); // remove translation from the view matrix
	glm::mat4 mvp_matrix = glm::perspective(glm::radians(float(fov)), float(w) / h, 0.001f, 100.f) * view * rot;
	skyboxShader->setMat4("MVP", mvp_matrix);
	
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	glDepthFunc(GL_LESS);
}

