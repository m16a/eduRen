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
#include <array>

class CShader;
constexpr const int kMaxMeshesCount = 100;
struct SResourceHandlers
{
	using TArr = std::array<GLuint, kMaxMeshesCount>;
	TArr VAOs;
	TArr vertIDs;
	TArr elemIDs;
	TArr normIDs;
	TArr uvIDs;
	TArr texturesIDs;

	std::map<std::string, GLuint> texturePathToID;

	GLuint skyboxTextID;

	GLuint cubeVertID;
	GLuint cubeElemID;
	GLuint cubeVAOID;

	void Release();
};

class MyDrawController
{
	public:
	MyDrawController();
	~MyDrawController();
	void Render(const Camera& cam);
	void Load();

	Camera& GetCam() { return m_cam;}
	CInputHandler& GetInputHandler() { return m_inputHandler; }
	const aiScene* GetScene() const { return m_pScene.get(); }
	
	static bool isWireMode;
	static bool isAmbient;
	static bool isDiffuse;
	static bool isSpecular;
	static bool drawSkybox;
	static bool drawNormals;
	static bool drawGradientReference;
	static bool isMSAA;
	static bool isGammaCorrection;
	static bool drawShadows;
	static bool debugCubeShadowMap;

	void DrawRect2d(float x, float y, float w, float h, const glm::vec3& color, bool doGammaCorrection);
	void DrawRect2d(float x, float y, float w, float h, GLuint textureId, bool doGammaCorrection, bool debugShadowMap);

	private:
	bool LoadScene(const std::string& path);
	void InitLightModel();
	void RecursiveRender(const aiScene& scene, const aiNode* nd, const Camera& cam, CShader* overrideProgram);
	void RenderSkyBox(const Camera& cam);
	void RenderLightModels(const Camera& cam);
	void BindTexture(const aiMaterial& mat, aiTextureType type, int startIndx);
	void SetupLights();
	void LoadMeshesData();
	void SetupMaterial(const aiMesh& mesh, CShader* overrideProgram);
	void SetupProgramTransforms(const Camera& cam, const glm::mat4& model,const glm::mat4& view, const glm::mat4& proj);
	void BuildShadowMaps();	
	void ReleaseShadowMaps();	
	void DebugCubeShadowMap();	

	// draw gradient to debug gamma correction
	void DrawGradientReference();

	private:
	struct SShadowMap
	{
		Camera frustum;
		GLuint textureId {0};
		std::vector<glm::mat4> transforms;
	};

	std::map<std::string, SShadowMap> m_shadowMaps;	

	private:
	Camera m_cam;
	CInputHandler m_inputHandler;
	SResourceHandlers m_resources;

	std::unique_ptr<const aiScene> m_pScene;
	aiVector3D m_scene_min, m_scene_max, m_scene_center;
	std::string m_dirPath;
};
