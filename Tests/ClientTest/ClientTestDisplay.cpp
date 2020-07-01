#include "ClientTest.h"

SDL_Window* Window = nullptr;
SDL_Rect	WindowRect;
SDL_Renderer* Renderer = nullptr;
SDL_Texture* bg = nullptr;


SDL_Texture* LoadTexture(const std::string& file, SDL_Renderer* ren)
{
	if (ren == nullptr)
		ren = Renderer;

	//Initialize to nullptr to avoid dangling pointer issues
	SDL_Texture* texture = IMG_LoadTexture(ren, getResourcePath(file).c_str());

	return texture;
}

void BlitTexture(SDL_Texture* tex, int x, int y, SDL_Renderer* ren)
{
	if (ren == nullptr)
		ren = Renderer;

	if (ren == nullptr)
		return;

	//Setup the destination rectangle to be at the position we want
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;

	//Query the texture to get its width and height to use
	SDL_QueryTexture(tex, nullptr, nullptr, &dst.w, &dst.h);
	SDL_RenderCopy(ren, tex, nullptr, &dst);
}

void BlitTexture(SDL_Texture* tex, int x, int y, int w, int h, SDL_Renderer* ren )
{
	if (ren == nullptr)
		ren = Renderer;

	if (ren == nullptr)
		return;

	//Setup the destination rectangle to be at the position we want
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	dst.w = w;
	dst.h = h;
	SDL_RenderCopyEx(ren, tex, nullptr, &dst, 0, nullptr, SDL_FLIP_NONE);
}

void BlitTexture(SDL_Texture* tex, SDL_Rect& dest, SDL_Renderer* ren)
{
	if (ren == nullptr)
		ren = Renderer;

	if (ren == nullptr)
		return;
	SDL_RenderCopyEx(ren, tex, nullptr, &dest, 0, nullptr, SDL_FLIP_NONE);
}

void DrawBG()
{
	int w, h;
	SDL_QueryTexture(bg, nullptr, nullptr, &w, &h);
	for (int y = 0; y < WindowRect.h; y += h)
	{
		for (int x = 0; x < WindowRect.w; x += w)
		{
			BlitTexture(bg, x, y);
		}
	}
	//BlitTexture(bg, WindowRect);
}

bool InitGraph()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return false;

	WindowRect.x = 0;
	WindowRect.y = 0;
	WindowRect.w = 800;
	WindowRect.h = 800;

	Window = SDL_CreateWindow("EntityNetworkClientTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowRect.w, WindowRect.h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (Window == nullptr)
		return false;

	Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (Renderer == nullptr)
		return false;

	bg = LoadTexture("backgrounds/tileGrass1.png");

	return true;
}

void RenderGraph()
{
	SDL_RenderClear(Renderer);
	DrawBG();
	SDL_RenderPresent(Renderer);
}
