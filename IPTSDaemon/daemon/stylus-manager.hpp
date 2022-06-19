/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef stylus_manager_hpp
#define stylus_manager_hpp

#include "config.hpp"

namespace iptsd::daemon {

class StylusInput {
public:
    bool proximity = false;
    bool contact = false;
    bool button = false;
    bool rubber = false;

    Float64 x = 0;
    Float64 y = 0;
    UInt16 pressure = 0;
    
//    UInt16 timestamp = 0;
//    UInt16 altitude = 0;
//    UInt16 azimuth = 0;
};

class StylusDFTData;

class StylusManager {
public:
    const Config &conf;
    
    StylusManager(const Config &conf) : conf(conf) {};
    
    StylusInput *process(const StylusDFTData &data);
    
private:
    StylusInput input;
    bool rubber = false;
    bool set_rubber = false;
    int real = 0;
    int imag = 0;
    
    StylusInput *stop_stylus();
};
    
} /* namespace iptsd::daemon */

#endif /* stylus_manager_hpp */
