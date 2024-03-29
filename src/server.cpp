/// @file server.cpp Contains server entry point and device implementation
///                  details.
#include "led_server/protocol_parser.hpp"

#include "utils/string_view.hpp"
#include "utils/enum_converter.hpp"

#include <iostream>
#include <string>
#include <utility>

#include <cstdio>

/// @brief Main namespace for LED server components.
namespace led_server {

/// @brief Represents a device with "raw" state.
struct led_device {
	enum color {
		red, green, blue
	};
	color current_color;
	enum state {
		off, on
	};
	state current_state;
	/// @brief Frequency rate (in Hz, 0-5).
	unsigned int rate;
};

/// @brief Helper conversions for known device enum types.
namespace enum_conversions {
	template <typename Enum>
	using enum_converter = ::utils::enum_converter<Enum>;
	using color = led_device::color;
	using state = led_device::state;

	static const enum_converter<color> color_to_string {
		MAKE_ENUM_MAP_ENTRY_WITH_NS(color, red),
		MAKE_ENUM_MAP_ENTRY_WITH_NS(color, green),
		MAKE_ENUM_MAP_ENTRY_WITH_NS(color, blue)
	};

	static const enum_converter<led_device::state> state_to_string = {
		MAKE_ENUM_MAP_ENTRY_WITH_NS(state, off),
		MAKE_ENUM_MAP_ENTRY_WITH_NS(state, on)
	};
}

/// @brief Interface for our protocol I/O
class led_device_controller {
	led_device &device;

	struct use_enum_conversions;
public:

	//
	led_device_controller(led_device &dev) : device(dev)
	{}

	/// @brief One place to add commands.
	void register_device_commands(protocol_parser &protocol)
	{
		protocol.add_command("set-led-color", [this](const utils::string_view &par) { return this->set_color(par); });
		protocol.add_command("get-led-color", [this](const utils::string_view &par) { return this->get_color(par); });
		protocol.add_command("set-led-state", [this](const utils::string_view &par) { return this->set_state(par); });
		protocol.add_command("get-led-state", [this](const utils::string_view &par) { return this->get_state(par); });
		protocol.add_command("set-led-rate", [this](const utils::string_view &par) { return this->set_rate(par); });
		protocol.add_command("get-led-rate", [this](const utils::string_view &par) { return this->get_rate(par); });
	}

	template <typename T>
	led_server::command_result set(T &param, const utils::string_view &str_value)
	{
		return make_failed_result();
	}

	template<typename T>
	led_server::command_result set(const T &enum_names_map, typename T::key_type &param,
		const utils::string_view &str_value)
	{
		using namespace enum_conversions;

		auto entry = enum_names_map.string_to_enum().find(str_value);
		if (entry == enum_names_map.string_to_enum().end()) {
			return make_failed_result();
		}

		param = entry->second;
		return make_success_result(str_value);
	}

	led_server::command_result set_color(const utils::string_view &param)
	{
		using namespace enum_conversions;

		auto entry = color_to_string.string_to_enum().find(param);
		if (entry == color_to_string.string_to_enum().end()) {
			return make_failed_result();
		}

		device.current_color = entry->second;
		return make_success_result(param);
	}

	led_server::command_result get_color(const utils::string_view &param)
	{
		using namespace enum_conversions;

		if (param.length() != 0) {
			return make_failed_result();
		}

		auto entry = color_to_string.enum_to_string().find(device.current_color);
		if (entry == color_to_string.enum_to_string().end()) {
			return make_failed_result();
		}
		return make_success_result(entry->second);
	}

	led_server::command_result set_state(const utils::string_view &param)
	{
		using namespace enum_conversions;

		auto entry = state_to_string.string_to_enum().find(param);
		if (entry == state_to_string.string_to_enum().end()) {
			return make_failed_result();
		}

		device.current_state = entry->second;
		return make_success_result(param);
	}

	led_server::command_result get_state(const utils::string_view &param)
	{
		using namespace enum_conversions;

		if (param.length() != 0) {
			return make_failed_result();
		}

		auto entry = state_to_string.enum_to_string().find(device.current_state);
		if (entry == state_to_string.enum_to_string().end()) {
			return make_failed_result();
		}
		return make_success_result(entry->second);
	}

	led_server::command_result set_rate(const utils::string_view &param)
	{
		unsigned int temp_rate = 0;
		if (std::sscanf(param.data(), "%u", &temp_rate) <= 0) {
			return make_failed_result();
		}

		if (temp_rate > 5) {
			return make_failed_result();
		}

		device.rate = temp_rate;
		return make_success_result(std::to_string(temp_rate));
	}

	led_server::command_result get_rate(const utils::string_view &param)
	{
		if (param.length() != 0) {
			return make_failed_result();
		}

		return make_success_result(std::to_string(device.rate));
	}
};

} // namespace led_server

/// @brief Smoke tests for protocol parser and device.
/// @todo  Use CTest for it.
namespace tests {

using namespace utils;

void test_led_protocol_parser()
{
	/// @brief Helper class for checking protocol messages parser.
	class TestCommands {
		bool flazhok = false;
	public:
		led_server::command_result set_flazhok(const utils::string_view &param)
		{
			if (param == "false") {
				flazhok = false;
			}
			else if (param == "true") {
				flazhok = true;
			}
			else {
				return led_server::make_failed_result();
			}
			return led_server::make_success_result(param);
		}

		led_server::command_result get_flazhok(const utils::string_view &param)
		{
			return led_server::make_success_result(std::string((flazhok) ? "true" : "false"));
		}
	};

	TestCommands testik;
	led_server::protocol_parser protocol;

	protocol.add_command("set-flag", [&testik](const utils::string_view &param) {return testik.set_flazhok(param); });
	protocol.add_command("get-flag", [&testik](const utils::string_view &param) {return testik.get_flazhok(param); });

	const char *test_commands[] = { "set-flag\n", "set-flag true\n", "get-flag\n",
		"set-flag false\n", "get-flag\n", "set-flag\n" };
	std::cout << "Test Client-server communications protocol parser, via std::cout\n";
	for (auto test_command : test_commands) {
		std::cout << "C: " << test_command;
		std::cout << "S: ";
		protocol.handle_result(std::cout, protocol.handle_command_line(test_command));
	}
}

void test_led_device()
{
	led_server::led_device model;
	led_server::led_device_controller model_protocol_controller(model);
	led_server::protocol_parser protocol_parser;
	model_protocol_controller.register_device_commands(protocol_parser);
	const char *test_commands[] = { "set-led-color red", "get-led-color",
		"set-led-color grin", "get-led-color", "set-led-color green",
		"set-led-colorf", "set-led-state on", "get-led-state", "set-led-state off",
		"set-led-rate 0", "get-led-rate", "get-led-rated", "set-led-rate 6",
		"get-led-rate", "set-led-rate 5"
	};
	std::cout << "Test LED device commands using std::cout\n";
	for (auto command : test_commands) {
		std::cout << "C: " << command << std::endl;
		std::cout << "S: ";
		protocol_parser.handle_result(std::cout, protocol_parser.handle_command_line(command));
	}
}

} // namespace tests

void start_pipe_server_transport(led_server::protocol_parser &parser);

int main(int argc, char *argv[])
{
	// smoke tests
	tests::test_led_protocol_parser();
	tests::test_led_device();

	// init device
	led_server::protocol_parser parser;
	led_server::led_device device;
	led_server::led_device_controller device_commands_controller(device);
	device_commands_controller.register_device_commands(parser);

	// start server
	start_pipe_server_transport(parser);
	return 0;
}
