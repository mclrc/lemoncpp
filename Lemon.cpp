#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <fstream>
#include "Lemon.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/forwards.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CONTROLLERS 8

using namespace Lemon;


void Lemon::init(int width, int height, const char* title, float scale) {
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);

	window = SDL_CreateWindow(title,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width*scale, height*scale,
		SDL_WINDOW_ALLOW_HIGHDPI
	);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_GL_SetSwapInterval(0);

	cursor = new Vector();
	Lemon::scale = scale;
	Lemon::width = width;
	Lemon::height = height;

	int maxJoysticks = SDL_NumJoysticks();
	int controllerIndex = 0;
	
	for(int joystickIndex=0; joystickIndex < maxJoysticks; ++joystickIndex)
	{
		if (!SDL_IsGameController(joystickIndex))
		{
			continue;
		}
		if (controllerIndex >= MAX_CONTROLLERS)
		{
			break;
		}
		controllerHandles[controllerIndex] = SDL_GameControllerOpen(joystickIndex);
		controllerIndex++;
	}
	print("Initialized\n");
}
void Lemon::update() {
	SDL_Event evt;

	if (SDL_PollEvent(&evt)) {
		if (SDL_QUIT == evt.type) {
			SDL_DestroyWindow(window);
			SDL_Quit();
			shouldEnd = true;
		}
		if (SDL_KEYDOWN == evt.type) {
			keys[evt.key.keysym.scancode] = true;

		}
		if (SDL_KEYUP == evt.type) {
			keys[evt.key.keysym.scancode] = false;
		}
	}
	SDL_PumpEvents();

	int x, y;
	SDL_GetMouseState(&x, &y);

	cursor->x = x / scale + currentScene->camera->x;
	cursor->y = y / scale + currentScene->camera->y;

	currentScene->update();
}
void Lemon::draw() {
	std::sort(currentScene->entities.begin(), currentScene->entities.end(),renderSort);
	
	canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_SetRenderTarget(renderer, canvas);
	SDL_RenderClear(renderer);
	currentScene->draw();

	SDL_Rect dstrect;
	dstrect.x = 0;
	dstrect.y = 0;
	dstrect.w = width *scale;
	dstrect.h = height *scale;

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderClear(renderer);
	SDL_RenderCopyEx(renderer, canvas, NULL, &dstrect, 0, NULL, SDL_FLIP_NONE);
	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(canvas);
}

void Lemon::fixedUpdate() {
	
	currentScene->fixedUpdate();

}
void Lemon::step() {

	double timestamp = now();
	if(lastTick == 0) {
		lastFixedUpdate = timestamp;
		lastTick = timestamp;
	}
	if(timestamp - lastFixedUpdate > fixedUpdateInterval) {
		
		fixedUpdate();
		lastFixedUpdate = timestamp - (timestamp - lastFixedUpdate - fixedUpdateInterval);
	}
	double currentTimeMS = timestamp;
	
	dt = currentTimeMS - lastTick;

	lastTick = currentTimeMS;

	update();
	draw();
	std::copy(std::begin(keys), std::end(keys), std::begin(previousKeys));
}
bool Lemon::renderSort(Entity *a, Entity *b) {
	if (a->layer == b->layer) {
		return (a->y + a->height / 2) < (b->y + b->height / 2);
	}
	return a->layer < b->layer;
}
Json::Value Lemon::loadJson(const char *path) {
	Json::Value root;
	Json::Reader reader;

	std::ifstream json(path, std::ifstream::binary);
	bool readingSuccessful = reader.parse(json, root, false);

	if (readingSuccessful) {
		return root;
	}

}
//Sprite
Sprite::Sprite(Image *image, int x, int y, int width, int height) {
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->image = image;
}
void Sprite::draw(double x, double y, double angle, SDL_Point *center) {
	SDL_Rect dstrect;
	dstrect.x = x;
	dstrect.y = y;
	dstrect.w = this->width;
	dstrect.h = this->height;

	SDL_Rect srcrect;
	srcrect.x = this->x;
	srcrect.y = this->y;

	srcrect.w = this->width;
	srcrect.h = this->height;

	SDL_RenderCopyEx(renderer, image->texture, &srcrect, &dstrect, angle, center, SDL_FLIP_NONE);
}

//Sprite
Entity::Entity(double x, double y, int w, int h, Sprite* sprite, std::vector<std::string> *tags, Box *hitbox) {
	this->x = x;
	this->y = y;
	this->width = w;
	this->height = h;

	this->tags = tags;
	this->sprite = sprite;

	useCustomHitbox = false;
	_hitbox = new Box(0, 0, width, height);
}
void Entity::update() {
	onUpdate();
	input();
	updatePos();
}
void Entity::draw() {
	if (sprite == nullptr) {
		return;
	}
	sprite->draw(x - currentScene->camera->x, y - currentScene->camera->y, rotation, NULL);
}
void Entity::updatePos() {
	x += xv * (dt / 10);
	y += yv * (dt / 10);
}
bool Entity::hasTag(std::string tag) {
	return std::find(tags->begin(), tags->end(), tag) != tags->end();
}
Box Entity::hitbox() {
	if(!useCustomHitbox){
		return Box(x, y, width, height);
	}
	return Box(x + _hitbox->x, y + _hitbox->y, _hitbox->w, _hitbox->h);
}
std::string Entity::bounce(std::string tag) {
	std::string collisionSide = "none";

	for (Entity *e : currentScene->entities) {
		if (std::find(e->tags->begin(), e->tags->end(), tag) != e->tags->end()) {
			double overlapX, overlapY;

			Box other = e->hitbox();
			Box self = this->hitbox();

			Vector vec(
				(self.x+self.w/2)-(other.x+other.w/2),
				(self.y+self.h/2)-(other.y+other.h/2)
			);

			double halfWidths = self.w / 2 + other.w / 2;
			double halfHeights = self.h / 2 + other.h / 2;

			if (halfWidths > abs(vec.x)) {
				if (halfHeights > abs(vec.y)) {

					overlapX = halfWidths - abs(vec.x);
					overlapY = halfHeights - abs(vec.y);

					if (overlapY > overlapX) {
						if (vec.x > 0) {
							collisionSide = "left";
							x += overlapX;
						}
						else {
							collisionSide = "right";
							x -= overlapX;
						}
					}
					if (overlapY < overlapX) {
						if (vec.y > 0) {
							collisionSide = "top";
							y += overlapY;
						}
						else {
							collisionSide = "bottom";
							y -= overlapY;
						}
					}
					//return collisionSide;
				}
			}
		}
	}
	return collisionSide;
}
Lemon::Vector Entity::getVelAsVec() {
	return Vector(xv, yv);
}
void Entity::setTotalVelocity(double v) {
	Vector vel = Vector(xv, yv).setLength(v);
	xv = vel.x;
	yv = vel.y;
}
void Entity::addToScene(Scene *scene) {

	scene->entities.push_back(this);

}
Scene::Scene() {
	camera = new Camera(0,0,100,100);
}
void Scene::add(Entity *ent) {
	entities.push_back(ent);
}
void Scene::remove(Entity *ent) {
	std::vector<Entity*>::iterator index = std::find(entities.begin(), entities.end(), ent);
	entities.erase(index);
}
void Scene::update() {
	for (Entity* e : entities) {
		e->update();
	}
}
void Scene::fixedUpdate() {

	for (Entity* e : entities) {
		e->fixedUpdate();
	}	
}
void Scene::draw() {
	
	for (Entity* e : entities) {
		e->draw();
	}
	for (Animation* a : animations) {
		a->update();
	}
}
std::vector<Entity*> Lemon::getAll(std::string tag) {
	std::vector<Entity*> ents;
	for (Entity *e : currentScene->entities) {
		if (std::find(e->tags->begin(), e->tags->end(), tag) != e->tags->end()) {
			ents.push_back(e);
		}
	}
	return ents;
}
std::vector<Sprite*> *Lemon::loadTileset(Json::Value data) {

	std::vector<Sprite*> *tiles = new std::vector<Sprite*>;
	Image *sheet = new Image(data["image"].asCString());
	int w, h;
	SDL_QueryTexture(sheet->texture, NULL, NULL, &w, &h);

	int tilesize = data["tilewidth"].asInt();
	
	for (int y = 0; y < h / tilesize; y ++) {
		for (int x = 0; x < w / tilesize; x ++) {
			tiles->push_back(new Sprite(sheet, x*tilesize, y*tilesize, tilesize, tilesize));
		}
	}
	return tiles;
}
void Scene::loadTilemap(Json::Value data) {
	width = data["width"].asInt() - data["tilewidth"].asInt();
	height = data["height"].asInt() - data["tileheight"].asInt();

	Image *sheet = new Image(data["tilesets"][0]["image"].asCString());

	tilesize = data["tilewidth"].asInt();

	std::vector<Sprite*> *tileset = loadTileset(data["tilesets"][0]);
	
	for (Json::Value::ArrayIndex i = 0; i < data["layers"].size(); i++) {
		
		if (data["layers"][i]["type"] == "objectgroup") {

		}
		else {
			for (Json::Value::ArrayIndex t = 0; t < data["layers"][i]["data"].size(); t++) {
				if (data["layers"][i]["data"][t] == 0) {
					continue;
				}
				
				double x = (t % data["width"].asInt())*tilesize;
				double y = (floor((t) / (data["width"]).asInt()))*tilesize;
				
				Entity *e = new Entity(x, y, tilesize, tilesize);
				int spriteIndex = data["layers"][i]["data"][t].asInt() - 1;

				e->layer = floor(i);
				e->tags->push_back(std::string(data["layers"][i]["name"].asCString()));

				add(e);

				if (spriteIndex < 0) {
					continue;
				}
				e->sprite = (*tileset)[spriteIndex];
			}
		}
	}

}

Image::Image(const char *src) {
	texture = IMG_LoadTexture(renderer, src);
}
//Vector
Vector::Vector(double x, double y) {
	this->x = x;
	this->y = y;
}
double Vector::length() {
	return sqrt(x*x + y * y);
}
Vector Vector::setLength(double length) {
	double l = this->length();

	Vector newVec(x, y);

	newVec.x /= l;
	newVec.y /= l;

	newVec.x *= length;
	newVec.y *= length;

	return newVec;
}
Vector Vector::normalise() {
	return setLength(1);
}

//Animation
Animation::Animation(Sprite *sprite, int frames) {

	this->sprite = sprite;
	this->frames = frames;

	currentScene->animations.push_back(this);
}
void Animation::update() {
	ended = false;
	if (!running) {
		return;
	}
	if (clock() - lastStep>frameDuration) {
		lastStep = clock();
		currentFrame++;
		if (currentFrame > frames) {
			currentFrame = 0;
			sprite->x = 0;
			ended = true;
			return;
		}
		sprite->x += sprite->width;
	}
}
void Animation::start() {
	sprite->x = 0;
	currentFrame = 0;
	lastStep = clock();

	running = true;
}
void Animation::stop() {
	sprite->x = 0;
	currentFrame = 0;

	running = false;
}
Camera::Camera(double x, double y, double w, double h) {
	trap = new Box();
	this->x = x;
	this->y = y;
	trap->x = (width - w) / 2;
	trap->y = (height - h) / 2;
	trap->w = w;
	trap->h = h;
}
void Camera::follow(Entity *ent, double factor) {
	double entRight = (ent->x + ent->width) - x;
	double entLeft = ent->x - x;
	double entBottom = (ent->y + ent->height) - y;
	double entTop = ent->y - y;

	double trapRight = (width / 2) + (trap->w / 2);
	double trapLeft = (width / 2) - (trap->w / 2);
	double trapBottom = (height / 2) + (trap->h / 2);
	double trapTop = (height / 2) - (trap->h / 2);

	if(entRight > trapRight) {
		x += ((entRight - trapRight) * factor) * (dt / 10);
	}
	if(entLeft < trapLeft) {
		x -= ((trapLeft - entLeft) * factor) * (dt / 10);
	}
	if(entBottom > trapBottom){
		y += ((entBottom - trapBottom) * factor) * (dt / 10);
	}
	if(entTop < trapTop){
		y -= ((trapTop - entTop) * factor) * (dt / 10);
	}
}
void Camera::lookAt(Entity *ent, double factor) {
	this->x += ((((ent->x + ent->width / 2) - width / 2) - x) * factor) * (dt / 10);
	this->y += ((((ent->y + ent->height / 2) - height / 2) - y) * factor) * (dt / 10);
}
Box::Box(double x, double y, double width, double height) {
	this->x = x;
	this->y = y;
	this->w = width;
	this->h = height;
}
bool Box::collidesWith(Box *other) {
	return this->x < other->x + this->w &&
		this->x + this->w > other->x &&
		this->y < other->y + this->h &&
		this->h + this->y > other->y;
}
Box Box::move(double x, double y) {
	double newX = this->x + x;
	double newY = this->y + y;

	return Box(newX, newY, w, h);
}

double Lemon::angleBetween(Vector *a, Vector *b) {
	return (atan2(b->y - a->y, b->x - a->x) * 180 / pi) + 90;
}

double Lemon::now(){
	timeval tv;
	gettimeofday(&tv, 0);
	double currentTimeMS = (double)(tv.tv_sec * 1000 + tv.tv_usec / 1000);

	return currentTimeMS;

}

bool Lemon::getGamepadButton(SDL_GameController* controller, SDL_GameControllerButton button) {
	return (bool)SDL_GameControllerGetButton(controller, button);
}
Sint16 Lemon::getGamepadAxis(SDL_GameController* controller, SDL_GameControllerAxis axis) {
	return SDL_GameControllerGetAxis(controller, axis);
}
GamepadState Lemon::getGamepadState(int index) {

	GamepadState state;
		
	state.axis[0] = getGamepadAxis(controllerHandles[index], SDL_CONTROLLER_AXIS_LEFTX);
	state.axis[1] = getGamepadAxis(controllerHandles[index], SDL_CONTROLLER_AXIS_LEFTY);

	state.axis[2] = getGamepadAxis(controllerHandles[index], SDL_CONTROLLER_AXIS_RIGHTX);
	state.axis[3] = getGamepadAxis(controllerHandles[index], SDL_CONTROLLER_AXIS_RIGHTY);
	
	state.triggers[0] = getGamepadAxis(controllerHandles[index], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	state.triggers[1] = getGamepadAxis(controllerHandles[index], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
	
};

void Lemon::print(const char* msg) {
	std::cout << "\033[0;32m" << "[Lemon]" << msg;
}
SDL_Renderer *Lemon::renderer;
SDL_Window *Lemon::window;
SDL_Texture *Lemon::canvas = SDL_CreateTexture(Lemon::renderer, SDL_PIXELFORMAT_RGBA8888,
	SDL_TEXTUREACCESS_TARGET, Lemon::width, Lemon::height);
SDL_GameController *Lemon::controllerHandles[MAX_CONTROLLERS];
double Lemon::dt = 0;
double Lemon::lastTick;

Sint16 Lemon::maxAxis  = 32767;
bool Lemon::keys[101] = {};
bool Lemon::previousKeys[101] = {};


double Lemon::pi = 3.141592653589;
float Lemon::scale = 1;
int Lemon::width = 0;
int Lemon::height = 0;
bool Lemon::shouldEnd = false;
double Lemon::lastFixedUpdate = 0;
double Lemon::fixedUpdateInterval = 10;


Vector *Lemon::cursor;
Scene *Lemon::currentScene;