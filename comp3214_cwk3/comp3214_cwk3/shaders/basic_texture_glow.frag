#version 400 core



// ins
in vec3 o_color;
in vec2 o_uv;

// uniforms
uniform mat4 u_m;
uniform mat4 u_v;
uniform mat4 u_p;

uniform sampler2D u_tex;
uniform vec3 u_glow_res;



void main() 
{	
	vec3 final_color;
	for(float x = -u_glow_res.z* u_glow_res.x; x < u_glow_res.z* u_glow_res.x; x+= u_glow_res.x)
		for(float y = -u_glow_res.z* u_glow_res.y; y < u_glow_res.z* u_glow_res.y; y+= u_glow_res.y)
			final_color += texture2D(u_tex,o_uv + vec2(x,y)).rgb;

	//texture2D(u_tex,o_uv).rgb +
	//texture2D(u_tex,o_uv + vec2(1,0) * u_glow_res.x).rgb + 
	//texture2D(u_tex,o_uv + vec2(-1,0) * u_glow_res.x).rgb + 
	//texture2D(u_tex,o_uv + vec2(-0,1) * u_glow_res.y).rgb + 
	//texture2D(u_tex,o_uv + vec2(0,1) * u_glow_res.y).rgb;

	final_color /= u_glow_res.z * u_glow_res.z * 4;
	final_color += texture2D(u_tex,o_uv).rgb;
	final_color /= 2.0f;

// apply fragment color
	gl_FragColor = vec4(final_color,1);
}