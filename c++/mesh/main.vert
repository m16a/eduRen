#version 400 core

layout( location = 0 ) in vec4 vPosition;

uniform mat4 MVP;


void main()
{
	gl_Position = MVP * vPosition;
	//gl_Position = vPosition;
}