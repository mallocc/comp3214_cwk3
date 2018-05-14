#include "cwk3.h"
#include "colors.h"
#include "lerp.h"
#include <chrono>
#include <thread>
#include <minmax.h>
#include <sstream>
#include "fbo.h"

#define COE 1
#define GRAVITY 0
#define MOTION_L 10

GLSLProgramManager	programs;

VarHandle
model_mat_handle,
view_mat_handle,
proj_mat_handle,
ambient_color_handle,
texture_handle,
normal_handle,
light_pos_handle,
eye_pos_handle,
light_properties_handle,
light_color_handle,

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

combine_model_mat_handle,
combine_view_mat_handle,
combine_proj_mat_handle,
combine_texture1_handle,
combine_texture2_handle,

shift_model_mat_handle,
shift_view_mat_handle,
shift_proj_mat_handle,
shift_texture_handle,
shift_handle,

basic_model_mat_handle,
basic_view_mat_handle,
basic_proj_mat_handle,
basic_texture_handle
	;

glm::vec2
	window_size(1280, 720);

glm::vec3
	eye_position(10000, 0, 0),
	eye_velocity,
	old_eye_position = eye_position,
	eye_direction,
	up(0, 1, 0),
	fov(45, 0, 0),
	ambient_color(0.1f,0.1f,0.1f),
	terrain_pos(-100.2f,0,0),
	asteroid_field_pos(0,0,4000000);

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

Light light;

struct Camera
{
	glm::vec3 pos, vel, dir, dir_vel, up, up_vel;

	Camera() {}

	Camera(glm::vec3 pos, glm::vec3 vel, glm::vec3 dir, glm::vec3 up)
	{
		this->pos = pos;
		this->vel = vel;
		this->dir = glm::normalize(dir);
		this->up = up;
	}

	void update(float dt, float friction)
	{
		vel *= friction;
		pos += vel * dt;
		dir_vel *= friction;
		dir += dir_vel;
		up_vel *= friction;
		up += up_vel;
	}

	void apply_force(glm::vec3 force)
	{
		vel += force;
	}
	void apply_force_foward()
	{
		vel += dir;
	}
	void apply_force_foward(float mag)
	{
		vel += dir * mag;
	}
	void apply_force_backward()
	{
		vel -= dir;
	}
	void apply_force_right()
	{
		vel += glm::cross(dir, up);
	}
	void apply_force_left()
	{
		vel += -glm::cross(dir, up);
	}

	void yaw_left(float amount)
	{
		dir = glm::quat(glm::vec3(0, glm::radians(amount), 0)) * dir;
	}
	void yaw_right(float amount) 
	{
		dir = glm::quat(glm::vec3(0, -glm::radians(amount), 0)) * dir;
	}
	void pitch_up(float amount);
	void pitch_down(float amount);
	void roll_left(float amount);
	void roll_right(float amount);

	void brake(float friction)
	{
		vel *= friction;
	}

	glm::vec3 get_position()
	{
		return pos;
	}

	glm::vec3 get_look_position()
	{
		return pos + dir;
	}

	glm::vec3 get_up()
	{
		return up;
	}
} camera = Camera(glm::vec3(10000,0,0), glm::vec3(), glm::vec3(1,0,0), glm::vec3(0, 1, 0));

Obj
terrain, water, falcon,
earth, mars, venus, mercury, neptune, uranus, saturn, jupiter,
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

bool override_eye_direction = false;

float pitch_d = 0.01f;
float yaw_d = 10.0f;

bool TOUR_ON = false;
bool TOUR_PAUSE = false;

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


void reset_scene(int scene)
{
	cameraSequences[scene].reset();
}

void transition_scene(int destiniation_scene, glm::vec3 current_position, glm::vec3 current_direction, glm::vec3 current_up, glm::vec3 current_fov)
{
	//cameraSequences[scene].reset();
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
		1, 0.0);
}

static void		key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == 2)
	{
		switch (key)
		{
			if (!TOUR_ON)
			{
		case GLFW_KEY_ENTER:
			//eye_velocity += glm::normalize(eye_direction - eye_position);
			camera.apply_force_foward(20.0f);
			break;
		case GLFW_KEY_UP:
			//eye_velocity += glm::normalize(eye_direction - eye_position);
			camera.apply_force_foward();
			break;
		case GLFW_KEY_DOWN:
			//eye_velocity -= glm::normalize(eye_direction - eye_position);
			camera.apply_force_backward();
			break;
		case GLFW_KEY_RIGHT:
			//eye_direction = glm::quat(glm::vec3(0, -glm::radians(yaw_d), 0)) * glm::normalize(eye_direction - eye_position) + eye_position;
			camera.yaw_right(yaw_d);
			break;
		case GLFW_KEY_LEFT:
			//eye_direction = glm::quat(glm::vec3(0, glm::radians(yaw_d), 0)) * glm::normalize(eye_direction - eye_position) + eye_position;
			camera.yaw_left(yaw_d);
			break;
		case GLFW_KEY_LEFT_SHIFT:
			//eye_direction = glm::quat(glm::vec3(0, glm::radians(yaw_d), 0)) * glm::normalize(eye_direction - eye_position) + eye_position;
			camera.brake(0.9);
			break;
			//case GLFW_KEY_SPACE:
			//	transition_scene(scene + 1, eye_position, eye_direction, up, fov);
			//	break;
		case GLFW_KEY_P:
			camera.pos = mars.pos + glm::vec3(100,100,100);
			camera.dir = glm::normalize(mars.pos +glm::vec3(-300.0f,0,0) - camera.pos);
			camera.up = glm::vec3(0, 1, 0);
			camera.vel = glm::vec3(); 

			break;
		case GLFW_KEY_V:
			EFFECTS = !EFFECTS;
			break;

			}
		case GLFW_KEY_T:
			if (!TOUR_ON)
			{
				TOUR_ON = true;
				TOUR_PAUSE = false;
				transition_scene(SCENE_1, eye_position, eye_direction, up, fov);
				//cameraSequences[SCENE_1].currentLerper = 25;
			}
			else
			{
				TOUR_PAUSE = !TOUR_PAUSE;
			}

			break;
		//case GLFW_KEY_M:
		//	MOTIONBLUR_ON = !MOTIONBLUR_ON;
		//	break;
		//case GLFW_KEY_G:
		//	GLOW_ON = !GLOW_ON;
		//	break;
		case GLFW_KEY_E:
			TOUR_ON = false;
			camera.pos = eye_position;
			camera.dir = glm::normalize(eye_direction - eye_position);
			camera.up = glm::vec3(0, 1, 0);
			camera.vel = glm::vec3();
			break;
		case GLFW_KEY_R:
			reset_scene(SCENE_1);
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
	if (!TOUR_PAUSE)
	{
		cameraSequences[scene].lerpStepSmooth();
		eye_position = cameraSequences[scene].currentPosition;
		eye_direction = cameraSequences[scene].currentDirection;
		up = cameraSequences[scene].currentUp;
	}
}



void physics()
{

	//earth.theta += 0.001f;
	mars.theta += 0.001f;
	star.theta += 0.001f;
	neptune.theta += 0.001f;



	//orbits
	//falcon.pos = glm::quat(glm::vec3(0, 0.0001f, 0)) * (falcon.pos - earth.pos) + earth.pos;
	moon.pos = glm::quat(glm::vec3(0, -0.005f, 0)) * (moon.pos - earth.pos) + earth.pos;

	if (TOUR_ON)
	{

		update_camera(SCENE_1, step);
		//switch (scene)
		//{
		//case SCENE_0:
		//	update_camera(SCENE_0, step);
		//	if (cameraSequences[SCENE_0].isFinished())
		//		scene = next_scene;
		//	break;
		//case SCENE_1:
		//	//eye_position = falcon.pos + glm::vec3(-0.1, 0.01, 0);
		//	//eye_direction = falcon.pos;
		//	update_camera(SCENE_1, step);
		//	break;
		//case SCENE_2:
		//	update_camera(SCENE_2, step);
		//	break;
		//case SCENE_3:
		//	update_camera(SCENE_3, step);
		//	break;
		//case SCENE_4:
		//	update_camera(SCENE_4, step);
		//	break;
		//default:
		//	break;
		//}
	}
	else
	{
		camera.update(1.0f, 0.999f);
		eye_position = camera.get_position();
		eye_direction = camera.get_look_position();
		up = camera.get_up();
	}
}



void glow_draw()
{
	stars.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);	
	earth.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	moon.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	mars.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	venus.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	mercury.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	jupiter.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	saturn.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	uranus.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	neptune.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	terrain.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	falcon.draw(1, &model_mat_handle, &texture_handle, &normal_handle, nullptr);


	programs.load_program(6);
	basic_view_mat_handle.load();
	basic_proj_mat_handle.load();
	star.draw(0, &basic_model_mat_handle, &basic_texture_handle, nullptr, nullptr);
}

void loop()
{

	earth.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	moon.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	mars.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	venus.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	mercury.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	jupiter.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	saturn.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	uranus.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	neptune.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	terrain.draw(0, &model_mat_handle, &texture_handle, &normal_handle, nullptr);
	falcon.draw(1, &model_mat_handle, &texture_handle, &normal_handle, nullptr);

	programs.load_program(6);
	basic_view_mat_handle.load();
	basic_proj_mat_handle.load();
	star.draw(0, &basic_model_mat_handle, &basic_texture_handle, nullptr, nullptr);
}


void init_camera_tour()
{
	// transitions
	cameraSequences.push_back(CameraSequencer());
	//scene1
	cameraSequences.push_back(CameraSequencer(
		asteroid_field_pos, asteroid_field_pos + glm::vec3(0,0,-100),
		asteroid_field_pos + glm::vec3(0, 0, 1000), asteroid_field_pos + glm::vec3(-100, 0, 0),
		glm::vec3(0,1,1), glm::vec3(1,-1,1),
		glm::vec3(45.0f,0,0), glm::vec3(45.0f, 0, 0),
		0.002,0.0
	));
	cameraSequences[SCENE_1].addLerper(asteroid_field_pos + glm::vec3(0, 0, 0), asteroid_field_pos + glm::vec3(0, 0, -100), glm::vec3(-1, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(asteroid_field_pos + glm::vec3(0, 200, 0), star.pos, glm::vec3(0, 1, 0), 0.003, 0);

	cameraSequences[SCENE_1].addLerper(star.pos + glm::vec3(0, 0, -3000), star.pos, glm::vec3(0, 1, 0), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(star.pos + glm::vec3(-1000, 0, -1200), mercury.pos, glm::vec3(0, 1, 0), 0.001, 0);

	cameraSequences[SCENE_1].addLerper(mercury.pos + glm::vec3(-300, 0, 0), mercury.pos, glm::vec3(0, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(mercury.pos + glm::vec3(-500, 200, 0), venus.pos, glm::vec3(0, 1, 0), 0.005, 0);

	cameraSequences[SCENE_1].addLerper(venus.pos + glm::vec3(-300, 0, 0), venus.pos, glm::vec3(0, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(venus.pos + glm::vec3(-500, 200, 0), earth.pos, glm::vec3(0, 1, 0), 0.005, 0);

	cameraSequences[SCENE_1].addLerper(earth.pos + glm::vec3(-300, 0, 0), earth.pos, glm::vec3(0, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(earth.pos + glm::vec3(-500, 200, 0), mars.pos, glm::vec3(0, 1, 0), 0.005, 0);

	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(-300, 0, 0), mars.pos, glm::vec3(0, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(0, 0, 200), mars.pos, glm::vec3(0, 1, 0), 0.003, 0);
	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(100, 200, 100), mars.pos, glm::vec3(0, 1, 0), 0.003, 0);
	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(0, 0, -200), mars.pos, glm::vec3(0, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(mars.pos + glm::vec3(0, 0, -200), mars.pos + glm::vec3(0, 0, 200), glm::vec3(0, 1, 0), 0.01, 0);

	cameraSequences[SCENE_1].addLerper(jupiter.pos + glm::vec3(-300, 0, 0), jupiter.pos, glm::vec3(0, 1, 0), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(jupiter.pos + glm::vec3(-500, 200, 0), saturn.pos, glm::vec3(0, 1, 0), 0.003, 0);

	cameraSequences[SCENE_1].addLerper(saturn.pos + glm::vec3(-300, 0, 0), saturn.pos, glm::vec3(0, 1, 0), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(saturn.pos + glm::vec3(-500, 200, 0), uranus.pos, glm::vec3(0, 1, 0), 0.003, 0);

	cameraSequences[SCENE_1].addLerper(uranus.pos + glm::vec3(-300, 0, 0), uranus.pos, glm::vec3(0, 1, 0), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(uranus.pos + glm::vec3(-500, 200, 0), neptune.pos, glm::vec3(0, 1, 0), 0.004, 0);

	cameraSequences[SCENE_1].addLerper(neptune.pos + glm::vec3(-300, 0, 0), neptune.pos, glm::vec3(0, 1, 0), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(neptune.pos + glm::vec3(300, 0, 200), neptune.pos + glm::vec3(-300, 0, 200), glm::vec3(0, 1, 0), 0.005, 0);
	cameraSequences[SCENE_1].addLerper(neptune.pos + glm::vec3(300, 0, 200), earth.pos, glm::vec3(0, 1, 0), 0.005, 0);

	cameraSequences[SCENE_1].addLerper(earth.pos + glm::vec3(100, 200, 200), earth.pos + glm::vec3(-150, 0, 0), glm::vec3(0, 1, 0), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(earth.pos + glm::vec3(-100, 0, 100), earth.pos+terrain_pos, glm::vec3(0, 0, 1), 0.002, 0);
	cameraSequences[SCENE_1].addLerper(earth.pos + terrain_pos + glm::vec3(-1,0,0), earth.pos + terrain_pos + glm::vec3(0,0,-5), glm::vec3(-1, 0, 0), 0.002, 0);

	//scene2
	cameraSequences.push_back(CameraSequencer(
		mars.pos + glm::vec3(100,200,300), mars.pos + glm::vec3(100, 200, 300),
		mars.pos + glm::vec3(-200,0,0), star.pos + glm::vec3(-200, 0, 0),
		glm::vec3(0, 1, 1), glm::vec3(0, 1, 1),
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
		c,
		n,
		t;



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


	falcon = Obj(//1,
		"objects/LOD0.obj",
		//"textures/199.bmp", "textures/199_norm.bmp", "",
		RED,
		eye_position + glm::vec3(100, 0, 0),
		glm::vec3(1, 0, 0),
		glm::radians(0.0f),
		glm::vec3(1, 1, 1) * 0.1f// glm::vec3(25, 25, 3000)
	);



	v = generate_sphere_invert(20, 20);

	stars = Obj(
		"textures/stars.jpg", "textures/blank.bmp", "",
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

	int res = 200;

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
		"textures/earth.jpg", "textures/blank.bmp", "",
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
		"textures/moon.jpg", "textures/blank.bmp", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS, BLACK),
		earth.pos + glm::vec3(0, 0, 25000) * 0.3f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x * 0.7f
	);

	mars = Obj(
		"textures/5672_mars_4k_color.jpg", "textures/5672_mars_4k_normal.jpg", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_UVS_SPHERE | GEN_NORMS | GEN_TANGS, BLACK),
		earth.pos * 1.52f,
		glm::vec3(0, 1, 0),
		glm::radians(0.0f),
		glm::vec3(1, 0, 0),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1) * earth.scale.x
	);

	venus = Obj(
		"textures/venus.jpg", "textures/blank.bmp", "",
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
		"textures/mercury.jpg", "textures/blank.bmp", "",
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
		"textures/jupiter.jpg", "textures/blank.bmp", "",
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
		"textures/saturn.jpg", "textures/blank.bmp", "",
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
		"textures/uranus.jpg", "textures/blank.bmp", "",
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
		"textures/neptune.jpg", "textures/blank.bmp", "",
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

	int res_t = 100;

	v = generate_square_mesh(res_t, res_t);
	generate_terrain(&v, res_t, res_t, 10.0f, 0.01f);
	generate_terrain(&v, res_t, res_t, 50.0f, 0.01f);
	c = generate_terrian_colour(v, 1.0f);

	terrain = Obj(
		"", "", "",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, &c, NULL, NULL, NULL),
		earth.pos + terrain_pos,
		glm::vec3(0, 0, 1),
		glm::radians(90.0f),
		glm::vec3(1, 1, 1)
	);
	terrain.scale *= 10;
}

void			init()
{
	light = Light{glm::vec3(), WHITE, glm::vec3(1,1,100)};
	
	//// CREATE GLSL PROGAMS
	printf("\n");
	printf("Initialising GLSL programs...\n");
	programs.add_program("shaders/basic.vert", "shaders/basic.frag");
	programs.add_program("shaders/phong_texture_normals.vert", "shaders/phong_texture_normals.frag");
	programs.add_program("shaders/render_to_texture.vert", "shaders/render_to_texture2.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/basic_texture_glow.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/blueshift.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/combine.frag");
	programs.add_program("shaders/basic_texture.vert", "shaders/basic_texture.frag");

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
	normal_handle = VarHandle("u_norm");
	normal_handle.init(programs.current_program);
	light_pos_handle = VarHandle("u_light_pos", &light.pos);
	light_pos_handle.init(programs.current_program);
	eye_pos_handle = VarHandle("u_eye_pos", &eye_position);
	eye_pos_handle.init(programs.current_program);
	light_properties_handle = VarHandle("u_light_properties", &light.brightness_specscale_shinniness);
	light_properties_handle.init(programs.current_program);
	light_color_handle = VarHandle("u_light_color", &light.color);
	light_color_handle.init(programs.current_program);

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
	//motion_handles[10] = VarHandle("u_tex10");
	//motion_handles[10].init(programs.current_program);
	//motion_handles[11] = VarHandle("u_tex11");
	//motion_handles[11].init(programs.current_program);
	//motion_handles[12] = VarHandle("u_tex12");
	//motion_handles[12].init(programs.current_program);
	//motion_handles[13] = VarHandle("u_tex13");
	//motion_handles[13].init(programs.current_program);
	//motion_handles[14] = VarHandle("u_tex14");
	//motion_handles[14].init(programs.current_program);
	//motion_handles[15] = VarHandle("u_tex15");
	//motion_handles[15].init(programs.current_program);
	//motion_handles[16] = VarHandle("u_tex16");
	//motion_handles[16].init(programs.current_program);
	//motion_handles[17] = VarHandle("u_tex17");
	//motion_handles[17].init(programs.current_program);
	//motion_handles[18] = VarHandle("u_tex18");
	//motion_handles[18].init(programs.current_program);
	//motion_handles[19] = VarHandle("u_tex19");
	//motion_handles[19].init(programs.current_program);
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

	programs.load_program(5);
	combine_model_mat_handle = VarHandle("u_m", &model);
	combine_model_mat_handle.init(programs.current_program);
	combine_view_mat_handle = VarHandle("u_v", &view);
	combine_view_mat_handle.init(programs.current_program);
	combine_proj_mat_handle = VarHandle("u_p", &projection);
	combine_proj_mat_handle.init(programs.current_program);
	combine_texture1_handle = VarHandle("u_tex0");
	combine_texture1_handle.init(programs.current_program);
	combine_texture2_handle = VarHandle("u_tex1");
	combine_texture2_handle.init(programs.current_program);

	programs.load_program(6);
	basic_model_mat_handle = VarHandle("u_m", &model);
	basic_model_mat_handle.init(programs.current_program);
	basic_view_mat_handle = VarHandle("u_v", &view);
	basic_view_mat_handle.init(programs.current_program);
	basic_proj_mat_handle = VarHandle("u_p", &projection);
	basic_proj_mat_handle.init(programs.current_program);
	basic_texture_handle = VarHandle("u_tex");
	basic_texture_handle.init(programs.current_program);
	
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

	init_objects();

	init_camera_tour();
}




glm::mat4 getOrtho()
{
	return glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 10.0f);
}
glm::mat4 getOrthoView()
{
	return glm::lookAt(glm::vec3(-0.866, 0, 0), glm::vec3(), glm::vec3(0, 1, 0));
}
glm::mat4 getPerspective()
{
	//lorentz = glm::clamp(lorentz, 1.0f, 4.0f);
	glm::mat4 p = glm::perspective(glm::radians(min(lorentz * 30.0f, 160.0f)), (float)window_size.x / (float)window_size.y, 1.0f, 20000000.0f);
	
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

		physics();

		lorentz = pow(glm::length(eye_position - old_eye_position), 1) / (star.scale.x) / max(step / step_d, 1) + 1;
		old_eye_position = eye_position;
		if (EFFECTS)
		{

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
			light_pos_handle.load();
			eye_pos_handle.load();
			light_properties_handle.load();
			light_color_handle.load();
			ambient_color_handle.load();
			//// DRAW
			glow_draw();
			//unbind the fbo
			basic_fbo.unbind();




			//// render normal scene to FBO
			// load glow shader
			programs.load_program(3);
			// bind motionblur part fbo to render
			glow_fbo.bind();
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
			glow_res_handle.load(glm::vec3(3 / window_size.x, 3 / window_size.y, 15));
			//// DRAW		
			// setting obj texture to be from the basic fbo
			screen_texture.setTex(basic_fbo.tex);
			// drawing texture onto a square mesh
			screen_texture.draw(0, &glow_model_mat_handle, &glow_texture_handle, nullptr, nullptr);
			// unbind motionblur part fbo
			//glow_fbo.unbind();

			//basic_fbo.unbind();
			//// draw normal scene
			// load basic shader
			programs.load_program(1);
			// bind basic fbo to render to
			glClear(GL_DEPTH_BUFFER_BIT);

			// update view and proj matrices
			view = getperspectiveView();
			projection = getPerspective();
			//// LOAD GLOBAL HANDLES
			view_mat_handle.load();
			proj_mat_handle.load();
			light_pos_handle.load();
			eye_pos_handle.load();
			light_properties_handle.load();
			light_color_handle.load();
			ambient_color_handle.load();
			//// DRAW
			graphics_loop();
			//unbind the fbo

			glow_fbo.unbind();

			//programs.load_program(3);
			//// clear screen
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//glClearColor(0.0f, 0.0f, 0.0f, 1.);
			//// update view and proj matrices
			//view = getOrthoView();
			//projection = getOrtho();
			////// LOAD GLOBAL HANDLES
			//view_mat_handle.load();
			//proj_mat_handle.load();
			//// set glow parameters
			//glow_res_handle.load(glm::vec3(1 / window_size.x * lorentz, 1 / window_size.y* lorentz, 10));
			////// DRAW		
			//// setting obj texture to be from the basic fbo
			//screen_texture.setTex(glow_fbo.tex);
			//// drawing texture onto a square mesh
			//screen_texture.draw(0, &glow_model_mat_handle, &glow_texture_handle, nullptr, nullptr);



			//// render normal scene to FBO
			// load glow shader
			programs.load_program(4);
			// bind motionblur part fbo to render
			motion_blur_parts[current_frame].bind();
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
			shift_handle.load(glm::vec3(max(lor / 15, 1), max(lor / 15, 1), max(lor/5, 1)));
			//// DRAW		
			// setting obj texture to be from the basic fbo
			screen_texture.setTex(glow_fbo.tex);
			// drawing texture onto a square mesh
			screen_texture.draw(0, &shift_model_mat_handle, &shift_texture_handle, nullptr, nullptr);
			// unbind motionblur part fbo
			motion_blur_parts[current_frame].unbind();

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
			light_pos_handle.load();
			eye_pos_handle.load();
			light_properties_handle.load();
			light_color_handle.load();
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