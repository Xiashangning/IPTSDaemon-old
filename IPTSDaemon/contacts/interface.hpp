/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_CONTACTS_INTERFACE_HPP
#define IPTSD_CONTACTS_INTERFACE_HPP

#include "eval/perf.hpp"

#include <common/types.hpp>
#include <container/image.hpp>
#include <math/mat2.hpp>
#include <math/vec2.hpp>

#include <vector>

namespace iptsd::contacts {

struct TouchPoint {
	Float32 confidence;
	Float32 scale;
	bool palm;
	math::Vec2<Float32> mean;
	math::Mat2s<Float32> cov;
};

struct Config {
	index2_t size {};
	Float32 basic_pressure = 0;
};

class ITouchProcessor {
public:
	virtual ~ITouchProcessor() = default;

	virtual container::Image<Float32> &hm() = 0;
	virtual const std::vector<TouchPoint> &process() = 0;

	[[nodiscard]] virtual const eval::perf::Registry &perf() const = 0;
};

} /* namespace iptsd::contacts */

#endif /* IPTSD_CONTACTS_INTERFACE_HPP */
