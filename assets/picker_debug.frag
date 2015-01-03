#version 330

out vec4 outputColor;

in vec2 texCoordFrag;

uniform usampler2D tex;

void main()
{
    outputColor = texture(tex, texCoordFrag).x == 0u ? vec4(1, 0, 0, 1) : vec4(0, 0, 0, 1);
}
