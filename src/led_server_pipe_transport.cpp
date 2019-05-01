/// @file led_pipe_server.cpp Contains server's transport implementation using 
///       Boost Async Pipes.
#include "led_server/protocol_parser.hpp"

#include "utils/string_view.hpp"
#include "utils/terminal.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/process/async_pipe.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

void start_pipe_server_transport(led_server::protocol_parser &parser)
{
	namespace asio = boost::asio;
	namespace bp = boost::process;
	asio::io_context pipe_io_ctx;
	std::ostringstream pipe_name_ostream;
	pipe_name_ostream << R"(\\.\pipe\)" << "led_server_pipe_" << std::this_thread::get_id();
	std::string pipe_name = pipe_name_ostream.str();
	std::cout << "Try to open named pipe " << pipe_name << std::endl;
	utils::adjust_terminal_charset();
	try {
		bp::async_pipe server_pipe(pipe_io_ctx, pipe_name);

		asio::streambuf buf;
		asio::async_read(server_pipe, buf,
			[&buf](const boost::system::error_code &ec, std::size_t size) {
			if (size != 0) {
				std::cout << "S: got string from pipe: \"" << &buf << "\"" << std::endl;
				// waiting until '\n'
			}
			else {
				std::cerr << "error " << ec << std::endl;
			}
		}
		);
		pipe_io_ctx.run();
	}
	catch (std::system_error &ec) {
		std::cout << "System Error: " << ec.what() << ", error code: " << ec.code() << std::endl;
	}

}