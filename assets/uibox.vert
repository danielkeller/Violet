in vec3 position;

uniform Material
{
	ivec2 viewport;
	ivec2 top;
	ivec2 size;
};

void main()
{
	vec3 pos = position / 2 + 0.5;
	float x = (top.x + pos.x * size.x + 0.5) / float(viewport.x);
	float y = (viewport.y - top.y - pos.y * size.y + 0.5) / float(viewport.y) ;
    gl_Position = vec4(vec2(x, y)*2-1, 0, 1);
}
