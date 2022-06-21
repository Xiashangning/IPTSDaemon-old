// SPDX-License-Identifier: GPL-2.0-or-later

#include "touch-manager.hpp"

#include "config.hpp"
#include "devices.hpp"

#include <common/types.hpp>
#include <common/cerror.hpp>
#include <contacts/processor.hpp>
#include <container/image.hpp>
#include <math/mat2.hpp>
#include <math/vec2.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <gsl/gsl>
#include <iterator>
#include <memory>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

namespace iptsd::daemon {

TouchManager::TouchManager(const Config &conf)
	: size(), conf(conf), max_contacts(conf.info.max_contacts), inputs(max_contacts),
	  last(max_contacts), distances(max_contacts * max_contacts)
{
	for (SInt32 i = 0; i < max_contacts; i++) {
		last[i].index = i;
		last[i].active = false;
	}

	processor.advanced = conf.touch_advanced;
	processor.conf.basic_pressure = conf.basic_pressure;
}

std::vector<TouchInput> &TouchManager::process(const Heatmap &data)
{
	processor.resize(index2_t {data.width, data.height});

	std::transform(data.data.begin(), data.data.end(), processor.hm().begin(),
		       [&](auto v) {
			       Float32 val = static_cast<Float32>(v - data.z_min) /
					 static_cast<Float32>(data.z_max - data.z_min);

			       return 1.0f - val;
		       });

	const std::vector<contacts::TouchPoint> &contacts = processor.process();

	UInt8 max_contacts = conf.info.max_contacts;
	UInt8 count = std::min(gsl::narrow_cast<UInt8>(contacts.size()), max_contacts);
    UInt8 actual_cnt = count;
    UInt8 palm_cnt = 0;

	for (int i = 0; i < actual_cnt; i++) {
		Float64 x = contacts[i+palm_cnt].mean.x;
		Float64 y = contacts[i+palm_cnt].mean.y;
		if (conf.invert_x)
			x = 1 - x;
		if (conf.invert_y)
			y = 1 - y;
        
        TouchInput *in;
        if (contacts[i+palm_cnt].palm) {
            i--;
            palm_cnt++;
            in = &inputs[--actual_cnt];
            in->palm = true;
        } else {
            in = &inputs[i];
            in->palm = false;
        }

        in->x = x;
        in->y = y;

		math::Eigen2<Float32> eigen = contacts[i+palm_cnt].cov.eigen();
		Float64 s1 = std::sqrt(eigen.w[0]);
		Float64 s2 = std::sqrt(eigen.w[1]);

		Float64 d1 = 4 * s1 / data.diagonal;
		Float64 d2 = 4 * s2 / data.diagonal;

        in->major = std::max(d1, d2);
        in->minor = std::min(d1, d2);

		math::Vec2<Float64> v1 = eigen.v[0].cast<Float64>() * s1;
		Float64 angle = M_PI_2 - std::atan2(v1.x, v1.y);
		// Make sure that the angle is always a positive number
		if (angle < 0)
			angle += M_PI;
		if (angle > M_PI)
			angle -= M_PI;
		in->orientation = gsl::narrow_cast<UInt16>(angle / M_PI * 180);

		in->ev1 = eigen.w[0];
		in->ev2 = eigen.w[1];

		in->index = i;
		in->active = true;
        in->tracked = false;
        in->instability = 0;
	}

	for (int i = count; i < max_contacts; i++) {
		inputs[i].index = i;
        inputs[i].palm = false;
		inputs[i].active = false;
        inputs[i].tracked = false;
        inputs[i].instability = 0;
		inputs[i].ev1 = 0;
		inputs[i].ev2 = 0;
	}

    if (touching)
        track(actual_cnt);

	if (conf.stylus_cone) {
		// Update touch rejection cones
		for (int i = 0; i < count; i++) {
			if (!inputs[i].palm)
				continue;

			update_cones(inputs[i]);
		}

		// Check if any contacts fall into the cones
		for (int i = 0; i < actual_cnt; i++) {
			if (inputs[i].palm)
				continue;

			inputs[i].palm = check_cones(inputs[i]);
            if (inputs[i].palm) {
                if (i != actual_cnt - 1)
                    std::swap(inputs[i], inputs[actual_cnt-1]);
                actual_cnt--;
            }
		}
	}
    
    touching = actual_cnt > 0;

	std::swap(inputs, last);
    last_touch_cnt = actual_cnt;
	return last;
}

void TouchManager::track(UInt8 &touch_cnt)
{
    // Delete instable touch inputs
    for (int j = 0; j < last_touch_cnt; j++) {
        if (last[j].instability >= IPTS_TOUCH_INSTABILITY_THRESH) {
            if (j != last_touch_cnt - 1) {
                std::swap(last[j], last[last_touch_cnt-1]);
                j--;
            }
            last_touch_cnt--;
        }
    }
	// Calculate the distances between current and previous valid touch inputs
	for (int i = 0; i < touch_cnt; i++) {
		for (int j = 0; j < last_touch_cnt; j++) {
			const TouchInput &in = inputs[i];
			const TouchInput &last_in = last[j];

			Float64 dx = 100 * (in.x - last_in.x);
			Float64 dy = 100 * (in.y - last_in.y);

            int idx = i * last_touch_cnt + j;
            distances[idx] = std::sqrt(dx * dx + dy * dy);
		}
	}

	// Select the smallest calculated distance to find the closest two inputs.
	// Copy the index from the previous to the current input. Then invalidate
	// all distance entries that contain the two inputs.
    int count = std::min(touch_cnt, last_touch_cnt);
    UInt16 index_used = 0;
	for (int k = 0; k < count; k++) {
        auto end = distances.begin();
        std::advance(end, touch_cnt * last_touch_cnt);
		auto it = std::min_element(distances.begin(), end);
		int idx = (UInt32)std::distance(distances.begin(), it);

		int i = idx / last_touch_cnt;
		int j = idx % last_touch_cnt;

        TouchInput &in = inputs[i];
        TouchInput &last_in = last[j];
        in.tracked = true;
		in.index = last_in.index;
        index_used |= 1 << in.index;
        in.instability = last_in.instability;

        if (conf.touch_stability) {
            Float32 dev1 = in.ev1 - last_in.ev1;
            Float32 dev2 = in.ev2 - last_in.ev2;
            if (dev1 < conf.stability_threshold && dev2 < conf.stability_threshold)
                in.instability = 0;
            else
                in.instability++;
        }

		// Set the distance of all pairs that contain one of
        // i and j to something much higher.
		// This prevents i and j from getting selected again.
		for (int x = 0; x < last_touch_cnt; x++) {
			int idx1 = i * last_touch_cnt + x;
			distances[idx1] = 1 << 30;
		}
        for (int x = 0; x < touch_cnt; x++) {
            int idx2 = x * last_touch_cnt + j;
            distances[idx2] = 1 << 30;
        }
	}
    
    if (touch_cnt > last_touch_cnt) {
        for (int i = 0; i < touch_cnt; i++) {
            if (!inputs[i].tracked) {
                int index = 0;
                while (index_used & (1 << index))
                    index++;
                inputs[i].index = index;
                index_used |= 1 << index;
            }
        }
    } else if (touch_cnt < last_touch_cnt) {    // Some finger lifted
        for (int j = 0; j < last_touch_cnt; j++) {
            if (!(index_used & (1 << last[j].index))) {
                for (int i = touch_cnt; i < max_contacts; i++) {
                    if (!inputs[i].active) {
                        if (i != touch_cnt)
                            std::swap(inputs[touch_cnt], inputs[i]);
                        inputs[touch_cnt] = last[j];
                        inputs[touch_cnt].instability++;
                        touch_cnt++;
                        break;
                    }
                }
            }
        }
    }
}

void TouchManager::update_cones(const TouchInput &palm)
{
	std::shared_ptr<Cone> cone {nullptr};
	Float64 d = INFINITY;

	// find closest cone (by center)
	for (auto &current : cones) {
		// This cone has never seen a position update, so its inactive
		if (!current->alive())
			continue;

		if (!current->active())
			continue;

		Float64 current_d = std::hypot(current->x - palm.x, current->y - palm.y);
		if (current_d < d) {
			d = current_d;
			cone = current;
		}
	}

	if (!cone)
		return;

	cone->update_direction(palm.x, palm.y);
}

bool TouchManager::check_cones(const TouchInput &input)
{
	for (const auto &cone : cones) {
		if (cone->check(input.x, input.y))
			return true;
	}

	return false;
}

} // namespace iptsd::daemon
