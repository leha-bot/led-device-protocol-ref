#include "utils/filesystem.hpp"
#include "utils/string_view.hpp"
#include "utils/enum_converter.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>

/// @brief Main namespace for LED server components.
namespace led_server {

/// @brief Represents one command response which will be serialized
///        in uniform way (e.g. "OK red\n" or "FAILED\n").
using command_result = std::pair<bool, const std::string>;
using command_handler = std::function<command_result(const utils::string_view &)>;

command_result make_success_result(std::string result)
{
	return { true, result };
}

command_result make_success_result(const utils::string_view &result)
{
	return { true, std::string(result.data(), result.length()) };
}

command_result make_failed_result()
{
	return { false, "" };
}

/// @brief Hosts commands and passes the appropriate commands to the handlers
class protocol_parser {
	std::map<std::string, command_handler, std::less<>> commands;

public:
	template <typename Ostream>
	static void handle_result(Ostream &os, const command_result &result)
	{
		if (result.first) {
			os << "OK";
			if (result.second.length() != 0) {
				os << " " << result.second;
			}
			os << "\n";
		} else {
			os << "FAILED\n";
		}
	}

	protocol_parser &add_command(std::string name, command_handler newCommand)
	{
		commands.emplace(name, newCommand);
		return *this;
	}

	command_result handle_command_line(const std::string &line)
	{
		// get command token delimiter pos
		auto delim = std::find_if(line.begin(), line.end(),
				[](const char c) {return c == ' ' || c == '\n';}
			);

		utils::string_view command_name_sv(line.data(),
			std::distance(line.begin(), delim));

		auto command = commands.find(command_name_sv);
		if (command == commands.end()) {
			// command is not found
			return make_failed_result();
		}
		if (delim == line.end() || *delim == '\n') {
			return command->second(utils::string_view{ "" });
		}
		else {
			utils::string_view parameter(&delim[1],
				std::distance(delim + 1,
					std::find(delim + 1, line.end(), '\n')
					)
			);

			return command->second(parameter);
		}
	}
};

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

} // namespace led_server

int main()
{
	utils::filesystem::directory_entry ent;
	return 0;
}
