out vec4 outputColor;

in vec2 texCoordFrag;

uniform usampler2D tex;

uniform Material
{
    uint selected[3];
};

void main()
{
    if (texture(tex, texCoordFrag).r == uint(-2)) //"none" index
        discard;
	//HACK HACK
    else if (texture(tex, texCoordFrag).r == selected[0])
        outputColor = vec4(1, 1, 1, 1);
    else if (texture(tex, texCoordFrag).r == selected[1])
        outputColor = vec4(.5, .5, .5, 1);
    else if (texture(tex, texCoordFrag).r == selected[2])
        outputColor = vec4(.5, .5, 1, 1);
	else
		discard;
}
