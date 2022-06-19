/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_DAEMON_CONE_HPP
#define IPTSD_DAEMON_CONE_HPP

#include <common/types.hpp>
#include <math/num.hpp>

#include <chrono>

namespace iptsd::daemon {

class Cone {
public:
	using clock = std::chrono::system_clock;

	clock::time_point position_update {};
	clock::time_point direction_update {};

	SInt32 x = 0;
	SInt32 y = 0;
	Float64 dx = 0;
	Float64 dy = 0;

	Float32 angle;
	Float32 distance;

	Cone(Float32 angle, Float32 distance)
		: angle {std::cos(angle / 180 * math::num<Float32>::pi)}, distance {distance} {};

	bool alive();
	bool active();

	void update_position(SInt32 x, SInt32 y);
	void update_direction(SInt32 x, SInt32 y);

	bool check(SInt32 x, SInt32 y);
};

} /* namespace iptsd::daemon */

#endif /* IPTSD_DAEMON_CONE_HPP */
