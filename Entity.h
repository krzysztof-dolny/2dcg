#pragma once
#include "EntityTile.h"
#include "Map.h"
#include <vector>

class Entity
{
protected:
	std::vector<EntityTile> _body;
	std::vector<Position> _collidingPositions;

public:
	Entity(std::vector<EntityTile> body);
	std::vector<Position> GetCollidingPositions() const;
	void SetCollidingPositions();
	bool CollidingWith(const std::vector<Position>& positions) const;
	bool CollidingWith(const Entity* entity) const;
	bool CollidingWith(const Map* map) const; // colliding with colliding positions in map
	std::vector<EntityTile> GetBody() const;
};

