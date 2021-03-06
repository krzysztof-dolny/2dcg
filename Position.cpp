#include "Position.h"

bool Position::operator==(Position const& a) const
{
	return (this->x == a.x and this->y == a.y);
}

Position Position::operator+(Position const& a)
{
	return { this->x + a.x, this->y + a.y };
}

Position& Position::operator+=(Position const& a)
{
	this->x += a.x;
	this->y += a.y;
	return *this;
}

Position Position::operator-(void)
{
	return { -this->x, -this->y };
}

Position Position::operator-(Position const& a)
{
	return { this->x - a.x, this->y - a.y };
}

bool Position::operator>(Position const& a) const
{
	return (((this->x * this->x) + (this->y * this->y)) > ((a.x * a.x) + (a.y * a.y)));
}

bool Position::operator<(Position const& a) const
{
	return (((this->x * this->x) + (this->y * this->y)) < ((a.x * a.x) + (a.y * a.y)));
}

bool Position::operator!=(Position const& a) const
{
	return ((this->x != a.x) or (this->y != a.y));
}