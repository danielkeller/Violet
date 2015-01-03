#version 330

uniform Common
{
	mat4 camera;
};

in mat4 transform;

void main()
{
    gl_Position = vec4(0, 0, 0, 1);
}
