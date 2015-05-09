out vec4 outputColor;

flat in ivec2 minBoxFrag;
flat in ivec2 maxBoxFrag;

in vec2 fragCoord;

uniform Material
{
	vec4 fill;
	vec4 stroke;
};

void main()
{
	ivec2 ifragCoord = ivec2(fragCoord);
	int edgeDist = min(
		min(ifragCoord.x - minBoxFrag.x, ifragCoord.y - minBoxFrag.y),
		min(maxBoxFrag.x - ifragCoord.x, maxBoxFrag.y - ifragCoord.y));

	if (edgeDist == 0)
		outputColor = stroke;
	else
		outputColor = fill;

	if (outputColor.a == 0)
		discard;
}