in vec2 texCoord;

uniform Common
{
	mat4 pixelMat;
};

uniform Material
{
	ivec2 top;
	ivec2 size;
};

out vec2 fragCoord;

void main()
{
	fragCoord = vec2(top + texCoord*size);
    gl_Position = pixelMat * vec4(fragCoord, 0, 1);
}
