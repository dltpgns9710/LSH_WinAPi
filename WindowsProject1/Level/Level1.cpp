#include "Level1.h"

void Level1::Load()
{
	y = 0.f;
	ySpeed = 0.f;

	//sprites = std::make_shared<SpriteSheet>(L"Resources/Texture2D/bf01_01_2a.png", graphics);
}

void Level1::UnLoad()
{
}

void Level1::Render()
{
	graphics->ClearScreen(.0f, .0f, .5f);
	graphics->DrawCircle(800, y, 50.f, 1.f, .0f, .0f, 1.0f);
	//sprites->DrawVer2();
	//sprites->Draw();
}

void Level1::Update(double deltaTime)
{
	ySpeed += 1.f;
	y += ySpeed;
	if (y > 600.f)
	{
		y = 600.f;
		ySpeed = -30.f;
	}
}
