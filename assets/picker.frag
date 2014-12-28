#version 130
#extension GL_ARB_uniform_buffer_object : require

flat in uint objectFrag;
out uint outputColor;

void main()
{
   outputColor = objectFrag;
}
