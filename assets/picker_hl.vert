in vec3 position;
in vec2 texCoord;

uniform Common
{
	mat4 camera;
};

out vec2 texCoordFrag;

in mat4 transform;

void main()
{
    gl_Position = transform * vec4(position.xy, 0, 1);
	texCoordFrag = texCoord;
}
