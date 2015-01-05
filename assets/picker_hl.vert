#version 130
#extension GL_ARB_uniform_buffer_object : require

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
    gl_Position = transform * vec4(position, 1);
	texCoordFrag = texCoord;
}
