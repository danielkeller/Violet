out vec4 color;
out uint picker;

in vec2 texCoordFrag;
in vec3 normalFrag;
in vec3 posFrag;

uniform Common
{
	mat4 camera;
	mat4 projection;
};

flat in uint objectFrag;

uniform sampler2D tex;

uniform Material
{
	vec3 lightPos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
    vec4 objColor;
	float specExp;
};

void main()
{
   vec3 normal = normalize(normalFrag);

   vec3 lightDir = normalize(lightPos - posFrag);
   float lambert = max(dot(lightDir, normal), 0.0);

   vec3 view = -normalize((camera * vec4(posFrag, 1)).xyz);
   vec3 refl = mat3(camera) * (2*lambert*normal - lightDir);
   float spec = pow(max(dot(refl, view), 0.0), specExp);
   if (lambert == 0.0)
      spec = 0.0;

   color = (texture(tex, texCoordFrag) + objColor) *
      vec4(ambient + lambert * diffuse + spec * specular, 1);
   
   //color = vec4(refl, 1);

   picker = objectFrag;
}
