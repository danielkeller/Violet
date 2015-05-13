out vec4 color;
out uint picker;

flat in uint objectFrag;
in vec3 screenLight;
in vec3 fragPos;

void main()
{
   //screenspace normal
   vec3 normal = normalize(cross(dFdy(fragPos), dFdx(fragPos)));
   float diffuse = dot(normal, screenLight);
   color = vec4(vec3(diffuse),1);
   picker = objectFrag;
}
