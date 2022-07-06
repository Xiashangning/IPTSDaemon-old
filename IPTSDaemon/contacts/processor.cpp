// SPDX-License-Identifier: GPL-2.0-or-later

#include "processor.hpp"

#include <container/image.hpp>

#include <gsl/gsl>

namespace iptsd::contacts {

container::Image<Float32> &TouchProcessor::hm()
{
	return tp->hm();
}

const std::vector<TouchPoint> &TouchProcessor::process()
{
	return tp->process();
}

const eval::perf::Registry &TouchProcessor::perf() const
{
	return tp->perf();
}

void TouchProcessor::resize(index2_t size)
{
	if (tp) {
		if (conf.size == size)
			return;

		tp.reset(nullptr);
	}

	conf.size = size;

	if (!advanced) {
		tp = std::make_unique<basic::TouchProcessor>(conf);
	} else {
		tp = std::make_unique<advanced::TouchProcessor>(conf.size);
	}

	diag = gsl::narrow_cast<UInt16>(std::sqrt(size.x * size.x + size.y * size.y));
}

UInt16 TouchProcessor::diagonal()
{
	return diag;
}

} // namespace iptsd::contacts
