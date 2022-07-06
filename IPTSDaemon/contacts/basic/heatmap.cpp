// SPDX-License-Identifier: GPL-2.0-or-later

#include "heatmap.hpp"

#include <common/types.hpp>

#include <cstddef>
#include <gsl/gsl>

namespace iptsd::contacts::basic {

Float32 Heatmap::value(index2_t x)
{
	if (x.x < 0 || x.x >= size.x)
		return 0;

	if (x.y < 0 || x.y >= size.y)
		return 0;

	Float32 val = data[x];
	if (val > average)
		return val - average;
	else
		return 0;
}

bool Heatmap::compare(index2_t x, index2_t y)
{
	Float64 v1 = value(x);
	Float64 v2 = value(y);

	if (v2 > v1)
		return false;

	if (v2 < v1)
		return true;

	if (y.x > x.x)
		return false;

	if (y.x < x.x)
		return true;

	if (y.y > x.y)
		return false;

	if (y.y < x.y)
		return true;

	return y.y == x.y;
}

bool Heatmap::get_visited(index2_t x)
{
	if (x.x < 0 || x.x >= size.x)
		return true;

	if (x.y < 0 || x.y >= size.y)
		return true;

	return visited[x];
}

void Heatmap::set_visited(index2_t x, bool value)
{
	if (x.x < 0 || x.x >= size.x)
		return;

	if (x.y < 0 || x.y >= size.y)
		return;

	visited[x] = value;
}

void Heatmap::reset()
{
	for (index_t x = 0; x < size.x; x++) {
		for (index_t y = 0; y < size.y; y++)
			set_visited(index2_t {x, y}, false);
	}

	Float32 value = 0;

	for (auto i : data)
		value += i;

	average = value / gsl::narrow_cast<Float32>(size.span());
}

} // namespace iptsd::contacts::basic
