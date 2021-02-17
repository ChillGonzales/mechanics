#pragma once
#include <iostream>
#include "..\mechanics\model.h"
#include <reactphysics3d/reactphysics3d.h>

using namespace reactphysics3d;

// Write these params to file
const int NUM_PHY_OBJECTS = 7;
const int NUM_RENDER_OBJECTS = 2;

struct SceneData
{
	unsigned int num_entities;
	string name;
};

struct RenderingState
{
	RenderingState(unsigned int count)
	{
		models = new Model[count];
		names = new string[count];
		length = count;
		shader_indices = new unsigned int[count];
		transforms = new Transform[count];
	}

	Model* models;
	Transform* transforms;
	unsigned int* shader_indices;
	string* names;
	unsigned int length;
};

struct PhysicsState
{
	PhysicsState(unsigned int count)
	{
		bodies = new RigidBody * [count];
		colliders = new Collider * [count];
		prev_transforms = new Transform[count];
		names = new string[count];
		length = count;
	}

	RigidBody** bodies;
	Collider** colliders;
	Transform* prev_transforms;
	string* names;
	unsigned int length;
};

struct SceneHeader
{
	string name;
	string path;
	unsigned int phy_obj_count;
	unsigned int render_obj_count;
	bool phy_sleeping_enabled;
	Vector3 phy_gravity;
	unsigned int phy_velocity_iterations;
	unsigned int phy_position_iterations;
	struct PhysicsState* physics;
	RenderingState* renders;
};

struct SceneEntity
{
	string model_path;
	glm::vec3 world_position;
	glm::vec3 world_rotation;
	Vector3 rigidbody_position;
	Vector3 rigidbody_rotation;
	unsigned int rigidbody_type;
	unsigned int collider_shape_type;
	// There will be logic in our file loader class that will tell us how many initializers our collider needs.
	float* collider_initializers;
	float collider_bounciness;
	float collider_friction;
	float collider_rolling_resist;
	float mass_density;
	bool collider_sleep;
	unsigned short collider_category_bits;
	unsigned short collider_collide_mask_bits;
	unsigned int id;
	unsigned int shaderIndex;
	string name;
};

