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
	return position_update > clock::from_time_t(0);
}

void Cone::update_position(Float64 rx, Float64 ry)
{
	x = rx;
	y = ry;
	position_update = clock::now();
}

bool Cone::active()
{
	return position_update + std::chrono::milliseconds(300) > clock::now();
}

void Cone::update_direction(Float64 rx, Float64 ry)
{
	clock::time_point timestamp = clock::now();

	auto time_diff = timestamp - direction_update;
	auto ms_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);

	Float32 weight = std::exp2f(gsl::narrow_cast<Float32>(-ms_diff.count()) / 1000.0f);
	Float64 d = std::hypot(x - rx, y - ry);

	Float64 drx = (rx - x) / (d + 1E-6);
	Float64 dry = (ry - y) / (d + 1E-6);

	dx = weight * dx + drx;
	dy = weight * dy + dry;

	// Normalize cone direction vector
	d = std::hypot(dx, dy) + 1E-6;
	dx /= d;
	dy /= d;

	direction_update = timestamp;
}

bool Cone::check(Float64 rx, Float64 ry)
{
	if (!active())
		return false;

    Float64 drx = rx - x;
    Float64 dry = ry - y;
	Float64 d = std::hypot(dx, dy);

	if (d > distance)
		return false;

	if (drx * dx + dry * dy > angle * d)
		return true;

	return false;
}

} // namespace iptsd::daemon
