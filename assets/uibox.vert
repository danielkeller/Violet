in vec2 texCoord;

uniform Common
{
	mat4 pixelMat;
};

in ivec2 minBox;
in ivec2 maxBox;

flat out ivec2 minBoxFrag;
flat out ivec2 maxBoxFrag;

out vec2 fragCoord;

void main()
{
	minBoxFrag = minBox; maxBoxFrag = maxBox - 1;
	fragCoord = vec2(minBox + texCoord*(maxBox-minBox));
    gl_Position = pixelMat * vec4(fragCoord, 0, 1);
}
