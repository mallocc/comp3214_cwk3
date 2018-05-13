#include "cwk3.h"
#include "colors.h"
#include "lerp.h"
#include <chrono>
#include <thread>
#include <minmax.h>
#include <glm\glm\gtc\noise.hpp>
#include <sstream>
#include "fbo.h"

#define COE 1
#define GRAVITY 0
#define MOTION_L 20

GLSLProgramManager	programs;

VarHandle
model_mat_handle,
view_mat_handle,
proj_mat_handle,
ambient_color_handle,
texture_handle,

motion_model_mat_handle,
motion_view_mat_handle,
motion_proj_mat_handle,
motion_texture_handle,
motion_handles[MOTION_L],
current_frame_handle,
motion_length_handle,

glow_model_mat_handle,
glow_view_mat_handle,
glow_proj_mat_handle,
glow_texture_handle,
glow_res_handle,

shift_model_mat_handle,
shift_view_mat_handle,
shift_proj_mat_handle,
shift_texture_handle,
shift_handle
	;

glm::vec2
	window_size(1280, 720);

glm::vec3
	eye_position(0, 5, -20),
	old_eye_position = eye_position,
	eye_direction,
	up(0, 1, 0),
	fov(45, 0, 0),
	ambient_color(0.1f,0.1f,0.1f),
	terrain_pos(0,0,1000.0f);

glm::mat4 
	model, 
	view, 
	projection;

FBO
	motion_blur_parts[MOTION_L],
	motion_blur_fbo,
	glow_fbo,
	basic_fbo,
	shift_fbo,
	combined_fbo;

Obj
terrain, water, falcon, 
earth,  mars, venus, mercury, neptune, uranus, saturn, jupiter,
star, moon, stars,
screen_texture;

std::vector<CameraSequencer> cameraSequences;

int current_frame_x = 0;
int current_frame = 0;

int l = MOTION_L;

#define SCENE_0 0
#define SCENE_1 1
#define SCENE_2 2
#define SCENE_3 3
#define SCENE_4 4
#define SCENE_N 5

int scene = SCENE_1;
int next_scene = 0;

bool MOTIONBLUR_ON = 0;
bool GLOW_ON = 1;
bool EFFECTS = 1;
float lorentz = 1;

float step_d = 0.0005f;
float step = step_d;

//Returns random float
inline float		randf()
{
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

void printV(glm::vec3 v)
{
	printf("x: %f - y: %f - z: %f\n", v.x, v.y, v.z);
}

//Error callback  
static void		error_callback(int error, const char* description)
{
	fputs(description, stderr);
	_fgetchar();
}




void transition_scene(int destiniation_scene, glm::vec3 current_position, glm::vec3 current_direction, glm::vec3 current_up, glm::vec3 current_fov)
{
	cameraSequences[scene].reset();
	next_scene = destiniation_scene;
	next_scene %= SCENE_4;
	if (next_scene == SCENE_0)
		next_scene = SCENE_1;
	scene = SCENE_0;
	cameraSequences[SCENE_0] = CameraSequencer(
		current_position, cameraSequences[destiniation_scene].sequence.front().getStart(), 
		current_direction, cameraSequences[destiniation_scene].directions.front().getStart(),
		current_up, cameraSequences[destiniation_scene].ups.front().getStart(),
		current_fov, cameraSequences[destiniation_scene].others.front().getStart(),
		0.001, 0.0);
}

static void		key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ENTER:
			break;
		case GLFW_KEY_UP:
			//eye_position += eye_position * 0.1f;
			step += step_d;
			break;
		case GLFW_KEY_DOWN:
			//eye_position -= eye_position * 0.1f;
			step -= step_d;
			step = max(step, 0);
			break;
		case GLFW_KEY_RIGHT:
			eye_position = glm::quat(glm::vec3(0, glm::radians(3.0f), 0)) * eye_position;
			break;
		case GLFW_KEY_LEFT:
			eye_position = glm::quat(glm::vec3(0, -glm::radians(3.0f), 0)) * eye_position;
			break;
		case GLFW_KEY_SPACE:
			transition_scene(scene + 1, eye_position, eye_direction, up, fov);
			break;
		case GLFW_KEY_M:
			MOTIONBLUR_ON = !MOTIONBLUR_ON;
			break;
		case GLFW_KEY_G:
			GLOW_ON = !GLOW_ON;
			break;
		case GLFW_KEY_E:
			EFFECTS = !EFFECTS;
			break;

		case GLFW_KEY_ESCAPE:
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		}
	}
}

void update_camera(int scene, float step)
{
	cameraSequences[scene].lerpStepSmooth(step);
	eye_position = cameraSequences[scene].currentPosition;
	eye_direction = cameraSequences[scene].currentDirection;
	up = cameraSequences[scene].currentUp;
}

void loop()
{

	//// DRAW OBJECTS
	//terrain.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	//water.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	//falcon.draw(1, &model_mat_handle, &texture_handle, nullptr, nullptr);
	//printf("%i\n", stars.tex);
	stars.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	star.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);

	earth.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	moon.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	mars.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	venus.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	mercury.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	jupiter.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	saturn.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	uranus.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	neptune.draw(0, &model_mat_handle, &texture_handle, nullptr, nullptr);
	
	earth.theta += 0.001f;
	mars.theta += 0.001f;
	star.theta += 0.001f;
	neptune.theta += 0.001f;


	//orbits
	falcon.pos = glm::quat(glm::vec3(0, 0.0001f, 0)) * (falcon.pos-earth.pos) + earth.pos;
	moon.pos = glm::quat(glm::vec3(0, -0.0001f, 0)) * (moon.pos-earth.pos) + earth.pos;
	
	switch (scene)
	{
	case SCENE_0:
		update_camera(SCENE_0, step);
		if (cameraSequences[SCENE_0].isFinished())
			scene = next_scene;
		break;
	case SCENE_1:
		//eye_position = falcon.pos + glm::vec3(-0.1, 0.01, 0);
		//eye_direction = falcon.pos;
		update_camera(SCENE_1, step);
		break;
	case SCENE_2:
		update_camera(SCENE_2, step);
		break;
	case SCENE_3:
		update_camera(SCENE_3, step);
		break;
	case SCENE_4:
		update_camera(SCENE_4, step);
		break;
	default:
		break;
	}	
}




void init_camera_tour()
{
	// transitions
	cameraSequences.push_back(CameraSequencer());
	//scene1
	cameraSequences.push_back(CameraSequencer(
		neptune.pos + glm::vec3(200, 0, 0), neptune.pos + glm::vec3(300, 0, 200),
		neptune.pos, mars.pos,
		glm::vec3(0,1,1), glm::vec3(0,1,1),
		glm::vec3(45.0f,0,0), glm::vec3(45.0f, 0, 0),
		step,0.0
	));

	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(300, 0, 0), mars.pos, glm::vec3(0, 1, 0), 0.001, 0);
	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(1000, 0, 500), star.pos, glm::vec3(0, 1, 0), 0.001, 0);
	cameraSequences[SCENE_1].addLerper(star.pos + glm::vec3(2000, 0, -1000), star.pos, glm::vec3(0, 1, 0), 0.001, 0);

	cameraSequences[SCENE_1].addLerper(star.pos + glm::vec3(-1000, 0, -1500), mars.pos, glm::vec3(0, 1, 0), 0.001, 0);
	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(-300, 0, 0), mars.pos, glm::vec3(0, 1, 0), 0.001, 0);

	//scene2
	cameraSequences.push_back(CameraSequencer(
		terrain_pos + glm::vec3(0,0,0.05), terrain_pos + glm::vec3(0, 0, 0.05), 
		terrain_pos + glm::vec3(1, 0, 0.04), terrain_pos + glm::vec3(1, 0, 0.04),
		glm::vec3(0, 0, 1), glm::vec3(0, 0, 1),
		glm::vec3(45.0f, 0, 0), glm::vec3(45.0f, 0, 0),
		0.005, 0.1
	));

	//scene3
	cameraSequences.push_back(CameraSequencer(
		star.pos + glm::vec3(-50000,0,0), star.pos + glm::vec3(-10000, 0, 0),
		star.pos, star.pos,
		glm::vec3(0, 1, 0), glm::vec3(0, 1, 0),
		glm::vec3(45.0f, 0, 0), glm::vec3(45.0f, 0, 0),
		0.005, 0.1
	));

	//scene 4
	cameraSequences.push_back(CameraSequencer(
		earth.pos + glm::vec3(5,0,0), earth.pos + glm::vec3(5,0,0),
		earth.pos, earth.pos,
		glm::vec3(0, 1, 0), glm::vec3(0, 1, 0),
		glm::vec3(45.0f, 0, 0), glm::vec3(45.0f, 0, 0),
		0.005, 0.1
	));

}

void init_objects()
{
	//// CREATE OBJECTS
	printf("\n");
	printf("Initialising objects...\n");
	// create sphere data for screen A, B and D
	std::vector<glm::vec3>
		v,
		c;

	int res_t = 100;

	//v = generate_square_mesh(res_t, res_t);
	//generate_terrain(&v, res_t, res_t, 10.0f, 0.01f);
	//generate_terrain(&v, res_t, res_t, 50.0f, 0.01f);
	//c = generate_terrian_colour(v, 1.0f);

	//terrain = Obj(
	//	"", "", "",
	//	//"textures/moss_color.jpg",
	//	//"textures/moss_norm.jpg",
	//	//"textures/moss_height.jpg",
	//	pack_object(&v, &c, NULL, NULL, NULL),
	//	terrain_pos,
	//	glm::vec3(1, 0, 0),
	//	glm::radians(90.0f),
	//	glm::vec3(1, 1, 1)
	//);
	//terrain.scale *= 5;

	//v = generate_square_mesh(res_t, res_t);
	//generate_terrain(&v, res_t, res_t, 1000.0f, 0.001f);
	//c = generate_water_colour(v);

	//water = Obj(
	//	"", "", "",
	//	//"textures/moss_color.jpg",
	//	//"textures/moss_norm.jpg",
	//	//"textures/moss_height.jpg",
	//	pack_object(&v, &c, NULL, NULL, NULL),
	//	terrain_pos,
	//	glm::vec3(1, 0, 0),
	//	glm::radians(90.0f),
	//	glm::vec3(1, 1, 1)
	//);
	//water.scale *= 5;


	//falcon = Obj(
	//	"objects/millenium-falcon.obj",
	//	RED,
	//	glm::vec3(0, 0, 1005),
	//	glm::vec3(1, 0, 0),
	//	glm::radians(0.0f),
	//	glm::vec3(1, 1, 1),
	//	glm::vec3(25, 25, 3000)
	//);
	//falcon.scale /= 10000;

	int res = 50;

	v = generate_sphere_invert(res, res);

	stars = Obj(
		"textures/stars.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		glm::vec3(10000, 0, 0),
		glm::vec3(1, 0, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1)
	);
	stars.scale *= 5000000;

	v = generate_sphere(res, res);

	star = Obj(
		"textures/sun.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		glm::vec3(0, 0, 0),
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * 1000.0f
	);

	earth = Obj(
		"textures/earth.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		glm::vec3(130000, 0, 0),
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * 100.0f
	);

	moon = Obj(
		"textures/moon.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos + glm::vec3(0, 0, 25000) * 0.003f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	mars = Obj(
		"textures/mars.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 1.52f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	venus = Obj(
		"textures/venus.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 0.72f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	mercury = Obj(
		"textures/mercury.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 0.39f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	jupiter = Obj(
		"textures/jupiter.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 5.2f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	saturn = Obj(
		"textures/saturn.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 9.58f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	uranus = Obj(
		"textures/uranus.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 19.2f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	neptune = Obj(
		"textures/neptune.jpg", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos * 30.0f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);
}

void			init()
{


	//// CREATE GLSL PROGAMS
	printf("\n");
	printf("Initialising GLSL programs...\n");
	programs.add_program("shaders/basic.vert", "shaders/basic.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/basic_texture.frag");
	programs.add_program("shaders/render_to_texture.vert", "shaders/render_to_texture.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/basic_texture_glow.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/blueshift.frag");


	//programs.add_program("shaders/phong.vert", "shaders/phong.frag");


	//// CREATE HANDLES
	printf("\n");
	printf("Initialising variable handles...\n");

	programs.load_program(1);
	model_mat_handle = VarHandle("u_m", &model);
	model_mat_handle.init(programs.current_program);
	view_mat_handle = VarHandle("u_v", &view);
	view_mat_handle.init(programs.current_program);
	proj_mat_handle = VarHandle("u_p", &projection);
	proj_mat_handle.init(programs.current_program);
	ambient_color_handle = VarHandle("u_ambient_color", &ambient_color);
	ambient_color_handle.init(programs.current_program);
	texture_handle = VarHandle("u_tex");
	texture_handle.init(programs.current_program);

	programs.load_program(2);
	motion_model_mat_handle = VarHandle("u_m", &model);
	motion_model_mat_handle.init(programs.current_program);
	motion_view_mat_handle = VarHandle("u_v", &view);
	motion_view_mat_handle.init(programs.current_program);
	motion_proj_mat_handle = VarHandle("u_p", &projection);
	motion_proj_mat_handle.init(programs.current_program);
	motion_texture_handle = VarHandle("u_tex");
	motion_texture_handle.init(programs.current_program);
	motion_handles[0] = VarHandle("u_tex0");
	motion_handles[0].init(programs.current_program);
	motion_handles[1] = VarHandle("u_tex1");
	motion_handles[1].init(programs.current_program);
	motion_handles[2] = VarHandle("u_tex2");
	motion_handles[2].init(programs.current_program);
	motion_handles[3] = VarHandle("u_tex3");
	motion_handles[3].init(programs.current_program);
	motion_handles[4] = VarHandle("u_tex4");
	motion_handles[4].init(programs.current_program);
	motion_handles[5] = VarHandle("u_tex5");
	motion_handles[5].init(programs.current_program);
	motion_handles[6] = VarHandle("u_tex6");
	motion_handles[6].init(programs.current_program);
	motion_handles[7] = VarHandle("u_tex7");
	motion_handles[7].init(programs.current_program);
	motion_handles[8] = VarHandle("u_tex8");
	motion_handles[8].init(programs.current_program);
	motion_handles[9] = VarHandle("u_tex9");
	motion_handles[9].init(programs.current_program);
	motion_handles[10] = VarHandle("u_tex10");
	motion_handles[10].init(programs.current_program);
	motion_handles[11] = VarHandle("u_tex11");
	motion_handles[11].init(programs.current_program);
	motion_handles[12] = VarHandle("u_tex12");
	motion_handles[12].init(programs.current_program);
	motion_handles[13] = VarHandle("u_tex13");
	motion_handles[13].init(programs.current_program);
	motion_handles[14] = VarHandle("u_tex14");
	motion_handles[14].init(programs.current_program);
	motion_handles[15] = VarHandle("u_tex15");
	motion_handles[15].init(programs.current_program);
	motion_handles[16] = VarHandle("u_tex16");
	motion_handles[16].init(programs.current_program);
	motion_handles[17] = VarHandle("u_tex17");
	motion_handles[17].init(programs.current_program);
	motion_handles[18] = VarHandle("u_tex18");
	motion_handles[18].init(programs.current_program);
	motion_handles[19] = VarHandle("u_tex19");
	motion_handles[19].init(programs.current_program);
	motion_length_handle = VarHandle("u_motion_length", &l);
	motion_length_handle.init(programs.current_program);
	current_frame_handle = VarHandle("u_current_frame", &current_frame);
	current_frame_handle.init(programs.current_program);

	programs.load_program(3);
	glow_model_mat_handle = VarHandle("u_m", &model);
	glow_model_mat_handle.init(programs.current_program);
	glow_view_mat_handle = VarHandle("u_v", &view);
	glow_view_mat_handle.init(programs.current_program);
	glow_proj_mat_handle = VarHandle("u_p", &projection);
	glow_proj_mat_handle.init(programs.current_program);
	glow_texture_handle = VarHandle("u_tex");
	glow_texture_handle.init(programs.current_program);
	glow_res_handle = VarHandle("u_glow_res");
	glow_res_handle.init(programs.current_program);

	programs.load_program(4);
	shift_model_mat_handle = VarHandle("u_m", &model);
	shift_model_mat_handle.init(programs.current_program);
	shift_view_mat_handle = VarHandle("u_v", &view);
	shift_view_mat_handle.init(programs.current_program);
	shift_proj_mat_handle = VarHandle("u_p", &projection);
	shift_proj_mat_handle.init(programs.current_program);
	shift_texture_handle = VarHandle("u_tex");
	shift_texture_handle.init(programs.current_program);
	shift_handle = VarHandle("u_shift");
	shift_handle.init(programs.current_program);
	
	basic_fbo = FBO(window_size.x, window_size.y);
	shift_fbo = FBO(window_size.x, window_size.y);
	glow_fbo = FBO(window_size.x, window_size.y);
	motion_blur_fbo = FBO(window_size.x, window_size.y);
	combined_fbo = FBO(window_size.x, window_size.y);
	for (int i = 0; i < MOTION_L; ++i)
		motion_blur_parts[i] = FBO(window_size.x, window_size.y);

	std::vector<glm::vec3>
		v,
		c;

	v = generate_square_mesh(1, 1);

	screen_texture = Obj(
		"", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_RECTS, BLACK),
		glm::vec3(),
		glm::vec3(0, 0, 1),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1)
	);
	screen_texture.scale *= 1;

	init_objects();

	init_camera_tour();
}




glm::mat4 getOrtho()
{
	return glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 1000000.0f);
}
glm::mat4 getOrthoView()
{
	return glm::lookAt(glm::vec3(-0.866, 0, 0), glm::vec3(), glm::vec3(0, 1, 0));
}
glm::mat4 getPerspective()
{
	lorentz = pow(glm::length(eye_position - old_eye_position),1) / (star.scale.x/2)  / (step/step_d) + 1;
	//lorentz = glm::clamp(lorentz, 1.0f, 4.0f);
	glm::mat4 p = glm::perspective(glm::radians(min(lorentz * 45.0f, 160.0f)), (float)window_size.x / (float)window_size.y, 0.1f, 10000000.0f);
	old_eye_position = eye_position;
	return p;
}
glm::mat4 getperspectiveView()
{
	return glm::lookAt(eye_position, eye_direction, up);
}

//GL graphics loop
void			glLoop(void(*graphics_loop)(), GLFWwindow * window)
{
	printf("\n");
	printf("Running GL loop...\n");

	//Main Loop  
	do
	{
		// start clock for this tick
		auto start = std::chrono::high_resolution_clock::now();

		//// draw normal scene
		// load basic shader
		programs.load_program(1);
		// bind basic fbo to render to
		basic_fbo.bind();
		// clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.);
		// update view and proj matrices
		view = getperspectiveView();
		projection = getPerspective();
		//// LOAD GLOBAL HANDLES
		view_mat_handle.load();
		proj_mat_handle.load();
		ambient_color_handle.load();
		//// DRAW
		graphics_loop();
		//unbind the fbo
		basic_fbo.unbind();

		//// render normal scene to FBO
		// load glow shader
		programs.load_program(4);
		// bind motionblur part fbo to render
		shift_fbo.bind();
		// clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.);
		// update view and proj matrices
		view = getOrthoView();
		projection = getOrtho();
		//// LOAD GLOBAL HANDLES
		shift_view_mat_handle.load();
		shift_proj_mat_handle.load();
		// set glow parameters
		float lor = pow(lorentz, 5);
		shift_handle.load(glm::vec3(max(lor / 8, 1),max(lor / 8, 1), max(lor, 1)));
		//// DRAW		
		// setting obj texture to be from the basic fbo
		screen_texture.setTex(basic_fbo.tex);
		// drawing texture onto a square mesh
		screen_texture.draw(0, &shift_model_mat_handle, &shift_texture_handle, nullptr, nullptr);
		// unbind motionblur part fbo
		shift_fbo.unbind();

		if (EFFECTS)
		{
			//// render normal scene to FBO
			// load glow shader
			programs.load_program(3);
			// bind motionblur part fbo to render
			motion_blur_parts[current_frame].bind();
			// clear screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.);
			// update view and proj matrices
			view = getOrthoView();
			projection = getOrtho();
			//// LOAD GLOBAL HANDLES
			glow_view_mat_handle.load();
			glow_proj_mat_handle.load();
			// set glow parameters
			glow_res_handle.load(glm::vec3(0.5 / window_size.x * lorentz, 0.5 / window_size.y * lorentz, 10));
			//// DRAW		
			// setting obj texture to be from the basic fbo
			screen_texture.setTex(shift_fbo.tex);
			// drawing texture onto a square mesh
			screen_texture.draw(0, &glow_model_mat_handle, &glow_texture_handle, nullptr, nullptr);
			// unbind motionblur part fbo
			motion_blur_parts[current_frame].unbind();					
			// update current frame for the motionblur
			current_frame++;
			current_frame %= MOTION_L;// INT32_MAX;		


			//// draw FBO from normal scene
			// load motionblur accumulator shader
			programs.load_program(2);
			// clear screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.);
			// update view and proj matrices
			view = getOrthoView();
			projection = getOrtho();
			//// LOAD GLOBAL HANDLES
			motion_view_mat_handle.load();
			motion_proj_mat_handle.load();
			// setting parameters
			current_frame_handle.load();
			motion_length_handle.load();
			//// DRAW
			// bind each texture seperately
			for (int i = 0; i < MOTION_L; ++i)
			{
				motion_handles[i].load(motion_blur_parts[i].tex);
				glActiveTexture(GL_TEXTURE0 + motion_blur_parts[i].tex);
				glBindTexture(GL_TEXTURE_2D, motion_blur_parts[i].tex);
			}
			// draw texture to square mesh
			screen_texture.draw(0, &motion_model_mat_handle, &motion_texture_handle, nullptr, nullptr);
			// unbind each texture seperately
			for (int i = 0; i < MOTION_L; ++i)
			{
				glActiveTexture(GL_TEXTURE0 + motion_blur_parts[i].tex);
				glBindTexture(GL_TEXTURE_2D, GL_TEXTURE0);
			}
		}		
		else
		{ 
			basic_fbo.unbind();
			//// draw normal scene
			// load basic shader
			programs.load_program(1);
			// bind basic fbo to render to
			
			// clear screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.);
			// update view and proj matrices
			view = getperspectiveView();
			projection = getPerspective();
			//// LOAD GLOBAL HANDLES
			view_mat_handle.load();
			proj_mat_handle.load();
			ambient_color_handle.load();
			//// DRAW
			graphics_loop();
			//unbind the fbo
		}

		//Swap buffers  
		glfwSwapBuffers(window);
		//Get and organize events, like keyboard and mouse input, window resizing, etc...  
		glfwPollEvents();

		// stop clock
		auto finish = std::chrono::high_resolution_clock::now();
		int ms = float(std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count());
		long newWait = 5 - ms;// -(gm.gameSpeed);
		newWait = newWait < 0 ? 0 : newWait;
		// throttle the graphics loop to cap at a certain fps
		std::this_thread::sleep_for(std::chrono::milliseconds(newWait));

	} //Check if the ESC or Q key had been pressed or if the window had been closed  
	while (!glfwWindowShouldClose(window));

	printf("\n");
	printf("Window has closed. Application will now exit.\n");

	//Close OpenGL window and terminate GLFW  
	glfwDestroyWindow(window);
	//Finalize and clean up GLFW  
	glfwTerminate();

	exit(EXIT_SUCCESS);
}
//GL window initialise
GLFWwindow *				initWindow()
{
	GLFWwindow * window;

	//Set the error callback  
	glfwSetErrorCallback(error_callback);

	printf("Initialising GLFW...\n");
	//Initialize GLFW  
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	printf("Creating window...\n");
	//Create a window and create its OpenGL context  
	window = glfwCreateWindow(window_size.x, window_size.y, "Test Window", NULL, NULL);
	//If the window couldn't be created  
	if (!window)
	{
		fprintf(stderr, "Failed to open GLFW window.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	printf("Setting window context...\n");
	//This function makes the context of the specified window current on the calling thread.   
	glfwMakeContextCurrent(window);

	//Sets the key callback  
	glfwSetKeyCallback(window, key_callback);

	printf("Initialising GLEW...\n");
	//Initialize GLEW  
	GLenum err = glewInit();
	//If GLEW hasn't initialized  
	if (err != GLEW_OK)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);
	// enable texturineg
	glEnable(GL_TEXTURE_2D);
	// init
	init();


	
	return window;
}

void run_cwk()
{
	glLoop(loop, initWindow());	
}