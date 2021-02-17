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

// TODO: Redo this alloc to be dynamic
struct RenderingState
{
	Model models[NUM_RENDER_OBJECTS];
	Transform transforms[NUM_RENDER_OBJECTS];
	unsigned int shader_indices[NUM_RENDER_OBJECTS];
	unsigned int length = NUM_RENDER_OBJECTS;
	string names[NUM_RENDER_OBJECTS];
};

// TODO: Redo this alloc to be dynamic
struct PhysicsState
{
	// Collision
	RigidBody* bodies[NUM_PHY_OBJECTS];
	Collider* colliders[NUM_PHY_OBJECTS];

	// Position
	Transform prev_transforms[NUM_PHY_OBJECTS];
	string names[NUM_PHY_OBJECTS];
	unsigned int length = NUM_PHY_OBJECTS;
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

