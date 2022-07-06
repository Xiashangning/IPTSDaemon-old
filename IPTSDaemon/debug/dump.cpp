// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gsl/gsl>
#include <iostream>
#include <iterator>
#include <spdlog/spdlog.h>
#include <string>

#include "control.hpp"
#include "parser.hpp"
#include "devices.hpp"

struct PrettyBuf {
	UInt8 *data;
	size_t size;
};

template <> struct fmt::formatter<PrettyBuf> {
	char hexfmt = 'x';
	char prefix = 'n';

	constexpr auto parse(format_parse_context &ctx)
	{
		auto it = ctx.begin(), end = ctx.end();

		while (it != end && *it != '}') {
			if (*it == 'x' || *it == 'X') {
				hexfmt = *it++;
			} else if (*it == 'n' || *it == 'o' || *it == 'O') {
				prefix = *it++;
			} else {
				throw format_error("invalid format");
			}
		}

		return it;
	}

	template <class FormatContext> auto format(PrettyBuf const &buf, FormatContext &ctx)
	{
		char const *pfxstr = prefix == 'o' ? "{:04x}: " : "{:04X}: ";
		char const *fmtstr = hexfmt == 'x' ? "{:02x} " : "{:02X} ";

		auto it = ctx.out();
		for (size_t i = 0; i < buf.size; i += 32) {
			size_t j = 0;

			if (prefix != 'n') {
				it = format_to(it, pfxstr, i);
			}

			for (; j < 8 && i + j < buf.size; j++) {
				it = format_to(it, fmtstr, buf.data[i + j]);
			}

			it = format_to(it, " ");

			for (; j < 16 && i + j < buf.size; j++) {
				it = format_to(it, fmtstr, buf.data[i + j]);
			}

			it = format_to(it, " ");

			for (; j < 24 && i + j < buf.size; j++) {
				it = format_to(it, fmtstr, buf.data[i + j]);
			}

			it = format_to(it, " ");

			for (; j < 32 && i + j < buf.size; j++) {
				it = format_to(it, fmtstr, buf.data[i + j]);
			}

			it = format_to(it, "\n");
		}

		return format_to(it, "\n");
	}
};

namespace iptsd::debug::dump {

static int main(int argc, char *argv[])
{
	std::ofstream file;
    file.exceptions(std::ios::badbit | std::ios::failbit);
    file.open("dump", std::ios::out | std::ios::binary);

    daemon::Control ctrl;

	fmt::print("Vendor:       {:04X}\n", ctrl.info.vendor_id);
	fmt::print("Product:      {:04X}\n", ctrl.info.product_id);
	fmt::print("Max Contacts: {}\n", ctrl.info.max_contacts);
	fmt::print("\n");

    UInt8 *data;
	while (true) {
        data = ctrl.read_input().data();
        IPTSDataHeader *header = reinterpret_cast<IPTSDataHeader *>(data);

        file.write((const char *)data, sizeof(IPTSDataHeader) + header->size);

        auto const header_type = header->type;
        auto const header_buffer = header->buffer;
        auto const header_size = header->size;
        const PrettyBuf buf {&data[sizeof(IPTSDataHeader)], header->size};
        fmt::print("====== Buffer: {} == Type: {} == Size: {} =====\n", header_type, header_buffer, header_size);
//        fmt::print("{:ox}\n", buf);
	}

	return 0;
}

} // namespace iptsd::debug::dump

int main(int argc, char *argv[])
{
	spdlog::set_pattern("[%X.%e] [%^%l%$] %v");
	try {
		return iptsd::debug::dump::main(argc, argv);
	} catch (std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
}
