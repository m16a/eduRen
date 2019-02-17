#pragma once

#include <glm/mat4x4.hpp>

extern glm::mat4 IBLCaptureProjection;
extern glm::mat4 IBLCaptureViews[6];

void renderQuad();
