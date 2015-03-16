in vec3 position;
in vec3 normal;
in vec2 texCoord;

uniform Common
{
	mat4 camera;
};

out vec2 texCoordFrag;

flat out uint objectFrag;

in mat4 transform;
in uint object;

void main()
{
    gl_Position = camera * transform * vec4(position, 1);
	texCoordFrag = texCoord;
    objectFrag = object;
}
