out vec4 outputColor;

in vec2 texCoordFrag;

uniform Material
{
	mat4 pixelMat;
	vec3 color;
	vec3 bgColor;
};

uniform sampler2D tex;

void main()
{
	//assume rgb pixels
	vec2 subPixOffs = vec2(1./512., 0);
	float ra = texture(tex, texCoordFrag - subPixOffs);
	float ga = texture(tex, texCoordFrag);
	float ba = texture(tex, texCoordFrag + subPixOffs);
	outputColor = vec4(
		ra*color.r + (1-ra)*bgColor.r,
		ga*color.g + (1-ga)*bgColor.g,
		ba*color.b + (1-ba)*bgColor.b,
		1);
}
