#include "j1Render.h"
#include "j1Textures.h"
#include "j1App.h"
#include "j1Collision.h"
#include "p2Log.h"
#include "j1Pathfinding.h"
#include "j1Input.h"
#include "j1Map.h"
#include "j1Enemy.h"
#include "j1EntityManager.h"


j1Enemy::j1Enemy(EntityType type, pugi::xml_node config, fPoint position, p2SString id) : j1Entity(type, config, position, id)
{
	animations = new Animation[TOTAL_ANIMATIONS];
	LoadAnimations(config);

	collider = App->collision->AddCollider(animation_frame, COLLIDER_ENEMY, App->entitymanager, true);
	collider->rect.x = position.x;
	collider->rect.y = position.y + collider_offset;
}


j1Enemy::~j1Enemy()
{}

bool j1Enemy::Awake()
{

	return true;
}

bool j1Enemy::Start()
{
	
	return true;
}

bool j1Enemy::Update(float dt)
{
	if (chase) 
	{
		GetPath();
		if(App->entitymanager->draw_path) DrawPath();
	}
	else
	{
		current_path.Clear();
		moving_right = false;
		moving_left = false;
		jump = false;
	}

	if (state == JUMPING)
	{
		target_speed.y += gravity*dt;
		if (target_speed.y > fall_speed) target_speed.y = fall_speed; //limit falling speed
	}

	velocity = (target_speed * acceleration + velocity * (1 - acceleration))*dt;
	StepY(dt);
	StepX(dt);
	CheckDeath();

	animation_frame = animations[IDLE].GetCurrentFrame();
	App->render->Blit(sprite, position.x, position.y, &animation_frame, 1.0f, flipX);

	return true;
}

bool j1Enemy::PreUpdate()
{
	if (position.DistanceManhattan(App->entitymanager->player->position) < MINIMUM_DISTANCE) 
		chase = true;
	else 
		chase = false;

	if (current_path.Count() > 0)
	{
		moving_right = false;
		moving_left = false;
		jump = false;

		reached_X = (current_path.At(previous_destination)->x <= current_path.At(current_destination)->x  && current_path.At(current_destination)->x <= position.x)
			|| (current_path.At(previous_destination)->x >= current_path.At(current_destination)->x && current_path.At(current_destination)->x >= position.x);

		reached_Y = (current_path.At(previous_destination)->y <= current_path.At(current_destination)->y && position.y >= current_path.At(current_destination)->y)
			|| (current_path.At(previous_destination)->y >= current_path.At(current_destination)->y && position.y <= current_path.At(current_destination)->y);


		if (!reached_X)
		{
			if (position.x < current_path.At(current_destination)->x)
				moving_right = true;
			else if (position.x > current_path.At(current_destination)->x)
				moving_left = true;
		}

		if (!reached_Y)
		{
			if (position.y > current_path.At(current_destination)->y)
				jump = true;
		}

		if (reached_X && reached_Y)
		{
			previous_destination = current_destination;
			current_destination++;
			next_destination = current_destination + 1;

			if (next_destination >= current_path.Count())
				next_destination = -1;

			if (current_destination >= current_path.Count())
				current_path.Clear();
		}
	}

	switch (state) {
	case IDLE: IdleUpdate();
		break;
	case MOVING: MovingUpdate();
		break;
	case JUMPING: JumpingUpdate();
		break;
	case DEAD:
		animation_frame = animations[DEAD].GetCurrentFrame();
		break;
	default:
		break;
	}

	return true;
}

bool j1Enemy::Load(pugi::xml_node &conf)
{
	j1Entity::Load(conf);
	state = (EntityState) conf.child("state").attribute("value").as_int();
	chase = conf.child("chase").attribute("value").as_bool();
	moving_right = conf.child("movement_controls").attribute("moving_right").as_bool();
	moving_left = conf.child("movement_controls").attribute("moving_left").as_bool();
	jump = conf.child("movement_controls").attribute("jump").as_bool();

	current_path.Clear();
	return true;
}

bool j1Enemy::Save(pugi::xml_node &conf) const
{
	j1Entity::Save(conf);
	conf.append_child("state").append_attribute("value") = state;
	conf.append_child("chase").append_attribute("value") = chase;
	conf.append_child("movement_controls").append_attribute("moving_right") = moving_right;
	conf.append_child("movement_controls").append_attribute("moving_left") = moving_left;
	conf.append_child("movement_controls").append_attribute("jump") = jump;
	return true;
}

void j1Enemy::StepX(float dt)
{
	if (velocity.x > 0) 
		velocity.x = MIN(velocity.x, App->collision->DistanceToRightCollider(collider)); //movement of the player is min between distance to collider or his velocity
	else if (velocity.x < 0)
		velocity.x = MAX(velocity.x, App->collision->DistanceToLeftCollider(collider)); //movement of the player is max between distance to collider or his velocity

	if (fabs(velocity.x) < threshold) 
		velocity.x = 0.0F;

	position.x += velocity.x;
	collider->rect.x = position.x;
}

void j1Enemy::StepY(float dt)
{
	if (velocity.y < 0)
	{
		velocity.y = MAX(velocity.y, App->collision->DistanceToTopCollider(collider)); //movement of the player is max between distance to collider or his velocity
		if (velocity.y == 0) 
			target_speed.y = 0.0F;
	}
	else
	{
		float distance = App->collision->DistanceToBottomCollider(collider);
		velocity.y = MIN(velocity.y, distance); //movement of the player is min between distance to collider or his velocity
		is_grounded = (distance == 0) ? true : false;
	}

	if (fabs(velocity.y) < threshold)
		velocity.y = 0.0F;

	position.y += velocity.y;
	collider->rect.y = position.y + collider_offset;
}

void j1Enemy::IdleUpdate()
{
	target_speed.x = 0.0F;
	if (moving_left != moving_right) 
		state = MOVING;
	if (jump) Jump();

	if (!is_grounded) state = JUMPING;
}

void j1Enemy::MovingUpdate()
{
	if (moving_left == moving_right)
	{
		state = IDLE;
		target_speed.x = 0.0F;
	}
	else if (moving_right)
	{
		target_speed.x = movement_speed;
		flipX = false;
	}
	else if (moving_left)
	{
		target_speed.x = -movement_speed;
		flipX = true;
	}

	if (jump)
	{
		Jump();
	}

	if (!is_grounded) 
		state = JUMPING;
}

void j1Enemy::JumpingUpdate()
{
	if (moving_left == moving_right)
	{
		target_speed.x = 0.0F;
	}
	else if (moving_right)
	{
		target_speed.x = movement_speed;
		flipX = false;
	}
	else if (moving_left)
	{
		target_speed.x = -movement_speed;
		flipX = true;
	}

	if (is_grounded)
	{
		if (moving_left == moving_right) state = IDLE;
		else state = MOVING;

		target_speed.y = 0.0F;
		velocity.y = 0.0F;
	}
}

void j1Enemy::Jump()
{
	target_speed.y = -jump_speed;
	is_grounded = false;
	state = JUMPING;
}

bool j1Enemy::GetPath()
{
	iPoint origin = App->map->WorldToMap(position.x, position.y);
	iPoint destination = App->map->WorldToMap(App->entitymanager->player->position.x, App->entitymanager->player->position.y);

 	App->pathfinding->CreatePath(origin, destination, 5, 5, jump_height);

	const p2DynArray<iPoint>* tmp_array = App->pathfinding->GetLastPath();
	current_path.Clear();
	for (int i = 0; i < tmp_array->Count(); i++)
	{
		iPoint p = App->map->MapToWorld(tmp_array->At(i)->x, tmp_array->At(i)->y);
		p.x += App->map->data.tile_width / 2;
		p.y += App->map->data.tile_height / 2;
		current_path.PushBack(p);
	}
	current_destination = current_path.Count() > 1 ? 1 : 0;
	previous_destination = 0;
	next_destination = current_path.Count() > 2 ? 2:-1;

	moving_right = false;
	moving_left = false;
	jump = false;

	return true;
}

void j1Enemy::DrawPath()
{
	for (int i = 0; i < current_path.Count(); i++)
	{
		iPoint p = { current_path.At(i)->x, current_path.At(i)->y };
		p.x -= App->map->data.tile_width / 2;
		p.y -= App->map->data.tile_height / 2;

		SDL_Rect quad = { p.x, p.y, App->map->data.tile_width , App->map->data.tile_height };
		App->render->DrawQuad(quad, 255, 255, 0, 75, true);
	}
}