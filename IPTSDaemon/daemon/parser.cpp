// SPDX-License-Identifier: GPL-2.0-or-later

#include "parser.hpp"

#include <common/access.hpp>
#include <common/types.hpp>

#include <bitset>
#include <cstring>
#include <gsl/gsl>
#include <memory>
#include <stdexcept>
#include <utility>

namespace iptsd::daemon {

template <class T> inline const T &Block::read()
{
    if (sizeof(T) > remaining()) throw std::out_of_range(typeid(T).name());
    T *r = (T *)&data->data()[index];
    index += sizeof(T);
    return *r;
}

inline void Block::skip(const size_t size)
{
    index += size;
}

inline size_t Block::remaining()
{
    return end - index;
}

inline Block Block::block(size_t size)
{
    if (size > remaining()) throw std::out_of_range("index");
    size_t start = index;
    index += size;
    return Block(data, start, index);
}

inline gsl::span<UInt8> Block::span()
{
    return gsl::span<UInt8>(&data->data()[index], end - index);
}

inline Block Parser::block()
{
    return Block(data, 0, data->size());
}

void Parser::prepare(gsl::span<UInt8> *buffer)
{
    data = buffer;
}

void Parser::parse(Block &b, bool ignore_truncated)
{
    const auto header = b.read<IPTSDataHeader>();
    if (ignore_truncated && header.size > b.remaining()) return;
    auto data = b.block(header.size);

    switch (header.type) {
    case IPTS_DATA_TYPE_PAYLOAD:
        parse_payload(data);
        break;
    case IPTS_DATA_TYPE_HID_REPORT:
        parse_hid(data);
        break;
    }
}

void Parser::parse()
{
    auto b = block();
    parse(b, false);
}

void Parser::parse_loop()
{
    auto b = block();
    while (b.remaining())
        parse(b, false);
}

void Parser::parse_payload(Block &b)
{
    const auto payload = b.read<IPTSPayloadHeader>();

    for (UInt32 i = 0; i < payload.frames; i++) {
        const auto frame = b.read<IPTSPayloadFrame>();
        auto data = b.block(frame.size);

        switch (frame.type) {
        case IPTS_PAYLOAD_FRAME_TYPE_STYLUS:
            parse_stylus(data);
            break;
        case IPTS_PAYLOAD_FRAME_TYPE_HEATMAP:
            parse_container_reports(data);
            break;
        }
    }
}

void Parser::parse_hid(Block &b)
{
    const auto report_code = b.read<UInt8>();

    if (report_code == IPTS_HID_REPORT_SINGLETOUCH)
        parse_singletouch(b);
    else if (IPTS_HID_REPORT_IS_CONTAINER(report_code))
        parse_hid_container(b);
}

void Parser::parse_singletouch(Block &b)
{
    const auto singletouch = b.read<IPTSFingerReport>();

    SingletouchData data;
    data.touch = singletouch.touch;
    data.x = singletouch.x;
    data.y = singletouch.y;

    if (on_singletouch)
        on_singletouch(data);
}

void Parser::parse_stylus(Block &b)
{
    while (b.remaining()) {
        const auto report = b.read<IPTSReportHeader>();
        auto data = b.block(report.size);

        switch (report.type) {
        case IPTS_REPORT_TYPE_STYLUS_V1:
            parse_stylus_report_v1(data);
            break;
        case IPTS_REPORT_TYPE_STYLUS_V2:
            parse_stylus_report_v2(data);
            break;
        }
    }
}

void Parser::parse_stylus_report_v1(Block &b)
{
    StylusData stylus;

    const auto stylus_report = b.read<IPTSStylusReportHeader>();
    stylus.serial = stylus_report.serial;

    for (UInt8 i = 0; i < stylus_report.elements; i++) {
        const auto data = b.read<IPTSStylusReportV1>();

        const std::bitset<8> mode(data.mode);
        stylus.proximity = mode[IPTS_STYLUS_REPORT_MODE_BIT_PROXIMITY];
        stylus.contact = mode[IPTS_STYLUS_REPORT_MODE_BIT_CONTACT];
        stylus.button = mode[IPTS_STYLUS_REPORT_MODE_BIT_BUTTON];
        stylus.rubber = mode[IPTS_STYLUS_REPORT_MODE_BIT_RUBBER];

        stylus.x = data.x;
        stylus.y = data.y;
        stylus.pressure = data.pressure * 4;
        stylus.azimuth = 0;
        stylus.altitude = 0;
        stylus.timestamp = 0;

        if (on_stylus)
            on_stylus(stylus);
    }
}

void Parser::parse_stylus_report_v2(Block &b)
{
    StylusData stylus;

    const auto stylus_report = b.read<IPTSStylusReportHeader>();
    stylus.serial = stylus_report.serial;

    for (UInt8 i = 0; i < stylus_report.elements; i++) {
        const auto data = b.read<IPTSStylusReportV2>();

        const std::bitset<16> mode(data.mode);
        stylus.proximity = mode[IPTS_STYLUS_REPORT_MODE_BIT_PROXIMITY];
        stylus.contact = mode[IPTS_STYLUS_REPORT_MODE_BIT_CONTACT];
        stylus.button = mode[IPTS_STYLUS_REPORT_MODE_BIT_BUTTON];
        stylus.rubber = mode[IPTS_STYLUS_REPORT_MODE_BIT_RUBBER];

        stylus.x = data.x;
        stylus.y = data.y;
        stylus.pressure = data.pressure;
        stylus.azimuth = data.azimuth;
        stylus.altitude = data.altitude;
        stylus.timestamp = data.timestamp;

        if (on_stylus)
            on_stylus(stylus);
    }
}

void Parser::parse_hid_container(Block &b)
{
    const auto timestamp = b.read<UInt16>();
    (void)timestamp;    // make complier happy
    const auto root = b.read<IPTSReportContainer>();
    auto root_data = b.block(root.size - sizeof(IPTSReportContainer));

    while (root_data.remaining()) {
        const auto container = root_data.read<IPTSReportContainer>();
        // On SP7 we receive 0x74 packets with 4 nul bytes of data, inside a container with an incorrect size. Let's just ignore these.
        if (root.size == 22 && container.type == 0xff && container.size == 11) return;
        auto data = root_data.block(container.size - sizeof(IPTSReportContainer));
        switch (container.type) {
            case IPTS_CONTAINER_TYPE_HEATMAP:
            {
                const auto hm = data.read<IPTSReportHeatmap>();
                heatmap.data = data.block(hm.size).span();
                break;
            }
            case IPTS_CONTAINER_TYPE_REPORT:
            {
                parse_container_reports(data);
                break;
            }
        }
    }
}

void Parser::parse_dft_stylus(Block &b)
{
    StylusDFTData data;
    const auto dft = b.read<IPTSStylusDFTWindow>();
    const IPTSStylusDFTWindowRow *dft_x[IPTS_DFT_MAX_ROWS], *dft_y[IPTS_DFT_MAX_ROWS];
    
    if (dft.num_rows > IPTS_DFT_MAX_ROWS)
        return;
    for (int i = 0; i < dft.num_rows; i++)
        dft_x[i] = &b.read<IPTSStylusDFTWindowRow>();
    for (int i = 0; i < dft.num_rows; i++)
        dft_y[i] = &b.read<IPTSStylusDFTWindowRow>();
    
    data.type = dft.data_type;
    data.timestamp = dft.timestamp;
    data.num_cols = num_cols;
    data.num_rows = num_rows;
    data.dft_x = dft_x;
    data.dft_y = dft_y;
    
    if (on_dft_stylus)
        on_dft_stylus(data);
}

void Parser::parse_container_reports(Block &b)
{
    const IPTSReportStart *start = nullptr;
    const IPTSHeatmapDimension *dim = nullptr;

    while (b.remaining()) {
        const auto report = b.read<IPTSReportHeader>();
        auto data = b.block(report.size);

        switch (report.type) {
        case IPTS_REPORT_TYPE_START:
            start = &data.read<IPTSReportStart>();
            break;
        case IPTS_REPORT_TYPE_HEATMAP_DIM:
            dim = &data.read<IPTSHeatmapDimension>();
            num_cols = dim->width;
            num_rows = dim->height;
            break;
        case IPTS_REPORT_TYPE_HEATMAP:
            if (dim) heatmap.data = data.block(dim->width * dim->height).span();
            break;
        case IPTS_REPORT_TYPE_PEN_DFT_WINDOW:
            parse_dft_stylus(data);
            break;
        }
    }

    if (on_heatmap && heatmap.data.size() && start && dim)
    {
        heatmap.timestamp = start->timestamp;
        heatmap.height = dim->height;
        heatmap.width = dim->width;
        heatmap.diagonal = std::sqrt(heatmap.height*heatmap.height + heatmap.width*heatmap.width);
        heatmap.y_min = dim->y_min;
        heatmap.y_max = dim->y_max;
        heatmap.x_min = dim->x_min;
        heatmap.x_max = dim->x_max;
        heatmap.z_min = dim->z_min;
        // z_min/z_max are both 0 in the HID data, which
        // doesnt make sense. Lets use sane values instead.
        heatmap.z_max = dim->z_max ? dim->z_max : 255;
        on_heatmap(heatmap);
    }
}

} // namespace iptsd::ipts
