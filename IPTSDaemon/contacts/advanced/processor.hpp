#pragma once

#include <common/types.hpp>

#include "algorithm/distance_transform.hpp"
#include "algorithm/gaussian_fitting.hpp"

#include <container/image.hpp>
#include <container/kernel.hpp>

#include <contacts/eval/perf.hpp>
#include <contacts/interface.hpp>

#include <math/vec2.hpp>
#include <math/mat2.hpp>

#include <array>
#include <vector>
#include <queue>

using namespace iptsd::container;
using namespace iptsd::math;


namespace iptsd::contacts::advanced {

struct ComponentStats {
    UInt32 size;
    Float32 volume;
    Float32 incoherence;
    UInt32 maximas;
};


class TouchProcessor : public ITouchProcessor {
public:
    TouchProcessor(index2_t size);

    auto hm() -> Image<Float32> & override;
    auto process() -> std::vector<TouchPoint> const& override;

    [[nodiscard]] auto perf() const -> eval::perf::Registry const& override;

private:
    auto process(Image<Float32> const& hm) -> std::vector<TouchPoint> const&;

    // performance measurements
    eval::perf::Registry m_perf_reg;
    eval::perf::Token m_perf_t_total;
    eval::perf::Token m_perf_t_prep;
    eval::perf::Token m_perf_t_st;
    eval::perf::Token m_perf_t_stev;
    eval::perf::Token m_perf_t_hess;
    eval::perf::Token m_perf_t_rdg;
    eval::perf::Token m_perf_t_obj;
    eval::perf::Token m_perf_t_lmax;
    eval::perf::Token m_perf_t_lbl;
    eval::perf::Token m_perf_t_cscr;
    eval::perf::Token m_perf_t_wdt;
    eval::perf::Token m_perf_t_flt;
    eval::perf::Token m_perf_t_lmaxf;
    eval::perf::Token m_perf_t_gfit;

    // temporary storage
    Image<Float32> m_hm;
    Image<Float32> m_img_pp;
    Image<Mat2s<Float32>> m_img_m2_1;
    Image<Mat2s<Float32>> m_img_m2_2;
    Image<std::array<Float32, 2>> m_img_stev;
    Image<Float32> m_img_rdg;
    Image<Float32> m_img_obj;
    Image<UInt16> m_img_lbl;
    Image<Float32> m_img_dm1;
    Image<Float32> m_img_dm2;
    Image<Float32> m_img_flt;
    Image<Float64> m_img_gftmp;

    std::priority_queue<alg::wdt::QItem<Float32>> m_wdt_queue;
    std::vector<alg::gfit::Parameters<Float64>> m_gf_params;

    std::vector<index_t> m_maximas;
    std::vector<ComponentStats> m_cstats;
    std::vector<Float32> m_cscore;

    // gauss kernels
    Kernel<Float32, 5, 5> m_kern_pp;
    Kernel<Float32, 5, 5> m_kern_st;
    Kernel<Float32, 5, 5> m_kern_hs;

    // parameters
    index2_t m_gf_window;

    // output
    std::vector<TouchPoint> m_touchpoints;
};


inline auto TouchProcessor::perf() const -> eval::perf::Registry const&
{
    return m_perf_reg;
}

inline auto TouchProcessor::hm() -> Image<Float32> &
{
    return m_hm;
}

inline auto TouchProcessor::process() -> std::vector<TouchPoint> const&
{
	return process(m_hm);
}

} /* namespace iptsd::contacts::advanced */
