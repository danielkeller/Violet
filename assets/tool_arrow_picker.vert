#version 330

in vec3 position;
in uint object;
in mat4 transform;

uniform Common
{
	mat4 camera;
};
uniform Material
{
    vec3 direction;
};

flat out uint objectFrag;

//screen size
float size = .1;

void main()
{
    vec4 dir = camera * transform * vec4(direction, 0);
    //project onto screen
    vec3 dir3 = normalize(vec3(dir.xy, 0));
    //create transform
    mat4 realTransform = mat4(1);
    realTransform[0].xyz = dir3*size;
    realTransform[1].xyz = cross(dir3, vec3(0,0,1))*size;
    realTransform[2].z = size;
    realTransform[3].xy = (camera * transform[3]).xy;
    
    gl_Position = realTransform * vec4(position, 1);
    objectFrag = object;
}
