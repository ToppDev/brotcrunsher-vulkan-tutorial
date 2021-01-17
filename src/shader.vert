#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 inUVCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUVCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragViewVec;
layout(location = 4) out vec3 fragLightVec;

layout(binding = 0) uniform UBO // Uniform Buffer Object
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightPosition;
} ubo;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos, 1.0);
    vec4 worldPos = ubo.model * vec4(pos, 1.0);

    fragColor = color;
    fragUVCoord = inUVCoord;
    fragNormal = mat3(ubo.view) * mat3(ubo.model) * inNormal; // Dont use translation, only rotation
    fragViewVec = -(ubo.view * worldPos).xyz;                             // Swizzle operator because vec4 --> vec3
    fragLightVec = mat3(ubo.view) * (ubo.lightPosition - vec3(worldPos)); // Casting operator because vec4 --> vec3
}