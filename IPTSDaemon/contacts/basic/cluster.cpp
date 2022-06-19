
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
	this->check(hm, center);
}

void Cluster::add(index2_t pos, Float32 val)
{
	Float32 x = gsl::narrow_cast<Float32>(pos.x);
	Float32 y = gsl::narrow_cast<Float32>(pos.y);

	this->x += val * x;
	this->y += val * y;
	this->xx += val * x * x;
	this->yy += val * y * y;
	this->xy += val * x * y;
	this->w += val;

	if (this->max_v < val)
		this->max_v = val;
}

math::Vec2<Float32> Cluster::mean()
{
	Float32 fx = (Float32)this->x;
	Float32 fy = (Float32)this->y;
	Float32 fw = (Float32)this->w;

	return math::Vec2<Float32> {fx / fw, fy / fw};
}

math::Mat2s<Float32> Cluster::cov()
{
	Float32 r1 = (this->xx - (this->x * this->x / this->w)) / this->w;
	Float32 r2 = (this->yy - (this->y * this->y / this->w)) / this->w;
	Float32 r3 = (this->xy - (this->x * this->y / this->w)) / this->w;

	return math::Mat2s<Float32> {r1, r3, r2};
}

void Cluster::check(Heatmap &hm, index2_t pos)
{
	Float32 v = hm.value(pos);

	if (hm.get_visited(pos))
		return;

	this->add(pos, v);
	hm.set_visited(pos, true);

	this->check(hm, index2_t {pos.x + 1, pos.y});
	this->check(hm, index2_t {pos.x - 1, pos.y});
	this->check(hm, index2_t {pos.x, pos.y + 1});
	this->check(hm, index2_t {pos.x, pos.y - 1});
}

} // namespace iptsd::contacts::basic
