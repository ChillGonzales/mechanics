#pragma once
#include <reactphysics3d/reactphysics3d.h>
#include <glad\glad.h>

using namespace reactphysics3d;

struct PhysicsDebugRenderer
{
	DebugRenderer* debugRenderer;
	static const int floatsPerLine = 2 * 3;
	static const int floatsPerTri = 3 * 3;
	float* lineVertices = nullptr;
	float* triVertices = nullptr;
	unsigned int debugLineVAO, debugLineVBO, debugTriVAO, debugTriVBO;
	int numLineVertices = 0;
	int numTriVertices = 0;

	PhysicsDebugRenderer(PhysicsWorld* world)
	{
		debugRenderer = &world->getDebugRenderer();
		debugRenderer->setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLISION_SHAPE, true);
		debugRenderer->setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLIDER_AABB, true);
		//debugRenderer->setIsDebugItemDisplayed(DebugRenderer::DebugItem::COLLIDER_BROADPHASE_AABB, true);

		// Init render buffers
		glGenVertexArrays(1, &debugLineVAO);
		glGenBuffers(1, &debugLineVBO);
		glGenVertexArrays(1, &debugTriVAO);
		glGenBuffers(1, &debugTriVBO);
	}

	void draw(bool disableDepthTest = true)
	{
		if (disableDepthTest)
		{
			glDisable(GL_DEPTH_TEST);
		}
		if (numLineVertices > 0)
		{
			glBindVertexArray(debugLineVAO);
			glDrawArrays(GL_LINES, 0, numLineVertices);
		}
		if (numTriVertices > 0)
		{
			//glBindVertexArray(debugTriVAO);
			//glDrawArrays(GL_TRIANGLES, 0, numTriVertices);
		}
		if (disableDepthTest)
		{
			glEnable(GL_DEPTH_TEST);
		}
	}

	void updateDebugState()
	{
		auto nbLines = debugRenderer->getNbLines();
		auto lineVtxCount = double(floatsPerLine) * nbLines;
		if (lineVertices == nullptr || lineVtxCount != numLineVertices)
		{
			numLineVertices = lineVtxCount;
			lineVertices = new float[numLineVertices];
		}
		if (numLineVertices > 0)
		{
			auto* debugLines = debugRenderer->getLinesArray();
			for (int i = 0; i < nbLines; i++)
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

		auto nbTris = debugRenderer->getNbTriangles();
		auto triVtxCount = double(floatsPerTri) * nbTris;
		if (triVertices == nullptr || triVtxCount != numTriVertices)
		{
			numTriVertices = triVtxCount;
			triVertices = new float[numTriVertices];
		}
		if (numTriVertices > 0)
		{
			auto* debugTris = debugRenderer->getTrianglesArray();
			for (int i = 0; i < nbTris; i++)
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

		glBindVertexArray(debugLineVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
		glBufferData(GL_ARRAY_BUFFER, numLineVertices * sizeof(float), &lineVertices[0], GL_STATIC_DRAW);

		// line vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindVertexArray(debugTriVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugTriVBO);
		glBufferData(GL_ARRAY_BUFFER, numTriVertices * sizeof(float), &triVertices[0], GL_STATIC_DRAW);

		// tri vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}
};
