// SPDX-License-Identifier: GPL-2.0-or-later

#include "cone.hpp"

#include <common/types.hpp>

#include <chrono>
#include <cmath>
#include <gsl/gsl>
#include <gsl/util>

namespace iptsd::daemon {

bool Cone::alive()
{
	return this->position_update > clock::from_time_t(0);
}

void Cone::update_position(SInt32 x, SInt32 y)
{
	this->x = x;
	this->y = y;
	this->position_update = clock::now();
}

bool Cone::active()
{
	return this->position_update + std::chrono::milliseconds(300) > clock::now();
}

void Cone::update_direction(SInt32 x, SInt32 y)
{
	clock::time_point timestamp = clock::now();

	auto time_diff = timestamp - this->direction_update;
	auto ms_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);

	Float32 weight = std::exp2f(gsl::narrow_cast<Float32>(-ms_diff.count()) / 1000.0f);
	Float64 d = std::hypot(this->x - x, this->y - y);

	Float64 dx = (x - this->x) / (d + 1E-6);
	Float64 dy = (y - this->y) / (d + 1E-6);

	this->dx = weight * this->dx + dx;
	this->dy = weight * this->dy + dy;

	// Normalize cone direction vector
	d = std::hypot(this->dx, this->dy) + 1E-6;
	this->dx /= d;
	this->dy /= d;

	this->direction_update = timestamp;
}

bool Cone::check(SInt32 x, SInt32 y)
{
	if (!this->active())
		return false;

	SInt32 dx = x - this->x;
	SInt32 dy = y - this->y;
	Float64 d = std::hypot(dx, dy);

	if (d > this->distance)
		return false;

	if (dx * this->dx + dy * this->dy > this->angle * d)
		return true;

	return false;
}

} // namespace iptsd::daemon
