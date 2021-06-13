#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in V_OUT {
in vec2 texCoords;
in vec3 vertNormal;
in mat3 TBN;
in vec3 fragPos;
} gs_in[];

out G_OUT{
out vec2 texCoords;
out vec3 vertNormal;
out mat3 TBN;
out vec3 fragPos;
}gs_out;

uniform float blow;
uniform bool collapse;

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 0.5;
    vec3 direction = normalize(normal + blow) * ((sin(blow)) / 2.0) * magnitude;
    return position + vec4(direction, 0.0);
}

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

void main() 
{
    if (collapse)
    {
        vec3 normal = GetNormal();
        
        gl_Position = explode(gl_in[0].gl_Position, normal);
        gs_out.texCoords =  gs_in[0].texCoords;
        gs_out.vertNormal = gs_in[0].vertNormal;
        gs_out.TBN =        gs_in[0].TBN;
        gs_out.fragPos =    gs_in[0].fragPos;
        EmitVertex();
        gl_Position = explode(gl_in[1].gl_Position, normal);
        gs_out.texCoords =  gs_in[1].texCoords;
        gs_out.vertNormal = gs_in[1].vertNormal;
        gs_out.TBN =        gs_in[1].TBN;
        gs_out.fragPos =    gs_in[1].fragPos;
        EmitVertex();
        gl_Position = explode(gl_in[2].gl_Position, normal);
        gs_out.texCoords =  gs_in[2].texCoords;
        gs_out.vertNormal = gs_in[2].vertNormal;
        gs_out.TBN =        gs_in[2].TBN;
        gs_out.fragPos =    gs_in[2].fragPos;
        EmitVertex();
        EndPrimitive();
    }
    else
    {
        gl_Position = gl_in[0].gl_Position;
        gs_out.texCoords =  gs_in[0].texCoords;
        gs_out.vertNormal = gs_in[0].vertNormal;
        gs_out.TBN =        gs_in[0].TBN;
        gs_out.fragPos =    gs_in[0].fragPos;
        EmitVertex();
        gl_Position = gl_in[1].gl_Position;
        gs_out.texCoords =  gs_in[1].texCoords;
        gs_out.vertNormal = gs_in[1].vertNormal;
        gs_out.TBN =        gs_in[1].TBN;
        gs_out.fragPos =    gs_in[1].fragPos;
        EmitVertex();
        gl_Position = gl_in[2].gl_Position;
        gs_out.texCoords =  gs_in[2].texCoords;
        gs_out.vertNormal = gs_in[2].vertNormal;
        gs_out.TBN =        gs_in[2].TBN;
        gs_out.fragPos =    gs_in[2].fragPos;
        EmitVertex();
        //EndPrimitive();
    }
}