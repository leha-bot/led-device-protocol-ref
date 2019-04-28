#include "utils/filesystem.hpp"
#include "utils/string_view.hpp"

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
			return {false, ""};
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

} // namespace led_server

int main()
{
	utils::filesystem::directory_entry ent;
	return 0;
}
