in vec3 position;

uniform Common
{
	mat4 camera;
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
    gl_Position = camera * transform * vec4(position, 1);

	screenLight = (camera * transform * vec4(light, 0)).xyz;
	fragPos = gl_Position.xyz;

    objectFrag = object;
}
