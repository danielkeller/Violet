#version 330

flat in uint objectFrag;
out uint outputColor;

void main()
{
   outputColor = objectFrag;
}