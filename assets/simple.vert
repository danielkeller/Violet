#version 330

in vec3 position;
in vec3 normal;
in vec2 texCoord;

uniform Common
{
	mat4 camera;
};

in mat4 transform;

void main()
{
    gl_Position = camera * transform * vec4(position, 1);
}