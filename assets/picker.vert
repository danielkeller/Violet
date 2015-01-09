in vec3 position;
in uint object;
flat out uint objectFrag;

uniform Common
{
	mat4 camera;
};

in mat4 transform;

void main()
{
    gl_Position = camera * transform * vec4(position, 1);
    objectFrag = object;
}
