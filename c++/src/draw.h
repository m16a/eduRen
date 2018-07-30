#include "vmath.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/gl3w.h>

#include <memory>

class MyDrawController
{
	public:
	void Init();
	void Render(int w, int h);

	//camera
	vmath::vec3 GetCamDir() const { return m_camDir;}
	
	//input handling
	void OnKeyUp();
	void OnKeyDown();
	void OnKeyRight();
	void OnKeyLeft();

	void OnKeyW();
	void OnKeyS();
	void OnKeyA();
	void OnKeyD();
	void OnKeySpace();

	public:
	bool LoadScene(const char* path);

	const aiScene* GetScene() const { return m_pScene.get(); }

	private:
	void RecursiveRender(const aiScene& scene, const aiNode* nd, int w, int h);

	private:
	vmath::vec3 m_camPos;
	vmath::vec3 m_camDir;

	vmath::mat4 m_cameraTransform;

	std::unique_ptr<const aiScene> m_pScene;
	aiVector3D m_scene_min, m_scene_max, m_scene_center;
};
