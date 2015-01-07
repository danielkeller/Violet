#version 330

out vec4 outputColor;

uniform Material
{
    vec3 direction;
};

void main()
{
    outputColor = vec4(direction, 1);
}
