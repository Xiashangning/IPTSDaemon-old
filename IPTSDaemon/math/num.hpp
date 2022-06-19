/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_MATH_NUM_HPP
#define IPTSD_MATH_NUM_HPP

#include <common/types.hpp>

namespace iptsd::math {

template <typename> struct num {};

template <> struct num<Float32> {
	static inline constexpr Float32 zero = 0.0f;
	static inline constexpr Float32 one = 1.0f;
	static inline constexpr Float32 pi = 3.14159265359f;
	static inline constexpr Float32 eps = 1e-20f;
};

template <> struct num<Float64> {
	static inline constexpr Float64 zero = 0.0;
	static inline constexpr Float64 one = 1.0;
	static inline constexpr Float64 pi = 3.14159265359;
	static inline constexpr Float64 eps = 1e-40;
};

} /* namespace iptsd::math */

#endif /* IPTSD_MATH_NUM_HPP */
