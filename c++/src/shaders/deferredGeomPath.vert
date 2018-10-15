#version 400 core

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec3 vNormal;
layout( location = 2 ) in vec2 vTexCoord;
layout( location = 3 ) in vec3 vTangent;
layout( location = 4 ) in vec3 vBitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 lightSpaceMatrix;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out mat3 TBN;

void main()
{
	//Normal = normalize(vec3(model * vec4(vNormal, 0.0)));
	Normal = vec3(model * vec4(vNormal, 0.0));
	FragPos = vec3(model * vPosition);
	TexCoords = vTexCoord;
	FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

  vec3 T = normalize(vec3(model * vec4(vTangent,   0.0)));
	vec3 B = normalize(vec3(model * vec4(vBitangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(vNormal,    0.0)));
	TBN = mat3(T, B, N);

	gl_Position = proj * view * model * vPosition;
}

