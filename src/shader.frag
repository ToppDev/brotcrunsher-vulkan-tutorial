#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUVCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVec;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;
layout(binding = 2) uniform sampler2D normalMap;

// Only one push constant and is minimal 128 Byte big (depends on graphics card)
layout(push_constant) uniform PushConstants
{
    bool usePhong;
} pushConts;

void main()
{
    vec3 texColor = texture(tex, fragUVCoord).xyz;

    // Dragon Code:
    //vec3 texColor = fragColor;
    //vec3 N = normalize(fragNormal);

    vec3 N = normalize(texture(normalMap, fragUVCoord).xyz);
    vec3 L = normalize(fragLightVec);
    vec3 V = normalize(fragViewVec);
    vec3 R = reflect(-L, N);

    if (pushConts.usePhong)
    {
        //                          ka = reflection constant for ambient lighting
        // ka * ia                  ia = global ambient lighting constant
        vec3 ambient = texColor * 0.1;

        //                          kd = reflection constant for diffuse lighting
        // kd * (L * N) * id        id = light color and stength of light source for diffuse lighting
        vec3 diffuse = 1.0 * max(dot(N, L), 0.0) * texColor;

        //                          ks = reflection constant for specular lighting
        //                          is = light color and stength of light source for specular lighting
        // ks * (R * V) ^ a * is    a  = shininess-constant
        vec3 specular = 1.0 * pow(max(dot(R, V), 0.0), 4.0) * vec3(0.35);

        outColor = vec4(ambient + diffuse + specular, 1.0);
    }
    else
    {
        // Cartoon Shader:
        if (pow(max(dot(R, V), 0.0), 5.0) > 0.5)
        {
            outColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
        else if (dot(V, N) < 0.5)
        {
            outColor = vec4(texColor / 10, 1.0);
        }
        else if (max(dot(N, L), 0.0) >= 0.1)
        {
            outColor = vec4(texColor, 1.0);
        }
        else
        {
            outColor = vec4(texColor / 5, 1.0);
        }
    }
}