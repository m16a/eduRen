#pragma once

#include "camera.h"
#include "input_handler.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <GL/gl3w.h>

#include <array>
#include <map>
#include <memory>
#include <string>

class CShader;
constexpr const int kMaxMeshesCount = 100;

struct SGBuffer {
  GLuint FBO{0};
  GLuint pos{0};
  GLuint normal{0};
  GLuint albedoSpec{0};
  GLuint depthRBO{0};
};

struct SResourceHandlers {
  using TArr = std::array<GLuint, kMaxMeshesCount>;
  TArr VAOs;
  TArr vertIDs;
  TArr elemIDs;
  TArr normIDs;
  TArr uvIDs;
  TArr texturesIDs;
  TArr tangentsIDs;
  TArr bitangentsIDs;

  std::map<std::string, GLuint> texturePathToID;

  GLuint skyboxTextID;

  GLuint cubeVertID;
  GLuint cubeElemID;
  GLuint cubeVAOID;

  SGBuffer GBuffer;

  void Release();
};

enum EBumpMappingType { Normal, Height };

class MyDrawController {
 public:
  MyDrawController();
  ~MyDrawController();
  void Render(const Camera& cam);
  void Load();

  Camera& GetCam() { return m_cam; }
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
  static bool debugShadowMaps;
  static std::string debugOnmiShadowLightName;
  static std::vector<std::string> pointLightNames;

  static bool bumpMapping;
  static EBumpMappingType bumpMappingType;

  static bool HDR;
  static bool deferredShading;
  static bool debugGBuffer;

  void DrawRect2d(float x, float y, float w, float h, const glm::vec3& color,
                  bool doGammaCorrection);
  void DrawRect2d(float x, float y, float w, float h, GLuint textureId,
                  bool doGammaCorrection, bool debugShadowMap,
                  float HDRexposure);

 private:
  bool LoadScene(const std::string& path);
  void InitLightModel();
  void RecursiveRender(const aiScene& scene, const aiNode* nd,
                       const Camera& cam, CShader* overrideProgram,
                       const std::string& shadowMapForLight);
  void RenderInternalForward(const aiScene& scene, const aiNode* nd,
                             const Camera& cam, CShader* overrideProgram,
                             const std::string& shadowMapForLight);
  void RenderInternalDeferred(const aiScene& scene, const aiNode* nd,
                              const Camera& cam, CShader* overrideProgram,
                              const std::string& shadowMapForLight);
  void RenderSkyBox(const Camera& cam);
  void RenderLightModels(const Camera& cam);
  void BindTexture(const aiMaterial& mat, aiTextureType type, int startIndx);
  void SetupLights(const std::string& onlyLight);
  void LoadMeshesData();
  void SetupMaterial(const aiMesh& mesh, CShader* overrideProgram);
  void SetupProgramTransforms(const Camera& cam, const glm::mat4& model,
                              const glm::mat4& view, const glm::mat4& proj);
  void BuildShadowMaps();
  void ReleaseShadowMaps();
  void DebugCubeShadowMap();

  // draw gradient to debug gamma correction
  void DrawGradientReference();

 private:
  struct SShadowMap {
    Camera frustum;
    GLuint textureId{0};
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
