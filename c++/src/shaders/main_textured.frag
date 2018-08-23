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

struct Texture
{
	sampler2D diff;
	sampler2D spec;
	sampler2D norm;
};

uniform Texture inTexture;

uniform mat4 model;

out vec4 fColor;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.pos - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    // attenuation
    float distance    = length(light.pos - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    // combine results
    vec3 ambient  = light.ambient  * texture(inTexture.diff, TexCoords).rgb;
    //vec3 diffuse  = light.diffuse * diff * (material.diffuse + texture(textureDiffuse, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * texture(inTexture.diff, TexCoords).rgb;
    vec3 specular = light.specular * spec * texture(inTexture.spec, TexCoords).rgb;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    // diffuse shading
    float diff = max(dot(normal, light.dir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-light.dir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);

    // combine results
    vec3 ambient  = light.ambient  * texture(inTexture.diff, TexCoords).rgb;
    //vec3 diffuse  = light.diffuse * diff * (material.diffuse + texture(textureDiffuse, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * texture(inTexture.diff, TexCoords).rgb;
    vec3 specular = light.specular * spec * texture(inTexture.spec, TexCoords).rgb;

    return ambient + diffuse + specular;
}

void main()
{
	vec3 res = vec3(0.0, 0.0, 0.0);	

	/*
	vec3 tNorm = vec3(texture(inTexture.norm, TexCoords));
	tNorm = vec3(model * vec4(tNorm, 0.0));
  vec3 norm = normalize(tNorm);
	*/

  vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(camPos - FragPos);


	for (int i = 0; i < nPointLights; i++)
		res += CalcPointLight(pointLights[i], norm, FragPos, viewDir); 

	for (int i = 0; i < nDirLights; i++)
		res += CalcDirLight(dirLights[i], norm, FragPos, viewDir); 

	fColor = vec4(res, 1.0);
	//fColor = vec4(norm, 1.0);
}