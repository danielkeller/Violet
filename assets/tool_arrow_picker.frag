#version 330

out uint outputColor;
flat in uint objectFrag;

void main()
{
    outputColor = objectFrag;
}
