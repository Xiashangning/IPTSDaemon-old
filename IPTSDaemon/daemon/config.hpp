/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_DAEMON_CONFIG_HPP
#define IPTSD_DAEMON_CONFIG_HPP

#include "../../IPTSKenerlUserShared.h"

#include <common/types.hpp>

#include <optional>
#include <string>

namespace iptsd::daemon {

class Config {
public:
	bool invert_x = false;
	bool invert_y = false;

	SInt32 width = 0;
	SInt32 height = 0;

	bool stylus_cone = true;
	bool stylus_disable_touch = false;

	bool touch_stability = true;
	bool touch_advanced = false;
	bool touch_disable_on_palm = false;

	Float32 basic_pressure = 0.04;

	Float32 cone_angle = 30;
	Float32 cone_distance = 1600;

	Float32 stability_threshold = 0.1;

	IPTSDeviceInfo info;

	Config(IPTSDeviceInfo info);

private:
	void load_dir(const std::string &name);
};

} /* namespace iptsd::daemon */

#endif /* IPTSD_DAEMON_CONFIG_HPP */
