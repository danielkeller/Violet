out vec4 outputColor;

in vec2 texCoordFrag;

uniform sampler2D tex;

void main()
{
	//assume rgb pixels
	vec2 subPixOffs = vec2(1./512., 0);
	float ra = texture(tex, texCoordFrag - subPixOffs).r;
	float ga = texture(tex, texCoordFrag).r;
	float ba = texture(tex, texCoordFrag + subPixOffs).r;
	outputColor = vec4(ra, ga, ba, 1);
}
