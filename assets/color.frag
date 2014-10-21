#version 330

out vec4 outputColor;
in vec3 vert_color;

void main()
{
    outputColor = vec4(vert_color, 1);
}