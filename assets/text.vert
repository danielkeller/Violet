in vec2 texCoord;

out vec2 texCoordFrag;

uniform Material
{
	mat4 pixelMat;
	vec3 color;
	vec3 bgColor;
};

in vec2 topLeft;
in vec2 topLeftTex;
in vec2 botRight;
in vec2 botRightTex;

vec2 pickVec(vec2 topleft, vec2 botright)
{
	return vec2(
		topleft.x*(1 - texCoord.x) + botright.x*texCoord.x,
		topleft.y*(1 - texCoord.y) + botright.y*texCoord.y);
}

void main()
{
    gl_Position = pixelMat*vec4(pickVec(topLeft, botRight), 0, 1);
	texCoordFrag = pickVec(topLeftTex, botRightTex);
}
