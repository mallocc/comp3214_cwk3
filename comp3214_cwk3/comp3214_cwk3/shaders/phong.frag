#version 400 core



// ins
in vec3 o_color;
in vec3 o_to_light;
in vec3 o_to_eye;
in vec3 o_norm;

// uniforms
uniform mat4 u_m;
uniform mat4 u_v;
uniform mat4 u_p;

uniform vec3 u_light_pos;
uniform vec3 u_eye_pos;

uniform float u_specular_scale;
uniform float u_shininess;
uniform float u_brightness;

uniform vec3 u_ambient_color;
uniform vec3 u_diffuse_color;

// hemisphereical lighting vars
vec3 up = vec3(0.0f,1.0f,0.0f);



void main() 
{	
// get distance to the light
	float r = length(o_to_light);
	float r2 = r*r; 

// work out after interpolation
	vec3 L = normalize(o_to_light);
	vec3 V = normalize(o_to_eye);
	vec3 N = normalize(o_norm);

// hemisphereical lighting
	float NdotL	= dot(N, up);
	float light_influence = NdotL * 0.5 + 0.5;
	vec3 hemi_color = mix(u_ambient_color / 10.0f, u_ambient_color, light_influence);

// calculate ambient effects
	vec3 amb = hemi_color;

// calculate diffuse effects
	vec3 dif = u_diffuse_color * clamp(dot(N, L), 0, 1) * u_brightness / r2;

// calculate specular effects
	float specular_term = 0;
// calculate specular reflection only if
// the surface is oriented to the light source
	if(dot(N, L) > 0)
	{
		// half vector
		vec3 H = normalize(L + V);
		specular_term = pow(dot(N, H), u_shininess);
	}
	float spe = u_specular_scale * specular_term;

// calculate final phong color
	vec3 final_color = (o_color) * (amb + dif + spe);

// apply fragment color
	gl_FragColor = vec4(final_color,1);
}
