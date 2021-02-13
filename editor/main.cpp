#pragma once
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#define _USE_MATH_DEFINES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nanogui\nanogui.h>
#include "../mechanics/camera.h"
#include "..\mechanics\model.h"
#include <reactphysics3d/reactphysics3d.h>
#include <iostream>
using namespace std;
using namespace reactphysics3d;
using namespace nanogui;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_btn_callback(GLFWwindow* window, int button, int action, int modifiers);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void drop_callback(GLFWwindow* window, int count, const char** filenames);
void char_callback(GLFWwindow*, unsigned int codepoint);
void processInput(GLFWwindow* window);
void setupNanoGui(GLFWwindow* window);

//nanogui
enum test_enum
{
	Item1 = 0,
	Item2,
	Item3
};

bool bvar = true;
int ivar = 12345678;
double dvar = 3.1415926;
float fvar = (float)dvar;
std::string strval = "A string";
test_enum enumval = Item2;
Color colval(0.5f, 0.5f, 0.7f, 1.f);
Screen* screen = nullptr;

const int SCR_WIDTH = 1920;
const int SCR_HEIGHT = 1080;
// Write these params to file
const int NUM_PHY_OBJECTS = 7;
const int NUM_RENDER_OBJECTS = 2;

// camera
Camera camera(glm::vec3(10.0f, -10.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// TODO: We probably don't want these structs stack init'd because we'll be adding/removing items all the time.
// Make them heap init'd
struct RenderingState
{
	Model models[NUM_RENDER_OBJECTS];
	Transform transforms[NUM_RENDER_OBJECTS];
};
struct PhysicsState
{
	// Collision
	RigidBody* bodies[NUM_PHY_OBJECTS];
	Collider* colliders[NUM_PHY_OBJECTS];

	// Position
	Transform prev_transforms[NUM_PHY_OBJECTS];
};


int main()
{
	glfwInit();
	glfwSetTime(0);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// nanogui hints, might need adjusting
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);
	glfwWindowHint(GLFW_STENCIL_BITS, 8);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Best Game Ever", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	GLint numDepthBits, numStencilBits = 0;
	GLboolean float_mode;

	screen = new Screen();
	screen->initialize(window, true);

	return 0;
	setupNanoGui(window);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_btn_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetDropCallback(window, drop_callback);
	glfwSetCharCallback(window, char_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw nanogui
		screen->draw_setup();
		screen->clear(); // glClear
		screen->draw_contents();
		screen->draw_widgets();
		screen->draw_teardown();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void setupNanoGui(GLFWwindow* window)
{
	// Create a nanogui screen and pass the glfw pointer to initialize
	screen = new Screen();
	screen->initialize(window, true);

	bool enabled = true;
	FormHelper* gui = new FormHelper(screen);
	nanogui::ref<Window> nanogui_window = gui->add_window(Vector2i(10, 10), "Form helper example");
	gui->add_group("Basic types");
	gui->add_variable("bool", bvar)->set_tooltip("Test tooltip.");
	gui->add_variable("string", strval);

	gui->add_group("Validating fields");
	gui->add_variable("int", ivar)->set_spinnable(true);
	gui->add_variable("float", fvar)->set_tooltip("Test.");
	gui->add_variable("double", dvar)->set_spinnable(true);

	gui->add_group("Complex types");
	gui->add_variable("Enumeration", enumval, enabled)->set_items({ "Item 1", "Item 2", "Item 3" });
	gui->add_variable("Color", colval);

	gui->add_group("Other widgets");
	gui->add_button("A button", []() { std::cout << "Button pressed." << std::endl; })
		->set_tooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");;

	screen->set_visible(true);
	screen->perform_layout();
	nanogui_window->center();
	screen->clear();
	screen->draw_all();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screen->resize_callback_event(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	screen->cursor_pos_callback_event(xpos, ypos);
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
	screen->scroll_callback_event(xoffset, yoffset);
}
void mouse_btn_callback(GLFWwindow* window, int button, int action, int modifiers)
{
	screen->mouse_button_callback_event(button, action, modifiers);
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
	//if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
	//	USE_PHY_DEBUG_RENDERING = !USE_PHY_DEBUG_RENDERING;
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	screen->key_callback_event(key, scancode, action, mods);
}
void drop_callback(GLFWwindow* window, int count, const char** filenames)
{
	screen->drop_callback_event(count, filenames);
}
void char_callback(GLFWwindow*, unsigned int codepoint)
{
	screen->char_callback_event(codepoint);
}
