#version 130
#extension GL_ARB_uniform_buffer_object : require

out vec4 outputColor;

in vec2 texCoordFrag;

uniform usampler2D tex;

void main()
{
    outputColor = texture(tex, texCoordFrag).x == 0u ? vec4(1, 0, 0, 1) : vec4(0, 0, 0, 1);
}
