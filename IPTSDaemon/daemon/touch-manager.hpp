/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_DAEMON_TOUCH_MANAGER_HPP
#define IPTSD_DAEMON_TOUCH_MANAGER_HPP

#include "cone.hpp"
#include "config.hpp"

#include <common/types.hpp>
#include <contacts/processor.hpp>
#include <container/image.hpp>

#include <memory>
#include <vector>

namespace iptsd::daemon {

#define IPTS_TOUCH_INSTABILITY_THRESH   3

class TouchInput {
public:
    // x, y, major, minor are in in range 0-1
	Float64 x = 0;
    Float64 y = 0;
	Float64 major = 0;
	Float64 minor = 0;
	UInt16  orientation = 0;
	UInt8   index = 0;
	bool palm = false;
	bool active = false;
    bool tracked = false;
    UInt8 instability = 0;

	Float32 ev1 = 0;
	Float32 ev2 = 0;
};

class Heatmap;

class TouchManager {
public:
	index2_t size;
	contacts::TouchProcessor processor;

	const Config &conf;
	const UInt8 max_contacts;

    bool touching = false;
	std::vector<TouchInput> inputs;

	std::vector<TouchInput> last;
    UInt8 last_touch_cnt;
	std::vector<Float64> distances;

	std::vector<std::shared_ptr<Cone>> cones;

	TouchManager(const Config &conf);

	std::vector<TouchInput> &process(const Heatmap &data);

private:
	void track(UInt8 &touch_cnt);
	void update_cones(const TouchInput &palm);
	bool check_cones(const TouchInput &input);
};

} /* namespace iptsd::daemon */

#endif /* IPTSD_DAEMON_TOUCH_MANAGER_HPP */
