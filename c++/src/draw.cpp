#include "draw.h"
#include "cube.h"
#include "input_handler.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include "shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool MyDrawController::clamp60FPS = true;
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
bool MyDrawController::isSSAO = false;
bool MyDrawController::debugSSAO = false;

bool MyDrawController::debugShadowMaps = false;
std::string MyDrawController::debugOnmiShadowLightName = std::string();
std::vector<std::string> MyDrawController::pointLightNames =
    std::vector<std::string>();

bool MyDrawController::bumpMapping = true;
EBumpMappingType MyDrawController::bumpMappingType = Normal;
bool MyDrawController::HDR = false;
float MyDrawController::HDR_exposure = 0.0f;
bool MyDrawController::deferredShading = false;
bool MyDrawController::debugGBuffer = false;

glm::vec4 MyDrawController::clearColor(57.f / 255.0f, 57.f / 255.0f,
                                       57.f / 255.0f, 1.00f);

enum Attrib_IDs {
  vPosition = 0,
  vNormals = 1,
  uvTextCoords = 2,
  vTangents = 3,
  vBitangents = 4
};

std::shared_ptr<CShader> mainShader;
std::shared_ptr<CShader> lightModelShader;
std::shared_ptr<CShader> skyboxShader;
std::shared_ptr<CShader> normalShader;
std::shared_ptr<CShader> rect2dShader;
std::shared_ptr<CShader> shadowMapShader;
std::shared_ptr<CShader> shadowCubeMapShader;
std::shared_ptr<CShader> debugShadowCubeMapShader;
std::shared_ptr<CShader> deferredGeomPathShader;
std::shared_ptr<CShader> deferredLightPathShader;
std::shared_ptr<CShader> ssaoShader;
std::shared_ptr<CShader> blurShader;
std::shared_ptr<CShader> currShader;

std::shared_ptr<CShader> nullShader;

static const float kTMPFarPlane = 100.0f;  // TODO: refactor

enum ETextureSlot {
  Empty = 0,
  Diffuse,
  Specular,
  Reflection,
  Normals,
  SkyBox,
  DirShadowMap,
  Opacity,
  OmniShadowMapStart = 10,
  OmniShadowMapEnd = 19
};

inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
  glm::mat4 to;

  to[0][0] = (GLfloat)from->a1;
  to[0][1] = (GLfloat)from->b1;
  to[0][2] = (GLfloat)from->c1;
  to[0][3] = (GLfloat)from->d1;
  to[1][0] = (GLfloat)from->a2;
  to[1][1] = (GLfloat)from->b2;
  to[1][2] = (GLfloat)from->c2;
  to[1][3] = (GLfloat)from->d2;
  to[2][0] = (GLfloat)from->a3;
  to[2][1] = (GLfloat)from->b3;
  to[2][2] = (GLfloat)from->c3;
  to[2][3] = (GLfloat)from->d3;
  to[3][0] = (GLfloat)from->a4;
  to[3][1] = (GLfloat)from->b4;
  to[3][2] = (GLfloat)from->c4;
  to[3][3] = (GLfloat)from->d4;

  return to;
}

unsigned int TextureFromFile(const char* path, const std::string& directory) {
  std::string filename = std::string(path);
  filename = directory + '/' + filename;

  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data =
      stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
  if (data) {
    GLenum format;
    if (nrComponents == 1) format = GL_RED;
    if (nrComponents == 2)
      format = GL_RG;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
  } else {
    std::cout << "Texture failed to load at path: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}

void MergeElements(const aiMesh& mesh, std::vector<unsigned int>& res) {
  for (int i = 0; i < mesh.mNumFaces; ++i) {
    assert(mesh.mFaces[i].mNumIndices == 3);
    res.push_back(mesh.mFaces[i].mIndices[0]);
    res.push_back(mesh.mFaces[i].mIndices[1]);
    res.push_back(mesh.mFaces[i].mIndices[2]);
  }
}

void MergeUV(const aiMesh& mesh, std::vector<glm::vec2>& res) {
  if (mesh.mTextureCoords == nullptr || mesh.mNumUVComponents[0] == 0) return;

  assert(mesh.mNumUVComponents[0] == 2);
  glm::vec2 tmp;
  for (int i = 0; i < mesh.mNumVertices; ++i) {
    tmp[0] = mesh.mTextureCoords[0][i][0];
    tmp[1] = mesh.mTextureCoords[0][i][1];
    res.push_back(tmp);
  }
}

std::string GetUniformTextureName(aiTextureType type) {
  switch (type) {
    case aiTextureType_DIFFUSE:
      return "inTexture.diff";
      break;
    case aiTextureType_SPECULAR:
      return "inTexture.spec";
      break;
    case aiTextureType_HEIGHT:
    case aiTextureType_NORMALS:
      return "inTexture.norm";
      break;
    case aiTextureType_AMBIENT:
      return "inTexture.reflection";
      break;
    case aiTextureType_OPACITY:
    case aiTextureType_UNKNOWN:
      return "inTexture.opacity";
      break;
  }

  assert(false && "provide texture unoform name");
  return "none";
}

unsigned int LoadCubemap() {
  static std::string pathToSkyboxFolder =
      "/home/m16a/Documents/github/eduRen/models/skybox/skybox/";
  static std::vector<std::string> faces = {"right.jpg",  "left.jpg",
                                           "bottom.jpg", "top.jpg",
                                           "front.jpg",  "back.jpg"};

  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  int width, height, nrChannels;
  for (unsigned int i = 0; i < faces.size(); i++) {
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load((pathToSkyboxFolder + faces[i]).c_str(),
                                    &width, &height, &nrChannels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      std::cout << "Cubemap texture failed to load at path: " << faces[i]
                << std::endl;
      stbi_image_free(data);
    }
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  return textureID;
}

//=========================================================================================

MyDrawController::MyDrawController() : m_inputHandler(*this) {}

MyDrawController::~MyDrawController() {
  m_resources.Release();
  ReleaseShadowMaps();
}

void MyDrawController::ReleaseShadowMaps() {
  for (auto& sm : m_shadowMaps) {
    if (0 != sm.second.textureId) {
      glDeleteTextures(1, &sm.second.textureId);
      sm.second.textureId = 0;
    }
  }
}

void MyDrawController::InitLightModel() {
  glGenVertexArrays(1, &m_resources.cubeVAOID);
  glGenBuffers(1, &m_resources.cubeVertID);

  glBindVertexArray(m_resources.cubeVAOID);

  glBindBuffer(GL_ARRAY_BUFFER, m_resources.cubeVertID);
  glBufferData(GL_ARRAY_BUFFER, 3 * cubeVerticiesCount * sizeof(GLfloat),
               cubeVertices, GL_STATIC_DRAW);

  lightModelShader =
      std::make_unique<CShader>("shaders/light.vert", "shaders/light.frag");

  glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(vPosition);

  glGenBuffers(1, &m_resources.cubeElemID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resources.cubeElemID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndiciesCount * sizeof(GLint),
               cubeIndices, GL_STATIC_DRAW);
}

void MyDrawController::InitFsQuad() {
  const float quadVertices[] = {
      // vertex attributes for a quad that fills the entire screen in
      // Normalized Device Coordinates.
      // positions   // texCoords
      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
      1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
  };

  // screen quad VAO
  glGenVertexArrays(1, &m_resources.fsQuadVAOID);
  glGenBuffers(1, &m_resources.fsQuadVBOID);
  glBindVertexArray(m_resources.fsQuadVAOID);
  glBindBuffer(GL_ARRAY_BUFFER, m_resources.fsQuadVBOID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));
}

void MyDrawController::RenderFsQuad() {
  glBindVertexArray(m_resources.fsQuadVAOID);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 20 /*sizeof(quadVertices)*/);
}

void MyDrawController::LoadMeshesData() {
  const unsigned int meshN = GetScene()->mNumMeshes;

  glGenVertexArrays(meshN, m_resources.VAOs.data());
  glGenBuffers(meshN, m_resources.vertIDs.data());
  glGenBuffers(meshN, m_resources.elemIDs.data());
  glGenBuffers(meshN, m_resources.normIDs.data());
  glGenBuffers(meshN, m_resources.uvIDs.data());
  glGenBuffers(meshN, m_resources.tangentsIDs.data());
  glGenBuffers(meshN, m_resources.bitangentsIDs.data());

  for (int i = 0; i < meshN; ++i) {
    const aiMesh* pMesh = GetScene()->mMeshes[i];
    assert(pMesh);

    glBindVertexArray(m_resources.VAOs[i]);

    glBindBuffer(GL_ARRAY_BUFFER, m_resources.vertIDs[i]);
    glBufferData(GL_ARRAY_BUFFER, pMesh->mNumVertices * sizeof(aiVector3D),
                 pMesh->mVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition);

    glBindBuffer(GL_ARRAY_BUFFER, m_resources.normIDs[i]);
    glBufferData(GL_ARRAY_BUFFER, pMesh->mNumVertices * sizeof(aiVector3D),
                 pMesh->mNormals, GL_STATIC_DRAW);
    glVertexAttribPointer(vNormals, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vNormals);

    // TextCoordBuffers
    std::vector<glm::vec2> uvTmp;
    MergeUV(*pMesh, uvTmp);
    if (!uvTmp.empty()) {
      glBindBuffer(GL_ARRAY_BUFFER, m_resources.uvIDs[i]);
      glBufferData(GL_ARRAY_BUFFER, pMesh->mNumVertices * sizeof(glm::vec2),
                   uvTmp.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(uvTextCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(uvTextCoords);
    }

    std::vector<unsigned int> elms;
    MergeElements(*pMesh, elms);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_resources.elemIDs[i]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elms.size() * sizeof(unsigned int),
                 elms.data(), GL_STATIC_DRAW);

    if (pMesh->mTangents) {
      glBindBuffer(GL_ARRAY_BUFFER, m_resources.tangentsIDs[i]);
      glBufferData(GL_ARRAY_BUFFER, pMesh->mNumVertices * sizeof(aiVector3D),
                   pMesh->mTangents, GL_STATIC_DRAW);
      glVertexAttribPointer(vTangents, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(vTangents);

      assert(pMesh->mTangents && "bitangents should exist");
      glBindBuffer(GL_ARRAY_BUFFER, m_resources.bitangentsIDs[i]);
      glBufferData(GL_ARRAY_BUFFER, pMesh->mNumVertices * sizeof(aiVector3D),
                   pMesh->mBitangents, GL_STATIC_DRAW);
      glVertexAttribPointer(vBitangents, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(vBitangents);
    }
  }
}

bool MyDrawController::LoadScene(const std::string& path) {
  bool res = false;
  if (const aiScene* p =
          aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality
#if 0
                                         | aiProcess_FlipWindingOrder
#endif
                       )) {
    const unsigned int meshN = p->mNumMeshes;
    if (meshN > kMaxMeshesCount) {
      std::cout << "[ERROR] Supported meshes overflow. " << meshN << "/"
                << kMaxMeshesCount << std::endl;
      return res;
    } else if (p->mNumTextures) {
      std::cout
          << "[ERROR][TODO] embeded textures are not handled. Textures count: "
          << p->mNumTextures << std::endl;
    } else {
      m_pScene = p;
      res = true;
    }
  } else
    std::cout << "[assimp error]" << aiGetErrorString() << std::endl;

  m_dirPath = path.substr(0, path.find_last_of('/'));

  if (res) {
    for (int i = 0; i < m_pScene->mNumLights; ++i) {
      const aiLight& light = *m_pScene->mLights[i];
      if (light.mType == aiLightSource_POINT)
        pointLightNames.push_back(light.mName.C_Str());
    }
  }

  return res;
}

void MyDrawController::Load() {
#if 1
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/sponza_cry/sponza.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/"
      "untitled.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/my_scenes/ssao/"
      "untitled.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/"
      "sponza.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/my_scenes/cubeWithLamp/"
      "sponza.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/paralax/"
      "sponzacry.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/earth/"
      "untitled.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/box/"
      "untitled.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/alpha_mask/untitled.blend");
#endif

#if 0
  bool res = LoadScene(
      "/home/m16a/Documents/github/eduRen/models/alpha_mask/obj/objects.blend");
#endif
  // bool res =
  // LoadScene("/home/m16a/Documents/github/eduRen/models/dragon_recon/dragon_vrip_res4.ply");
  // bool res =
  // LoadScene("/home/m16a/Documents/github/eduRen/models/bunny/reconstruction/bun_zipper_res4.ply");
  assert(res && "cannot load scene");

  LoadMeshesData();
  mainShader =
      std::make_shared<CShader>("shaders/main.vert", "shaders/main.frag");

  skyboxShader =
      std::make_shared<CShader>("shaders/skybox.vert", "shaders/skybox.frag");
  normalShader = std::make_shared<CShader>(
      "shaders/normal.vert", "shaders/normal.frag", "shaders/normal.geom");
  rect2dShader =
      std::make_shared<CShader>("shaders/rect2d.vert", "shaders/rect2d.frag");
  shadowMapShader = std::make_shared<CShader>("shaders/shadowMap.vert",
                                              "shaders/shadowMap.frag");
  shadowCubeMapShader = std::make_shared<CShader>("shaders/shadowCubeMap.vert",
                                                  "shaders/shadowCubeMap.frag",
                                                  "shaders/shadowCubeMap.geom");
  debugShadowCubeMapShader = std::make_shared<CShader>(
      "shaders/debugCubeShadowMap.vert", "shaders/debugCubeShadowMap.frag");
  deferredGeomPathShader = std::make_shared<CShader>(
      "shaders/deferredGeomPath.vert", "shaders/deferredGeomPath.frag");
  deferredLightPathShader = std::make_shared<CShader>(
      "shaders/deferredLightPath.vert", "shaders/deferredLightPath.frag");

  ssaoShader =
      std::make_shared<CShader>("shaders/ssao.vert", "shaders/ssao.frag");

  blurShader =
      std::make_shared<CShader>("shaders/blur.vert", "shaders/blur.frag");

  InitLightModel();
  InitFsQuad();
  m_resources.skyboxTextID = LoadCubemap();
}

bool MyDrawController::BindTexture(const aiMaterial& mat, aiTextureType type,
                                   int indx) {
  bool res = false;
  // lazy loading textures
  unsigned int texCnt = mat.GetTextureCount(type);
  // assert(texCnt <= 1 && "multiple textures for one type is not supported");

  if (texCnt) {
    int i = 0;
    aiString path;
    if (!mat.GetTexture(type, i, &path)) {
      auto it = m_resources.texturePathToID.find(std::string(path.C_Str()));
      GLuint id = 0;

      if (it == m_resources.texturePathToID.end()) {
        id = TextureFromFile(path.C_Str(), m_dirPath);
        m_resources.texturePathToID[std::string(path.C_Str())] = id;
      } else
        id = it->second;

      glActiveTexture(GL_TEXTURE0 + indx);
      currShader->setInt(GetUniformTextureName(type).c_str(), indx);
      glBindTexture(GL_TEXTURE_2D, id);
      res = true;
    } else {
      std::cout << "Texture reading fail\n";
    }
  } else {
    glActiveTexture(GL_TEXTURE0 + indx);
    currShader->setInt(GetUniformTextureName(type).c_str(), indx);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  return res;
}

void MyDrawController::SetupMaterial(
    const aiMesh& mesh, std::shared_ptr<CShader>& overrideProgram) {
  assert(GetScene()->mNumMaterials);

  unsigned int matIndx = mesh.mMaterialIndex;
  // std::cout << "matIndx: " << matIndx << std::endl;
  const aiMaterial& material = *m_pScene->mMaterials[matIndx];

  if (overrideProgram)
    currShader = overrideProgram;
  else
    currShader = mainShader;

  currShader->use();

  if (currShader == mainShader || currShader == deferredGeomPathShader) {
    CShader::TSubroutineTypeToInstance data;
    if (material.GetTextureCount(aiTextureType_DIFFUSE)) {
      data.push_back(std::pair<std::string, std::string>("baseColorSelection",
                                                         "textColor"));
      BindTexture(material, aiTextureType_DIFFUSE, ETextureSlot::Diffuse);
      BindTexture(material, aiTextureType_SPECULAR, ETextureSlot::Specular);
      BindTexture(material, aiTextureType_AMBIENT, ETextureSlot::Reflection);

      // normal map can be placed under different names. can't figure out
      if (!BindTexture(material, aiTextureType_NORMALS, ETextureSlot::Normals))
        BindTexture(material, aiTextureType_HEIGHT, ETextureSlot::Normals);

      // if (BindTexture(material, aiTextureType_OPACITY,
      // ETextureSlot::Opacity)) {
      if (BindTexture(material, aiTextureType_UNKNOWN, ETextureSlot::Opacity)) {
        data.push_back(std::pair<std::string, std::string>("opacitySelection",
                                                           "maskOpacity"));
      } else {
        data.push_back(std::pair<std::string, std::string>("opacitySelection",
                                                           "emptyOpacity"));
      }

    } else {
      data.push_back(std::pair<std::string, std::string>("baseColorSelection",
                                                         "plainColor"));

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

    if (drawSkybox) {
      if (material.GetTextureCount(aiTextureType_AMBIENT))
        data.push_back(std::pair<std::string, std::string>(
            "reflectionMapSelection", "reflectionTexture"));
      else if (!material.GetTextureCount(aiTextureType_DIFFUSE))
        data.push_back(std::pair<std::string, std::string>(
            "reflectionMapSelection", "reflectionColor"));
      else
        data.push_back(std::pair<std::string, std::string>(
            "reflectionMapSelection", "emptyReflectionMap"));
    } else
      data.push_back(std::pair<std::string, std::string>(
          "reflectionMapSelection", "emptyReflectionMap"));

    if (drawShadows)
      data.push_back(std::pair<std::string, std::string>("shadowMapSelection",
                                                         "globalShadowMap"));
    else
      data.push_back(std::pair<std::string, std::string>("shadowMapSelection",
                                                         "emptyShadowMap"));

    if (bumpMapping && (material.GetTextureCount(aiTextureType_HEIGHT) ||
                        material.GetTextureCount(aiTextureType_NORMALS))) {
      if (bumpMappingType == Normal) {
        data.push_back(std::pair<std::string, std::string>("getNormalSelection",
                                                           "getNormalBumped"));
        // data.push_back(std::pair<std::string,
        // std::string>("getHeightSelection", "getHeightEmpty"));
      } else if (bumpMappingType == Height) {
        // data.push_back(std::pair<std::string,
        // std::string>("getHeightlSelection", "getHeightBumped"));

        data.push_back(std::pair<std::string, std::string>(
            "getNormalSelection", "getNormalFromHeight"));

        data.push_back(std::pair<std::string, std::string>("baseColorSelection",
                                                           "textHeightColor"));
      } else
        assert(0);
    } else {
      data.push_back(std::pair<std::string, std::string>("getNormalSelection",
                                                         "getNormalSimple"));
    }

    currShader->setSubroutine(GL_FRAGMENT_SHADER, data);
  }
}

void MyDrawController::SetupProgramTransforms(const Camera& cam,
                                              const glm::mat4& model,
                                              const glm::mat4& view,
                                              const glm::mat4& proj) {
  currShader->setMat4("model", model);
  currShader->setMat4("view", view);
  currShader->setMat4("proj", proj);
  currShader->setVec3("camPos", cam.Position);

  if (drawSkybox) {
    glm::mat4 rot =
        glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(1, 0, 0));
    currShader->setMat4("rotfix", rot);
  }
}

void MyDrawController::RecursiveRender(
    const aiScene& scene, const aiNode* nd, const Camera& cam,
    std::shared_ptr<CShader>& overrideProgram,
    const std::string& shadowMapForLight) {
  aiMatrix4x4 m = nd->mTransformation;
  glm::mat4 model = aiMatrix4x4ToGlm(&m);
  glm::mat4 view = cam.GetViewMatrix();
  glm::mat4 proj = cam.GetProjMatrix();

  for (int i = 0; i < nd->mNumMeshes; ++i) {
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
    RecursiveRender(scene, nd->mChildren[i], cam, overrideProgram,
                    shadowMapForLight);
  }
}

void MyDrawController::SetupLights(const std::string& onlyLight) {
  if (drawSkybox) {
    glActiveTexture(GL_TEXTURE0 + ETextureSlot::SkyBox);
    currShader->setInt("skybox", ETextureSlot::SkyBox);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);
  }

  int pointLightIndx = 0;
  int dirLightIndx = 0;

  for (int i = 0; i < m_pScene->mNumLights; ++i) {
    const aiLight& light = *m_pScene->mLights[i];

    assert(light.mType == aiLightSource_POINT ||
           light.mType == aiLightSource_DIRECTIONAL);
    aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName.data);
    assert(pLightNode);

    if (onlyLight != "" && onlyLight != light.mName.C_Str()) continue;

    aiMatrix4x4 m = pLightNode->mTransformation;
    glm::mat4 t = aiMatrix4x4ToGlm(&m);

    char buff[100];
    std::string lightI;
    if (light.mType == aiLightSource_POINT) {
      snprintf(buff, sizeof(buff), "pointLights[%d].", pointLightIndx++);
      lightI = buff;
      currShader->setVec3(lightI + "pos", glm::vec3(t[3]));
      currShader->setFloat(lightI + "constant", light.mAttenuationConstant);
      currShader->setFloat(lightI + "linear", light.mAttenuationLinear);
      currShader->setFloat(lightI + "quadratic", light.mAttenuationQuadratic);
      currShader->setFloat(lightI + "farPlane", kTMPFarPlane);

      currShader->setVec3("lightPos", glm::vec3(t[3]));  // TODO: refactor

      if (drawShadows) {
        SShadowMap& sm = m_shadowMaps[light.mName.C_Str()];

        //	currShader->setMat4(lightI + "lightSpaceMatrix", proj*view);

        glActiveTexture(GL_TEXTURE0 + ETextureSlot::OmniShadowMapStart +
                        pointLightIndx - 1);
        currShader->setInt(
            lightI + "shadowMapTexture",
            ETextureSlot::OmniShadowMapStart + pointLightIndx - 1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sm.textureId);

        char buff2[100];
        std::string tmp2;
        for (int i = 0; i < sm.transforms.size(); ++i) {
          snprintf(buff2, sizeof(buff2), "shadowMatrices[%d]", i);
          tmp2 = buff2;
          currShader->setMat4(tmp2, sm.transforms[i]);
        }

        currShader->setFloat("farPlane", kTMPFarPlane);
      } else {
        // TODO:ugly!!!
        glActiveTexture(GL_TEXTURE0 + ETextureSlot::OmniShadowMapStart +
                        pointLightIndx - 1);
        currShader->setInt(
            lightI + "shadowMapTexture",
            ETextureSlot::OmniShadowMapStart + pointLightIndx - 1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);
        // currShader->setInt(lightI + "shadowMapTexture", 0);
      }

      /*
      std::cout << "----" << std::endl;
      std::cout << light.mAttenuationConstant << std::endl;
      std::cout << light.mAttenuationLinear << std::endl;
      std::cout << light.mAttenuationQuadratic << std::endl;
      */
    } else if (light.mType == aiLightSource_DIRECTIONAL) {
      snprintf(buff, sizeof(buff), "dirLights[%d].", dirLightIndx++);
      lightI = buff;
      currShader->setVec3(lightI + "dir", glm::vec3(t[2]));

      if (drawShadows) {
        SShadowMap& sm = m_shadowMaps[light.mName.C_Str()];

        const glm::mat4& view = sm.frustum.GetViewMatrix();
        const glm::mat4& proj = sm.frustum.GetProjMatrix();
        currShader->setMat4("lightSpaceMatrix", proj * view);

        glActiveTexture(GL_TEXTURE0 + ETextureSlot::DirShadowMap);
        currShader->setInt(lightI + "shadowMapTexture",
                           ETextureSlot::DirShadowMap);
        glBindTexture(GL_TEXTURE_2D, sm.textureId);
      } else {
        glActiveTexture(GL_TEXTURE0 + ETextureSlot::DirShadowMap);
        currShader->setInt(lightI + "shadowMapTexture",
                           ETextureSlot::DirShadowMap);
        glBindTexture(GL_TEXTURE_2D, 0);
      }
    }

    aiColor3D tmp = light.mColorAmbient;

    if (isAmbient)
      // mainShader->setVec3(lightI + "ambient", tmp[0], tmp[1], tmp[2]);
      currShader->setVec3(lightI + "ambient", 0.2, 0.2, 0.2);
    else
      currShader->setVec3(lightI + "ambient", glm::vec3());

    tmp = light.mColorDiffuse;
    if (isDiffuse)
      currShader->setVec3(lightI + "diffuse", tmp[0], tmp[1], tmp[2]);
    else
      currShader->setVec3(lightI + "diffuse", glm::vec3());
    // printf("%.1f %.1f %.1f\n", diffCol[0], diffCol[1], diffCol[2] );

    tmp = light.mColorSpecular;
    if (isSpecular)
      currShader->setVec3(lightI + "specular", tmp[0], tmp[1], tmp[2]);
    else
      currShader->setVec3(lightI + "specular", glm::vec3());
    // printf("%.1f %.1f %.1f\n", diffCol[0], diffCol[1], diffCol[2] );
  }

  assert(
      dirLightIndx < 2 &&
      "more than one dir light is not supported now due to one dir shadowmap.");
  currShader->setInt("nDirLights", dirLightIndx);
  currShader->setInt("nPointLights", pointLightIndx);
}

void MyDrawController::BuildShadowMaps() {
  const Camera& currCam = GetCam();
  for (int i = 0; i < m_pScene->mNumLights; ++i) {
    const aiLight& light = *m_pScene->mLights[i];
    assert(light.mType == aiLightSource_POINT ||
           light.mType == aiLightSource_DIRECTIONAL);

    aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName.data);
    assert(pLightNode);

    aiMatrix4x4 m = pLightNode->mTransformation;
    glm::mat4 t = aiMatrix4x4ToGlm(&m);

    if (m_shadowMaps.find(light.mName.C_Str()) == m_shadowMaps.end())
      m_shadowMaps.emplace(light.mName.C_Str(), SShadowMap());

    SShadowMap& shadowMap = m_shadowMaps[light.mName.C_Str()];

    if (light.mType == aiLightSource_DIRECTIONAL) {
      const float SHADOW_WIDTH = 1024.0f;
      const float SHADOW_HEIGHT = 1024.0f;

      GLuint depthMapFBO;
      glGenFramebuffers(1, &depthMapFBO);

      if (!shadowMap.textureId) glGenTextures(1, &shadowMap.textureId);

      GLint oldFBO = 0;
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

      glBindTexture(GL_TEXTURE_2D, shadowMap.textureId);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                   SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

      glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                             shadowMap.textureId, 0);

      glBindTexture(GL_TEXTURE_2D, 0);
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);

      glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
      glClear(GL_DEPTH_BUFFER_BIT);

      const glm::vec3 center(
          0.0f, 0.0f, 0.0f);  // = currCam.Position + 5.0f * currCam.Front;
      const glm::vec3 pos =
          center + 19.0f * glm::vec3(t[2]);  // + 1 * currCam.Front;

      Camera lightCam;
      lightCam.Position = pos;
      lightCam.Front = center - pos;
      lightCam.Up = glm::vec3(0.0f, 0.0f, 1.0f);
      lightCam.IsPerspective = false;

      glCullFace(GL_FRONT);
      RecursiveRender(*m_pScene, m_pScene->mRootNode, lightCam, shadowMapShader,
                      light.mName.C_Str());
      glCullFace(GL_BACK);

      glDeleteFramebuffers(1, &depthMapFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
      glViewport(0, 0, currCam.Width, currCam.Height);

      if (debugShadowMaps)
        DrawRect2d(currCam.Width - 215, 10, 200, 200, shadowMap.textureId,
                   false, true, -1.0f);

      shadowMap.frustum = lightCam;
    } else {
      GLint oldFBO = 0;
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

      GLuint depthCubemapFBO;
      glGenFramebuffers(1, &depthCubemapFBO);

      if (0 == shadowMap.textureId) glGenTextures(1, &shadowMap.textureId);

      const float SHADOW_WIDTH = 1024.0f;
      const float SHADOW_HEIGHT = 1024.0f;

      glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap.textureId);
      for (int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                     SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT,
                     GL_FLOAT, NULL);

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           shadowMap.textureId, 0);

      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);

      const float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
      const float near = 0.01f;
      const glm::mat4 shadowProj =
          glm::perspective(glm::radians(90.0f), aspect, near, kTMPFarPlane);
      const glm::vec3& lightPos = glm::vec3(t[3]);

      shadowMap.transforms.clear();
      std::vector<glm::mat4> shadowTransforms;
      shadowMap.transforms.push_back(
          shadowProj * glm::lookAt(lightPos,
                                   lightPos + glm::vec3(1.0, 0.0, 0.0),
                                   glm::vec3(0.0, -1.0, 0.0)));
      shadowMap.transforms.push_back(
          shadowProj * glm::lookAt(lightPos,
                                   lightPos + glm::vec3(-1.0, 0.0, 0.0),
                                   glm::vec3(0.0, -1.0, 0.0)));
      shadowMap.transforms.push_back(
          shadowProj * glm::lookAt(lightPos,
                                   lightPos + glm::vec3(0.0, 1.0, 0.0),
                                   glm::vec3(0.0, 1.0, 1.0)));
      shadowMap.transforms.push_back(
          shadowProj * glm::lookAt(lightPos,
                                   lightPos + glm::vec3(0.0, -1.0, 0.0),
                                   glm::vec3(0.0, 0.0, -1.0)));
      shadowMap.transforms.push_back(
          shadowProj * glm::lookAt(lightPos,
                                   lightPos + glm::vec3(0.0, 0.0, 1.0),
                                   glm::vec3(0.0, -1.0, 0.0)));
      shadowMap.transforms.push_back(
          shadowProj * glm::lookAt(lightPos,
                                   lightPos + glm::vec3(0.0, 0.0, -1.0),
                                   glm::vec3(0.0, -1.0, 0.0)));

      glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
      glClear(GL_DEPTH_BUFFER_BIT);

      Camera emptyCam;
      RecursiveRender(*m_pScene, m_pScene->mRootNode, emptyCam,
                      shadowCubeMapShader, light.mName.C_Str());

      glDeleteFramebuffers(1, &depthCubemapFBO);

      glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
      glViewport(0, 0, currCam.Width, currCam.Height);
    }
  }
}

void MyDrawController::RenderInternalForward(
    const aiScene& scene, const aiNode* nd, const Camera& cam,
    std::shared_ptr<CShader>& overrideProgram,
    const std::string& shadowMapForLight) {
  RecursiveRender(scene, nd, cam, overrideProgram, shadowMapForLight);
}

static void GenGBuffer(SGBuffer& gBuffer, const Camera& cam) {
  // check window resize
  GLint tw = 0;
  GLint th = 0;
  bool needReGenOnResize = false;
  if (gBuffer.FBO) {
    glBindTexture(GL_TEXTURE_2D, gBuffer.pos);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
    // glGetTextureLevelParameteriv(gBuffer.pos, 0, GL_TEXTURE_WIDTH, &tw);
    // glGetTextureLevelParameteriv(gBuffer.pos, 0, GL_TEXTURE_HEIGHT, &th);
  }

  const int w = (int)cam.Width;
  const int h = (int)cam.Height;

  needReGenOnResize = (w != tw || h != th);

  if (gBuffer.FBO && !needReGenOnResize) return;
  // std::cout << "recalc" << w << " " << tw << " " << h << " " << th<<
  // std::endl;
  //
  // delete previous
  if (needReGenOnResize && gBuffer.FBO) {
    GLuint arr[] = {gBuffer.pos, gBuffer.normal, gBuffer.albedoSpec};
    glDeleteTextures(3, arr);

    glDeleteRenderbuffers(1, &gBuffer.depth);
  }

  GLint oldFBO = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

  glGenFramebuffers(1, &gBuffer.FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

  // - position color buffer
  glGenTextures(1, &gBuffer.pos);
  glBindTexture(GL_TEXTURE_2D, gBuffer.pos);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
  // NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         gBuffer.pos, 0);

  // - normal color buffer
  glGenTextures(1, &gBuffer.normal);
  glBindTexture(GL_TEXTURE_2D, gBuffer.normal);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         gBuffer.normal, 0);

  // - color + specular color buffer
  glGenTextures(1, &gBuffer.albedoSpec);
  glBindTexture(GL_TEXTURE_2D, gBuffer.albedoSpec);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                         gBuffer.albedoSpec, 0);

  // - tell OpenGL which color attachments we'll use (of this framebuffer) for
  // rendering
  unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                 GL_COLOR_ATTACHMENT2};
  glDrawBuffers(3, attachments);

  // create depth texture
  glGenTextures(1, &gBuffer.depth);
  glBindTexture(GL_TEXTURE_2D, gBuffer.depth);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         gBuffer.depth, 0);

  glBindTexture(GL_TEXTURE_2D, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    assert(!"Framebuffer not complete!");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
}

float lerp(float a, float b, float f) { return a + f * (b - a); }

void InitSSAO(SSSAO& ssao, const Camera& cam) {
  if (!ssao.pass1FBO) {
    // ssao noize texture
    {
      std::uniform_real_distribution<float> randomFloats(
          0.0, 1.0);  // random floats between 0.0 - 1.0
      std::default_random_engine generator;
      std::vector<glm::vec3> noises;
      for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0,
                        randomFloats(generator) * 2.0 - 1.0, 0.0f);
        noises.push_back(noise);
      }

      glGenTextures(1, &ssao.noiseTxt);
      glBindTexture(GL_TEXTURE_2D, ssao.noiseTxt);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT,
                   &noises[0]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    glGenFramebuffers(1, &ssao.pass1FBO);
  }

  // ssao textures
  if (!ssao.pass1Txt) {
    GLint oldFBO = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, ssao.pass1FBO);

    glGenTextures(1, &ssao.pass1Txt);  // TODO: delete on recreation
    glBindTexture(GL_TEXTURE_2D, ssao.pass1Txt);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (int)cam.Width, (int)cam.Height, 0,
                 GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           ssao.pass1Txt, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
  }

  if (!ssao.FBO) {
    GLint oldFBO = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

    glGenFramebuffers(1, &ssao.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao.FBO);
    glGenTextures(1, &ssao.colorTxt);
    glBindTexture(GL_TEXTURE_2D, ssao.colorTxt);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (int)cam.Width, (int)cam.Height, 0,
                 GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           ssao.colorTxt, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
  }

  // setup kernel
  if (ssao.kernel.empty()) {
    // std::cout << "Samples:" << std::endl;
    std::uniform_real_distribution<float> randomFloats(
        0.0, 1.0);  // random floats between 0.0 - 1.0
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i) {
      glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
                       randomFloats(generator) * 2.0 - 1.0,
                       randomFloats(generator) * 0.8 + 0.2);
      sample = glm::normalize(sample);
      sample *= randomFloats(generator);
      float scale = (float)i / 64.0;
      scale = lerp(0.1f, 1.0f, scale * scale);
      sample *= scale;

      // std::cout << "\t" << sample[0] << " " << sample[1] << " " << sample[2]
      //          << std::endl;
      ssao.kernel.push_back(sample);
    }
  }
}

void MyDrawController::PerformSSAO(const SSSAO& ssao, const SGBuffer& gBuffer,
                                   const Camera& cam) {
  ssaoShader->use();

  GLint oldFBO = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);

  // pass1
  {
    glBindFramebuffer(GL_FRAMEBUFFER, ssao.pass1FBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.pos);
    ssaoShader->setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.normal);
    ssaoShader->setInt("gNormal", 1);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, ssao.noiseTxt);
    ssaoShader->setInt("noiseTxt", 2);

    for (unsigned int i = 0; i < 64; ++i)
      ssaoShader->setVec3("samples[" + std::to_string(i) + "]", ssao.kernel[i]);

    ssaoShader->setMat4("viewMat", cam.GetViewMatrix());
    ssaoShader->setMat4("vpMat", cam.GetProjMatrix() * cam.GetViewMatrix());

    RenderFsQuad();
  }

  // blur
  {
    glBindFramebuffer(GL_FRAMEBUFFER, ssao.FBO);
    blurShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssao.pass1Txt);
    blurShader->setInt("inTexture", 0);

    RenderFsQuad();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
}

void MyDrawController::RenderInternalDeferred(
    const aiScene& scene, const aiNode* nd, const Camera& cam,
    std::shared_ptr<CShader>& overrideProgram,
    const std::string& shadowMapForLight) {
  GenGBuffer(m_resources.GBuffer, cam);
  InitSSAO(m_resources.ssao, cam);

  GLint oldFBO = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_resources.GBuffer.FBO);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // geometry path
  RenderInternalForward(*m_pScene, m_pScene->mRootNode, cam,
                        deferredGeomPathShader, "");

  glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);

  // copy depth buffer to default FBO
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_resources.GBuffer.FBO);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldFBO);
  glBlitFramebuffer(0, 0, (int)cam.Width, (int)cam.Height, 0, 0, (int)cam.Width,
                    (int)cam.Height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);

  PerformSSAO(m_resources.ssao, m_resources.GBuffer, cam);

  // light path
  deferredLightPathShader->use();
  currShader = deferredLightPathShader;
  SetupLights("");

  deferredLightPathShader->setVec3("camPos", cam.Position);

  CShader::TSubroutineTypeToInstance data;
  if (drawShadows)
    data.push_back(std::pair<std::string, std::string>("shadowMapSelection",
                                                       "globalShadowMap"));
  else
    data.push_back(std::pair<std::string, std::string>("shadowMapSelection",
                                                       "emptyShadowMap"));

  currShader->setSubroutine(GL_FRAGMENT_SHADER, data);

  {
    glActiveTexture(GL_TEXTURE0 + 1);
    deferredLightPathShader->setInt("gPosition", 1);
    glBindTexture(GL_TEXTURE_2D, m_resources.GBuffer.pos);

    glActiveTexture(GL_TEXTURE0 + 2);
    deferredLightPathShader->setInt("gNormal", 2);
    glBindTexture(GL_TEXTURE_2D, m_resources.GBuffer.normal);

    glActiveTexture(GL_TEXTURE0 + 3);
    deferredLightPathShader->setInt("gAlbedoSpec", 3);
    glBindTexture(GL_TEXTURE_2D, m_resources.GBuffer.albedoSpec);

    glActiveTexture(GL_TEXTURE0 + 4);
    deferredLightPathShader->setInt("gDepth", 4);
    glBindTexture(GL_TEXTURE_2D, m_resources.GBuffer.depth);

    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glDisable(GL_DEPTH_TEST);
    RenderFsQuad();
    glEnable(GL_DEPTH_TEST);
  }

  if (debugGBuffer) {
    glDisable(GL_DEPTH_TEST);
    DrawRect2d(cam.Width - 315, 730, 300, 200, m_resources.GBuffer.pos, false,
               false, -1.0f);
    DrawRect2d(cam.Width - 315, 515, 300, 200, m_resources.GBuffer.normal,
               false, false, -1.0f);
    DrawRect2d(cam.Width - 315, 300, 300, 200, m_resources.GBuffer.albedoSpec,
               false, false, -1.0f);
    // DrawRect2d(cam.Width - 315, 75, 300, 200, m_resources.GBuffer.albedoSpec,
    //           false, true, -1.0f);
    DrawRect2d(cam.Width - 315, 75, 300, 200, m_resources.ssao.colorTxt, false,
               true, -1.0f);
    glEnable(GL_DEPTH_TEST);
  }
}

void MyDrawController::Render(const Camera& cam) {
  if (deferredShading) {
    if (isMSAA) isMSAA = false;

    if (isWireMode) isWireMode = false;
  }

  glPolygonMode(GL_FRONT_AND_BACK, isWireMode ? GL_LINE : GL_FILL);

  if (drawShadows)
    BuildShadowMaps();
  else
    ReleaseShadowMaps();

  if (deferredShading)
    RenderInternalDeferred(*m_pScene, m_pScene->mRootNode, cam, nullShader, "");
  else
    RenderInternalForward(*m_pScene, m_pScene->mRootNode, cam, nullShader, "");

  if (drawNormals)
    RenderInternalForward(*m_pScene, m_pScene->mRootNode, cam, normalShader,
                          "");

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  RenderLightModels(cam);

  if (drawSkybox)
    RenderSkyBox(cam);
  else if (debugShadowMaps)
    DebugCubeShadowMap();

  if (drawGradientReference) DrawGradientReference();
}

void MyDrawController::RenderLightModels(const Camera& cam) {
  glBindVertexArray(m_resources.cubeVAOID);
  lightModelShader->use();

  for (int i = 0; i < m_pScene->mNumLights; ++i) {
    const aiLight& light = *m_pScene->mLights[i];

    aiNode* pLightNode = m_pScene->mRootNode->FindNode(light.mName.data);
    assert(pLightNode);

    aiMatrix4x4 m = pLightNode->mTransformation;
    glm::mat4 t = aiMatrix4x4ToGlm(&m);

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
    glm::mat4 mvp_matrix =
        cam.GetProjMatrix() * cam.GetViewMatrix() * t * scale;

    aiColor3D tmp = light.mColorDiffuse;
    lightModelShader->setVec3("lightColor", tmp[0], tmp[1], tmp[2]);
    // printf("%.1f %.1f %.1f\n", tmp[0], tmp[1], tmp[2] );

    lightModelShader->setMat4("MVP", mvp_matrix);

    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
  }
}

void MyDrawController::RenderSkyBox(const Camera& cam) {
  glDepthFunc(GL_LEQUAL);
  skyboxShader->use();
  glBindVertexArray(m_resources.cubeVAOID);

  glActiveTexture(GL_TEXTURE0 + ETextureSlot::SkyBox);
  skyboxShader->setInt("skybox", ETextureSlot::SkyBox);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_resources.skyboxTextID);

  glm::mat4 rot =
      glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f, glm::vec3(-1, 0, 0));

  glm::mat4 view = glm::mat4(glm::mat3(
      cam.GetViewMatrix()));  // remove translation from the view matrix
  glm::mat4 mvp_matrix = cam.GetProjMatrix() * view * rot;
  skyboxShader->setMat4("MVP", mvp_matrix);

  GLint size = 0;
  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
  glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
  glDepthFunc(GL_LESS);
}

void SResourceHandlers::Release() {
  // TODO: release resources
}

static float screenToNDC(float val, float scrSize) {
  return (val / scrSize - 0.5f) * 2.0f;
}

static void DrawRect2d(float x, float y, float w, float h,
                       const glm::vec3& color, GLuint textureId, Camera& cam,
                       bool doGammaCorrection, bool bOneColorChannel,
                       float HDRexposure) {
  const float scrW = cam.Width;
  const float scrH = cam.Height;

  const float x0 = screenToNDC(x, scrW);
  const float y0 = screenToNDC(y, scrH);

  const float x1 = screenToNDC(x, scrW);
  const float y1 = screenToNDC(y + h, scrH);

  const float x2 = screenToNDC(x + w, scrW);
  const float y2 = screenToNDC(y, scrH);

  const float x3 = screenToNDC(x + w, scrW);
  const float y3 = screenToNDC(y + h, scrH);

  const float quadVertices[] = {
      // vertex attributes for a quad that fills the entire screen in Normalized
      // Device Coordinates.
      // positions   // texCoords
      x0, y0, 0.0f, 0.0f, x2, y2, 1.0f, 0.0f, x1, y1, 0.0f, 1.0f,

      x3, y3, 1.0f, 1.0f, x1, y1, 0.0f, 1.0f, x2, y2, 1.0f, 0.0f};

  // screen quad VAO
  GLuint quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void*)(2 * sizeof(float)));

  rect2dShader->use();
  if (textureId) {
    rect2dShader->setBool("useColor", false);
    glActiveTexture(GL_TEXTURE0);
    rect2dShader->setInt("in_texture", 0);
    glBindTexture(GL_TEXTURE_2D, textureId);
  } else {
    rect2dShader->setVec3("color", color);
    rect2dShader->setBool("useColor", true);
  }

  rect2dShader->setBool("bOneColorChannel", bOneColorChannel);

  rect2dShader->setBool("doGammaCorrection", doGammaCorrection);

  rect2dShader->setFloat("HDR_exposure", HDRexposure);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(quadVertices));

  glBindTexture(GL_TEXTURE_2D, 0);
  glDeleteBuffers(1, &quadVBO);
  glDeleteVertexArrays(1, &quadVAO);
}

void MyDrawController::DrawRect2d(float x, float y, float w, float h,
                                  const glm::vec3& color,
                                  bool doGammaCorrection) {
  ::DrawRect2d(x, y, w, h, color, 0, GetCam(), doGammaCorrection, false, -1.0f);
}

void MyDrawController::DrawRect2d(float x, float y, float w, float h,
                                  GLuint textureId, bool doGammaCorrection,
                                  bool bOneColorChannel, float HDRexposure) {
  glm::vec3 color;
  ::DrawRect2d(x, y, w, h, color, textureId, GetCam(), doGammaCorrection,
               bOneColorChannel, HDRexposure);
}

void MyDrawController::DrawGradientReference() {
  const int cnt = 30;
  const int rectW = 30;
  const int rectH = 90;
  const int borderSize = 5;

  GLboolean wasDepthTest = 1;
  glGetBooleanv(GL_DEPTH_TEST, &wasDepthTest);
  glDisable(GL_DEPTH_TEST);

  DrawRect2d(10, 10, cnt * rectW + 2 * borderSize, rectH + 2 * borderSize,
             glm::vec3(1, 1, 1), false);

  for (int i = 0; i < cnt; ++i) {
    const float color = float(i) / (cnt - 1);
    DrawRect2d(10 + borderSize + i * rectW, 10 + borderSize, rectW, rectH,
               glm::vec3(color), false);
  }

  if (wasDepthTest) glEnable(GL_DEPTH_TEST);
}

void MyDrawController::DebugCubeShadowMap() {
  if (debugOnmiShadowLightName.empty()) return;

  auto it = m_shadowMaps.find(debugOnmiShadowLightName.c_str());

  if (it == m_shadowMaps.end()) return;

  SShadowMap& shadowMap = it->second;

  glDepthFunc(GL_LEQUAL);
  debugShadowCubeMapShader->use();
  glBindVertexArray(m_resources.cubeVAOID);

  glActiveTexture(GL_TEXTURE0 + ETextureSlot::SkyBox);
  debugShadowCubeMapShader->setInt("in_texture", ETextureSlot::SkyBox);
  glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap.textureId);

  // glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)M_PI / 2.0f,
  // glm::vec3(-1, 0, 0));

  glm::mat4 view = glm::mat4(glm::mat3(
      m_cam.GetViewMatrix()));  // remove translation from the view matrix
  glm::mat4 mvp_matrix = m_cam.GetProjMatrix() * view;  //* rot;
  debugShadowCubeMapShader->setMat4("MVP", mvp_matrix);
  debugShadowCubeMapShader->setFloat("farPlane", kTMPFarPlane);

  GLint size = 0;
  glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
  glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glDepthFunc(GL_LESS);
}
