out vec4 outputColor;

in vec2 texCoordFrag;

uniform sampler2D color;
uniform usampler2D picker;

uniform Material
{
    uint selected[3];
};

vec4 halfBlend(vec4 a, vec4 b)
{
	return a*0.5 + b*0.5;
}

void main()
{
	outputColor = texture(color, texCoordFrag);

    if (texture(picker, texCoordFrag).r == uint(-2)) //"none" index
        ;//discard;
    else if (texture(picker, texCoordFrag).r == selected[0])
        outputColor = halfBlend(outputColor, vec4(1, 1, 1, 1));
    else if (texture(picker, texCoordFrag).r == selected[1])
        outputColor = halfBlend(outputColor, vec4(.5, .5, .5, 1));
    else if (texture(picker, texCoordFrag).r == selected[2])
        outputColor = halfBlend(outputColor, vec4(.5, .5, 1, 1));
}
