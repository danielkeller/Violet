#version 330

out vec4 outputColor;

in vec2 texCoordFrag;

uniform usampler2D tex;

uniform Material
{
    uint selected;
};

void main()
{
    if (texture(tex, texCoordFrag).r != selected
        || selected == uint(-2)) //"none" index
        discard;
    else
        outputColor = vec4(1, 0, 0, 1);
}
