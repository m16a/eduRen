#version 400 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in mat3 TBN;

uniform vec3 camPos;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};
uniform Material material;

struct Texture
{
	sampler2D diff;
	sampler2D spec;
	sampler2D reflection;
	sampler2D norm;
};
uniform Texture inTexture;

uniform samplerCube skybox;
uniform mat4 rotfix;

uniform mat4 model;

// ---------------------- subroutines ------------------------

subroutine vec3 getNormal(vec2 uv);

subroutine (getNormal) vec3 getNormalSimple(vec2 uv)
{
	return normalize(Normal);
}

subroutine (getNormal) vec3 getNormalBumped(vec2 uv)
{
	//TODO: bad for performance, but less code
	vec3 n = normalize(texture(inTexture.norm, uv).rgb);
	n = normalize(n * 2.0 - 1.0);
	n = normalize(TBN * n);
	return n;
}

subroutine uniform getNormal getNormalSelection;
// -----------------------------------------------------------
struct Color
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

subroutine Color baseColor(vec2 uv);

subroutine (baseColor) Color plainColor(vec2 uv)
{
	Color c;
	c.ambient = vec4(material.diffuse, 1.0);
	c.diffuse = vec4(material.diffuse, 1.0);
	c.specular = vec4(material.specular, 1.0);
	c.shininess = material.shininess;
	return c;
}

subroutine (baseColor) Color textColor(vec2 uv)
{
	Color c;
	c.diffuse  = texture(inTexture.diff, uv);
	c.ambient = c.diffuse;
	c.specular = texture(inTexture.spec, uv);
	c.shininess = 16;
	return c;
}

subroutine uniform baseColor baseColorSelection;

// ----------------------reflection map-------------------------------------
subroutine vec3 reflectionMap();
subroutine (reflectionMap) vec3 emptyReflectionMap()
{
	return vec3(0.0, 0.0, 0.0);
}

subroutine (reflectionMap) vec3 reflectionTexture()
{
	return vec3(0.0, 0.0, 0.0);
}

subroutine (reflectionMap) vec3 reflectionColor()
{
	return vec3(0.0, 0.0, 0.0);
}

subroutine uniform reflectionMap reflectionMapSelection;


// ----------------------shadow map-------------------------------------
subroutine float shadowMap();

subroutine (shadowMap) float emptyShadowMap()
{
	return 0.0f;
}

subroutine (shadowMap) float globalShadowMap()
{
	return 0.0f;
}

subroutine uniform shadowMap shadowMapSelection;
// -----------------------------------------------------------

void main()
{
  vec3 norm = getNormalSelection(TexCoords);
	Color baseColor = baseColorSelection(TexCoords);

	gPosition = FragPos;
	gNormal = norm; 
	gAlbedoSpec.rgb = baseColor.diffuse.rgb;

	//emty calls for backward compatibility with forward rendering
	reflectionMapSelection();
	shadowMapSelection();
}
