#version 130
#extension GL_ARB_uniform_buffer_object : require

out vec4 outputColor;

in vec2 texCoordFrag;

uniform usampler2D tex;

uniform Material
{
    uint selected;
};

void main()
{
    uint num = texture(tex, texCoordFrag).r;
    outputColor = vec4(num, 0,0, selected);
    /*
    if (texture(tex, texCoordFrag).r != 1u)
        discard;
    else
        outputColor = vec4(1, 0, 0, selected);
        */
}
