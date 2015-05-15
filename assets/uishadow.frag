out vec4 outputColor;

uniform sampler2D boxes;

uniform Common
{
	mat4 pixelMat;
};

flat in ivec2 minBoxFrag;
flat in ivec2 maxBoxFrag;

in float zFrag;

float PI = 3.1415927;

float shadow2(float zDiff, vec2 coord, float apparentSz)
{
	vec2 dist = max(coord - vec2(maxBoxFrag), vec2(minBoxFrag) - coord);

	vec2 dx = dist*apparentSz / zDiff;
	vec2 area = acos(dx) + dx * (dx*dx - 1);
	area = area / PI;

	if (dx.x > 1) area.x = 0;
	if (dx.x < -1) area.x = 1;
	if (dx.y > 1) area.y = 0;
	if (dx.y < -1) area.y = 1;

	return area.x*area.y;
}

void main()
{
	vec2 texCoord = (pixelMat * gl_FragCoord).xy * .5 + .5;
	float otherZ = texture(boxes, texCoord).a;

	float zScale = 40;
	float zDiff = (otherZ - zFrag) * zScale;

	float sh = shadow2(zDiff, gl_FragCoord.xy, 1)*.3
		+ shadow2(zDiff, gl_FragCoord.xy + vec2(0, zDiff*.6), 2)*.5;
	outputColor = (1 - sh)*vec4(1);
}