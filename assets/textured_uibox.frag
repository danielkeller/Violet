out vec4 outputColor;

in vec2 texCoordFrag;

uniform sampler2D tex;

void main()
{
	outputColor = texture(tex, texCoordFrag);

	if (outputColor.a == 0)
		discard;
}
