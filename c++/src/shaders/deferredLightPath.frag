#version 400 core

in vec2 TexCoords;
//in vec4 FragPosLightSpace;

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
	float farPlane;
};

#define NR_POINT_LIGHTS 2
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

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform mat4 lightSpaceMatrix;

out vec4 fColor;

struct Color
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

// ----------------------shadow map-------------------------------------
subroutine float shadowMap(vec3 fragPos, vec3 normal, vec4 fragPosLightSpace, DirLight light);

subroutine (shadowMap) float emptyShadowMap(vec3 fragPos, vec3 normal, vec4 fragPosLightSpace, DirLight light)
{
	return 0.0f;
}

subroutine (shadowMap) float globalShadowMap(vec3 fragPos, vec3 normal, vec4 fragPosLightSpace, DirLight light)
{
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	float shadow = 0.0;

	if (projCoords.z <= 1.0)
	{
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

		vec2 texelSize = 1.0 / textureSize(light.shadowMapTexture, 0);
		for(int x = -1; x <= 1; ++x)
		{
				for(int y = -1; y <= 1; ++y)
				{
						float pcfDepth = texture(light.shadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r; 
						shadow += currentDepth - bias > pcfDepth ? 0.5 : 0.0;        
				}    
		}
		shadow /= 9.0;
	}

	//calculate omnidirectional shadows
	for (int i = 0; i < nPointLights; i++)
	{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - pointLights[i].pos;

    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);

		if (currentDepth < pointLights[i].farPlane)
		{
			// use the light to fragment vector to sample from the depth map    
			float closestDepth = texture(pointLights[i].shadowMapTexture, (fragToLight)).r;
			// it is currently in linear range between [0,1]. Re-transform back to original value
			closestDepth *= pointLights[i].farPlane;
			// now test for shadows
			float bias = 0.5; 
			shadow += (currentDepth -  bias > closestDepth ? 0.5 : 0.0);
		}
	}

	return shadow;
}

subroutine uniform shadowMap shadowMapSelection;
// -----------------------------------------------------------
void main()
{    
	Color res;
	res.ambient = vec4(0.0);
	res.diffuse = vec4(0.0);
	res.specular = vec4(0.0);

	// retrieve data from gbuffer
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;
	
	// then calculate lighting as usual
	vec3 viewDir  = normalize(camPos - FragPos);
	for(int i = 0; i < nPointLights; ++i)
	{
			// diffuse
			vec3 lightDir = normalize(pointLights[i].pos - FragPos);
			vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * pointLights[i].diffuse;
			// specular
			vec3 halfwayDir = normalize(lightDir + viewDir);  
			float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
			vec3 specular = pointLights[i].specular * spec * Specular;
			// attenuation
			float distance = length(pointLights[i].pos - FragPos);
			float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);
			res.ambient += vec4(Diffuse * pointLights[i].ambient * attenuation, 1.0);
			diffuse *= attenuation;
			specular *= attenuation;
			res.diffuse += vec4(diffuse, 1.0);
			res.specular += vec4(specular, 1.0);
	}

	for (int i=0; i < nDirLights; ++i)
	{
		float diff = max(dot(Normal, dirLights[i].dir), 0.0);

		// specular shading
		vec3 reflectDir = reflect(-dirLights[i].dir, Normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);

		// combine results
		res.ambient  += vec4(dirLights[i].ambient * Diffuse.rgb, 1.0);
		res.diffuse  += vec4(dirLights[i].diffuse * diff * Diffuse.rgb, 1.0);
		res.specular += vec4(dirLights[i].specular * spec * Specular, 1.0);
	}

	vec4 dirFragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
	float shadow = shadowMapSelection(FragPos, Normal, dirFragPosLightSpace, dirLights[0]);
	fColor = vec4(vec3(res.ambient + (res.diffuse + res.specular) * (1.0 - shadow)), 1.0f);
}
