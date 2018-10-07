#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform vec3 color;
uniform bool useColor;
uniform bool doGammaCorrection;
uniform bool debugShadowMap;

uniform sampler2D in_texture;

void main()
{
	vec3 resColor;
	if (useColor)
	{
		resColor = color;
	}
	else
	{
		if (debugShadowMap)
		{
			float depthValue = texture(in_texture, TexCoords).r;
			resColor = vec3(depthValue);
		}
		else
		{
			resColor = texture(in_texture, TexCoords).rgb;
		}
	}
	
	if (doGammaCorrection)
	{
		float gamma = 2.2;
		FragColor.rgb = pow(resColor, vec3(1.0/gamma));
	}
	else
	{
		FragColor = vec4(resColor, 1.0);
	}
} 
