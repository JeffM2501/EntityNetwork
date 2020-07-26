#pragma once

#include "EntityNetwork.h"
#include <SDL.h>
#include <SDL_image.h>

using namespace EntityNetwork;
using namespace EntityNetwork::Client;

extern SDL_Window* Window ;
extern SDL_Rect	WindowRect;
extern SDL_Renderer* Renderer;
extern SDL_Texture* BackgroundTexture;

extern ClientWorld WorldData;

extern bool Quit;

class FPSTimer
{
public:
	uint32_t LastTickCount = 0;
	uint32_t DesiredFPS  = 0;

	inline FPSTimer() {}

	inline FPSTimer(uint32_t desiredFPS = 60)
	{
		desiredFPS = 0;
		Init();
	}

	inline void Init()
	{
		uint32_t ticksPerFrame = static_cast<uint32_t>((1.0 / DesiredFPS) * 1000);
		LastTickCount = SDL_GetTicks() - ticksPerFrame;
	}

	inline bool Update()
	{
		uint32_t now = SDL_GetTicks();
		uint32_t ticksPerFrame = static_cast<uint32_t>((1.0 / DesiredFPS) * 1000);
		uint32_t nextFrameTick = LastTickCount + ticksPerFrame;

		if (now >= nextFrameTick)
		{
			LastTickCount = now;
			return true;
		}
		return false;
	}

	inline void Delay()
	{
		uint32_t now = SDL_GetTicks();
		uint32_t ticksPerFrame = static_cast<uint32_t>((1.0 / DesiredFPS) * 1000);
		uint32_t nextFrameTick = LastTickCount + ticksPerFrame;

		if (now < nextFrameTick)
			SDL_Delay(nextFrameTick - now);
	}
};

extern FPSTimer RenderTimer;
extern FPSTimer EnityUpdateTimer;

// resources and errors
void LogSDLError(const std::string& msg);
std::string getResourcePath(const std::string& subDir = "");

// network status
bool NetConnected();

// events
void LoadWorld(ClientEntityController::Ptr, int);

// display
SDL_Texture* LoadTexture(const std::string& file, SDL_Renderer* ren = nullptr);
void BlitTexture(SDL_Texture* tex, int x, int y, SDL_Renderer* ren = nullptr);
void BlitTexture(SDL_Texture* tex, int x, int y, int w, int h, SDL_Renderer* ren = nullptr);
void BlitTexture(SDL_Texture* tex, SDL_Rect& dest, SDL_Renderer* ren = nullptr);
void BlitTextureCenter(SDL_Texture* tex, SDL_Point& pos, SDL_Renderer* ren = nullptr);
void BlitTextureCenter(SDL_Texture* tex, SDL_Point& pos, float angle, SDL_Renderer* ren = nullptr);
bool InitGraph();

void RenderGraph();

void HandleEntityAdd(EntityInstance::Ptr entity);

class PlayerTank : public EntityInstance
{
public:
	PlayerTank(EntityDesc::Ptr desc) : EntityInstance(desc), AvatarPicture(nullptr){};

	SDL_Texture* AvatarPicture = nullptr;
	SDL_Point DrawPoint;
	float DrawAngle = 0;

	float RealPosX = 0;
	float RealPosY = 0;

	typedef std::shared_ptr<PlayerTank> Ptr;

	inline virtual void Created()
	{
		StatePtr = FindProperty("State");
		PropertyChanged(StatePtr);
	}

	inline virtual void PropertyChanged(PropertyData::Ptr ptr)
	{
		if (ptr == StatePtr)
		{
			float* state = ptr->GetValue3F();
			DrawPoint.x = (int)(state[0]);
			DrawPoint.y = (int)(state[1]);
			RealPosX = state[0];
			RealPosY = state[1];
			DrawAngle = state[2];
		}
	}

	inline void UpdateState()
	{
		if (StatePtr != nullptr)
		{
			float pos[3] = { 0 };
			pos[0] = RealPosX;
			pos[1] = RealPosY;
			pos[2] = DrawAngle;
			StatePtr->SetValue3F(pos);
		}

		DrawPoint.x = (int)(RealPosX);
		DrawPoint.y = (int)(RealPosY);
	}

	inline void SetRotation(float angle)
	{
		DrawAngle = angle;
		UpdateState();
	}

	inline void IncrementRotation(float angle)
	{
		SetRotation(DrawAngle + angle);
	}

	inline void SetPostion(float x, float y)
	{
		RealPosX = x;
		RealPosY = y;
		UpdateState();
	}

protected: 
	PropertyData::Ptr StatePtr = nullptr;
};

extern PlayerTank::Ptr SelfPointer;
extern int PlayerTankDefID;