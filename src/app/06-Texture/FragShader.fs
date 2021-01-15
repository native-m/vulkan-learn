#version 460

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 o_color;

void main()
{
    vec3 c = texture(tex, uv).rgb;
    o_color = vec4(c, 1.0);
}
