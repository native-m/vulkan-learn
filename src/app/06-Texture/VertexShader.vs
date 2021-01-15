#version 460

layout(push_constant) uniform pushConstant
{
    mat4 wvpMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 o_uv;

void main()
{
    o_uv = uv;
    gl_Position = wvpMatrix * vec4(pos, 1.0);
    o_uv.y = 1.0 - o_uv.y; // invert uv vertically
    gl_Position.y = -gl_Position.y; // invert y axis
}
