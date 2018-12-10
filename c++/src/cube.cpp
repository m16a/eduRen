#include "cube.h"

GLfloat cubeVertices[cubeVerticiesCount][3] = {
    {-1.0, -1.0, -1.0},  // Vertex 0
    {1.0, -1.0, -1.0},   // Vertex 1
    {1.0, 1.0, -1.0},    // Vertex 2
    {-1.0, 1.0, -1.0},   // Vertex 3
    {-1.0, -1.0, 1.0},   // Vertex 4
    {1.0, -1.0, 1.0},    // Vertex 5
    {1.0, 1.0, 1.0},     // Vertex 6
    {-1.0, 1.0, 1.0}     // Vertex 7
};

// 12 triangles * 3 verticies
GLint cubeIndices[cubeIndiciesCount] = {0, 1, 2, 0, 2, 3, 5, 4, 6, 4, 7, 6,
                                        7, 3, 2, 6, 7, 2, 4, 1, 0, 4, 5, 1,
                                        6, 2, 1, 5, 6, 1, 4, 0, 7, 3, 7, 0};
