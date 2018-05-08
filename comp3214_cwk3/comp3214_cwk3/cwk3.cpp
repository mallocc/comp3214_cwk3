#include "cwk3.h"
#include "colors.h"
#include "lerp.h"
#include <chrono>
#include <thread>
#include <minmax.h>
#include <glm\glm\gtc\noise.hpp>
#include <btBulletDynamicsCommon.h>

#define COE 1
#define GRAVITY 0

// bullet declarations
btBroadphaseInterface* broadphase;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;
std::vector<btRigidBody*> MovingBits; // so that can get at all bits



GLSLProgramManager	programs;

VarHandle
	model_mat_handle,
	view_mat_handle,
	proj_mat_handle;

glm::vec2
	window_size(1280, 720);

glm::vec3
	eye_position(0, 5, -20),
	eye_direction,
	ambient_color = glm::vec3(0.1f,0.2f,0.3f);

glm::mat4 
	model, 
	view, 
	projection;

Obj
	terrain, container;


LerperSequencer cameraSequence;

//Returns random float
inline float		randf()
{
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

//Error callback  
static void		error_callback(int error, const char* description)
{
	fputs(description, stderr);
	_fgetchar();
}

static void		key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ENTER:
			eye_position = glm::vec3(0,0,1) * eye_position;
			break;
		case GLFW_KEY_UP:
			eye_position += eye_position * 0.1f;
			break;
		case GLFW_KEY_DOWN:
			eye_position -= eye_position * 0.1f;
			break;
		case GLFW_KEY_RIGHT:
			eye_position = glm::quat(glm::vec3(0, glm::radians(3.0f), 0)) * eye_position;
			break;
		case GLFW_KEY_LEFT:
			eye_position = glm::quat(glm::vec3(0, -glm::radians(3.0f), 0)) * eye_position;
			break;
		case GLFW_KEY_SPACE:
			cameraSequence.addLerper(glm::vec3((randf() - 0.5f) * 10, (randf() - 0.5f) * 10, (randf() - 0.5f) * 10));
			break;
		case GLFW_KEY_ESCAPE:
		case GLFW_KEY_Q:
			glfwSetWindowShouldClose(window, GL_TRUE);
			break;
		}
	}
}

void printV(glm::vec3 v)
{
	printf("x: %f - y: %f - z: %f\n", v.x, v.y, v.z);
}



glm::vec3 getVec3(btVector3 v)
{
	return glm::vec3(v[0], v[1], v[2]);
}

btVector3 getBtVector3(glm::vec3 v)
{
	return btVector3(v.x, v.y, v.z);
}


btRigidBody* SetSphere(float size, btTransform T) {
	btCollisionShape* fallshape = new btSphereShape(size);
	btDefaultMotionState* fallMotionState = new btDefaultMotionState(T);
	btScalar mass = 1;
	btVector3 fallInertia(0, 0, 0);
	fallshape->calculateLocalInertia(mass, fallInertia);
	btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallshape, fallInertia);
	btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
	fallRigidBody->setLinearVelocity(btVector3(randf(), randf(), randf()) * 100.0f);
	fallRigidBody->setRestitution(COE);
	//fallRigidBody->setFriction(0.0f);
	//fallRigidBody->setRollingFriction(0.0f);
	//fallRigidBody->setDamping(0.0f, 0.0f);
	dynamicsWorld->addRigidBody(fallRigidBody);
	return fallRigidBody;
	return nullptr;
}

btRigidBody* SetCube(btVector3 size, btTransform T) {
	btCollisionShape* fallshape = new btBoxShape(size);
	btDefaultMotionState* fallMotionState = new btDefaultMotionState(T);
	btScalar mass = 1;
	btVector3 fallInertia(0, 0, 0);
	fallshape->calculateLocalInertia(mass, fallInertia);
	btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallshape, fallInertia);
	btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
	fallRigidBody->setLinearVelocity(btVector3(randf(), randf(), randf()) * 100.0f);
	fallRigidBody->setRestitution(COE);
	//fallRigidBody->setFriction(0.0f);
	//fallRigidBody->setRollingFriction(0.0f);
	//fallRigidBody->setDamping(0.0f, 0.0f);
	dynamicsWorld->addRigidBody(fallRigidBody);
	return fallRigidBody;
	return nullptr;
}


void bullet_step(int i, Obj * obj) {
	btTransform trans;
	btRigidBody* moveRigidBody;
	int n = MovingBits.size();
	moveRigidBody = MovingBits[i];
	moveRigidBody->getMotionState()->getWorldTransform(trans);
	btQuaternion rot = moveRigidBody->getCenterOfMassTransform().getRotation();
	obj->pos = glm::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(), trans.getOrigin().getZ());
	obj->rotation = glm::vec3(rot.getAxis().getX(), rot.getAxis().getY(), rot.getAxis().getZ());
	obj->theta = rot.getAngle();
}

void bullet_close() {
  /*
   * This is very minimal and relies on OS to tidy up.
   */
	if (MovingBits.size() > 0)
	{
		btRigidBody* moveRigidBody;
		moveRigidBody = MovingBits[0];
		dynamicsWorld->removeRigidBody(moveRigidBody);
		delete moveRigidBody->getMotionState();
		delete moveRigidBody;
	}
  delete dynamicsWorld;
  delete solver;
  delete collisionConfiguration;
  delete dispatcher;
  delete broadphase;
}

void loop()
{
	//lights.pos = glm::quat(glm::vec3(0, 0.005f, 0)) * lights.pos;
	//eye_position = glm::quat(glm::vec3(0, 0.001f, 0)) * eye_position;

	eye_position = cameraSequence.lerpStepSmooth();
	//printV(eye_position);


	//// LOAD GLOBAL HANDLES
	view_mat_handle.load();
	proj_mat_handle.load();

	//// BULLET UPDATE
	dynamicsWorld->stepSimulation(0.01, 5);

	//bullet_step(0, &sphere);

	//// DRAW OBJECTS

	terrain.draw(1, &model_mat_handle, nullptr, nullptr, nullptr);
	container.draw(1, &model_mat_handle, nullptr, nullptr, nullptr);

	//printf("%f  %f  %f\n", sphere.pos.x,sphere.pos.y,sphere.pos.z);

	//eye_direction = sphere.pos;
}

void init_bullet()
{
	/*
	* set up world
	*/
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0., GRAVITY, 0));

	/*
	* Set up sphere 0
	*/
	//MovingBits.push_back(SetSphere(1, btTransform(btQuaternion(0,0,1,1),btVector3(sphere.pos.x,sphere.pos.y,sphere.pos.z))));

}

float LoG(float x, float y, float sigma)
{
	return (1 / sqrt(glm::pi<float>() * 2 * sigma*sigma)) * exp(-((x*x + y*y) / 2 * sigma*sigma));
}

std::vector<glm::vec3>			generateTerrain(int w, int h, int seed)
{
	glm::vec3
		s = glm::vec3(1 / (float)w, 0, 1 / (float)h),
		a = glm::vec3(1, 0, 1) * s,
		b = glm::vec3(1, 0, -1) * s,
		c = glm::vec3(-1, 0, -1) * s,
		d = glm::vec3(-1, 0, 1) * s;
	std::vector<glm::vec3> n;
	for (int x = 0; x < w; ++x)
		for (int y = 0; y < h; ++y)
		{
			glm::vec3 t = s * glm::vec3(x, 0, y) - glm::vec3(0.5f, glm::perlin(glm::vec3(x,0,y)), 0.5f);
			n.push_back(a + t);
			n.push_back(b + t);
			n.push_back(c + t);
			n.push_back(c + t);
			n.push_back(d + t);
			n.push_back(a + t);
		}
	return n;
}

//Initilise custom objects
void			init()
{
	//// CREATE GLSL PROGAMS
	printf("\n");
	printf("Initialising GLSL programs...\n");
	programs.add_program("shaders/basic.vert", "shaders/basic.frag");


	//// CREATE HANDLES
	printf("\n");
	printf("Initialising variable handles...\n");
	model_mat_handle = VarHandle("u_m", &model);
	model_mat_handle.init(programs.current_program);

	view_mat_handle = VarHandle("u_v", &view);
	view_mat_handle.init(programs.current_program);

	proj_mat_handle = VarHandle("u_p", &projection);
	proj_mat_handle.init(programs.current_program);
	
	//// CREATE OBJECTS
	printf("\n");
	printf("Initialising objects...\n");
	// create sphere data for screen A, B and D
	std::vector<glm::vec3> v = generateTerrain(100,100,0);

	terrain = Obj(
		"","","",
		//"textures/moss_color.jpg",
		//"textures/moss_norm.jpg",
		//"textures/moss_height.jpg",
		pack_object(&v, GEN_COLOR_RAND, WHITE),
		glm::vec3(0,0,0),
		glm::vec3(1, 0, 0),
		glm::radians(0.0f),
		glm::vec3(1, 1, 1)
	);
	terrain.scale = glm::vec3(5, 5, 5);

	v = generate_cube();

	container = Obj(
		"", "", "",
		pack_object(&v, GEN_COLOR_RAND, WHITE),
		glm::vec3(0, 0, 0),
		glm::vec3(1, 0, 0),
		glm::radians(0.0f),
		glm::vec3(1, 1, 1)
	);;
	container.scale = glm::vec3(5, 5, 5);

	cameraSequence = LerperSequencer(glm::vec3(), glm::vec3(0,5,-5), 0.005, 0.1);


	init_bullet();
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

		//Clear color buffer  
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// set clear color
		glClearColor(0.0f,0.0f,0.0f, 1.);

		projection = glm::perspective(glm::radians(45.0f), (float)window_size.x / (float)window_size.y, 0.001f, 1000.0f);
		view = glm::lookAt(eye_position, eye_direction, glm::vec3(0,1,0));

		// call the graphics loop
		graphics_loop();

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

	bullet_close();

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