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
constexpr const int kMaxMeshesCount = 400;

struct SGBuffer {
  GLuint FBO{0};
  GLuint pos{0};
  GLuint normal{0};
  GLuint albedoSpec{0};
  GLuint depth{0};  // redundant in general, pos buffer can be reused
};

struct SSSAO {
  GLuint pass1FBO{0};
  GLuint pass1Txt{0};
  GLuint noiseTxt{0};
  std::vector<glm::vec3> kernel;

  // blured
  GLuint FBO{0};
  GLuint colorTxt{0};
};

struct SEnvProbe {
  GLuint cubeMap{0};
  GLuint irradianceMap{0};
  GLuint prefilterdMap{0};
  GLuint brdfLUT{0};
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

  // full-screen quad
  GLuint fsQuadVAOID;
  GLuint fsQuadVBOID;

  SGBuffer GBuffer;
  SSSAO ssao;

  SEnvProbe envProbe;

  void Release();
};

enum EBumpMappingType { Normal, Height };

enum ECustomPBRTextureType { Albedo = 0, Norm, Metallic, Roughness, AO, Count };

class MyDrawController {
 public:
  MyDrawController();
  ~MyDrawController();
  void Render(const Camera& cam);
  void Load();

  Camera& GetCam() { return m_cam; }
  CInputHandler& GetInputHandler() { return m_inputHandler; }
  const aiScene* GetScene() const { return m_pScene; }

  static bool clamp60FPS;
  static bool isWireMode;
  static bool isAmbient;
  static bool isDiffuse;
  static bool isSpecular;
  static bool drawSkybox;
  static bool drawNormals;
  static bool drawGradientReference;
  static bool isMSAA;
  static bool isGammaCorrection;
  static bool isSSAO;
  static bool debugSSAO;

  static bool isPBR;
  static bool isIBL;

  static bool drawShadows;
  static bool debugShadowMaps;
  static std::string debugOnmiShadowLightName;
  static std::vector<std::string> pointLightNames;

  static bool bumpMapping;
  static EBumpMappingType bumpMappingType;

  static bool HDR;
  static float HDR_exposure;
  static bool deferredShading;
  static bool debugGBuffer;

  static glm::vec4 clearColor;

  void DrawRect2d(float x, float y, float w, float h, const glm::vec3& color,
                  bool doGammaCorrection);
  void DrawRect2d(float x, float y, float w, float h, GLuint textureId,
                  bool doGammaCorrection, bool bOneColorChannel,
                  float HDRexposure);

  SResourceHandlers m_resources;

 private:
  bool LoadScene(const std::string& path);
  void InitLightModel();
  void InitFsQuad();
  void RenderFsQuad();
  void RecursiveRender(const aiScene& scene, const aiNode* nd,
                       const Camera& cam,
                       std::shared_ptr<CShader>& overrideProgram,
                       const std::string& shadowMapForLight);
  void RenderInternalForward(const aiScene& scene, const aiNode* nd,
                             const Camera& cam,
                             std::shared_ptr<CShader>& overrideProgram,
                             const std::string& shadowMapForLight);
  void RenderInternalDeferred(const aiScene& scene, const aiNode* nd,
                              const Camera& cam,
                              std::shared_ptr<CShader>& overrideProgram,
                              const std::string& shadowMapForLight);
  void RenderSkyBox(const Camera& cam);
  void RenderLightModels(const Camera& cam);

  void PerformSSAO(const SSSAO& ssao, const SGBuffer& gBuffer,
                   const Camera& cam);
  // returns true on success
  bool BindTexture(const aiMaterial& mat, aiTextureType type, int indx);
  bool BindPBRTexture(ECustomPBRTextureType type, const std::string& path);
  void SetupLights(const std::string& onlyLight);
  void LoadMeshesData();
  void SetupMaterial(const aiMesh& mesh,
                     std::shared_ptr<CShader>& overrideProgram);
  void SetupProgramTransforms(const Camera& cam, const glm::mat4& model,
                              const glm::mat4& view, const glm::mat4& proj);
  void BuildShadowMaps();
  void ReleaseShadowMaps();
  void DebugCubeShadowMap();

  // draw gradient to debug gamma correction
  void DrawGradientReference();

  void IBL_PrecomputeEnvProbe(const Camera& cam, SEnvProbe& out_probe);

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

  const aiScene* m_pScene;
  aiVector3D m_scene_min, m_scene_max, m_scene_center;
  std::string m_dirPath;
};
