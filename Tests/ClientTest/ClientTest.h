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
	uint32_t LastTickCount;
	uint32_t DesiredFPS;

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
	PlayerTank(const EntityDesc& desc) : EntityInstance(desc), AvatarPicture(nullptr){};

	SDL_Texture* AvatarPicture = nullptr;
	SDL_Point DrawPoint;

	typedef std::shared_ptr<PlayerTank> Ptr;
};