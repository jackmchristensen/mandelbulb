#version 400
uniform float time;
uniform float order;
uniform float ray_scalar;
uniform float focus_dist;
uniform int window_width;
uniform int window_height;
uniform int AA_level;
uniform int num_iterations;
uniform vec3 cam_pos;
uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform bool view_depth;

uniform vec3 color1;
uniform vec3 color2;
uniform vec3 color3;
uniform vec3 color4;
uniform vec3 color5;

in vec4 v_pos;

out vec4 fragcolor; //the output color for this fragment 

int iterations = 0;

float DE(vec3 pos) {
	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	float delta_rad = 1.0;
	float rad = 0.0;

	for (int i = 0; i < num_iterations; i++) {
		rad = length(vec3(x, y, z));
		if (rad > 2.0) {
			iterations = i;
			break;
		}

		// convert to polar coordinates
		float theta = acos(z / rad);
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
	float epsilon = 0.001;
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

float calculateSoftShadow(vec3 pos, vec3 lightDirection, float startOffset, float endDistance, float decayFactor) {
	int stepNum = 10;
	
	float shadowStrength = 1.0;
	float currentDistance = startOffset;
	float stepSize = (endDistance - startOffset) / float(stepNum);

	for (int i = 0; i < stepNum; i++) {
		vec3 currentPoint = pos + lightDirection * currentDistance;
		float distanceToSurface = DE(currentPoint);

		if (distanceToSurface < 0.001) {
			return 0.0;  // The point is in shadow
		}

		float shadowAttenuation = decayFactor * distanceToSurface / currentDistance;
		shadowStrength = min(shadowStrength, shadowAttenuation);
		currentDistance += stepSize;

		if (currentDistance >= endDistance) {
			break;
		}
	}

	return shadowStrength;
}


vec3 increaseSaturation(vec3 color, float saturationFactor) {
	float luminance = dot(color, vec3(0.299, 0.587, 0.114));
	return mix(vec3(luminance), color, saturationFactor);
}

vec4 getColorFromDensity(float density, vec3 position) {
	vec3 color;
	//float normDensity = exp(clamp(density, 0.0, 1.0)); // Normalize density between 0 and 
	float normDensity = density;
	normDensity *= length(position);

	// Interpolate between colors based on normalized density
	if (normDensity < 0.8) {
		color = mix(color1, color2, normDensity / 0.8);
	}
	else if (normDensity < 0.9) {
		color = mix(color2, color3, (normDensity - 0.8) / 0.1);
	}
	else if (normDensity < 0.95) {
		color = mix(color3, color4, (normDensity - 0.9) / 0.1);
	}
	else if (normDensity < 0.99) {
		color = mix(color4, color5, (normDensity - 0.95) / 0.1);
	}
	else {
		color = color5;
	}

	color = clamp(color, 0.0, 1.0);

	return vec4(color, density); // Full opacity
}

vec4 ColorMandelbulb(vec3 pos, vec3 normal, vec4 trap, vec3 rayDir, int escape_iterations, float dist, float firstDist) {

	// return if depth pass
	if (dist > 10.0) return vec4(0.0);
	if (view_depth) {
		vec3 focalDist = abs(vec3(dist / 1.25) - focus_dist);
		focalDist *= focalDist;
		return vec4(focalDist, 1.0);
	}

	vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
	vec3 lightColor = vec3(0.25);

	vec3 albedo = getColorFromDensity(float(escape_iterations) / 6.0, pos).xyz;

	vec3 viewDir = normalize(-rayDir);
	vec3 reflectDir = reflect(lightDir, normal);
	float spec_exp = 30.0;
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), spec_exp);
	vec3 specularColor = spec * lightColor * (1.0 - (1.0 / spec_exp));

	float shadow = calculateSoftShadow(pos, lightDir, 0.1, 100.0, 1.0);
	vec3 lighting = mix(albedo * (vec3(0.75) / 10.0), (albedo + (specularColor * 10.0)), shadow);

	return vec4(lighting, 1.0);
}

vec4 RayMarch(vec2 coords, vec2 offset)
{
	vec4 color = vec4(0.0);
	vec4 ray_pos = vec4(cam_pos, 1.0);

	float ray_dist = 0.0;
	float max_dist = 10.0;
	float epsilon = 0.0001;
	int escape_iterations = 0;
	int steps = 0;
	int max_steps = 1000;
	float firstDist = DE(ray_pos.yxz);
	epsilon *= firstDist;

	vec4 cam_dir = inverse(P) * vec4(coords + offset, 1.0, 1.0);
	cam_dir /= cam_dir.w;
	cam_dir = vec4(cam_dir.xyz, 0.0);

	vec4 ray_dir = normalize(inverse(V) * cam_dir);
	float dist;

	vec4 trap = vec4(99999.0); // Initialize with a high value

	while (ray_dist < max_dist && steps < max_steps)
	{
		ray_pos.xyz = cam_pos + ray_dir.xyz * ray_dist;

		dist = DE(ray_pos.xyz);

		if (dist <= epsilon) break;

		ray_dist += dist * ray_scalar;
		steps++;
	}

	escape_iterations = iterations;
	vec2 color_scalar = vec2(length(ray_pos.xyz / sqrt(2.0)));
	vec3 normal = estimateNormal(ray_pos.xyz - ray_dir.xyz * epsilon * 2.0);

	color = ColorMandelbulb(ray_pos.xyz, normal, trap, -ray_dir.xyz, escape_iterations, ray_dist, firstDist);
	//color = getColorFromDensity(float(escape_iterations) / float(num_iterations), ray_pos.xyz);

	if (view_depth) return color;
	return vec4(color.rgb * ((float(escape_iterations) / 16.0) * 8.0), color.a);
}

void main(void)
{
   	vec3 color = vec3(0.05);
	vec4 finalColor = vec4(0.0);
	int AA = AA_level;  // Set the level of anti-aliasing
	float invAA = 1.0 / float(AA);

	vec2 ndc_pos = 2.0 * vec2(gl_FragCoord.x / window_width, gl_FragCoord.y / window_height) - 1.0;	

	for (int y = 0; y < AA; ++y) {
		for (int x = 0; x < AA; ++x) {
			vec2 offset = vec2(float(x)/float(window_width), float(y)/float(window_height));
			finalColor += RayMarch(ndc_pos, offset * invAA);
		}
	}


	finalColor.rgb /= float(AA * AA);
	//finalColor.rgb = increaseSaturation(finalColor.rgb, 2.0);
	//finalColor.rgb = pow(finalColor.rgb, vec3(1.0 / 2.2)); // Gamma correction RGB->sRGB

	fragcolor = finalColor;
}