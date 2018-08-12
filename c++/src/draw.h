#include "camera.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/gl3w.h>

#include <memory>
#include <map>
#include <string>

class MyDrawController
{
	public:
	void Init();
	void Render(int w, int h, int fov);

	Camera& GetCam() { return m_cam;}
	
	//input handling
	void OnKeyUp(float dt);
	void OnKeyDown(float dt);
	void OnKeyRight(float dt);
	void OnKeyLeft(float dt);

	void OnKeyW(float dt, bool haste);
	void OnKeyS(float dt, bool haste);
	void OnKeyA(float dt, bool haste);
	void OnKeyD(float dt, bool haste);
	void OnKeySpace(float dt);

	bool LoadScene(const std::string& path);
	const aiScene* GetScene() const { return m_pScene.get(); }
	void InitTextures(const aiScene& scene);
	void LoadTextureForMaterial(const aiMaterial& mat);

	void InitLightModel();

	static bool isWireMode;
	static bool isAmbient;
	static bool isDiffuse;
	static bool isSpecular;

	private:
	void RecursiveRender(const aiScene& scene, const aiNode* nd, int w, int h, int fov);

	private:
	Camera m_cam;

	std::unique_ptr<const aiScene> m_pScene;
	aiVector3D m_scene_min, m_scene_max, m_scene_center;
	std::map<std::string, GLuint> m_texturePathToID;
	std::string m_dirPath;
};
