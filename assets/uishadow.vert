in vec3 position;

uniform Common
{
	mat4 pixelMat;
};

in ivec2 minBox;
in ivec2 maxBox;
in int z;

flat out ivec2 minBoxFrag;
flat out ivec2 maxBoxFrag;

out float zFrag;

void main()
{
	minBoxFrag = minBox; maxBoxFrag = maxBox;
	zFrag = .999 - z / 10.f;
	gl_Position = vec4(position.xy, zFrag, 1);
}
