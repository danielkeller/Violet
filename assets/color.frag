#version 130
#extension GL_ARB_uniform_buffer_object : require

out vec4 outputColor;
in vec3 vert_color;

void main()
{
    outputColor = vec4(vert_color, 1);
}
