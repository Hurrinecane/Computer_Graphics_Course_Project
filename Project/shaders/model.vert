#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoords;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

out V_OUT {
out vec2 texCoords;
out vec3 vertNormal;
out mat3 TBN;
out vec3 fragPos;
} vs_out;

uniform mat4 pv;
uniform mat4 model;

void main()
{
	vec4 vertPos = model * vec4(inPos, 1.0);
	gl_Position = pv * vertPos;
	vs_out.texCoords = inTexCoords;
	vs_out.vertNormal = mat3(model)*inNormal;
	vs_out.fragPos = vertPos.xyz;
	vec3 T = normalize((model*vec4(inTangent, 0.0f)).xyz);
	vec3 B = normalize((model*vec4(inBiTangent, 0.0f)).xyz);
	vec3 N = normalize((model*vec4(inNormal, 0.0f)).xyz);
	vs_out.TBN = mat3(T,B,N);
}