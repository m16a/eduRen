#version 400 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;


struct PointLight
{
	vec3 pos;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};
#define NR_POINT_LIGHTS 10
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform int nPointLights;

struct DirLight
{
	vec3 dir;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
#define NR_DIR_LIGHTS 10
uniform DirLight dirLights[NR_DIR_LIGHTS];
uniform int nDirLights;

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
	sampler2D norm;
};

uniform Texture inTexture;

uniform mat4 model;

out vec4 fColor;

// ---------------------- subroutines ------------------------
struct Color
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

subroutine Color baseColor(vec2 uv);


subroutine (baseColor) Color plainColor(vec2 uv)
{
	Color c;
	c.ambient= material.diffuse;
	c.diffuse = material.diffuse;
	c.specular = material.specular;
	c.shininess = material.shininess;
	return c;
}

subroutine (baseColor) Color textColor(vec2 uv)
{
	Color c;
	c.ambient = texture(inTexture.diff, uv).rgb;
	c.diffuse = texture(inTexture.diff, uv).rgb;
	c.specular = texture(inTexture.spec, uv).rgb;
	c.shininess = 16;
	return c;
}

// ---------------------- subroutines ------------------------

subroutine uniform baseColor baseColorSelection;

vec3 CalcPointLight(Color baseColor, PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.pos - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), baseColor.shininess);
    // attenuation

    float distance    = length(light.pos - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    // combine results
		vec3 ambient  = light.ambient  * baseColor.diffuse;
    vec3 diffuse  = light.diffuse * diff * baseColor.diffuse;
    vec3 specular = light.specular * spec * baseColor.specular;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
		/*
		*/

    return ambient + diffuse + specular;
}

vec3 CalcDirLight(Color baseColor, DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    // diffuse shading
    float diff = max(dot(normal, light.dir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-light.dir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), baseColor.shininess);

    // combine results
    vec3 ambient  = light.ambient  * baseColor.diffuse;
    vec3 diffuse = light.diffuse * diff * baseColor.diffuse;
    vec3 specular = light.specular * spec * baseColor.specular;

    return ambient + diffuse + specular;
}

void main()
{
	vec3 res = vec3(0.0, 0.0, 0.0);	

  vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(camPos - FragPos);

	Color baseColor = baseColorSelection(TexCoords);
	//Color baseColor = plainColor(TexCoords);

	for (int i = 0; i < nPointLights; i++)
		res += CalcPointLight(baseColor, pointLights[i], norm, FragPos, viewDir); 

	for (int i = 0; i < nDirLights; i++)
		res += CalcDirLight(baseColor, dirLights[i], norm, FragPos, viewDir); 

	fColor = vec4(res, 1.0);
	//fColor = vec4(norm, 1.0);
}
