#version 400 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

struct PointLight
{
	vec3 pos;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
	
	samplerCube shadowMapTexture;
	//sampler2D shadowMapTexture;
	float farPlane;
};

#define NR_POINT_LIGHTS 1
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform int nPointLights;

struct DirLight
{
	vec3 dir;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	sampler2D shadowMapTexture;
};

#define NR_DIR_LIGHTS 1
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
	sampler2D reflection;
	sampler2D norm;
};

uniform Texture inTexture;

uniform samplerCube skybox;
uniform mat4 rotfix;

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
subroutine uniform baseColor baseColorSelection;

// ----------------------reflection map-------------------------------------
subroutine vec3 reflectionMap(vec2 uv);
subroutine (reflectionMap) vec3 emptyReflectionMap(vec2 uv)
{
	return vec3(0.0, 0.0, 0.0);
}

subroutine (reflectionMap) vec3 reflectionTexture(vec2 uv)
{
	vec3 I = normalize(FragPos - camPos);
	vec3 R = reflect(I, normalize(Normal));
	R = normalize(vec3( rotfix * vec4(R, 0.0)));

	return texture(inTexture.reflection, uv).rgb * texture(skybox, R).rgb;
}

subroutine (reflectionMap) vec3 reflectionColor(vec2 uv)
{
	vec3 I = normalize(FragPos - camPos);
	vec3 R = reflect(I, normalize(Normal));
	R = normalize(vec3( rotfix * vec4(R, 0.0)));

	return material.diffuse * texture(skybox, R).rgb;
}

subroutine uniform reflectionMap reflectionMapSelection;


// ----------------------shadow map-------------------------------------
subroutine float shadowMap(vec4 fragPosLightSpace, vec3 normal, DirLight light);

subroutine (shadowMap) float emptyShadowMap(vec4 fragPosLightSpace, vec3 normal, DirLight light)
{
	return 0.0f;
}

subroutine (shadowMap) float globalShadowMap(vec4 fragPosLightSpace, vec3 normal, DirLight light)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	if (projCoords.z > 1.0)
	{
		return 0.0;
	}
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	//float closestDepth = texture(shadowMapTexture, projCoords.xy).r; 
	
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	
	float bias = 0.0005;	
	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
	// check whether current frag pos is in shadow
	//
	//float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(light.shadowMapTexture, 0);
	for(int x = -1; x <= 1; ++x)
	{
			for(int y = -1; y <= 1; ++y)
			{
					float pcfDepth = texture(light.shadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
					shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
			}    
	}
	shadow /= 9.0;

	return shadow;
}

subroutine uniform shadowMap shadowMapSelection;
// -----------------------------------------------------------


vec3 CalcPointLight(Color baseColor, PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.pos - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
		vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), baseColor.shininess);

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

	for (int i = 0; i < nPointLights; i++)
		res += CalcPointLight(baseColor, pointLights[i], norm, FragPos, viewDir); 

	for (int i = 0; i < nDirLights; i++)
		res += CalcDirLight(baseColor, dirLights[i], norm, FragPos, viewDir); 

	//res += emptyReflectionMap(TexCoords);
	res += reflectionMapSelection(TexCoords); 
	//res += reflectionMapSelection(TexCoords); 
	//
	float shadow = shadowMapSelection(FragPosLightSpace, norm, dirLights[0]);
	//shadowMapSelection(FragPosLightSpace, norm, dirLights[0]);

	res *= vec3(1.0 - shadow);

	fColor = vec4(res, 1.0);
	//fColor = vec4(norm, 1.0);
}
