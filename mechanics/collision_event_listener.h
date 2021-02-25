#pragma once

#include <reactphysics3d/reactphysics3d.h>
#include "collision_categories.h"
using namespace reactphysics3d;
using namespace std;

class CollisionEventListener : public EventListener
{

	virtual void onContact(const CollisionCallback::CallbackData& callbackData) override
	{
		for (uint p = 0; p < callbackData.getNbContactPairs(); p++)
		{
			auto contactPair = callbackData.getContactPair(p);

			auto containsBall = (contactPair.getCollider1()->getCollisionCategoryBits() & CollisionCategories::BALL) || (contactPair.getCollider2()->getCollisionCategoryBits() & CollisionCategories::BALL);
			auto containsFloor = (contactPair.getCollider1()->getCollisionCategoryBits() & CollisionCategories::FLOOR) || (contactPair.getCollider2()->getCollisionCategoryBits() & CollisionCategories::FLOOR);
			auto containsNet = (contactPair.getCollider1()->getCollisionCategoryBits() & CollisionCategories::NET) || (contactPair.getCollider2()->getCollisionCategoryBits() & CollisionCategories::NET);
			auto eventType = contactPair.getEventType();

			if (!containsBall)
				continue;
			if (containsFloor && eventType == CollisionCallback::ContactPair::EventType::ContactStart)
				cout << "The ball touched the floor!" << endl;
			if (containsNet && eventType == CollisionCallback::ContactPair::EventType::ContactStart)
				cout << "The ball hit the net!" << endl;

			for (uint c = 0; c < contactPair.getNbContactPoints(); c++)
			{
				auto point = contactPair.getContactPoint(c);
				Vector3 worldPoint = contactPair.getCollider1()->getLocalToWorldTransform() * point.getLocalPointOnCollider1();
			}
		}
	}
};
