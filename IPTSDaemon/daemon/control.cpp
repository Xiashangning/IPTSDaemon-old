// SPDX-License-Identifier: GPL-2.0-or-later

#include "control.hpp"

#include <common/cerror.hpp>
#include <iostream>

namespace iptsd::daemon {

Control::Control()
{
    io_iterator_t   iterator;
    kern_return_t ret = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("IntelPreciseTouchStylusDriver"), &iterator);
    if (ret != KERN_SUCCESS)
        throw common::cerror("Failed to match services");
    
    service = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (service == IO_OBJECT_NULL)
        throw common::cerror("Could not find IntelPreciseTouchStylusDriver!");
    
    ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
    if (ret != kIOReturnSuccess)
        throw common::cerror("Failed to establish a connection to the driver!");

    size_t info_size = sizeof(IPTSDeviceInfo);
    ret = IOConnectCallStructMethod(connect, kMethodGetDeviceInfo, nullptr, 0, &info, &info_size);
    if (ret != kIOReturnSuccess)
        throw common::cerror("Failed to get IPTS device info from driver!");
    
    IOVirtualAddress addr;
    UInt64 size;
    for (int i=0; i < IPTS_BUFFER_NUM; i++) {
        ret = IOConnectMapMemory(connect, i, mach_task_self(), &addr, &size, kIOMapAnywhere | kIOMapInhibitCache);
        if (ret != kIOReturnSuccess)
            throw common::cerror("Failed to map receive buffer into user space!");

        buffers[i] = gsl::span(reinterpret_cast<UInt8 *>(addr), size);
    }
}

Control::~Control()
{
    for (int i=0; i < IPTS_BUFFER_NUM; i++)
        IOConnectUnmapMemory(connect, i, mach_task_self(), (IOVirtualAddress)buffers[i].data());
    
    IOServiceClose(connect);
    IOObjectRelease(service);
}

gsl::span<UInt8> &Control::read_input()
{
    UInt64 idx;
    UInt32 output_size = sizeof(UInt8);
    kern_return_t ret = IOConnectCallScalarMethod(connect, kMethodReceiveInput, nullptr, 0, &idx, &output_size);
    if (ret != kIOReturnSuccess)
        throw common::cerror("Failed to receive input!");
    
    return buffers[idx];
}

void Control::send_hid_report(IPTSHIDReport &report) {
    kern_return_t ret = IOConnectCallStructMethod(connect, kMethodSendHIDReport, &report, sizeof(IPTSHIDReport), nullptr, nullptr);
    if (ret != kIOReturnSuccess)
        throw common::cerror("Failed to send HID report!");
}

void Control::reset()
{
    
}

} // namespace iptsd::ipts
