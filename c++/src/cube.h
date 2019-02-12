#pragma once

#include <GL/gl3w.h>

constexpr int cubeVerticiesCount = 8;
extern GLfloat cubeVertices[cubeVerticiesCount][3];

constexpr int cubeIndiciesCount = 36;
extern GLint cubeIndices[cubeIndiciesCount];

// verticies, normals, uv
void renderFullCube();
