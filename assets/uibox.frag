out vec4 outputColor;

flat in ivec2 minBoxFrag;
flat in ivec2 maxBoxFrag;

in vec2 fragCoord;

void main()
{
	ivec2 ifragCoord = ivec2(fragCoord);
	int edgeDist = min(
		min(ifragCoord.x - minBoxFrag.x, ifragCoord.y - minBoxFrag.y),
		min(maxBoxFrag.x - ifragCoord.x, maxBoxFrag.y - ifragCoord.y));

	if (edgeDist == 0)
		outputColor = vec4(.5, .5, .5, 1);
	else
		outputColor = vec4(1, 1, 1, 1);
}