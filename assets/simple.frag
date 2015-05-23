out vec4 color;
out uint picker;

in vec2 texCoordFrag;
in vec3 normalFrag;
in vec3 posFrag;

flat in uint objectFrag;

uniform sampler2D tex;

uniform Material
{
	vec3 lightPos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

void main()
{
   vec3 lightDir = normalize(lightPos - posFrag);
   float lambert = max(dot(lightDir, normalFrag), 0.0);

   color = texture(tex, texCoordFrag) * 
      1;//vec4(ambient + lambert * diffuse, 1);
   picker = objectFrag;
}
