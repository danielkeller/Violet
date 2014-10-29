#version 130
#extension GL_ARB_uniform_buffer_object : require

uniform Common
{
	mat4 camera;
};
in mat4 transform;

in vec3 position;
in vec3 color;
out vec3 vert_color;

void main()
{
    gl_Position = camera * transform * vec4(position, 1);
    vert_color = color;
}
