#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 o_color;

void main()
{
    o_color = color;
    gl_Position = vec4(pos, 1.0);
}
