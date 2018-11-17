#ifndef __J1ENTITY_H__
#define __J1ENTITY_H__

#include "j1Animation.h"
#include "p2Point.h"
#include "p2DynArray.h"

struct Collider;

enum class EntityType {
	PLAYER,
	ENEMY,
	FLIER,
	COUNT
};

enum EntityState {
	NO_STATE = -1,
	IDLE,
	MOVING,
	JUMPING,
	DEAD,
	CHARGE,
	WIN,
	GOD,
};


class j1Entity
{
protected:
	EntityType type;
	
	p2SString sprite_route;
	SDL_Texture* sprite;
	
	float movement_speed = 0.0F;
	float jump_speed = 0.0F;
	float gravity = 0.0F;
	float acceleration = 0.0F;
	float fall_speed = 0.0F;
	fPoint target_speed = { 0.0F, 0.0F };

	int collider_offset = 0;
	bool flipX = true;
	bool is_grounded = true;

	Animation* animations;
	SDL_Rect animation_frame;
	unsigned int jump_fx;

	virtual void StepX(float dt);
	virtual void StepY(float dt);
	virtual void IdleUpdate();
	virtual void MovingUpdate();
	virtual void JumpingUpdate();
	virtual void Jump();
	virtual void LoadAnimations(pugi::xml_node conf);

public:
	fPoint position = { 0.0F, 0.0F };
	fPoint velocity = { 0.0F, 0.0F };
	Collider * collider;
	EntityState state = IDLE;
	float threshold = 0.0F;
	
	j1Entity(EntityType type, pugi::xml_node config, fPoint position);
	~j1Entity();
	virtual bool Awake();
	virtual bool Start();
	virtual bool Update(float dt);
	virtual bool PreUpdate();
	bool CleanUp();
	virtual bool Load(pugi::xml_node&);
	virtual bool Save(pugi::xml_node&) const;

	virtual void CheckDeath();
	virtual void SetPosition(float x, float y);
};

#endif