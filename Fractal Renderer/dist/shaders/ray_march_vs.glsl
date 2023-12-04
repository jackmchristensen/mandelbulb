#version 400
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform float time;

in vec3 pos_attrib; //this variable holds the position of mesh vertices

out vec4 v_pos;

float rand(vec2 co) {
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) * 2.0 - 1.0;
}

void main(void)
{
	//gl_PointSize = 10.0;
	//vec3 rand_position = vec3(pos_attrib.x + (rand(vec2(pos_attrib.y, pos_attrib.z)) / 100.0), 
	//						  pos_attrib.y + (rand(vec2(pos_attrib.x, pos_attrib.z)) / 100.0), 
	//						  pos_attrib.z + (rand(vec2(pos_attrib.y, pos_attrib.x)) / 100.0));
	//gl_Position = P*V*M*vec4(rand_position, 1.0); //transform vertices and send result into pipeline
	//v_pos = vec4(rand_position, 1.0);

	gl_Position = vec4(pos_attrib, 1.0);
	v_pos = vec4(pos_attrib, 1.0);
}