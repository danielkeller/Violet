in vec3 position;

uniform Common
{
	mat4 camera;
	mat4 projection;
};

uniform Material
{
   vec3 light;
};

out vec3 screenLight;
out vec3 fragPos;

flat out uint objectFrag;

in mat4 transform;
in uint object;

void main()
{
    gl_Position = projection * camera * transform * vec4(position, 1);

	screenLight = (projection * camera * transform * vec4(light, 0)).xyz;
	fragPos = gl_Position.xyz;

    objectFrag = object;
}
