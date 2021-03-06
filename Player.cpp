#include "Player.h"
//#include "Level.h"
Player::Player(const std::vector<EntityTile>& body, const int& maxHp, const int& jumpHeight) : Entity(body)
{
	_maxHp = maxHp;
	_currentHp = _maxHp;
	_dead = false;
	_direction = { 0,0 };
	_jumpHeight = jumpHeight;
}

Player::~Player() {}

void Player::LoseHp(const int& amount)
{
	_currentHp -= amount;
	if (_currentHp <= 0)
	{
		Die();
	}
}

int Player::Hp() const
{
	return _currentHp;
}

void Player::Die()
{
	_dead = true;
}

bool Player::Dead() const
{
	return _dead;
}

void Player::SetDirection(const Position& direction)
{
	_direction = direction;
}

Position Player::GetDirection() const
{
	return _direction;
}

int Player::MaxHp() const
{
	return _maxHp;
}

void Player::Update()
{
	for (EntityTile& tile : _body)
	{
		tile.SetPosition(tile.GetPosition() + _direction);
		SetCollidingPositions();
	}
	_direction = { 0,0 };
}

int Player::GetJumpHeight() const
{
	return _jumpHeight;
}