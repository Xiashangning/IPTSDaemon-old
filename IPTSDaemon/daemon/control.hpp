/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_IPTS_CONTROL_HPP
#define IPTSD_IPTS_CONTROL_HPP

#include "../../IPTSKenerlUserShared.h"

#include <IOKit/IOKitLib.h>
#include <gsl/gsl>

namespace iptsd::daemon {

class Control {
public:
	IPTSDeviceInfo      info;
    gsl::span<UInt8>    buffers[IPTS_BUFFER_NUM];

	Control();
	~Control();

	gsl::span<UInt8> &read_input();
    void send_hid_report(IPTSHIDReport &report);
    void reset();
    
private:
    io_connect_t connect;
    io_service_t service;
};

} /* namespace iptsd::ipts */

#endif /* IPTSD_IPTS_CONTROL_HPP */
