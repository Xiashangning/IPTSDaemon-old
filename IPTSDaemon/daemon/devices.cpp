// SPDX-License-Identifier: GPL-2.0-or-later

#include "devices.hpp"
#include "config.hpp"

#include <common/types.hpp>

#include <climits>
#include <cmath>
#include <cstddef>
#include <gsl/gsl>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace iptsd::daemon {

void TouchDevice::process_singletouch_input(const SingletouchData &data, IPTSHIDReport &report) {
    memset(&report, 0, sizeof(IPTSHIDReport));
    
    report.report_id = IPTS_TOUCH_REPORT_ID;
    report.data.touch.fingers[0].touch = data.touch;
    report.data.touch.fingers[0].contact_id = 0;
    report.data.touch.fingers[0].x = data.x;
    report.data.touch.fingers[0].y = data.y;
    report.data.touch.contact_num = data.touch;
}

bool TouchDevice::process_heatmap_input(const Heatmap &data, IPTSHIDReport &report)
{
    memset(&report, 0, sizeof(IPTSHIDReport));
    
    const std::vector<TouchInput> &inputs = manager.process(data);

    if (disable_on_palm) {
        for (const auto &p : inputs) {
            if (p.palm)
                return false;
        }
    }

    report.report_id = IPTS_TOUCH_REPORT_ID;
    int contact_cnt = 0;
    for (const TouchInput &in : inputs) {
        if (in.active && !in.palm) {
            IPTSFingerReport *finger = &report.data.touch.fingers[in.index];
            finger->touch = in.instability < IPTS_TOUCH_INSTABILITY_THRESH;
            finger->contact_id = in.index;
            finger->x = gsl::narrow_cast<UInt16>(in.x * IPTS_SINGLETOUCH_MAX_VALUE);
            finger->y = gsl::narrow_cast<UInt16>(in.y * IPTS_SINGLETOUCH_MAX_VALUE);
            
            contact_cnt++;
        }
    }
    report.data.touch.contact_num = contact_cnt;
    
    return true;
}

static std::tuple<SInt32, SInt32> get_tilt(UInt32 altitude, UInt32 azimuth)
{
    if (altitude <= 0)
        return {0, 0};

    Float64 alt = static_cast<Float64>(altitude) / 18000 * M_PI;
    Float64 azm = static_cast<Float64>(azimuth) / 18000 * M_PI;

    Float64 sin_alt = std::sin(alt);
    Float64 sin_azm = std::sin(azm);

    Float64 cos_alt = std::cos(alt);
    Float64 cos_azm = std::cos(azm);

    Float64 atan_x = std::atan2(cos_alt, sin_alt * cos_azm);
    Float64 atan_y = std::atan2(cos_alt, sin_alt * sin_azm);

    SInt32 tx = 9000 - gsl::narrow_cast<SInt32>(atan_x * 4500 / M_PI_4);
    SInt32 ty = gsl::narrow_cast<SInt32>(atan_y * 4500 / M_PI_4) - 9000;

    return {tx, ty};
}

int StylusDevice::process_stylus_input(const StylusData &data, IPTSHIDReport &report)
{
    memset(&report, 0, sizeof(IPTSHIDReport));

    int status = 0;
    if (!active && data.proximity) {
        active = true;
        status = 1;
    } else if (active && !data.proximity) {
        active = false;
        status = -1;
    }
    
    if (data.proximity && stylus_cone)
        cone->update_position(data.x, data.y);

    const auto [tx, ty] = get_tilt(data.altitude, data.azimuth);
    
    report.report_id = IPTS_STYLUS_REPORT_ID;
    
    IPTSStylusHIDReport *stylus = &report.data.stylus;
    stylus->in_range = data.proximity;
    stylus->touch = data.contact;
    stylus->side_button = data.button;
    stylus->inverted = 0;
    stylus->eraser = data.rubber;
    stylus->x = data.x;
    stylus->y = data.y;
    stylus->tip_pressure = data.pressure;
    stylus->x_tilt = tx;
    stylus->y_tilt = ty;
    stylus->scan_time = data.timestamp;

    return status;
}

int DFTStylusDevice::process_dft_stylus_input(const StylusDFTData &data, IPTSHIDReport &report)
{
    memset(&report, 0, sizeof(IPTSHIDReport));
    
    const StylusInput *input = manager.process(data);
    if (!input)
        return -2;  // Stylus data not finished yet
    
    int status = 0;
    if (!active && input->proximity) {
        active = true;
        status = 1;
    } else if (active && !input->proximity) {
        active = false;
        status = -1;
    }
    
    report.report_id = IPTS_STYLUS_REPORT_ID;
    IPTSStylusHIDReport *stylus = &report.data.stylus;
    stylus->in_range = input->proximity;
    stylus->touch = input->contact;
    stylus->side_button = input->button;
    stylus->inverted = 0;
    stylus->eraser = input->rubber;
    stylus->x = gsl::narrow_cast<UInt16>(input->x * IPTS_MAX_X);
    stylus->y = gsl::narrow_cast<UInt16>(input->y * IPTS_MAX_Y);
    stylus->tip_pressure = input->pressure;
    stylus->x_tilt = 0;
    stylus->y_tilt = 0;
    stylus->scan_time = data.timestamp;
    
    return status;
}

DeviceManager::DeviceManager(IPTSDeviceInfo info) : conf(Config(info)), touch(conf), dft_stylus(conf, IPTS_DFT_STYLUS_SERIAL, std::make_shared<Cone>(conf.cone_angle, conf.cone_distance))
{
	if (conf.width == 0 || conf.height == 0)
		throw std::runtime_error("Display size is 0");

    touch.manager.cones.push_back(dft_stylus.cone);
    create_stylus(0);
}

StylusDevice &DeviceManager::create_stylus(UInt32 serial)
{
	std::shared_ptr<Cone> cone = std::make_shared<Cone>(conf.cone_angle, conf.cone_distance);
	touch.manager.cones.push_back(cone);
	return stylus_list.emplace_back(conf.stylus_cone, serial, std::move(cone));
}

StylusDevice &DeviceManager::get_stylus(UInt32 serial)
{
	StylusDevice &stylus = stylus_list.back();

	if (stylus.serial == serial)
		return stylus;

	if (stylus.serial == 0) {
		stylus.serial = serial;
		return stylus;
	}

	for (StylusDevice &s : stylus_list) {
		if (s.serial != serial)
			continue;

		std::swap(s, stylus);
		return s;
	}

	return create_stylus(serial);
}

} // namespace iptsd::daemon
