#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#include "camera.h"
#include "model.h"
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <reactphysics3d/reactphysics3d.h>
using namespace reactphysics3d;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
Vector3 toPhysVec(glm::vec3 vec);

// settings
const int SCR_WIDTH = 1000;
const int SCR_HEIGHT = 700;
const int NUM_OBJECTS = 7;
const float _physicsTimestep = 1.0f / 60.0f;
bool enablePhysics = false;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// positions
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

struct Meshes
{
	Meshes()
	{ }

	// Rendering
	Model* models[NUM_OBJECTS];
	glm::mat4 model_mats[NUM_OBJECTS];

	// Physics
	//Transform transforms[NUM_OBJECTS];
	Transform prev_transforms[NUM_OBJECTS];
	//Vector3 positions[NUM_OBJECTS];
	//Quaternion rotations[NUM_OBJECTS];
	RigidBody* bodies[NUM_OBJECTS];
	Collider* colliders[NUM_OBJECTS];
};

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	// Register a callback for when the user resizes the window to tell open GL the new window size
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// With our nice shader class, just give the paths and call use
	Shader textureCubeShader("shaders/textured-cube/vertex.glsl", "shaders/textured-cube/fragment.glsl");
	Shader lightShader("shaders/light/vertex.glsl", "shaders/light/fragment.glsl");
	Shader lampShader("shaders/lamp/vertex.glsl", "shaders/lamp/fragment.glsl");
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.

	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	};

	lightShader.use();
	for (unsigned int i = 0; i < 4; i++)
	{
		auto s = std::to_string(i);
		lightShader.setVec3("pointLights[" + s + "].position", pointLightPositions[i]);
		lightShader.setFloat("pointLights[" + s + "].constant", 1.0f);
		lightShader.setFloat("pointLights[" + s + "].linear", 0.09f);
		lightShader.setFloat("pointLights[" + s + "].quadratic", 0.032f);
		lightShader.setVec3("pointLights[" + s + "].ambient", 0.05f, 0.05f, 0.05f);
		lightShader.setVec3("pointLights[" + s + "].diffuse", 0.8f, 0.8f, 0.8f);
		lightShader.setVec3("pointLights[" + s + "].specular", 1.0f, 1.0f, 1.0f);
	}

	// Set up our objects for physics and rendering
	Meshes meshes;
	
	auto ball = Model("assets/ball/ball.obj");
	meshes.models[0] = &ball;
	for (unsigned int i = 1; i <= 6; i++)
	{
		auto floor = Model("assets/plank/plank.obj");
		meshes.models[i] = &floor;
	}

	// **** PHYSICS SETUP ****
	// Create the world settings 
	PhysicsWorld::WorldSettings settings;
	settings.isSleepingEnabled = true;
	settings.gravity = Vector3(0, -9.81f, 0);
	PhysicsCommon common;
	auto* world = common.createPhysicsWorld(settings);
	// Defaults are 10 and 5 so if this is laggy then change it.
	// Change the number of iterations of the velocity solver 
	world->setNbIterationsVelocitySolver(15);
	// Change the number of iterations of the position solver 
	world->setNbIterationsPositionSolver(8);

	// Rigidbody setup
	auto ballTransform = Transform(Vector3(0.0f, 10.0f, 0.0f), Quaternion::identity());
	meshes.prev_transforms[0] = ballTransform;
	auto rBody = world->createRigidBody(ballTransform);
	rBody->setType(BodyType::DYNAMIC);
	float radius = 3.0f;
	SphereShape* sphereShape = common.createSphereShape(radius);
	BoxShape* boxShape = common.createBoxShape(Vector3(10.0f, 1.0f, 10.0f));

	// Relative transform of the collider relative to the body origin 
	Transform ident = Transform::identity();

	// Add the collider to the rigid body 
	auto collider = rBody->addCollider(sphereShape, ident);
	collider->getMaterial().setBounciness(0.8f);
	meshes.colliders[0] = collider;

	Vector3 angles[] = {
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(90.0f, 0.0f, 0.0f),
		Vector3(90.0f, 0.0f, 0.0f),
		Vector3(0.0f, 0.0f, 90.0f),
		Vector3(0.0f, 0.0f, 90.0f)
	};
	Vector3 positions[] = {
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 30.0f, 0.0f),
		Vector3(0.0f, 0.0f, -15.0f),
		Vector3(0.0f, 0.0f, 15.0f),
		Vector3(-15.0f, 0.0f, 0.0f),
		Vector3(15.0f, 0.0f, 0.0f)
	};
	for (unsigned int i = 1; i <= 6; i++)
	{
		auto trans = Transform(positions[i-1], Quaternion::fromEulerAngles(angles[i-1]));
		//meshes.transforms[i] = trans;
		meshes.prev_transforms[i] = trans;
		auto rBody = world->createRigidBody(trans);
		rBody->setType(BodyType::STATIC);
		meshes.bodies[i] = rBody;
		auto coll = rBody->addCollider(boxShape, ident);
		meshes.colliders[i] = coll;
	}

	// Init variables for main loop
	float accumulator = 0.0f;
	float modelMatrix[16];
	float factor = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		if (enablePhysics)
		{
			accumulator += deltaTime;
			while (accumulator >= _physicsTimestep)
			{
				// Update the physics sim
				world->update(_physicsTimestep);
				accumulator -= _physicsTimestep;
			}

			// Compute the time interpolation factor 
			decimal factor = accumulator / _physicsTimestep;

			for (unsigned int i = 0; i < NUM_OBJECTS; i++)
			{
				auto currTrans = meshes.bodies[i]->getTransform();
				// Compute the interpolated transform of the rigid body 
				Transform interpolatedTransform = Transform::interpolateTransforms(meshes.prev_transforms[i], currTrans, factor);

				// Update the previous transform 
				meshes.prev_transforms[i] = currTrans;
			}
		}

		// Rendering logic
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Activate our shader program
		lightShader.use();
		lightShader.setVec3("viewPos", camera.Position);
		lightShader.setFloat("material.shininess", 64.0f);
		lightShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		lightShader.setVec3("dirLight.ambient", 0.2f, 0.2f, 0.2f);
		lightShader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
		lightShader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

		lightShader.setVec3("spotLight.position", camera.Position);
		lightShader.setVec3("spotLight.direction", camera.Front);
		lightShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		lightShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
		lightShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		lightShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		lightShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		lightShader.setFloat("spotLight.constant", 1.0f);
		lightShader.setFloat("spotLight.linear", 0.09f);
		lightShader.setFloat("spotLight.quadratic", 0.032f);

		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		lightShader.setMat4("projection", projection);

		// camera/view transformation
		glm::mat4 view = camera.GetViewMatrix();
		lightShader.setMat4("view", view);
		for (unsigned int i = 0; i < NUM_OBJECTS; i++)
		{
			meshes.prev_transforms[i].getOpenGLMatrix(modelMatrix);
			glm::mat4 model = glm::make_mat4(modelMatrix);
			//model = glm::translate(model, trans.getPosition()); // add the translation from our source of truth translation to the model matrix
			//model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
			lightShader.setMat4("model", model);
			meshes.models[i]->Draw(lightShader);
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up physics memory
	for (unsigned int i = 0; i < NUM_OBJECTS; i++)
	{
		world->destroyRigidBody(meshes.bodies[i]);
	}
	common.destroyPhysicsWorld(world);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		enablePhysics = true;
}

Vector3 toPhysVec(glm::vec3 vec)
{
	return Vector3(vec.x, vec.y, vec.z);
}
