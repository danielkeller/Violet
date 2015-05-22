in vec2 texCoord;

out vec2 texCoordFrag;

uniform Common
{
	mat4 pixelMat;
};

in vec2 minBox;
in vec2 minBoxTex;
in vec2 maxBox;
in vec2 maxBoxTex;
in int z;

vec2 pickVec(vec2 minbox, vec2 maxbox)
{
	return minbox*(1 - texCoord) + maxbox*texCoord;
}

void main()
{
	gl_Position = pixelMat*vec4(pickVec(minBox, maxBox), z + 0.2, 1);
	texCoordFrag = vec2(0, 1) + vec2(1, -1)*pickVec(minBoxTex, maxBoxTex);
}
