out vec4 color;
out uint picker;

flat in uint objectFrag;
in vec2 texCoordFrag;

uniform sampler2D tex;

void main()
{
   color = texture(tex, texCoordFrag);
   picker = objectFrag;
}
