/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_COMMON_TYPES_HPP
#define IPTSD_COMMON_TYPES_HPP

#include <IOKit/IOTypes.h>

#include <cmath>
#include <cstdint>
#include <iostream>

using index_t = int;

struct index2_t {
public:
	index_t x, y;

public:
	constexpr auto operator+=(index2_t const &v) -> index2_t &;
	constexpr auto operator-=(index2_t const &v) -> index2_t &;

	[[nodiscard]] constexpr auto span() const -> index_t;
};

inline constexpr auto index2_t::operator+=(index2_t const &v) -> index2_t &
{
	x += v.x;
	y += v.y;
	return *this;
}

inline constexpr auto index2_t::operator-=(index2_t const &v) -> index2_t &
{
	x -= v.x;
	y -= v.y;
	return *this;
}

inline constexpr auto index2_t::span() const -> index_t
{
	return x * y;
}

inline auto operator<<(std::ostream &os, index2_t const &i) -> std::ostream &
{
	return os << "[" << i.x << ", " << i.y << "]";
}

inline constexpr auto operator==(index2_t const &a, index2_t const &b) -> bool
{
	return a.x == b.x && a.y == b.y;
}

inline constexpr auto operator!=(index2_t const &a, index2_t const &b) -> bool
{
	return !(a == b);
}

inline constexpr auto operator>(index2_t const &a, index2_t const &b) -> bool
{
	return a.x > b.x && a.y > b.y;
}

inline constexpr auto operator>=(index2_t const &a, index2_t const &b) -> bool
{
	return a.x >= b.x && a.y >= b.y;
}

inline constexpr auto operator<(index2_t const &a, index2_t const &b) -> bool
{
	return a.x < b.x && a.y < b.y;
}

inline constexpr auto operator<=(index2_t const &a, index2_t const &b) -> bool
{
	return a.x <= b.x && a.y <= b.y;
}

inline constexpr auto operator+(index2_t const &a, index2_t const &b) -> index2_t
{
	return {a.x + b.x, a.y + b.y};
}

inline constexpr auto operator-(index2_t const &a, index2_t const &b) -> index2_t
{
	return {a.x - b.x, a.y - b.y};
}

#endif /* IPTSD_COMMON_TYPES_HPP */
