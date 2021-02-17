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

struct Entities
{
	Model* models;
	Transform* transforms;
	Transform* prev_transforms;
	bool* is_rendered_flags;
	bool* has_phy_flags;
	RigidBody** bodies;
	Collider** colliders;
	unsigned int* ids;
	unsigned int* shaderIndices;
	string* names;
};

struct RenderingState
{
	Model models[NUM_RENDER_OBJECTS];
	Transform transforms[NUM_RENDER_OBJECTS];
	unsigned int length = NUM_RENDER_OBJECTS;
};

struct PhysicsState
{
	// Collision
	RigidBody* bodies[NUM_PHY_OBJECTS];
	Collider* colliders[NUM_PHY_OBJECTS];

	// Position
	Transform prev_transforms[NUM_PHY_OBJECTS];
	unsigned int length = NUM_PHY_OBJECTS;
};

