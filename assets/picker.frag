#version 330

flat in uint objectFrag;
out uint outputColor;

void main()
{
   outputColor = 5u; //objectFrag;
}
