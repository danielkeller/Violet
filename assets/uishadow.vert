in vec3 position;
in vec2 texCoord;

uniform Common
{
	mat4 pixelMat;
};

in ivec2 minBox;
in ivec2 maxBox;
in int z;

flat out vec2 minBoxFrag;
flat out vec2 maxBoxFrag;

out float zFrag;

out vec2 positionFrag;
out vec2 texCoordFrag;

void main()
{
	minBoxFrag = (pixelMat * vec4(minBox, 0, 1)).xy;
    maxBoxFrag = (pixelMat * vec4(maxBox, 0, 1)).xy;
    texCoordFrag = texCoord;
    positionFrag = position.xy;
    
	zFrag = .999 - z / 50.f; //FIXME
	gl_Position = vec4(position.xy, zFrag, 1);
}
