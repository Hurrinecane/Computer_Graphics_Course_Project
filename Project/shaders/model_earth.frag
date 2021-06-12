#version 330 core

in vec3 vertColor;
in vec2 texCoords;
in vec3 vertNormal;
in vec3 FragPos;
in mat3 TBN;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 brightColor;


struct Light {
    int type;

    vec3 position;
    vec3 direction;
    float cutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

#define MAX_LIGHTS 4
uniform Light light[MAX_LIGHTS];
uniform int lights_count;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform float shininess = 64.0f;

uniform vec3 viewPos;

uniform samplerCube depthMap;
uniform float far_plane;
uniform bool shadows;

// ћассив направлений смещени€ дл€ сэмплировани€
vec3 gridSamplingDisk[20] = vec3[](
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float getAtten(int i){
    float dist = distance(light[i].position, FragPos);
    float attenuation = 1.0 / (light[i].constant + light[i].linear*dist + light[i].quadratic * dist * dist);
    return attenuation;
}

vec3 CalcDiffusePlusSpecular(int i, vec3 lightDir){
    vec3 norm = texture(texture_normal1, texCoords).rgb;
    norm = normalize(norm * 2.0f - 1.0f);
    norm = normalize(TBN * norm);
    //vec3 norm = normalize(vertNormal);
    float diff_koef = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light[i].diffuse * diff_koef * vec3(texture(texture_diffuse1, texCoords));

    // specular
    vec3 reflectDir = reflect(lightDir, norm);
    vec3 viewDir = normalize(FragPos-viewPos);
    float spec_koef = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    vec3 specular = light[i].specular * spec_koef * vec3(texture(texture_specular1, texCoords));

    return diffuse + specular;
}

float ShadowCalculation(vec3 fragPos, int i){
    // ѕолучаем вектор между положением фрагмента и положением источника света
    vec3 fragToLight = fragPos - light[i].position;
	
    // »спользуем полученный вектор дл€ выборки из карты глубины    
    // float closestDepth = texture(depthMap, fragToLight).r;
	
    // ¬ данный момент значени€ лежат в диапазоне [0,1]. ѕреобразуем их обратно к исходным значени€м
    // closestDepth *= far_plane;
	
    // “еперь получим текущую линейную глубину как длину между фрагментом и положением источника света
    float currentDepth = length(fragToLight);
	
    // “еперь проводим проверку на нахождение в тени
    // float bias = 0.05; // мы используем гораздо большее теневое смещение, так как значение глубины теперь находитс€ в диапазоне [near_plane, far_plane]
    // float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
	
    // PCF
    // float shadow = 0.0;
    // float bias = 0.05; 
    // float samples = 4.0;
    // float offset = 0.1;
    // for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    // {
        // for(float y = -offset; y < offset; y += offset / (samples * 0.5))
        // {
            // for(float z = -offset; z < offset; z += offset / (samples * 0.5))
            // {
                // float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r; // используем lightdir дл€ осмотра кубической карты
                // closestDepth *= far_plane;   // Undo mapping [0;1]
                // if(currentDepth - bias > closestDepth)
                    // shadow += 1.0;
            // }
        // }
    // }
    // shadow /= (samples * samples * samples);
    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth = 0.81f;
        closestDepth *= far_plane; 
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    // ќтладка - отображение значений переменной  closestDepth (дл€ визуализации кубической карты глубины)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
        
    return shadow;
}

void main()
{
    vec3 lresult;
    for (int i = 0; i<lights_count; i++)
    {
        float shadow = shadows ? ShadowCalculation(FragPos, i) : 0.0;       
        //float shadow = 0.1f; 
        if (light[i].type == 1) // Directional Light
        {
            vec3 lightDir = -light[i].direction;

            vec3 ambient = light[i].ambient * texture(texture_diffuse1, texCoords).rgb;
            vec3 diffspec = CalcDiffusePlusSpecular(i, lightDir);

            lresult = ambient + (1.0 - shadow) * diffspec;
        }
        else 
        { 
            vec3 ambient = light[i].ambient * vec3(texture(texture_diffuse1, texCoords));
            vec3 lightDir = normalize(light[i].position - FragPos);
            if (light[i].type == 2) // Point Light
            {
                float attenuation = getAtten(i);
                vec3 diffspec = CalcDiffusePlusSpecular(i, lightDir);

                lresult = (ambient + (1.0 - shadow) * diffspec) * attenuation;
            }
            else if (light[i].type == 3) // SpotLight
            {
                float angle = acos(dot(lightDir, normalize(-light[i].direction)));

                if (angle <= light[i].cutOff*2.0f)
                {
                    float koef  = 1.0f;
                    if (angle >= light[i].cutOff)
                    {
                        koef = (light[i].cutOff*2.0f - angle) / light[i].cutOff;
                    }

                    float attenuation = getAtten(i);
                    vec3 diffspec = CalcDiffusePlusSpecular(i, lightDir) * koef;

                    lresult = (ambient + (1.0 - shadow) * diffspec) * attenuation;
                }
                else
                {
                    lresult =  vec3(texture(texture_diffuse1, texCoords)) * light[i].ambient;
                }
            }
        }
    //float brightness = dot(lresult, vec3(0.2126, 0.7152, 0.0722));
    //if(brightness < 1.0)
        //brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    //else
        brightColor += vec4(lresult, 1.0f);
    outColor += vec4(lresult, 1.0f);

    }// end of for
}