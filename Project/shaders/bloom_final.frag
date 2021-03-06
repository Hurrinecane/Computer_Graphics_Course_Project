#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{             
    const float gamma = 1.8;
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor += bloomColor; // аддитивное смешение
    
	// Тональная компрессия
    vec3 result = vec3(1.0) - exp(-hdrColor * 1.f);
    
	// Гамма-коррекция       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}