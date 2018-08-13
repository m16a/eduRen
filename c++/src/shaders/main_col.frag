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
};

#define NR_POINT_LIGHTS 10
uniform PointLight pointLights[NR_POINT_LIGHTS];

vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 camPos;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform Material material;

uniform mat4 model;

out vec4 fColor;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.pos - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
		/*
    float distance    = length(light.pos - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
		*/
    // combine results
		vec3 ambient  = light.ambient  * material.diffuse;
    vec3 diffuse  = light.diffuse * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;

		/*
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
		*/
    return ambient + diffuse + specular;
}

void main()
{
	vec3 res = vec3(0.0, 0.0, 0.0);	

  vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(camPos - FragPos);


	for (int i = 0; i < NR_POINT_LIGHTS; i++)
		res += CalcPointLight(pointLights[i], norm, FragPos, viewDir); 

	fColor = vec4(res, 1.0);
	//fColor = vec4(norm, 1.0);
}
