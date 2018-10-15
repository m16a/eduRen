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
bool MyDrawController::isMSAA = false;
bool MyDrawController::isGammaCorrection = false;
bool MyDrawController::drawGradientReference = false;
bool MyDrawController::drawShadows = false;

bool MyDrawController::debugShadowMaps = false;
std::string MyDrawController::debugOnmiShadowLightName = std::string();
std::vector<std::string> MyDrawController::pointLightNames = std::vector<std::string>();

bool MyDrawController::bumpMapping = true;
bool MyDrawController::HDR = false;
bool MyDrawController::deferredShading = false;
bool MyDrawController::debugGBuffer = false;

enum Attrib_IDs {vPosition = 0, vNormals = 1, uvTextCoords = 2, vTangents = 3, vBitangents = 4};

CShader* mainShader = nullptr;
CShader* lightModelShader = nullptr;
CShader* skyboxShader = nullptr;
CShader* normalShader = nullptr;
CShader* rect2dShader = nullptr;
CShader* shadowMapShader = nullptr;
CShader* shadowCubeMapShader = nullptr;
CShader* debugShadowCubeMapShader = nullptr;
CShader* deferredGeomPathShader = nullptr;
CShader* currShader = nullptr;

static const float kTMPFarPlane = 100.0f; //TODO: refactor

enum ETextureSlot
{
	Empty = 0,
	Diffuse,
	Specular,
	Reflection,
	Normals,
	SkyBox,
	DirShadowMap,
	OmniShadowMapStart = 7,
	OmniShadowMapEnd = 16
};

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
		case aiTextureType_HEIGHT:
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

	delete mainShader;
	delete skyboxShader;
	delete normalShader;
	delete rect2dShader;
	delete shadowMapShader;
	delete shadowCubeMapShader;
	delete debugShadowCubeMapShader;
	delete deferredGeomPathShader;

	ReleaseShadowMaps();
}

void MyDrawController::ReleaseShadowMaps()
{
	for (auto& sm: m_shadowMaps)
	{
		if (0 != sm.second.textureId)
		{
			glDeleteTextures(1, &sm.second.textureId);
			sm.second.textureId = 0;
		}
	}
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
	glGenBuffers(meshN, m_resources.tangentsIDs.data());
	glGenBuffers(meshN, m_resources.bitangentsIDs.data());

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

		
		if (pMesh->mTangents)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_resources.tangentsIDs[i]);
			glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mTangents, GL_STATIC_DRAW);
			glVertexAttribPointer(vTangents, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(vTangents);

			assert(pMesh->mTangents && "bitangents should exist");
			glBindBuffer(GL_ARRAY_BUFFER, m_resources.bitangentsIDs[i]);
			glBufferData(GL_ARRAY_BUFFER,	pMesh->mNumVertices * sizeof(aiVector3D), pMesh->mBitangents, GL_STATIC_DRAW);
			glVertexAttribPointer(vBitangents, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(vBitangents);
		}
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

	if (res)
	{
		for (int i=0; i < m_pScene->mNumLights; ++i)
		{
			const aiLight& light = *m_pScene->mLights[i];
			if(light.mType == aiLightSource_POINT) 
				pointLightNames.push_back(light.mName.C_Str());
		}
	}

	return res;
}

void MyDrawController::Load()
{	
	bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/untitled.blend");
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/nanosuit/untitled.blend");
	
	//bool res = LoadScene("/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/cubePointLight.blend");
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
	rect2dShader = new CShader("shaders/rect2d.vert", "shaders/rect2d.frag");
	shadowMapShader = new CShader("shaders/shadowMap.vert", "shaders/shadowMap.frag");
	shadowCubeMapShader = new CShader("shaders/shadowCubeMap.vert", "shaders/shadowCubeMap.frag", "shaders/shadowCubeMap.geom");
	debugShadowCubeMapShader = new CShader("shaders/debugCubeShadowMap.vert", "shaders/debugCubeShadowMap.frag"); 
	deferredGeomPathShader = new CShader("shaders/deferredGeomPath.vert", "shaders/deferredGeomPath.frag"); 

	InitLightModel();
	m_resources.skyboxTextID = LoadCubemap();
}

void MyDrawController::BindTexture(const aiMaterial& mat, aiTextureType type, int startIndx)
{
	//lazy loading textures
	unsigned int texCnt = mat.GetTextureCount(type);
	assert(texCnt <= 1 && "multiple textures for one type is not supported");

	if (texCnt)
	{
		int i = 0;
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
			currShader->setInt(GetUniformTextureName(type).c_str(), startIndx);
			glBindTexture(GL_TEXTURE_2D, id);
		}
		else
		{
			std::cout << "Texture reading fail\n";
		}
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + startIndx);
		currShader->setInt(GetUniformTextureName(type).c_str(), 0);
		glBindTexture(GL_TEXTURE_2D, 0);
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

	if (currShader == mainShader || currShader == deferredGeomPathShader)
	{
		CShader::TSubroutineTypeToInstance data;
		if (material.GetTextureCount(aiTextureType_DIFFUSE))
		{
			data.push_back(std::pair<std::string, std::string>("baseColorSelection", "textColor"));
			BindTexture(material, aiTextureType_DIFFUSE, ETextureSlot::Diffuse);
			BindTexture(material, aiTextureType_SPECULAR, ETextureSlot::Specular);
			BindTexture(material, aiTextureType_AMBIENT, ETextureSlot::Reflection);
			BindTexture(material, aiTextureType_HEIGHT, ETextureSlot::Normals);
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


		if (drawSkybox)
		{
			if (material.GetTextureCount(aiTextureType_AMBIENT))
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "reflectionTexture"));
			else if (!material.GetTextureCount(aiTextureType_DIFFUSE))
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "reflectionColor"));
			else
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "emptyReflectionMap"));
		}
		else
				data.push_back(std::pair<std::string, std::string>("reflectionMapSelection", "emptyReflectionMap"));

		
		if (drawShadows)
			data.push_back(std::pair<std::string, std::string>("shadowMapSelection", "globalShadowMap"));
		else
			data.push_back(std::pair<std::string, std::string>("shadowMapSelection", "emptyShadowMap"));

		if (bumpMapping && material.GetTextureCount(aiTextureType_HEIGHT))
			data.push_back(std::pair<std::string, std::string>("getNormalSelection", "getNormalBumped"));
		else
			data.push_back(std::pair<std::string, std::string>("getNormalSelection", "getNormalSimple"));

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

void MyDrawController::RecursiveRender(const aiScene& scene, const aiNode* nd, const Camera& cam, CShader* overrideProgram, const std::string& shadowMapForLight)
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
		SetupLights(shadowMapForLight);
		SetupProgramTransforms(cam, model, view, proj);
		
		GLint size = 0;
		glBindVertexArray(m_resources.VAOs[nd->mMeshes[i]]);
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	}

	for (int i = 0; i < nd->mNumChildren; ++i) {
		RecursiveRender(scene, nd->mChildren[i], cam, overrideProgram, shadowMapForLight);
	}
}

void MyDrawController::SetupLights(const std::string& onlyLight)
{
	if (drawSkybox)
	{
		glActiveTexture(GL_TEXTURE0 + ETextureSlot::SkyBox);
		currShader->setInt("skybox", ETextureSlot::SkyBox);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);
	}

	int pointLightIndx = 0;
	int dirLightIndx = 0;

	for (int i =0; i < m_pScene->mNumLights; ++i)
	{
		const aiLight& light= *m_pScene->mLights[i];

		assert(light.mType == aiLightSource_POINT || light.mType == aiLightSource_DIRECTIONAL);
		aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName);
		assert(pLightNode);

		if (onlyLight != "" && onlyLight != light.mName.C_Str())
			continue;

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
			currShader->setFloat(lightI + "farPlane", kTMPFarPlane);

			currShader->setVec3("lightPos", glm::vec3(t[3]));//TODO: refactor

			if (drawShadows)
			{
				SShadowMap& sm = m_shadowMaps[light.mName.C_Str()];

			//	currShader->setMat4(lightI + "lightSpaceMatrix", proj*view);	

				glActiveTexture(GL_TEXTURE0 + ETextureSlot::OmniShadowMapStart + pointLightIndx-1);
				currShader->setInt(lightI + "shadowMapTexture", ETextureSlot::OmniShadowMapStart + pointLightIndx-1);
				glBindTexture(GL_TEXTURE_CUBE_MAP, sm.textureId);

				char buff2[100];
				std::string tmp2;
				for (int i=0; i < sm.transforms.size(); ++i)
				{
					snprintf(buff2, sizeof(buff2), "shadowMatrices[%d]", i);
					tmp2 = buff2; 
					currShader->setMat4(tmp2, sm.transforms[i]);
				}

				currShader->setFloat("farPlane", kTMPFarPlane);
			}
			else if (0)
			{
				glActiveTexture(GL_TEXTURE0 + ETextureSlot::OmniShadowMapStart + pointLightIndx-1);
				currShader->setInt(lightI + "shadowMapTexture", ETextureSlot::OmniShadowMapStart + pointLightIndx-1);
				//glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);
				currShader->setInt(lightI + "shadowMapTexture", 0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}

			/*
			std::cout << "----" << std::endl;
			std::cout << light.mAttenuationConstant << std::endl;
			std::cout << light.mAttenuationLinear << std::endl;
			std::cout << light.mAttenuationQuadratic << std::endl;
			*/
		}
		else if (light.mType == aiLightSource_DIRECTIONAL)
		{
			snprintf(buff, sizeof(buff), "dirLights[%d].", dirLightIndx++);
			lightI = buff; 
			currShader->setVec3(lightI + "dir", glm::vec3(t[2]));

			if (drawShadows && 1)
			{
				SShadowMap& sm = m_shadowMaps[light.mName.C_Str()];

				const glm::mat4& view = sm.frustum.GetViewMatrix();
				const glm::mat4& proj = sm.frustum.GetProjMatrix();
				currShader->setMat4("lightSpaceMatrix", proj*view);	

				glActiveTexture(GL_TEXTURE0 + ETextureSlot::DirShadowMap);
				currShader->setInt(lightI + "shadowMapTexture", ETextureSlot::DirShadowMap);
				glBindTexture(GL_TEXTURE_2D, sm.textureId);
			}
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

	assert(dirLightIndx < 2 && "more than one dir light is not supported now due to one dir shadowmap.");
	currShader->setInt("nDirLights", dirLightIndx);
	currShader->setInt("nPointLights", pointLightIndx);
}

void MyDrawController::BuildShadowMaps()
{
	const Camera& currCam = GetCam();
	for (int i =0; i < m_pScene->mNumLights; ++i)
	{
		const aiLight& light= *m_pScene->mLights[i];
		assert(light.mType == aiLightSource_POINT || light.mType == aiLightSource_DIRECTIONAL);
		
		aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName);
		assert(pLightNode);

		aiMatrix4x4 m = pLightNode->mTransformation;
		glm::mat4 t = aiMatrix4x4ToGlm(&m);

		if (m_shadowMaps.find(light.mName.C_Str()) == m_shadowMaps.end())
			m_shadowMaps.emplace(light.mName.C_Str(), SShadowMap());
		
		SShadowMap& shadowMap = m_shadowMaps[light.mName.C_Str()];

		if (light.mType == aiLightSource_DIRECTIONAL)
		{
			const float SHADOW_WIDTH = 1024.0f;
			const float SHADOW_HEIGHT = 1024.0f;

			GLuint depthMapFBO;	
			glGenFramebuffers(1, &depthMapFBO);

			if (!shadowMap.textureId)
				glGenTextures(1, &shadowMap.textureId);

			GLint oldFBO = 0;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

			glBindTexture(GL_TEXTURE_2D, shadowMap.textureId);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
			float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap.textureId, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glClear(GL_DEPTH_BUFFER_BIT);

			const glm::vec3 center(0.0f, 0.0f, 0.0f);// = currCam.Position + 5.0f * currCam.Front;
			const glm::vec3 pos = center + 19.0f * glm::vec3(t[2]);// + 1 * currCam.Front;
			
			Camera lightCam;
			lightCam.Position = pos;
			lightCam.Front= center - pos;
			lightCam.Up = glm::vec3(0.0f, 0.0f, 1.0f);
			lightCam.IsPerspective = false;

			glCullFace(GL_FRONT);
			RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, lightCam, shadowMapShader, light.mName.C_Str());
			glCullFace(GL_BACK);

			glDeleteFramebuffers(1, &depthMapFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);  
			glViewport(0, 0, currCam.Width, currCam.Height);

			if (debugShadowMaps)
				DrawRect2d(currCam.Width - 215, 10, 200, 200, shadowMap.textureId, false, true, -1.0f);
			
			shadowMap.frustum = lightCam;
		}
		else
		{
			GLint oldFBO = 0;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);
			
			GLuint depthCubemapFBO;
			glGenFramebuffers(1, &depthCubemapFBO);

			if (0 == shadowMap.textureId)
				glGenTextures(1, &shadowMap.textureId);

			const float SHADOW_WIDTH = 1024.0f;
			const float SHADOW_HEIGHT = 1024.0f;

			glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap.textureId);
			for (int i=0; i < 6; ++i)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 	

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
			
			glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMap.textureId, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);

			const float aspect = (float)SHADOW_WIDTH/(float)SHADOW_HEIGHT;
			const float near = 0.01f;
			const glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, kTMPFarPlane); 
			const glm::vec3& lightPos = glm::vec3(t[3]);

			shadowMap.transforms.clear();
			std::vector<glm::mat4> shadowTransforms;
			shadowMap.transforms.push_back(shadowProj * 
											 glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowMap.transforms.push_back(shadowProj * 
											 glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowMap.transforms.push_back(shadowProj * 
											 glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 1.0, 1.0)));
			shadowMap.transforms.push_back(shadowProj * 
											 glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
			shadowMap.transforms.push_back(shadowProj * 
											 glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
			shadowMap.transforms.push_back(shadowProj * 
											 glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0, -1.0, 0.0)));

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glClear(GL_DEPTH_BUFFER_BIT);

			Camera emptyCam;
			RecursiveRender(*m_pScene.get(), m_pScene->mRootNode, emptyCam, shadowCubeMapShader, light.mName.C_Str());

			glDeleteFramebuffers(1, &depthCubemapFBO);

			glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);  
			glViewport(0, 0, currCam.Width, currCam.Height);
		}
	}
}

void MyDrawController::RenderInternalForward(const aiScene& scene, const aiNode* nd, const Camera& cam, CShader* overrideProgram, const std::string& shadowMapForLight)
{
	RecursiveRender(scene, nd, cam, overrideProgram, shadowMapForLight);
}

static void GenGBuffer(SGBuffer& gBuffer, const Camera& cam)
{
	//check window resize	
	
	
	if (gBuffer.FBO)
		return;

	GLint oldFBO = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

	const int w = (int)cam.Width;
	const int h = (int)cam.Height;

	glGenFramebuffers(1, &gBuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
	unsigned int gPosition, gNormal, gColorSpec;
		
	// - position color buffer
	glGenTextures(1, &gBuffer.pos);
	glBindTexture(GL_TEXTURE_2D, gBuffer.pos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.pos, 0);
		
	// - normal color buffer
	glGenTextures(1, &gBuffer.normal);
	glBindTexture(GL_TEXTURE_2D, gBuffer.normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.normal, 0);
		
	// - color + specular color buffer
	glGenTextures(1, &gBuffer.albedoSpec);
	glBindTexture(GL_TEXTURE_2D, gBuffer.albedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBuffer.albedoSpec, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
		
	// - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//create depth RBO	
	glGenRenderbuffers(1, &gBuffer.depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, gBuffer.depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gBuffer.depthRBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
			assert(!"Framebuffer not complete!");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
}

void MyDrawController::RenderInternalDeferred(const aiScene& scene, const aiNode* nd, const Camera& cam, CShader* overrideProgram, const std::string& shadowMapForLight)
{
	GenGBuffer(m_resources.GBuffer, cam);

	GLint oldFBO = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_resources.GBuffer.FBO);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	//geometry path
	RenderInternalForward(*m_pScene.get(), m_pScene->mRootNode, cam, deferredGeomPathShader, "");

	glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);

	if (debugGBuffer)
	{
		DrawRect2d(cam.Width - 315, 730, 300, 200, m_resources.GBuffer.pos, false, false, -1.0f);
		DrawRect2d(cam.Width - 315, 515, 300, 200, m_resources.GBuffer.normal, false, false, -1.0f);
		DrawRect2d(cam.Width - 315, 300, 300, 200, m_resources.GBuffer.albedoSpec, false, false, -1.0f);
	}
}

void MyDrawController::Render(const Camera& cam)
{
	glPolygonMode(GL_FRONT_AND_BACK, isWireMode ? GL_LINE : GL_FILL);

	if (drawShadows)
		BuildShadowMaps();	
	else
		ReleaseShadowMaps();

	for (int i=0; i<20; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (deferredShading)
		RenderInternalDeferred(*m_pScene.get(), m_pScene->mRootNode, cam, nullptr, "");
	else
		RenderInternalForward(*m_pScene.get(), m_pScene->mRootNode, cam, nullptr, "");

	if (drawNormals)
		RenderInternalForward(*m_pScene.get(), m_pScene->mRootNode, cam, normalShader, "");
	
	RenderLightModels(cam);

	if (drawSkybox)
		RenderSkyBox(cam);
	else if (debugShadowMaps)
		DebugCubeShadowMap();	

	if (drawGradientReference)
		DrawGradientReference();
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

	glActiveTexture(GL_TEXTURE0 + 4);
	skyboxShader->setInt("skybox", 4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);

	glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(-1, 0, 0));
	
  glm::mat4 view = glm::mat4(glm::mat3(cam.GetViewMatrix())); // remove translation from the view matrix
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

static float screenToNDC(float val, float scrSize)
{
	return (val / scrSize - 0.5f) * 2.0f;
}

static void DrawRect2d(float x, float y, float w, float h, const glm::vec3& color, GLuint textureId, Camera& cam, bool doGammaCorrection, bool debugShadowMap, float HDRexposure)
{
	const float scrW = cam.Width;
	const float scrH = cam.Height;

	const float x0 = screenToNDC(x, scrW);
	const float y0 = screenToNDC(y, scrH);

	const float x1 = screenToNDC(x, scrW);
	const float y1 = screenToNDC(y+h, scrH);

	const float x2 = screenToNDC(x+w, scrW);
	const float y2 = screenToNDC(y, scrH);

	const float x3 = screenToNDC(x+w, scrW);
	const float y3 = screenToNDC(y+h, scrH);

	const float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		x0,  y0,  0.0f, 0.0f,
		x2,  y2,  1.0f, 0.0f,
		x1,  y1,  0.0f, 1.0f,

		x3,  y3,  1.0f, 1.0f,
		x1,  y1,  0.0f, 1.0f,
		x2,  y2,  1.0f, 0.0f
	};

	// screen quad VAO
	GLuint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	rect2dShader->use();
	if (textureId)
	{
		rect2dShader->setBool("useColor", false);
		glActiveTexture(GL_TEXTURE0);
		rect2dShader->setInt("in_texture", 0);
		glBindTexture(GL_TEXTURE_2D, textureId);
	}
	else
	{
		rect2dShader->setVec3("color", color);
		rect2dShader->setBool("useColor", true);
	}

	rect2dShader->setBool("debugShadowMap", debugShadowMap);

	rect2dShader->setBool("doGammaCorrection", doGammaCorrection);

	rect2dShader->setFloat("HDR_exposure", HDRexposure);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, sizeof(quadVertices));	

	glDeleteBuffers(1, &quadVBO);
	glDeleteVertexArrays(1, &quadVAO);
}

void MyDrawController::DrawRect2d(float x, float y, float w, float h, const glm::vec3& color, bool doGammaCorrection)
{
	::DrawRect2d(x, y, w, h, color, 0, GetCam(), doGammaCorrection, false, -1.0f);
}

void MyDrawController::DrawRect2d(float x, float y, float w, float h, GLuint textureId, bool doGammaCorrection, bool debugShadowMap, float HDRexposure)
{
	glm::vec3 color;
	::DrawRect2d(x, y, w, h, color, textureId, GetCam(), doGammaCorrection, debugShadowMap, HDRexposure);
}

void MyDrawController::DrawGradientReference()
{
	const int cnt = 30;
	const int rectW = 30;
	const int rectH = 90;
	const int borderSize = 5;
	
	GLboolean wasDepthTest = 1;
	glGetBooleanv(GL_DEPTH_TEST, &wasDepthTest);
	glDisable(GL_DEPTH_TEST);

	DrawRect2d(10, 10, cnt * rectW + 2*borderSize, rectH + 2*borderSize, glm::vec3(1, 1, 1), false);

	for (int i=0; i<cnt; ++i)
	{
		const float color = float(i) / (cnt-1);
		DrawRect2d(10 + borderSize + i*rectW, 10 + borderSize, rectW, rectH, glm::vec3(color), false);
	}

	if (wasDepthTest)
		glEnable(GL_DEPTH_TEST);
		

}

void MyDrawController::DebugCubeShadowMap()
{
	
	if (debugOnmiShadowLightName.empty())
		return;

	auto it = m_shadowMaps.find(debugOnmiShadowLightName.c_str());

	if (it == m_shadowMaps.end())
		return;

	SShadowMap& shadowMap = it->second;

	glDepthFunc(GL_LEQUAL);
	debugShadowCubeMapShader->use();
	glBindVertexArray(m_resources.cubeVAOID);

	glActiveTexture(GL_TEXTURE0 + ETextureSlot::SkyBox);
	debugShadowCubeMapShader->setInt("in_texture", ETextureSlot::SkyBox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap.textureId);

	//glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(-1, 0, 0));
	
  glm::mat4 view = glm::mat4(glm::mat3(m_cam.GetViewMatrix())); // remove translation from the view matrix
	glm::mat4 mvp_matrix = m_cam.GetProjMatrix() * view ;//* rot;
	debugShadowCubeMapShader->setMat4("MVP", mvp_matrix);
	debugShadowCubeMapShader->setFloat("farPlane", kTMPFarPlane);
	
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
	glDepthFunc(GL_LESS);
}
