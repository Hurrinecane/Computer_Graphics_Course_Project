#version 330 core
in vec3 texCoords;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

uniform samplerCube skybox;

void main()
{    
    brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    fragColor = texture(skybox, texCoords);
}