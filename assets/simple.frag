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
	float specExp;
};

void main()
{
   vec3 lightDir = normalize(lightPos - posFrag);
   float lambert = max(dot(lightDir, normalFrag), 0.0);

   //FIXME
   vec3 camera = vec3(0, 3, 0);
   vec3 view = normalize(camera - posFrag);
   vec3 refl = 2*lambert*normalFrag - lightDir;
   float spec = lambert == 0.0 ? 0.0 : pow(max(dot(refl, view), 0.0), specExp);

   color = texture(tex, texCoordFrag) *
      vec4(ambient + lambert * diffuse + spec*specular, 1);
	  color = vec4(0,0,0.5,1);
   picker = objectFrag;
}
