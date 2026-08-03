#include "../include/GameLevel.h"
#include "../../Core/include/RenderObject.h"
#include "../../Core/include/NonRenderObject.h"
#include "../../Component/include/CollisionComponent.h"
#include "../../Physics/include/CollisionCalc.h"
#include "../../Camera/include/Camera.h"

std::shared_ptr<Graphics> GameLevel::graphics;

GameLevel::GameLevel()
{
	camera = std::make_shared<Camera>();
}

void GameLevel::Render()
{
	if (camera) camera->Render();

	/*
	for (auto& renderObject : renderObjects)
	{
		renderObject->Render();
	}
	*/
}

void GameLevel::Update(double deltaTime)
{
	for (auto& nonRenderObject : nonRenderObjects)
	{
		nonRenderObject->Update(deltaTime);
	}
	for (auto& renderObject : renderObjects)
	{
		renderObject->Update(deltaTime);
	}
}

void GameLevel::PostUpdate()
{
	//This project does not use physics
	//CollisionCheck();

	for (auto& nonRenderObject : nonRenderObjects)
	{
		nonRenderObject->PostUpdate();
	}
	for (auto& renderObject : renderObjects)
	{
		renderObject->PostUpdate();
	}

	DeleteGarbageObjects();
}

void GameLevel::CollisionCheck()
{
	for (int i = 0; i < renderObjects.size(); ++i)
	{
		for (int j = i + 1; j < renderObjects.size(); ++j)
		{
			std::shared_ptr<CollisionComponent> collisionA = renderObjects[i]->GetComponent<CollisionComponent>();
			std::shared_ptr<CollisionComponent> collisionB = renderObjects[j]->GetComponent<CollisionComponent>();
			if (!collisionA || !collisionB) continue;

			bool isOverlapping = CollisionCalc::IsEnterCollision(collisionA, collisionB);
			bool wasColliding = collisionA->IsColliding(collisionB);

			if (isOverlapping && !wasColliding)
			{
				collisionA->AddCollidingObject(collisionB);
				collisionB->AddCollidingObject(collisionA);
				collisionA->EnterCollision(collisionB);
				collisionB->EnterCollision(collisionA);
			}
			else if (isOverlapping && wasColliding)
			{
				collisionA->StayCollision(collisionB);
				collisionB->StayCollision(collisionA);
			}
			else if (!isOverlapping && wasColliding)
			{
				collisionA->RemoveCollidingObject(collisionB);
				collisionB->RemoveCollidingObject(collisionA);
				collisionA->ExitCollision(collisionB);
				collisionB->ExitCollision(collisionA);
			}
		}
	}
}

void GameLevel::DeleteGarbageObjects()
{
	std::erase_if(nonRenderObjects, [](std::shared_ptr<NonRenderObject> object) {return object->ShouldDelete();});
	std::erase_if(renderObjects, [](std::shared_ptr<RenderObject> object) {return object->ShouldDelete();});
}
