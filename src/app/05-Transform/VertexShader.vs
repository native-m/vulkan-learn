#version 460

layout(push_constant) uniform pushConstant
{
    mat4 wvpMatrix;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 o_color;

void main()
{
    o_color = color;
    gl_Position = wvpMatrix * vec4(pos, 1.0);
    gl_Position.y = -gl_Position.y;
}
