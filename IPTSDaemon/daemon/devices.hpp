/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_DAEMON_DEVICES_HPP
#define IPTSD_DAEMON_DEVICES_HPP

#include "cone.hpp"
#include "config.hpp"
#include "touch-manager.hpp"
#include "stylus-manager.hpp"

#include <common/types.hpp>

#include <memory>
#include <vector>
#include <gsl/gsl>

namespace iptsd::daemon {

#define IPTS_DFT_STYLUS_SERIAL  0xffffffff

class StylusData {
public:
    bool proximity = false;
    bool contact = false;
    bool button = false;
    bool rubber = false;

    UInt16 timestamp = 0;
    UInt16 x = 0;
    UInt16 y = 0;
    UInt16 pressure = 0;
    UInt16 altitude = 0;
    UInt16 azimuth = 0;
    UInt32 serial = 0;
};

class StylusDFTData {
public:
    UInt8 type;
    UInt16 timestamp;
    UInt16 num_cols;
    UInt16 num_rows;
    const IPTSStylusDFTWindowRow **dft_x;
    const IPTSStylusDFTWindowRow **dft_y;
};

class SingletouchData {
public:
    bool touch = false;
    UInt16 x = 0;
    UInt16 y = 0;
};

class Heatmap {
public:
    UInt16 width = 0;
    UInt16 height = 0;
    UInt16 diagonal = 0;

    UInt8 y_min = 0;
    UInt8 y_max = 0;
    UInt8 x_min = 0;
    UInt8 x_max = 0;
    UInt8 z_min = 0;
    UInt8 z_max = 0;
    UInt32 timestamp = 0;

    gsl::span<UInt8> data;
};

class StylusDevice {
public:
	UInt32 serial;
	bool active = false;
	std::shared_ptr<Cone> cone;

    StylusDevice(bool stylus_cone, UInt32 serial, std::shared_ptr<Cone> cone) : stylus_cone(stylus_cone), serial(serial), cone(std::move(cone)) {};
    
    int process_stylus_input(const StylusData &data, IPTSHIDReport &report);
    
private:
    bool stylus_cone;
};

class DFTStylusDevice : public StylusDevice {
public:
    StylusManager manager;
    
    DFTStylusDevice(Config &conf, UInt32 serial, std::shared_ptr<Cone> cone) : StylusDevice(conf.stylus_cone, serial, cone), manager(conf) {};
    
    int process_dft_stylus_input(const StylusDFTData &data, IPTSHIDReport &report);
    
private:
    StylusData stylus;
};

class TouchDevice {
public:
	TouchManager manager;

    TouchDevice(const Config &conf) : disable_on_palm(conf.touch_disable_on_palm), manager(conf) {};
    
    void process_singletouch_input(const SingletouchData &data, IPTSHIDReport &report);
    bool process_heatmap_input(const Heatmap &data, IPTSHIDReport &report);
    
private:
    bool disable_on_palm;
};

class DeviceManager {
public:
	Config conf;
    
	TouchDevice touch;
    DFTStylusDevice dft_stylus;
	std::vector<StylusDevice> stylus_list;
	UInt32 active_stylus_cnt = 0;

	DeviceManager(IPTSDeviceInfo info);

	StylusDevice &create_stylus(UInt32 serial);
	StylusDevice &get_stylus(UInt32 serial);
};

} /* namespace iptsd::daemon */

#endif /* IPTSD_DAEMON_DEVICES_HPP */
