/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef IPTSD_IPTS_PARSER_HPP
#define IPTSD_IPTS_PARSER_HPP

#include "../../IPTSKenerlUserShared.h"

#include "devices.hpp"

#include <common/types.hpp>

#include <cstddef>
#include <functional>
#include <gsl/gsl>
#include <memory>
#include <vector>

namespace iptsd::daemon {

class Block {
private:
    gsl::span<UInt8> *data;
    size_t index, end;

public:
    Block(gsl::span<UInt8> *data, size_t index, size_t end) : data(data), index(index), end(end) {};
    template <class T> const T& read();
    inline void skip(const size_t size);
    inline size_t remaining();
    inline Block block(size_t size);
    inline gsl::span<UInt8> span();
};

class Parser {
private:
    gsl::span<UInt8> *data = nullptr;

    Heatmap heatmap;
    UInt16 num_cols = 0;
    UInt16 num_rows = 0;

    inline Block block();

    void parse(Block &data, bool ignore_truncated);
    void parse_payload(Block &b);
    void parse_hid(Block &b);

    void parse_singletouch(Block &b);
    void parse_hid_container(Block &b);
    void parse_container_reports(Block &b);
    void parse_heatmap_data(Block &b);

    void parse_stylus(Block &b);
    void parse_stylus_report_v1(Block &b);
    void parse_stylus_report_v2(Block &b);

    void parse_dft_stylus(Block &b);

public:
	std::function<void(const SingletouchData &)> on_singletouch;
    std::function<void(const Heatmap &)> on_heatmap;
    std::function<void(const StylusData &)> on_stylus;
    std::function<void(const StylusDFTData &)> on_dft_stylus;
    
    void prepare(gsl::span<UInt8> *buffer);

	void parse();
	void parse_loop();
};

} /* namespace iptsd::ipts */

#endif /* IPTSD_IPTS_PARSER_HPP */
