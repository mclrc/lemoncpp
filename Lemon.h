#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/forwards.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CONTROLLERS 8

namespace Lemon {
	
	struct Animation;
	struct Scene;
	struct Vector;
	struct Sprite;
	struct Entity;
	struct Box;
	struct Camera;
	struct Image;

	extern SDL_Renderer *renderer;
	extern SDL_Window *window;
	extern SDL_Texture *canvas;
	extern SDL_GameController *controllerHandles[MAX_CONTROLLERS];
	extern Sint16 maxAxis;

	extern bool keys[101];
	extern bool previousKeys[101];

	extern double dt;
	extern double lastTick;
	extern double lastFixedUpdate;
	extern double fixedUpdateInterval;

	extern float scale;
	extern int width;
	extern int height;
	extern double pi;

	extern bool shouldEnd;

	void init(int width = 600, int height = 600, const char* title = "Game", float scale = 1);
	void update();
	void draw();
	void fixedUpdate();
	void step();
	bool renderSort(Entity *a, Entity *b);
	Json::Value loadJson(const char *path);
	std::vector<Sprite*> *loadTileset(Json::Value data);
	std::vector<Entity*> getAll(std::string tag);

	extern Vector *cursor;
	extern Scene* currentScene;

	bool getGamepadButton(SDL_GameController*, SDL_GameControllerButton button);
	Sint16 getGamepadAxis(SDL_GameController*, SDL_GameControllerAxis);

	struct GamepadState {
		Sint16 axis[4];
		Sint16 triggers[2];
		bool buttons[14];
	};
	GamepadState getGamepadState(int index);
	struct Sprite {
	
		int x; int y;
		int width; int height;

		SDL_Texture *sprite;
		Image *image;
		Sprite(Image *image = NULL, int x = 0, int y = 0, int width = 0, int height = 0);
		void draw(double x, double y, double angle = 0, SDL_Point *center = NULL);
	};
	struct Entity {
	
		double x; double y;
		double xv = 0; double yv = 0;
		int layer = 1;
		int rotation = 0;
		int width; int height;
		Sprite *sprite;
		Animation *animation;
		std::vector<std::string> *tags;
		Box *_hitbox;

		Entity(double x = 0, double y = 0, int w = 1, int h = 1, Sprite* sprite = nullptr, std::vector<std::string> *tags = new std::vector<std::string>(1, "Entity"), Box *hitbox = NULL);
		virtual void update();
		virtual void draw();
		virtual void fixedUpdate() {};
		virtual void updatePos();
		virtual void onUpdate() {};
		virtual void input() {};
		virtual void init() {};
		virtual void setTotalVelocity(double v);
		virtual void addToScene(Lemon::Scene *scene);
		virtual Lemon::Vector getVelAsVec();
		virtual bool hasTag(std::string tag);
		virtual std::string bounce(std::string tag);
		virtual Box hitbox();
		bool useCustomHitbox = false;
	};
	struct Vector {

		double x, y;

		Vector(double x = 0, double y = 0);
		double length();
		Vector setLength(double length);
		Vector normalise();
	};
	struct Scene {
	
		Scene();
		std::vector<Entity*> entities;
		std::vector<Animation*> animations;
		Camera *camera;
		int tilesize = 16;
		void add(Lemon::Entity *ent);
		void remove(Lemon::Entity *ent);
		int width, height;
		void update();
		virtual void draw();
		virtual void fixedUpdate();
		void loadTilemap(Json::Value data);
	};
	struct Box {
	
		double x, y, w, h;
		Box(double x = 0, double y = 0, double width = 1, double height = 1);
		bool collidesWith(Box *other);
		Box move(double x, double y);
	};
	struct Camera {
	
		double x, y;
		Box *trap;
		Camera(double x, double y, double w, double h);
		void follow(Entity *ent, double factor = 1);
		void lookAt(Entity *ent, double factor = 1);
	};
	struct Animation {
	
		Animation(Sprite *sprite, int frames);
		virtual void update();
		virtual void start();
		virtual void stop();
		int frames;
		Sprite *sprite;
		int frameDuration = 120;
		double lastStep = 0;
		int currentFrame = 0;
		bool running = false;
		bool ended = false;
	};
	struct Image {
	
		SDL_Texture *texture;
		const char *src;
		Image(const char *src);
	};
	std::vector<Sprite*> *createTileset(Image *sheet, int tilewidth = 16, int tileheight = 16);
	double angleBetween(Lemon::Vector *a, Lemon::Vector *b);
	double now();
	void print(const char* msg);
}