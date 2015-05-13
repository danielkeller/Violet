out vec4 outputColor;

flat in ivec2 minBoxFrag;
flat in ivec2 maxBoxFrag;

in vec2 fragCoord;
in float zFrag;
flat in vec4 fillFrag;
flat in vec4 strokeFrag;

void main()
{
	ivec2 ifragCoord = ivec2(fragCoord);
	int edgeDist = min(
		min(ifragCoord.x - minBoxFrag.x, ifragCoord.y - minBoxFrag.y),
		min(maxBoxFrag.x - ifragCoord.x, maxBoxFrag.y - ifragCoord.y));

	if (edgeDist == 0)
		outputColor = strokeFrag;
	else
		outputColor = fillFrag;

	if (outputColor.a == 0)
		discard;

	outputColor.a = zFrag;
}