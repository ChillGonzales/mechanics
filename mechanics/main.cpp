#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#define _USE_MATH_DEFINES
#define NANOGUI_USE_OPENGL 1
#define NANOGUI_GLAD 1
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
#include "skybox.h"
#include "physics_debug_renderer.h"
#include <cmath>
#include "collision_categories.h"
#include "../editor/scene_loader.h"
#include "../editor/scene_manager.h"
using namespace reactphysics3d;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void setupPointLights(Shader* shader);
void performPunch();

const Vector3 toPhysVec(glm::vec3 vec);
const glm::vec3 toGlm(Vector3 vec);
bool USE_PHY_DEBUG_RENDERING = false;

// settings
const int SCR_WIDTH = 1920;
const int SCR_HEIGHT = 1080;
const float _physicsTimestep = 1.0f / 60.0f;
const int CAMERA_INDEX = NUM_PHY_OBJECTS - 1;

// camera
Camera camera(glm::vec3(10.0f, -10.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// positions
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// physics
RigidBody* ballBody = nullptr;
Collider* ballCollider = nullptr;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Best Game Ever", NULL, NULL);
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
	glEnable(GL_FRAMEBUFFER_SRGB);

	// With our nice shader class, just give the paths and call use
	Shader lightShader("shaders/light/vertex.glsl", "shaders/light/fragment.glsl");
	Shader skyboxShader("shaders/skybox/vertex.glsl", "shaders/skybox/fragment.glsl");
	Shader lampShader("shaders/lamp/vertex.glsl", "shaders/lamp/fragment.glsl");
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	Shader shaders[3] = {
		lightShader, skyboxShader, lampShader
	};

	setupPointLights(&lightShader);

	Skybox skybox;
	// Set up our skybox
	vector<std::string> faces
	{
		"assets/skybox/right.jpg",
		"assets/skybox/left.jpg",
		"assets/skybox/top.jpg",
		"assets/skybox/bottom.jpg",
		"assets/skybox/front.jpg",
		"assets/skybox/back.jpg"
	};
	skybox.init(faces);

	// Set up our objects for physics and rendering
	RenderingState renders(NUM_RENDER_OBJECTS);
	PhysicsState physics(NUM_PHY_OBJECTS);

	auto ball = Model("assets/ball/ball.obj");
	renders.models[0] = ball;
	renders.shader_indices[0] = 0;
	renders.names[0] = "ball";

	for (unsigned int i = 1; i < NUM_RENDER_OBJECTS; i++)
	{
		renders.models[i] = Model("assets/plank/plank.obj");
		renders.transforms[i] = Transform(Vector3::zero(), Quaternion::identity());
		renders.shader_indices[i] = 0;
		renders.names[i] = "environment" + to_string(i);
	}

	// Create the world settings 
	PhysicsWorld::WorldSettings settings;
	settings.isSleepingEnabled = true;
	settings.gravity = Vector3(0, -9.81f, 0);
	PhysicsCommon common;
	auto* world = common.createPhysicsWorld(settings);
	world->setIsDebugRenderingEnabled(true);
	// Defaults are 10 and 5 so if this is laggy then change it.
	world->setNbIterationsVelocitySolver(15);
	world->setNbIterationsPositionSolver(8);

	// Rigidbody setup
	auto ballTransform = Transform(Vector3(15.0f, 30.0f, -50.0f), Quaternion::identity());
	physics.prev_transforms[0] = ballTransform;
	renders.transforms[0] = ballTransform;
	ballBody = world->createRigidBody(ballTransform);
	ballBody->setType(BodyType::DYNAMIC);
	physics.bodies[0] = ballBody;
	physics.names[0] = "ball";
	float radius = 3.0f;
	// TODO: how to get physics shapes to match extents of meshes?
	SphereShape* sphereShape = common.createSphereShape(radius);
	Vector3 floorExtents(160.0f, 1.0f, 160.0f);
	Vector3 wallExtents(75.0, 1.0f, floorExtents.z);
	BoxShape* floorShape = common.createBoxShape(floorExtents);
	BoxShape* wallShape = common.createBoxShape(wallExtents);
	CapsuleShape* capsuleShape = common.createCapsuleShape(3.0f, 8.0f);

	// Relative transform of the collider relative to the body origin 
	Transform ident = Transform::identity();

	// Add the collider to the rigid body 
	ballCollider = ballBody->addCollider(sphereShape, ident);
	ballCollider->getMaterial().setBounciness(0.6f);
	ballCollider->getMaterial().setFrictionCoefficient(0.7f);
	ballCollider->getMaterial().setRollingResistance(2.0f);
	ballCollider->getMaterial().setMassDensity(3.0f);
	ballBody->setIsAllowedToSleep(false);
	ballCollider->setCollisionCategoryBits(CollisionCategories::BALL);
	ballCollider->setCollideWithMaskBits(CollisionCategories::BALL | CollisionCategories::ENVIRONMENT | CollisionCategories::CAMERA);
	physics.colliders[0] = ballCollider;

	auto cameraTransform = Transform(toPhysVec(camera.Position), Quaternion::identity());
	auto cameraBody = world->createRigidBody(cameraTransform);
	cameraBody->setType(BodyType::STATIC);
	auto cameraCollider = cameraBody->addCollider(capsuleShape, ident);
	cameraCollider->setCollisionCategoryBits(CollisionCategories::CAMERA);
	cameraCollider->setCollideWithMaskBits(CollisionCategories::BALL | CollisionCategories::ENVIRONMENT);
	physics.bodies[CAMERA_INDEX] = cameraBody;
	physics.prev_transforms[CAMERA_INDEX] = cameraTransform;
	physics.colliders[CAMERA_INDEX] = cameraCollider;
	physics.names[CAMERA_INDEX] = "camera";

	constexpr float rad90 = glm::radians(90.0f);
	const auto envCount = NUM_PHY_OBJECTS - 2;
	Vector3 angles[envCount] = {
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 0.0f, rad90),
		Vector3(rad90, 0.0f, rad90),
		Vector3(0.0f, 0.0f, rad90),
		Vector3(rad90, 0.0f, rad90),
	};

	Vector3 origin(25.0f, -25.0f, -25.0f);
	Vector3 start(25.0f + floorExtents.x / 2, -25.0f, -25.0f + -1.0f * (floorExtents.z / 2));
	Vector3 positions[envCount] = {
		Vector3(origin.x + floorExtents.x / 2, origin.y, origin.z + -1.0f * (floorExtents.z / 2)), // floor
		Vector3(origin.x - floorExtents.x / 2, floorExtents.y, origin.z - floorExtents.z / 2), // left wall
		Vector3(origin.x + floorExtents.x / 2, floorExtents.y, 4 * origin.z - floorExtents.z), // back wall
		Vector3(4 * origin.x + floorExtents.x, floorExtents.y, origin.z - floorExtents.z / 2), // right wall
		Vector3(origin.x + floorExtents.x / 2, floorExtents.y, origin.z + floorExtents.z / 2), // front wall
	};
	BoxShape* boxShapes[envCount] = {
		floorShape,
		wallShape,
		wallShape,
		wallShape,
		wallShape,
	};
	// Start at 1 because we set up the ball collider manually
	// Stop before the end because we set up the camera collider manually
	for (unsigned int i = 1; i < (NUM_PHY_OBJECTS - 1); i++)
	{
		auto trans = Transform(positions[i - 1], Quaternion::fromEulerAngles(angles[i - 1]));
		physics.prev_transforms[i] = trans;
		auto rBody = world->createRigidBody(trans);
		rBody->setType(BodyType::STATIC);
		physics.bodies[i] = rBody;
		auto coll = rBody->addCollider(boxShapes[i - 1], ident);
		coll->setCollisionCategoryBits(CollisionCategories::ENVIRONMENT);
		coll->setCollideWithMaskBits(CollisionCategories::BALL | CollisionCategories::CAMERA);
		physics.colliders[i] = coll;
		physics.names[i] = "wall" + to_string(i);
	}

	// Init variables for main loop
	float accumulator = 0.0f;
	float modelMatrix[16];

	PhysicsDebugRenderer phyDebugRenderer(world);

	SceneLoader loader;
	loader.writeSceneToDisk("scene1.scene", "scene1", &renders, &physics, world);
	loader.loadScene("scene1.scene", "scene1");

	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		//cout << "Camera position. X: " << camera.Position.x << " Y: " << camera.Position.y << " Z: " << camera.Position.z << endl;

		cameraBody->setTransform(Transform(toPhysVec(camera.Position), Quaternion::identity()));

		accumulator += deltaTime;
		while (accumulator >= _physicsTimestep)
		{
			// Update the physics sim
			world->update(_physicsTimestep);
			accumulator -= _physicsTimestep;
		}

		// Compute the time interpolation factor 
		decimal factor = accumulator / _physicsTimestep;

		for (unsigned int i = 0; i < NUM_PHY_OBJECTS; i++)
		{
			const auto& currTrans = physics.bodies[i]->getTransform();
			// Compute the interpolated transform of the rigid body 
			Transform interpolatedTransform = Transform::interpolateTransforms(physics.prev_transforms[i], currTrans, factor);

			// Update the previous transform 
			physics.prev_transforms[i] = currTrans;
			if (i < (NUM_RENDER_OBJECTS - 1))
			{
				renders.transforms[i] = interpolatedTransform;
			}
		}

		if (USE_PHY_DEBUG_RENDERING)
		{
			phyDebugRenderer.updateDebugState();
		}

		// Rendering logic
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// TODO: Be able to handle different shaders based on what is read from the scene
		lightShader.use();

		// Activate our shader program
		lightShader.setVec3("viewPos", camera.Position);
		lightShader.setFloat("material.shininess", 64.0f);
		lightShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		lightShader.setVec3("dirLight.ambient", 0.2f, 0.2f, 0.2f);
		lightShader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
		lightShader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

		// camera/view transformation
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 800.0f);
		lightShader.setMat4("view", view);
		lightShader.setMat4("projection", projection);
		for (unsigned int i = 0; i < NUM_RENDER_OBJECTS; i++)
		{
			renders.transforms[i].getOpenGLMatrix(modelMatrix);
			glm::mat4 model = glm::make_mat4(modelMatrix);
			//model = glm::translate(model, trans.getPosition()); // add the translation from our source of truth translation to the model matrix
			//model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
			lightShader.setMat4("model", model);
			renders.models[i].Draw(lightShader);
		}

		// draw skybox last
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		skyboxShader.use();
		auto skyView = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
		skyboxShader.setMat4("projection", projection);
		skyboxShader.setMat4("view", skyView);
		// skybox cube
		glBindVertexArray(skybox.VAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default

		if (USE_PHY_DEBUG_RENDERING)
		{
			lampShader.use();
			lampShader.setMat4("projection", projection);
			lampShader.setMat4("view", view);
			lampShader.setMat4("model", glm::mat4(1.0f));
			phyDebugRenderer.draw();
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up physics memory
	for (unsigned int i = 0; i < NUM_PHY_OBJECTS; i++)
	{
		world->destroyRigidBody(physics.bodies[i]);
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

void performPunch()
{
	// 1. Send raycast forward
	float magnitude = 25.0f;
	Vector3 direction = toPhysVec(camera.Front);

	// Create the ray 
	Vector3 startPoint = toPhysVec(camera.Position);
	Vector3 endPoint = startPoint + (magnitude * direction);
	Ray ray(startPoint, endPoint);

	// Create the raycast info object for the 
	// raycast result 
	RaycastInfo raycastInfo;

	// Test raycasting against a collider 
	bool isHit = ballBody->raycast(ray, raycastInfo);
	if (isHit)
	{
		cout << "We hit something!" << endl;
		float force = (1 - raycastInfo.hitFraction) * magnitude * 150;
		ballBody->applyForceToCenterOfMass(force * direction);
	}
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
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
		USE_PHY_DEBUG_RENDERING = !USE_PHY_DEBUG_RENDERING;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		performPunch();
}

void setupPointLights(Shader* shader)
{
	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f, 35.2f, 2.0f),
		glm::vec3(2.3f, 32.3f, -4.0f),
		glm::vec3(-4.0f, 36.0f, -12.0f),
		glm::vec3(0.0f, 40.0f, -3.0f)
	};

	shader->use();
	for (unsigned int i = 0; i < 4; i++)
	{
		auto s = std::to_string(i);
		shader->setVec3("pointLights[" + s + "].position", pointLightPositions[i]);
		shader->setFloat("pointLights[" + s + "].constant", 1.0f);
		shader->setFloat("pointLights[" + s + "].linear", 0.09f);
		shader->setFloat("pointLights[" + s + "].quadratic", 0.032f);
		shader->setVec3("pointLights[" + s + "].ambient", 0.05f, 0.05f, 0.05f);
		shader->setVec3("pointLights[" + s + "].diffuse", 0.8f, 0.8f, 0.8f);
		shader->setVec3("pointLights[" + s + "].specular", 1.0f, 1.0f, 1.0f);
	}
}

const Vector3 toPhysVec(glm::vec3 vec)
{
	return Vector3(vec.x, vec.y, vec.z);
}

const glm::vec3 toGlm(Vector3 vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}
