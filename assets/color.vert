uniform Common
{
	mat4 camera;
	mat4 projection;
};
in mat4 transform;

in vec3 position;
in vec3 color;
out vec3 vert_color;

void main()
{
    gl_Position = projection * camera * transform * vec4(position, 1);
    vert_color = color;
}
