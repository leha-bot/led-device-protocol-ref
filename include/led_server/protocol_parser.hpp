/// @file led_server/protocol_parser.hpp Contains definitions for main protocol
///       parser class and related types.
#ifndef LED_SERVER_PROTOCOL_PARSER_H_
#define LED_SERVER_PROTOCOL_PARSER_H_
#include "utils/string_view.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace led_server {

/// @brief Represents one command response which will be serialized
///        in uniform way (e.g. "OK red\n" or "FAILED\n").
using command_result = std::pair<bool, const std::string>;
using command_handler = std::function<command_result(const utils::string_view &)>;

inline command_result make_success_result(std::string result)
{
	return { true, result };
}

inline command_result make_success_result(const utils::string_view &result)
{
	return { true, std::string(result.data(), result.length()) };
}

inline command_result make_failed_result()
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
		}
		else {
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
			[](const char c) {return c == ' ' || c == '\n'; }
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

}

#endif