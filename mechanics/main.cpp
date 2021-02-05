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
#include "skybox.h"
using namespace reactphysics3d;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void setupPointLights(Shader* shader);
const Vector3 toPhysVec(glm::vec3 vec);

// settings
const int SCR_WIDTH = 1920;
const int SCR_HEIGHT = 1080;
const int NUM_OBJECTS = 2;
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

struct EntityState
{
	// Shared
	Transform prev_transforms[NUM_OBJECTS];

	// Physics
	RigidBody* bodies[NUM_OBJECTS];
	Collider* colliders[NUM_OBJECTS];

	// Rendering
	Model models[NUM_OBJECTS];
	Transform render_transform[NUM_OBJECTS];
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
	Shader lightShader("shaders/light/vertex.glsl", "shaders/light/fragment.glsl");
	Shader skyboxShader("shaders/skybox/vertex.glsl", "shaders/skybox/fragment.glsl");
	Shader lampShader("shaders/lamp/vertex.glsl", "shaders/lamp/fragment.glsl");
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.

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
	EntityState entities;

	auto ball = Model("assets/ball/ball.obj");
	entities.models[0] = ball;

	for (unsigned int i = 1; i < NUM_OBJECTS; i++)
	{
		entities.models[i] = Model("assets/plank/plank.obj");
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
	DebugRenderer& debugRenderer = world->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLISION_SHAPE, true);

	// Rigidbody setup
	auto ballTransform = Transform(Vector3(0.0f, 30.0f, 0.0f), Quaternion::identity());
	entities.prev_transforms[0] = ballTransform;
	entities.render_transform[0] = ballTransform;
	auto rBody = world->createRigidBody(ballTransform);
	rBody->setType(BodyType::DYNAMIC);
	entities.bodies[0] = rBody;
	float radius = 10.0f;
	// TODO: how to get physics shapes to match extents of meshes?
	SphereShape* sphereShape = common.createSphereShape(radius);
	BoxShape* boxShape = common.createBoxShape(Vector3(20.0f, 1.0f, 20.0f));
	CapsuleShape* capsuleShape = common.createCapsuleShape(3.0f, 8.0f);

	// Relative transform of the collider relative to the body origin 
	Transform ident = Transform::identity();

	// Add the collider to the rigid body 
	auto collider = rBody->addCollider(sphereShape, ident);
	collider->getMaterial().setBounciness(0.6f);
	entities.colliders[0] = collider;

	auto cameraTransform = Transform(toPhysVec(camera.Position), Quaternion::identity());
	auto cameraBody = world->createRigidBody(cameraTransform);
	cameraBody->setType(BodyType::STATIC);
	auto cameraCollider = cameraBody->addCollider(capsuleShape, ident);

	Vector3 angles[] = {
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 0.0f, 0.0f)
	};
	Vector3 positions[] = {
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 60.0f, 0.0f)
	};
	glm::vec3 modelOffsets[] = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 25.0f, 0.0f)
	};
	for (unsigned int i = 1; i < NUM_OBJECTS; i++)
	{
		auto trans = Transform(positions[i - 1], Quaternion::fromEulerAngles(angles[i - 1]));
		entities.prev_transforms[i] = trans;
		entities.render_transform[i] = trans;
		auto rBody = world->createRigidBody(trans);
		rBody->setType(BodyType::STATIC);
		entities.bodies[i] = rBody;
		auto coll = rBody->addCollider(boxShape, ident);
		entities.colliders[i] = coll;
	}

	// Init variables for main loop
	float accumulator = 0.0f;
	float modelMatrix[16];
	const unsigned int floatsPerLine = 2 * 3;
	const unsigned int floatsPerTri = 3 * 3;

	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

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
			const auto& currTrans = entities.bodies[i]->getTransform();
			// Compute the interpolated transform of the rigid body 
			Transform interpolatedTransform = Transform::interpolateTransforms(entities.prev_transforms[i], currTrans, factor);

			// Update the previous transform 
			entities.prev_transforms[i] = currTrans;
		}

		auto numDebugLines = debugRenderer.getNbLines();
		float* lineVertices = new float[floatsPerLine * numDebugLines];
		if (numDebugLines > 0)
		{
			auto* debugLines = debugRenderer.getLinesArray();
			for (int i = 0; i < numDebugLines; i++)
			{
				int vertexIx = i * floatsPerLine;
				float floats[floatsPerLine] = {
					debugLines[i].point1.x,
					debugLines[i].point1.y,
					debugLines[i].point1.z,
					debugLines[i].point2.x,
					debugLines[i].point2.y,
					debugLines[i].point2.z
				};
				for (int j = 0; j < floatsPerLine; j++)
				{
					lineVertices[vertexIx + j] = floats[j];
				}
			}
		}
		auto numDebugTris = debugRenderer.getNbTriangles();
		float* triVertices = new float[floatsPerTri * numDebugTris];
		if (numDebugTris > 0)
		{
			auto* debugTris = debugRenderer.getTrianglesArray();
			for (int i = 0; i < numDebugTris; i++)
			{
				int vertexIx = i * floatsPerTri;
				float floats[floatsPerTri] = {
					debugTris[i].point1.x,
					debugTris[i].point1.y,
					debugTris[i].point1.z,
					debugTris[i].point2.x,
					debugTris[i].point2.y,
					debugTris[i].point2.z,
					debugTris[i].point3.x,
					debugTris[i].point3.y,
					debugTris[i].point3.z
				};
				for (int j = 0; j < floatsPerTri; j++)
				{
					triVertices[vertexIx + j] = floats[j];
				}
			}
		}

		unsigned int debugLineVAO, debugLineVBO, debugTriVAO, debugTriVBO;

		glGenVertexArrays(1, &debugLineVAO);
		glGenBuffers(1, &debugLineVBO);
		glGenVertexArrays(1, &debugTriVAO);
		glGenBuffers(1, &debugTriVBO);

		glBindVertexArray(debugLineVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
		glBufferData(GL_ARRAY_BUFFER, numDebugLines * floatsPerLine * sizeof(float), &lineVertices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindVertexArray(debugTriVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugTriVBO);
		glBufferData(GL_ARRAY_BUFFER, numDebugTris * floatsPerTri * sizeof(float), &triVertices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		// Rendering logic
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		for (unsigned int i = 0; i < NUM_OBJECTS; i++)
		{
			entities.render_transform[i].getOpenGLMatrix(modelMatrix);
			glm::mat4 model = glm::make_mat4(modelMatrix);
			model = glm::translate(model, modelOffsets[i]);
			//model = glm::translate(model, trans.getPosition()); // add the translation from our source of truth translation to the model matrix
			//model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
			lightShader.setMat4("model", model);
			entities.models[i].Draw(lightShader);
		}

		lampShader.use();
		if (numDebugLines > 0)
		{
			glBindVertexArray(debugLineVAO);
			glDrawArrays(GL_LINES, 0, numDebugLines);
		}
		glBindVertexArray(debugTriVAO);
		glDrawArrays(GL_TRIANGLES, 0, numDebugTris);


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

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up physics memory
	for (unsigned int i = 0; i < NUM_OBJECTS; i++)
	{
		world->destroyRigidBody(entities.bodies[i]);
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
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		enablePhysics = true;
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
