/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_CONTACTS_BASIC_CLUSTER_HPP
#define IPTSD_CONTACTS_BASIC_CLUSTER_HPP

#include "heatmap.hpp"

#include <common/types.hpp>
#include <math/mat2.hpp>
#include <math/vec2.hpp>

#include <cstddef>
#include <tuple>
#include <vector>

namespace iptsd::contacts::basic {

class Cluster {
public:
	Float32 x = 0;
	Float32 y = 0;
	Float32 xx = 0;
	Float32 yy = 0;
	Float32 xy = 0;
	Float32 w = 0;
	Float32 max_v = 0;

	Cluster(Heatmap &hm, index2_t center);

	void add(index2_t pos, Float32 val);

	math::Vec2<Float32> mean();
	math::Mat2s<Float32> cov();

private:
	void check(Heatmap &hm, index2_t pos);
};

} /* namespace iptsd::contacts::basic */

#endif /* IPTSD_CONTACTS_BASIC_CLUSTER_HPP */
