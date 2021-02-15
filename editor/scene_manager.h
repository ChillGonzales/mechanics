#pragma once
#include <iostream>
#include "..\mechanics\model.h"
#include <reactphysics3d/reactphysics3d.h>

using namespace reactphysics3d;

// Write these params to file
const int NUM_PHY_OBJECTS = 7;
const int NUM_RENDER_OBJECTS = 2;

struct Entities
{
	Model* models;
	glm::vec3* positions;
	glm::vec3* euler_rotations;
	RigidBody* bodies;
	Collider* colliders;
	unsigned int* ids;
	unsigned int* shaderIndices;
	string* names;
};

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

