in vec3 position;

in mat4 transform;
in uint object;

flat out uint objectFrag;

uniform Common
{
	mat4 camera;
	mat4 projection;
};

uniform Material
{
    vec3 direction;
};

//screen size
float size = .1;

void main()
{
	vec4 screenpos = projection * camera * transform[3];
	screenpos /= screenpos[3];
    vec4 dir = projection * camera * transform * vec4(direction, 0);
    //project onto screen
    vec3 dir3 = normalize(vec3(dir.xy, 0));
    //create transform
    mat4 realTransform = mat4(1);
    realTransform[0].xyz = dir3*size;
    realTransform[1].xyz = -cross(dir3, vec3(0,0,1))*size;
    realTransform[2].z = size;
    realTransform[3].xyz = vec3((screenpos).xy, .01);
    gl_Position = realTransform * vec4(position, 1);

    objectFrag = object;
}
