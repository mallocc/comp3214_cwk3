#version 400 core



// ins
layout(location = 0) in vec3 i_vert;
layout(location = 1) in vec3 i_color;
layout(location = 2) in vec3 i_norm;
layout(location = 3) in vec2 i_uv;

// uniforms
uniform mat4 u_m;
uniform mat4 u_v;
uniform mat4 u_p;

uniform vec3 u_light_pos;
uniform vec3 u_eye_pos;

// outs
out vec3 o_color;
out vec2 o_uv;
out vec3 o_norm;
out vec3 o_v_pos;

void main()
{
	mat4 n = transpose(inverse(u_v*u_m));

	mat4 v_m = u_v * u_m;
	vec4 v_m_pos = v_m * vec4(i_vert, 1.0f);

// color of vertex
	o_color			= i_color;

// uv tex coord
	o_uv			= i_uv;

// normal in world space
	vec4 new_norm  = n * vec4(i_norm,1);
	o_norm         = new_norm.xyz;

// view position	
	o_v_pos = v_m_pos.xyz;

// set projected point
	gl_Position		= u_p * v_m_pos;	
}