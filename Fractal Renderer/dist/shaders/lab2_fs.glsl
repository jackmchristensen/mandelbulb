#version 400
uniform float time;
uniform float order;
uniform int window_width;
uniform int window_height;
uniform vec3 cam_pos;
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

in vec4 v_pos;

out vec4 fragcolor; //the output color for this fragment 

int iterations = 0;

float sphere(vec3 r_pos, vec3 center, float rad)
{
	return distance(r_pos, center) - rad;
}

float DE(vec3 pos) {
	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	float delta_rad = 1.0;
	float rad = 0.0;

	for (int i = 0; i < 15; i++) {
		rad = length(vec3(x, y, z));
		if (rad > 2.0) {
			iterations = i;
			break;
		}

		// convert to polar coordinates
		float theta = acos(z / rad);				// <- This is what was used in the code I found for the DE. Seems to run just a tad smoother
		//float theta = atan(sqrt(x*x + y*y), z);	// <- This is the equation I used in the original point renderer
		float phi = atan(y, x);
		delta_rad = pow(rad, order - 1.0) * order * delta_rad + 1.0;

		// scale and rotate the point
		float zeta_r = pow(rad, order);
		theta = theta * order;
		phi = phi * order;

		// convert back to cartesian coordinates
		x = zeta_r * sin(theta) * cos(phi) + pos.x;
		y = zeta_r * sin(phi) * sin(theta) + pos.y;
		z = zeta_r * cos(theta) + pos.z;
	}
	return 0.5 * log(rad) * rad / delta_rad;
}

vec3 estimateNormal(vec3 p)
{
	float epsilon = 0.0001;
	float x = DE(vec3(p.x + epsilon, p.y, p.z)) - DE(vec3(p.x - epsilon, p.y, p.z));
	float y = DE(vec3(p.x, p.y + epsilon, p.z)) - DE(vec3(p.x, p.y - epsilon, p.z));
	float z = DE(vec3(p.x, p.y, p.z + epsilon)) - DE(vec3(p.x, p.y, p.z - epsilon));
	return normalize(vec3(x, y, z));
}

float ambientOcclusion(vec3 pos, vec3 normal, float step_dist, float step_num)
{
	float occlusion = 1.0f;
	while (step_num > 0.0)
	{
		occlusion -= pow(step_num * step_dist - DE(pos + normal * step_num * step_dist), 2) / step_num;
		step_num--;
	}

	return occlusion;
}

void main(void)
{
   	vec3 color = vec3(0.05);
	vec4 ray_pos = vec4(cam_pos, 1.0);
	float ray_dist = 0.0;
	float max_dist = 100.0;
	float epsilon = 0.0001 * length(cam_pos);
	float escape_iterations = 0.0;
	int steps = 0;
	int max_steps = 1000;

	vec2 ndc_pos = 2.0 * vec2(gl_FragCoord.x / window_width, gl_FragCoord.y / window_height) - 1.0;	
	vec4 cam_dir = inverse(P) * vec4(ndc_pos, 1.0, 1.0);
	cam_dir /= cam_dir.w;
	cam_dir = vec4(cam_dir.xyz, 0.0);

	vec4 ray_dir = normalize(inverse(V) * cam_dir);

	while (ray_dist < max_dist && steps < max_steps)
	{
		steps++;
		float dist = DE(ray_pos.yxz);

		if (dist <= epsilon)
		{
			escape_iterations = iterations;
			vec2 color_scalar = vec2(length(ray_pos.yxz / sqrt(2.0)));
			vec3 normal = estimateNormal(ray_pos.yxz-ray_dir.xyz * epsilon * 2.0);
			float ao = pow(ambientOcclusion(ray_pos.yxz, normal, 0.015, 20.0), 10);

			color = vec3(color_scalar, 3.0) * (1.0-ao);
			//color = normal;
			break;
			
		}
		ray_pos += ray_dir * dist;
		ray_dist += dist;
	}

	fragcolor = vec4(color, 1.0) + (escape_iterations / 16.0);
}

//float pointRender(vec4 position)
//{
//	float x = 0.f;
//	float y = 0.f;
//	float z = 0.f;
//
//	for (unsigned int iter = 0; iter <= 20; ++iter) {
//		float xx = x * x;
//		float yy = y * y;
//		float zz = z * z;
//		float rad = sqrt(xx + yy + zz);
//
//		if (rad > 2.f) {
//			discard;
//		}
//
//		float theta = atan(sqrt(xx + yy), z);
//		float phi = atan(y, x);
//
//		// Mandelbulb algorithm
//		x = v_pos.x + pow(rad, order) * sin(theta * order) * cos(phi * order);
//		y = v_pos.y + pow(rad, order) * sin(theta * order) * sin(phi * order);
//		z = v_pos.z + pow(rad, order) * cos(theta * order);
//	}
//	float dist = sqrt(position.x * position.x + position.y * position.y + position.z * position.z) / sqrt(2.0);
//	return dist;
//}
//
//void main(void)
//{
//	float dist = pointRender(v_pos);
//	fragcolor = vec4(vec2(dist), 1.0, 1.0);
//}