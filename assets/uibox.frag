out vec4 outputColor;

uniform Material
{
	ivec2 top;
	ivec2 size;
};

in vec2 fragCoord;

void main()
{
	ivec2 ifragCoord = ivec2(fragCoord);
	ivec2 bot = top + size - 1;
	int edgeDist = min(
		min(ifragCoord.x - top.x, ifragCoord.y - top.y),
		min(bot.x - ifragCoord.x, bot.y - ifragCoord.y));

	if (edgeDist == 0)
		outputColor = vec4(.5, .5, .5, 1);
	else
		outputColor = vec4(1, 1, 1, 1);
}