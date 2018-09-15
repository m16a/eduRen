#pragma once

#include "camera.h"
#include "input_handler.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/gl3w.h>

#include <memory>
#include <map>
#include <string>

class Shader;

class MyDrawController
{
	public:
	MyDrawController();
	void Init();
	void Render(const Camera& cam);

	Camera& GetCam() { return m_cam;}
	CInputHandler& GetInputHandler() { return m_inputHandler; }
	const aiScene* GetScene() const { return m_pScene.get(); }
	
	bool LoadScene(const std::string& path);
	void InitTextures(const aiScene& scene);
	void LoadTextureForMaterial(const aiMaterial& mat);

	void InitLightModel();

	static bool isWireMode;
	static bool isAmbient;
	static bool isDiffuse;
	static bool isSpecular;
	static bool drawSkybox;
	static bool drawNormals;

	private:
	void RecursiveRender(const aiScene& scene, const aiNode* nd, const Camera& cam, bool drawNormals);
	void RenderSkyBox(const Camera& cam);
	void RenderLightModels(const Camera& cam);
	void BindTexture(const aiMaterial& mat, aiTextureType type, int startIndx);
	void SetupLights(Shader* currShader);

	private:
	Camera m_cam;
	CInputHandler m_inputHandler;

	std::unique_ptr<const aiScene> m_pScene;
	aiVector3D m_scene_min, m_scene_max, m_scene_center;
	std::map<std::string, GLuint> m_texturePathToID;
	std::string m_dirPath;
};
