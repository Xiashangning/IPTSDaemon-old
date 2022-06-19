// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.hpp"

#include <common/types.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <ini.h>

namespace iptsd::daemon {

struct iptsd_config_device {
	UInt16 vendor;
	UInt16 product;
};

static bool to_bool(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	return value == "true" || value == "yes" || value == "on" || value == "1";
}

static int parse_dev(void *user, const char *c_section, const char *c_name, const char *c_value)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto *dev = reinterpret_cast<struct iptsd_config_device *>(user);

	std::string section(c_section);
	std::string name(c_name);
	std::string value(c_value);

	if (section != "Device")
		return 1;

	if (name == "Vendor")
		dev->vendor = std::stoi(value, nullptr, 16);

	if (name == "Product")
		dev->product = std::stoi(value, nullptr, 16);

	return 1;
}

static int parse_conf(void *user, const char *c_section, const char *c_name, const char *c_value)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto *config = reinterpret_cast<Config *>(user);

	std::string section(c_section);
	std::string name(c_name);
	std::string value(c_value);

	if (section == "Config" && name == "InvertX")
		config->invert_x = to_bool(value);

	if (section == "Config" && name == "InvertY")
		config->invert_y = to_bool(value);

	if (section == "Config" && name == "Width")
		config->width = std::stoi(value);

	if (section == "Config" && name == "Height")
		config->height = std::stoi(value);

	if (section == "Stylus" && name == "Cone")
		config->stylus_cone = to_bool(value);

	if (section == "Stylus" && name == "DisableTouch")
		config->stylus_disable_touch = to_bool(value);

	if (section == "Touch" && name == "Stability")
		config->touch_stability = to_bool(value);

	if (section == "Touch" && name == "Processing") {
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		config->touch_advanced = value == "advanced";
	}

	if (section == "Touch" && name == "DisableOnPalm")
		config->touch_disable_on_palm = to_bool(value);

	if (section == "Basic" && name == "Pressure")
		config->basic_pressure = std::stof(value);

	if (section == "Cone" && name == "Angle")
		config->cone_angle = std::stof(value);

	if (section == "Cone" && name == "Distance")
		config->cone_distance = std::stof(value);

	if (section == "Stability" && name == "Threshold")
		config->stability_threshold = std::stof(value);

	return 1;
}

void Config::load_dir(const std::string &name)
{
	if (!std::filesystem::exists(name))
		return;

	for (auto &p : std::filesystem::directory_iterator(name)) {
		if (!p.is_regular_file())
			continue;

		struct iptsd_config_device dev {};
		ini_parse(p.path().c_str(), parse_dev, &dev);

		if (dev.vendor != this->info.vendor_id || dev.product != this->info.product_id)
			continue;

		ini_parse(p.path().c_str(), parse_conf, this);
	}
}

Config::Config(IPTSDeviceInfo info) : info(info)
{
	this->load_dir("/usr/local/ipts_config");
}

} // namespace iptsd::daemon
