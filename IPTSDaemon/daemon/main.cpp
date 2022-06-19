//
//  main.cpp
//  IPTSDaemon
//
//  Created by Xavier on 2022/6/11.
//

#include <common/signal.hpp>
#include <common/types.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fmt/format.h>
#include <functional>
#include <spdlog/spdlog.h>
#include <stdexcept>

#include "control.hpp"
#include "parser.hpp"
#include "devices.hpp"

using namespace std::chrono;

namespace iptsd::daemon {

static int main()
{
    std::atomic_bool should_exit {false};
    std::atomic_bool should_reset {false};
    auto const _sigusr1 = common::signal<SIGUSR1>([&](int) { should_reset = true; });
    auto const _sigterm = common::signal<SIGTERM>([&](int) { should_exit = true; });
    auto const _sigint = common::signal<SIGINT>([&](int) { should_exit = true; });

    Control ctrl;
    Parser parser;
    DeviceManager devices(ctrl.info);
    
	spdlog::info("Connected to device {:04X}:{:04X}", ctrl.info.vendor_id, ctrl.info.product_id);

	parser.on_singletouch = [&](const auto &data) {
        IPTSHIDReport report;
        devices.touch.process_singletouch_input(data, report);
        ctrl.send_hid_report(report);
    };
    parser.on_heatmap = [&](const auto &data) {
        if (devices.active_stylus_cnt > 0 && devices.conf.stylus_disable_touch)
            return;
        IPTSHIDReport report;
        if (devices.touch.process_heatmap_input(data, report))
            ctrl.send_hid_report(report);
        usleep(5000);
    };
    parser.on_stylus = [&](const auto &data) {
        StylusDevice &stylus = devices.get_stylus(data.serial);
        IPTSHIDReport report;
        int status = stylus.process_stylus_input(data, report);
        ctrl.send_hid_report(report);
        devices.active_stylus_cnt += status;
    };
    parser.on_dft_stylus = [&](const auto &data) {
        DFTStylusDevice &stylus = devices.dft_stylus;
        IPTSHIDReport report;
        int status = stylus.process_dft_stylus_input(data, report);
        if (status < -1)
            return;
        ctrl.send_hid_report(report);
        devices.active_stylus_cnt += status;
    };

	while (true) {
        gsl::span<UInt8> &data = ctrl.read_input();
        parser.prepare(&data);
        
        try {
            parser.parse();
        } catch (std::out_of_range &e) {
            spdlog::error(e.what());
        }

		if (should_reset) {
			spdlog::info("Resetting touch sensor");
			ctrl.reset();
			should_reset = false;
		}
		if (should_exit) {
			spdlog::info("Stopping");
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

} // namespace iptsd::daemon

int main()
{
	spdlog::set_pattern("[%X.%e] [%^%l%$] %v");
	try {
		return iptsd::daemon::main();
	} catch (std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
}
