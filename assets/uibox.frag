out vec4 outputColor;

uniform Material
{
	ivec2 viewport;
	ivec2 top;
	ivec2 size;
};

in vec4 gl_FragCoord;

void main()
{
	ivec2 fragCoord = ivec2(gl_FragCoord.x - 0.5, viewport.y - gl_FragCoord.y - 0.5);
	ivec2 bot = top + size - 1;
	int edgeDist = min(
		min(fragCoord.x - top.x, fragCoord.y - top.y),
		min(bot.x - fragCoord.x, bot.y - fragCoord.y));

	if (edgeDist == 0)
		outputColor = vec4(.5, .5, .5, 1);
	else
		outputColor = vec4(1, 1, 1, 1);
}