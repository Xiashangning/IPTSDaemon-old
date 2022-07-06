/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_MATH_MAT2_HPP
#define IPTSD_MATH_MAT2_HPP

#include "num.hpp"
#include "poly2.hpp"
#include "vec2.hpp"

#include <iostream>
#include <optional>

namespace iptsd::math {

template <class T> struct Eigen2 {
	std::array<T, 2> w;
	std::array<Vec2<T>, 2> v;
};

template <class T> struct Mat2s {
public:
	T xx, xy, yy;

public:
	using value_type = T;

public:
	static constexpr auto identity() -> Mat2s<T>;

	constexpr auto operator+=(Mat2s<T> const &m) -> Mat2s<T> &;
	constexpr auto operator+=(T const &s) -> Mat2s<T> &;

	constexpr auto operator-=(Mat2s<T> const &m) -> Mat2s<T> &;
	constexpr auto operator-=(T const &s) -> Mat2s<T> &;

	constexpr auto operator*=(T const &s) -> Mat2s<T> &;
	constexpr auto operator/=(T const &s) -> Mat2s<T> &;

	[[nodiscard]] constexpr auto vtmv(Vec2<T> const &v) const -> T;

	[[nodiscard]] constexpr auto inverse(T eps = num<T>::eps) const -> std::optional<Mat2s<T>>;

	[[nodiscard]] constexpr auto det() const -> T;
	[[nodiscard]] constexpr auto trace() const -> T;

	[[nodiscard]] constexpr auto eigen(T eps = num<T>::eps) const -> Eigen2<T>;
	[[nodiscard]] constexpr auto eigenvalues(T eps = num<T>::eps) const -> std::array<T, 2>;
	[[nodiscard]] constexpr auto eigenvector(T eigenvalue) const -> Vec2<T>;

	template <class S> constexpr auto cast() const -> Mat2s<S>;
};

template <class T> inline constexpr auto Mat2s<T>::identity() -> Mat2s<T>
{
	return {num<T>::one, num<T>::zero, num<T>::one};
}

template <class T> inline constexpr auto Mat2s<T>::operator+=(Mat2s<T> const &m) -> Mat2s<T> &
{
	xx += m.xx;
	xy += m.xy;
	yy += m.yy;
	return *this;
}

template <class T> inline constexpr auto Mat2s<T>::operator+=(T const &s) -> Mat2s<T> &
{
	xx += s;
	xy += s;
	yy += s;
	return *this;
}

template <class T> inline constexpr auto Mat2s<T>::operator-=(Mat2s<T> const &m) -> Mat2s<T> &
{
	xx -= m.xx;
	xy -= m.xy;
	yy -= m.yy;
	return *this;
}

template <class T> inline constexpr auto Mat2s<T>::operator-=(T const &s) -> Mat2s<T> &
{
	xx -= s;
	xy -= s;
	yy -= s;
	return *this;
}

template <class T> inline constexpr auto Mat2s<T>::operator*=(T const &s) -> Mat2s<T> &
{
	xx *= s;
	xy *= s;
	yy *= s;
	return *this;
}

template <class T> inline constexpr auto Mat2s<T>::operator/=(T const &s) -> Mat2s<T> &
{
	xx /= s;
	xy /= s;
	yy /= s;
	return *this;
}

template <class T> inline constexpr auto Mat2s<T>::vtmv(Vec2<T> const &v) const -> T
{
	return v.x * v.x * xx + v.x * v.y * xy + v.y * v.x * xy +
	       v.y * v.y * yy;
}

template <class T> inline constexpr auto Mat2s<T>::inverse(T eps) const -> std::optional<Mat2s<T>>
{
	auto const d = det();

	if (std::abs(d) <= eps)
		return std::nullopt;

	return {{yy / d, -xy / d, xx / d}};
}

template <class T> inline constexpr auto Mat2s<T>::det() const -> T
{
	return xx * yy - xy * xy;
}

template <class T> inline constexpr auto Mat2s<T>::trace() const -> T
{
	return xx + yy;
}

template <class T> inline constexpr auto Mat2s<T>::eigen(T eps) const -> Eigen2<T>
{
	auto const [ew1, ew2] = eigenvalues(eps);

	return {{ew1, ew2}, {eigenvector(ew1), eigenvector(ew2)}};
}

template <class T> inline constexpr auto Mat2s<T>::eigenvalues(T eps) const -> std::array<T, 2>
{
	return solve_quadratic(num<T>::one, -trace(), det(), eps);
}

template <class T> inline constexpr auto Mat2s<T>::eigenvector(T eigenvalue) const -> Vec2<T>
{
	auto ev = Vec2<T> {};

	/*
	 * This 'if' should prevent two problems:
	 * 1. Cancellation due to small values in subtraction.
	 * 2. The vector being { 0, 0 }.
	 */
	if (std::abs(xx - eigenvalue) > std::abs(yy - eigenvalue)) {
		ev = {-xy, xx - eigenvalue};
	} else {
		ev = {yy - eigenvalue, -xy};
	}

	return ev / ev.norm_l2();
}

template <class T>
template <class S>
[[nodiscard]] inline constexpr auto Mat2s<T>::cast() const -> Mat2s<S>
{
	return {static_cast<S>(xx), static_cast<S>(xy), static_cast<S>(yy)};
}

template <typename T> auto operator<<(std::ostream &os, Mat2s<T> const &m) -> std::ostream &
{
	return os << "[[" << m.xx << ", " << m.xy << "], [" << m.xy << ", " << m.yy << "]]";
}

template <class T> inline constexpr auto operator+(Mat2s<T> const &a, Mat2s<T> const &b) -> Mat2s<T>
{
	return {a.xx + b.xx, a.xy + b.xy, a.yy + b.yy};
}

template <class T> inline constexpr auto operator+(Mat2s<T> const &m, T const &s) -> Mat2s<T>
{
	return {m.xx + s, m.xy + s, m.yy + s};
}

template <class T> inline constexpr auto operator+(T const &s, Mat2s<T> const &m) -> Mat2s<T>
{
	return {s + m.xx, s + m.xy, s + m.yy};
}

template <class T> inline constexpr auto operator-(Mat2s<T> const &a, Mat2s<T> const &b) -> Mat2s<T>
{
	return {a.xx - b.xx, a.xy - b.xy, a.yy - b.yy};
}

template <class T> inline constexpr auto operator-(Mat2s<T> const &m, T const &s) -> Mat2s<T>
{
	return {m.xx - s, m.xy - s, m.yy - s};
}

template <class T> inline constexpr auto operator-(T const &s, Mat2s<T> const &m) -> Mat2s<T>
{
	return {s - m.xx, s - m.xy, s - m.yy};
}

template <class T> inline constexpr auto operator*(Mat2s<T> const &m, T const &s) -> Mat2s<T>
{
	return {m.xx * s, m.xy * s, m.yy * s};
}

template <class T> inline constexpr auto operator*(T const &s, Mat2s<T> const &m) -> Mat2s<T>
{
	return {s * m.xx, s * m.xy, s * m.yy};
}

template <class T> inline constexpr auto operator/(Mat2s<T> const &m, T const &s) -> Mat2s<T>
{
	return {m.xx / s, m.xy / s, m.yy / s};
}

template <class T> inline constexpr auto operator/(T const &s, Mat2s<T> const &m) -> Mat2s<T>
{
	return {s / m.xx, s / m.xy, s / m.yy};
}

template <class T> struct num<Mat2s<T>> {
	static inline constexpr Mat2s<T> zero = {num<T>::zero, num<T>::zero, num<T>::zero};
};

} /* namespace iptsd::math */

#endif /* IPTSD_MATH_MAT2_HPP */
