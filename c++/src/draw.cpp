#include "draw.h"
#include "input_handler.h"
#include "cube.h"

#include "shader.h"
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool MyDrawController::isWireMode = false;
bool MyDrawController::isAmbient = true;
bool MyDrawController::isDiffuse = true;
bool MyDrawController::isSpecular = true;
bool MyDrawController::drawSkybox = false;
bool MyDrawController::drawNormals = false;

enum Attrib_IDs { vPosition = 0, vNormals = 1, uvTextCoords = 2};

CShader* mainShader = nullptr;
CShader* lightModelShader = nullptr;
CShader* skyboxShader = nullptr;
CShader* normalShader = nullptr;

CShader* currShader = nullptr;

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
		case aiTextureType_AMBIENT:
			return "inTexture.reflection";
			break;
	}
	
	assert(false && "provide texture unoform name");
	return "none";
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

//=========================================================================================

MyDrawController::MyDrawController() : m_inputHandler(*this)
{
}

MyDrawController::~MyDrawController()
{
	m_resources.Release();
}

void MyDrawController::InitLightModel()
{
	glGenVertexArrays(1, &m_resources.cubeVAOID);
	glGenBuffers(1, &m_resources.cubeVertID);

	glBindVertexArray(m_resources.cubeVAOID);

	glBindBuffer(GL_ARRAY_BUFFER, m_resources.cubeVertID);		
	glBufferData(GL_ARRAY_BUFFER,	3 * cubeVerticiesCount * sizeof(GLfloat), cubeVertices, GL_STATIC_DRAW);

	lightModelShader = new CShader("shaders/light.vert", "shaders/light.frag");

	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	glGenBuffers(1, &m_resources.cubeElemID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resources.cubeElemID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,  cubeIndiciesCount * sizeof(GLint), cubeIndices, GL_STATIC_DRAW);
}

void MyDrawController::LoadMeshesData()
{
	const unsigned int meshN = GetScene()->mNumMeshes;

	glGenVertexArrays(meshN, m_resources.VAOs.data());
	glGenBuffers(meshN, m_resources.vertIDs.data());
	glGenBuffers(meshN, m_resources.elemIDs.data());
	glGenBuffers(meshN, m_resources.normIDs.data());
	glGenBuffers(meshN, m_resources.uvIDs.data());

	for (int i = 0; i < meshN; ++i)
	{
		const aiMesh* pMesh = GetScene()->mMeshes[i];
		assert(pMesh);

		glBindVertexArray(m_resources.VAOs[i]);

		glBindBuffer(GL_ARRAY_BUFFER, m_resources.vertIDs[i]);
		glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vPosition);

		glBindBuffer(GL_ARRAY_BUFFER, m_resources.normIDs[i]);
		glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mNormals, GL_STATIC_DRAW);
		glVertexAttribPointer(vNormals, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vNormals);

		//TextCoordBuffers
		std::vector<glm::vec2> uvTmp;
		MergeUV(*pMesh, uvTmp);
		if (!uvTmp.empty())
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_resources.uvIDs[i]);
			glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(glm::vec2), uvTmp.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(uvTextCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(uvTextCoords);
		}

		std::vector<unsigned int> elms;
		MergeElements(*pMesh, elms);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resources.elemIDs[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,  elms.size() * sizeof(unsigned int), elms.data(), GL_STATIC_DRAW);
	}

}

bool MyDrawController::LoadScene(const std::string& path)
{
	bool res = false;
	if (const aiScene* p = aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality))
	{
		if (p->mNumTextures)
		{
			std::cout << "[ERROR][TODO] embeded textures are not handled"  << std::endl;
		}
		else
		{
			m_pScene.reset(p);
			res = true;
		}
	}
	else
		std::cout << "[assimp error]" << aiGetErrorString() << std::endl;

	m_dirPath = path.substr(0, path.find_last_of('/'));

	return res;
}

void MyDrawController::Load()
{
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/nanosuit/untitled.blend");
	bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/untitled.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/sponza.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/sponza_cry.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/sponza/sponza.obj");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/dragon_recon/dragon_vrip_res4.ply");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/bunny/reconstruction/bun_zipper_res4.ply");
	assert(res && "cannot load scene");

	LoadMeshesData();
	mainShader = new CShader("shaders/main.vert", "shaders/main.frag");
	skyboxShader  = new CShader("shaders/skybox.vert", "shaders/skybox.frag");
	normalShader = new CShader("shaders/normal.vert", "shaders/normal.frag", "shaders/normal.geom");

	InitLightModel();
	m_resources.skyboxTextID = LoadCubemap();
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
			auto it = m_resources.texturePathToID.find(std::string(path.C_Str()));
			GLuint id = 0;

			if (it == m_resources.texturePathToID.end())
			{
				id = TextureFromFile(path.C_Str(), m_dirPath);
				m_resources.texturePathToID[std::string(path.C_Str())] = id;
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

void MyDrawController::SetupMaterial(const aiMesh& mesh, CShader* overrideProgram)
{
	assert(GetScene()->mNumMaterials);

	unsigned int matIndx = mesh.mMaterialIndex;
		//std::cout << "matIndx: " << matIndx << std::endl;
	const aiMaterial& material = *m_pScene->mMaterials[matIndx];

	if (overrideProgram)
		currShader = overrideProgram;
	else
		currShader = mainShader;
	
	currShader->use();

	if (currShader == mainShader)
	{
		CShader::TSubroutineTypeToInstance data;
		if (material.GetTextureCount(aiTextureType_DIFFUSE))
		{
			data.push_back(std::pair<std::string, std::string>("baseColorSelection", "textColor"));
			BindTexture(material, aiTextureType_DIFFUSE, 0);
			BindTexture(material, aiTextureType_SPECULAR, 1);
			BindTexture(material, aiTextureType_AMBIENT, 2);
			//BindTexture(material, aiTextureType_NORMALS, 2);
		}
		else
		{
			data.push_back(std::pair<std::string, std::string>("baseColorSelection", "plainColor"));
			aiColor3D col;

			if (!material.Get(AI_MATKEY_COLOR_AMBIENT, col))
				currShader->setVec3("material.ambient", col[0], col[1], col[2]);

			if (!material.Get(AI_MATKEY_COLOR_DIFFUSE, col))
				currShader->setVec3("material.diffuse", col[0], col[1], col[2]);

			if (!material.Get(AI_MATKEY_COLOR_SPECULAR, col))
				currShader->setVec3("material.specular", col[0], col[1], col[2]);

			float shininess; 
			if (!material.Get(AI_MATKEY_SHININESS, shininess))
				currShader->setFloat("material.shininess", shininess);
		}

		/*
		if (drawSkybox)
		{
			if (0 && material.GetTextureCount(aiTextureType_AMBIENT))
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "reflectionTexture"));
			else
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "emptyReflectionMap"));
		}
		else
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "emptyReflectionMap"));

		*/
			//	data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "reflectionTexture"));
		data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "emptyReflectionMap"));

		currShader->setSubroutine(GL_FRAGMENT_SHADER, data);
	}
}

void MyDrawController::SetupProgramTransforms(const Camera& cam, const glm::mat4& model,const glm::mat4& view, const glm::mat4& proj)
{
		currShader->setMat4("model", model);
		currShader->setMat4("view", view);
		currShader->setMat4("proj", proj);
		currShader->setVec3("camPos", cam.Position);

		if (drawSkybox)
		{
				glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1, 0, 0));
				currShader->setMat4("rotfix", rot);
		}
}

void MyDrawController::RecursiveRender(const aiScene& scene, const aiNode* nd, const Camera& cam, CShader* overrideProgram)
{
	aiMatrix4x4 m = nd->mTransformation;
	glm::mat4 model = aiMatrix4x4ToGlm(&m);
	glm::mat4 view = cam.GetViewMatrix();
	glm::mat4 proj = cam.GetProjMatrix();

	for (int i=0; i < nd->mNumMeshes; ++i) 
	{
		const aiMesh* pMesh = scene.mMeshes[nd->mMeshes[i]];
		assert(pMesh);

		SetupMaterial(*pMesh, overrideProgram);
		SetupLights();
		SetupProgramTransforms(cam, model, view, proj);
		
		GLint size = 0;
		glBindVertexArray(m_resources.VAOs[nd->mMeshes[i]]);
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	}

	for (int i = 0; i < nd->mNumChildren; ++i) {
		RecursiveRender(scene, nd->mChildren[i], cam, overrideProgram);
	}
}

void MyDrawController::SetupLights()
{
	if (drawSkybox)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);
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

void MyDrawController::Render(const Camera& cam)
{
	glPolygonMode(GL_FRONT_AND_BACK, isWireMode ? GL_LINE : GL_FILL);

	RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, cam, nullptr);

	if (drawNormals)
		RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, cam, normalShader);
	
	//if (!drawSkybox)
		RenderLightModels(cam);

	if (drawSkybox)
		RenderSkyBox(cam);
}

void MyDrawController::RenderLightModels(const Camera& cam)
{
	glBindVertexArray(m_resources.cubeVAOID);
	lightModelShader->use();

	for (int i =0; i < m_pScene->mNumLights; ++i)
	{
		const aiLight& light= *m_pScene->mLights[i];

		aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName);
		assert(pLightNode);

		aiMatrix4x4 m = pLightNode->mTransformation;
		glm::mat4 t = aiMatrix4x4ToGlm(&m);

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat4 mvp_matrix = cam.GetProjMatrix() * cam.GetViewMatrix() * t * scale;
		
		aiColor3D tmp = light.mColorDiffuse;	
		lightModelShader->setVec3("lightColor", tmp[0], tmp[1], tmp[2]);
		//printf("%.1f %.1f %.1f\n", tmp[0], tmp[1], tmp[2] );

		lightModelShader->setMat4("MVP", mvp_matrix);

		GLint size = 0;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	}
}

void MyDrawController::RenderSkyBox(const Camera& cam)
{
	glDepthFunc(GL_LEQUAL);
	skyboxShader->use();
	glBindVertexArray(m_resources.cubeVAOID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);

	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(-1, 0, 0));
	
  glm::mat4 view = glm::mat4(glm::mat3(m_cam.GetViewMatrix())); // remove translation from the view matrix
	glm::mat4 mvp_matrix = cam.GetProjMatrix() * view * rot;
	skyboxShader->setMat4("MVP", mvp_matrix);
	
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	glDepthFunc(GL_LESS);
}

void SResourceHandlers::Release()
{
//TODO: release resources
}
