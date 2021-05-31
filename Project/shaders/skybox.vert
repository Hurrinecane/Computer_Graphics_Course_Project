#version 330 core
layout (location = 0) in vec3 inPos;

out vec3 texCoords;

uniform mat4 pv;
uniform mat4 model;

void main()
{
    
	vec4 vertPos = model * vec4(inPos, 1.0);
    gl_Position = pv* vec4(inPos, 1.0);
    texCoords = inPos;

}  