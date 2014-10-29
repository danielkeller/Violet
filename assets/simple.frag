#version 130
#extension GL_ARB_uniform_buffer_object : require

out vec4 outputColor;

in vec2 texCoordFrag;

uniform sampler2D tex;

void main()
{
   outputColor = texture(tex, texCoordFrag);
}
