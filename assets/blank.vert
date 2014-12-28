#version 130
#extension GL_ARB_uniform_buffer_object : require

uniform Common
{
	mat4 camera;
};

in mat4 transform;

void main()
{
    gl_Position = vec4(0, 0, 0, 1);
}
