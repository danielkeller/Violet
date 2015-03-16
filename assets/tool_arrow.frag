out vec4 color;
out uint picker;

flat in uint objectFrag;

uniform Material
{
    vec3 direction;
};

void main()
{
    color = vec4(direction, 1);
    picker = objectFrag;
}
