#version 460

layout(push_constant) uniform pushConstant
{
    vec4 color;
    float size;
};

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = vec4(color.rgb, 1.0);
}
