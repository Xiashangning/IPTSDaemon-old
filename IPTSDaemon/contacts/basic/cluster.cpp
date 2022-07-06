
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cluster.hpp"

#include "heatmap.hpp"

#include <common/types.hpp>

#include <cmath>
#include <cstddef>
#include <gsl/util>
#include <iterator>
#include <memory>
#include <tuple>

namespace iptsd::contacts::basic {

Cluster::Cluster(Heatmap &hm, index2_t center)
{
	check(hm, center);
}

void Cluster::add(index2_t pos, Float32 val)
{
	Float32 px = gsl::narrow_cast<Float32>(pos.x);
	Float32 py = gsl::narrow_cast<Float32>(pos.y);

	x += val * px;
	y += val * py;
	xx += val * px * px;
	yy += val * py * py;
	xy += val * px * py;
	w += val;

	if (max_v < val)
		max_v = val;
}

math::Vec2<Float32> Cluster::mean()
{
	return math::Vec2<Float32> {x / w, y / w};
}

math::Mat2s<Float32> Cluster::cov()
{
	Float32 r1 = (xx - (x * x / w)) / w;
	Float32 r2 = (yy - (y * y / w)) / w;
	Float32 r3 = (xy - (x * y / w)) / w;

	return math::Mat2s<Float32> {r1, r3, r2};
}

void Cluster::check(Heatmap &hm, index2_t pos)
{
	Float32 v = hm.value(pos);

	if (hm.get_visited(pos))
		return;

	add(pos, v);
	hm.set_visited(pos, true);

	check(hm, index2_t {pos.x + 1, pos.y});
	check(hm, index2_t {pos.x - 1, pos.y});
	check(hm, index2_t {pos.x, pos.y + 1});
	check(hm, index2_t {pos.x, pos.y - 1});
}

} // namespace iptsd::contacts::basic
