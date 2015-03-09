in vec3 position;
in vec2 texCoord;

out vec2 texCoordFrag;

void main()
{
    gl_Position = vec4(position.xy, 0, 1);
	texCoordFrag = texCoord;
}
