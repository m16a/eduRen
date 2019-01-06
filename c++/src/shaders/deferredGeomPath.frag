#version 400 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in mat3 TBN;

in vec3 TangentCamPos;
in vec3 TangentFragPos;

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
	sampler2D opacity;
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

subroutine (getNormal) vec3 getNormalFromHeight(vec2 uv)
{
	const vec2 size = vec2(1.0,0.0);
	const ivec3 off = ivec3(-1,0,1);

	vec4 wave = texture(inTexture.norm, uv);
	float s11 = wave.x;
#if 0
	float s01 = textureOffset(inTexture.norm, uv, off.xy).x;
	float s21 = textureOffset(inTexture.norm, uv, off.zy).x;
	float s10 = textureOffset(inTexture.norm, uv, off.yx).x;
	float s12 = textureOffset(inTexture.norm, uv, off.yz).x;
#else
	float s01 = 1-textureOffset(inTexture.norm, uv, off.xy).x;
	float s21 = 1-textureOffset(inTexture.norm, uv, off.zy).x;
	float s10 = 1-textureOffset(inTexture.norm, uv, off.yx).x;
	float s12 = 1-textureOffset(inTexture.norm, uv, off.yz).x;
#endif

	float scale = 0.3;
	vec3 va = scale * normalize(vec3(size.xy,scale * (-s21+s01)));
	vec3 vb = scale * normalize(vec3(size.yx,scale * (s12-s10)));
	//vec4 bump = vec4( cross(va,vb), s11 );	
	vec3 n = normalize(cross(va, vb));
	n = normalize(TBN * n);

	return n;
}

subroutine uniform getNormal getNormalSelection;
// -----------------------------------------------------------

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    float height =  texture(inTexture.norm, texCoords).r;    
		height = 1.0 - height;
    vec2 p = viewDir.xy / viewDir.z * (height * 0.001);
    return texCoords - p;    
} 


vec4 getHeightBumped(sampler2D tex, vec2 uv)
{
	vec3 viewDir   = normalize(TangentCamPos - TangentFragPos);
  vec2 texCoords = ParallaxMapping(uv,  viewDir);
	return texture(tex, texCoords);
}

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
	c.diffuse = texture(inTexture.diff, uv);
	c.ambient = c.diffuse;
	c.specular = texture(inTexture.spec, uv);
	c.shininess = 16;
	return c;
}

subroutine (baseColor) Color textHeightColor(vec2 uv)
{
	Color c;
	c.diffuse  = getHeightBumped(inTexture.diff, uv);
	c.ambient = c.diffuse;
	c.specular = getHeightBumped(inTexture.spec, uv);
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

// ---------------------- opacity ------------------------

subroutine float getOpacity(vec2 uv);

subroutine (getOpacity) float emptyOpacity(vec2 uv)
{
	return 1.0;
}

subroutine (getOpacity) float maskOpacity(vec2 uv)
{
	return texture(inTexture.opacity, uv).r;
}

subroutine uniform getOpacity opacitySelection;

// -----------------------------------------------------------

void main()
{
	float opacity = opacitySelection(TexCoords);

	if (opacity < 0.03)
		discard;

  vec3 norm = getNormalSelection(TexCoords);
	Color baseColor = baseColorSelection(TexCoords);

	gPosition = FragPos;
	gNormal = norm; 
	gAlbedoSpec.rgb = baseColor.diffuse.rgb;
	gAlbedoSpec.a = baseColor.specular.r;

	//emty calls for backward compatibility with forward rendering
	reflectionMapSelection();
	shadowMapSelection();
}
