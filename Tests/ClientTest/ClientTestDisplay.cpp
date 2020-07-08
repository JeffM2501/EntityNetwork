#include "ClientTest.h"

SDL_Window* Window = nullptr;
SDL_Rect	WindowRect;
SDL_Renderer* Renderer = nullptr;
SDL_Texture* BackgroundTexture = nullptr;

SDL_Texture* BorderTexture = nullptr;

SDL_Texture* Signal3 = nullptr;
SDL_Texture* Cross = nullptr;

SDL_Texture* UserIcon = nullptr;

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

void BlitTexture(SDL_Texture* tex, int x, int y, int w, int h, SDL_Renderer* ren)
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

void BlitTextureCenter(SDL_Texture* tex, SDL_Point &pos, SDL_Renderer* ren)
{
	if (ren == nullptr)
		ren = Renderer;

	if (ren == nullptr)
		return;

	SDL_Rect dst;
	SDL_QueryTexture(tex, nullptr, nullptr, &dst.w, &dst.h);

	dst.x = pos.x - dst.w / 2;
	dst.y = pos.y - dst.h / 2;

	SDL_RenderCopyEx(ren, tex, nullptr, &dst, 0, nullptr, SDL_FLIP_NONE);
}

void BlitTextureCenter(SDL_Texture* tex, SDL_Point& pos, float angle, SDL_Renderer* ren)
{
	if (ren == nullptr)
		ren = Renderer;

	if (ren == nullptr)
		return;

	SDL_Rect dst;
	SDL_QueryTexture(tex, nullptr, nullptr, &dst.w, &dst.h);

	dst.x = pos.x - dst.w / 2;
	dst.y = pos.y - dst.h / 2;

	SDL_RenderCopyEx(ren, tex, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
}

void BlitTextureCenter(SDL_Texture* tex, int x, int y, float angle, SDL_Renderer* ren)
{
	if (ren == nullptr)
		ren = Renderer;

	if (ren == nullptr)
		return;

	SDL_Rect dst;
	SDL_QueryTexture(tex, nullptr, nullptr, &dst.w, &dst.h);

	dst.x = x - dst.w / 2;
	dst.y = y - dst.h / 2;

	SDL_RenderCopyEx(ren, tex, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
}

void DrawBG()
{
	int w, h;
	SDL_QueryTexture(BackgroundTexture, nullptr, nullptr, &w, &h);
	for (int y = 0; y < WindowRect.h; y += h)
	{
		for (int x = 0; x < WindowRect.w; x += w)
		{
			BlitTexture(BackgroundTexture, x, y);
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
	WindowRect.w = 875;
	WindowRect.h = 825;

	Window = SDL_CreateWindow("EntityNetworkClientTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowRect.w, WindowRect.h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (Window == nullptr)
		return false;

	Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (Renderer == nullptr)
		return false;

	BackgroundTexture = LoadTexture("backgrounds/tileGrass1.png");
	BorderTexture = LoadTexture("sprites/obstacles/fenceRed.png");
	Signal3 = LoadTexture("sprites/icons/1x/signal3.png");
	Cross = LoadTexture("sprites/icons/1x/cross.png");
	UserIcon = LoadTexture("sprites/icons/1x/singleplayer.png");
	return true;
}

SDL_Point MapSize;
std::map<int, SDL_Texture*> ObjectTextures;
std::vector<std::pair<int, SDL_Point>> MapObjects;

void LoadWorld(ClientEntityController::Ptr, int)
{
	MapSize.x = WorldData.GetWorldPropertyData("Width")->GetValueI();
	MapSize.y = WorldData.GetWorldPropertyData("Height")->GetValueI();

	auto map = WorldData.GetWorldPropertyData("Map")->GetValueReader();

	for (std::map<int, SDL_Texture*>::iterator itr = ObjectTextures.begin(); itr != ObjectTextures.end(); itr++)
		SDL_DestroyTexture(itr->second);
	ObjectTextures.clear();

	int count = map.ReadInt();
	for (int i = 0; i < count; i++)
		ObjectTextures[i] = LoadTexture("sprites/obstacles/" + map.ReadString());

	if (BackgroundTexture != nullptr)
		SDL_DestroyTexture(BackgroundTexture);

	BackgroundTexture = LoadTexture("backgrounds/" + map.ReadString());

	count = map.ReadInt();
	for (int i = 0; i < count; i++)
	{
		int id = map.ReadInt();
		SDL_Point p;
		p.x = map.ReadInt();
		p.y = map.ReadInt();

		MapObjects.push_back(std::pair<int, SDL_Point>(id, p));
	}
}

void DrawNetStatus()
{
	auto tx = Cross;
	if (NetConnected())
		tx = Signal3;
	
	BlitTexture(tx, WindowRect.w - 64, 0);

	int offset = 38;
	WorldData.Peers.DoForEach([&offset](int64_t& id, ClientEntityController::Ptr& peer)
		{
			BlitTexture(UserIcon, WindowRect.w - 64, offset);
			offset += 38;
		});
}

void DrawMap()
{
	if (ObjectTextures.size() == 0 || MapSize.x <= 0 || MapSize.y <= 0)
		return;

	// border
	if (BorderTexture != nullptr)
	{
		int w, h;
		SDL_QueryTexture(BackgroundTexture, nullptr, nullptr, &w, &h);

		// do the top and bottom
		for (int i = 0; i < MapSize.x; i += w)
		{
			BlitTexture(BorderTexture, i, 0);
			BlitTexture(BorderTexture, i, MapSize.y);
		}

		// do the left and right
		int count = (MapSize.y / w);

		int vOffset = MapSize.y / count;

		for (int i = vOffset; i < MapSize.y; i += w)
		{
			BlitTextureCenter(BorderTexture, h/4, i, 90, nullptr);
			BlitTextureCenter(BorderTexture, MapSize.x, i, 90, nullptr);
		}
	}

	//obstacles
	for (auto& obj : MapObjects)
	{
		auto tx = ObjectTextures[obj.first];
		BlitTextureCenter(tx, obj.second);
	}
}

std::map <std::string, SDL_Texture*> TankTextures;

void HandleEntityAdd(EntityInstance::Ptr entity)
{
	if (entity->Descriptor.Name == "PlayerTank")
	{
		PlayerTank::Ptr player = std::dynamic_pointer_cast<PlayerTank>(entity);
		std::string texture = player->Properties[0]->GetValueStr();
		if (TankTextures.find(texture) == TankTextures.end())
			TankTextures[texture] = LoadTexture("sprites/tanks/" + texture + ".png");

		player->AvatarPicture = TankTextures[texture];
	}
}

void RenderGraph()
{
	SDL_RenderClear(Renderer);
	DrawBG();
	DrawMap();
	DrawNetStatus();

	SDL_RenderPresent(Renderer);
}
