in vec2 texCoord;

uniform Common
{
	mat4 pixelMat;
};

in ivec2 minBox;
in ivec2 maxBox;
in int z;
in vec4 fill;
in vec4 stroke;

flat out ivec2 minBoxFrag;
flat out ivec2 maxBoxFrag;

out vec2 fragCoord;
out float zFrag;
flat out vec4 fillFrag;
flat out vec4 strokeFrag;

void main()
{
	fillFrag = fill; strokeFrag = stroke;
	minBoxFrag = minBox; maxBoxFrag = maxBox;
	fragCoord = vec2(minBox + texCoord*(maxBox + 1 - minBox));
    gl_Position = pixelMat * vec4(fragCoord, z, 1);
	zFrag = gl_Position.z;
}
