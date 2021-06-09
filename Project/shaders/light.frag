#version 330 core

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 brightColor;

uniform vec3 lightColor;

void main()
{
	outColor = vec4(lightColor, 1.0f);
	//float brightness = dot(outColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    //if(brightness < 1.0)
	//	brightColor = vec4(0.0, 0.0, 0.0, 1.0);
	//else
        brightColor = vec4(lightColor, 1.0);
}
