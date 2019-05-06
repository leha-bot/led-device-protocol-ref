#include <functional>
#include <iostream>
#include <string>
#include <sstream>

#include "led_server/protocol_parser.hpp"
#include "utils/string_view.hpp"
#define BOOST_ASIO_HAS_BOOST_REGEX 0
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#ifdef _WIN32
#include <boost/asio/windows/overlapped_ptr.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/pipes.hpp>

auto create_native_asio_named_pipe_handle(const utils::string_view &name)
{
	// Windows pipes must have name in specified form: R"\\.\pipe\<name>"
	std::string s = name.find(R"(\\.\pipe\)") == 0 ? name.to_string()
		: (std::string(R"(\\.\pipe\)") + name.to_string());
	return ::boost::winapi::create_named_pipe(s.c_str(),
		::boost::winapi::PIPE_ACCESS_DUPLEX_ |
		::boost::winapi::FILE_FLAG_OVERLAPPED_ |
		::boost::winapi::FILE_FLAG_FIRST_PIPE_INSTANCE_,
		0, 1, 65536, 65, 0, nullptr);
}

#else
#include <boost/asio/posix/stream_descriptor.hpp>
#include <sys/stat.h>
auto create_native_asio_named_pipe_handle(const utils::string_view &name)
{
	return mkfifo(name.data(), 0666);
}
#endif

class server_protocol_handler {
	led_server::protocol_parser& parser;
public:
	server_protocol_handler(led_server::protocol_parser& parser)
		:parser(parser) {}

	void handle(boost::system::error_code ec, size_t sz)
	{
		if (sz != 0) {
			parser.handle_command_line("");
		}
		else {
			std::cout << "S: Pipe read error " << ec << std::endl;
		}
	}
};

void start_pipe_server_transport(led_server::protocol_parser &parser)
{
	namespace asio = ::boost::asio;
	auto pipe_native_handle = create_native_asio_named_pipe_handle("test");
	asio::io_context pipe_ctx;
	asio::streambuf buf;	
	// server_protocol_handler handler(parser);
#ifdef _WIN32
	asio::windows::stream_handle pipe_stream_handle(pipe_ctx, p);
	asio::windows::overlapped_ptr pipe_overlapped(pipe_ctx,
		[](boost::system::error_code ec, size_t sz){});
	BOOL ok = ConnectNamedPipe(p, pipe_overlapped.get());
	auto last_error = GetLastError();
	if (!ok && last_error == ERROR_IO_PENDING) {
		// we need to complete pending I/O requests.
		// ownership of the OVERLAPPED object passes to the io_context.
		boost::system::error_code ec(last_error, asio::error::get_system_category());
		pipe_overlapped.complete(ec, 0);
	}
	else {
		if (last_error != 0) {
			// fail;
			std::cout << "S: error " << last_error << " connecting to named pipe!" << std::endl;
			return;
		}
		// connection succeded, ownership of the OVERLAPPED passes to the io_context,
		// we should release this handle.
		pipe_overlapped.release();
	}

	std::string s;
	std::function<void(boost::system::error_code, size_t)> handler = [&pipe_stream_handle, &buf, &parser, &handler](boost::system::error_code ec, size_t sz)
	{
		if (ec) {
			std::cout << "S: error while reading from pipe: " << ec << std::endl;
			return;
		}
		if (sz) {
			std::stringstream ss;
			auto buf_data = buf.data();
			const std::string s(asio::buffer_cast<const char*>(buf_data), buf.size());
			parser.handle_result(ss, parser.handle_command_line(s));
			asio::async_write(pipe_stream_handle, asio::buffer(ss.str()),
				[](boost::system::error_code ec, size_t sz) {});
			asio::async_read_until(pipe_stream_handle, buf, '\n', handler);
		}
		else {
			std::cout << "S: got empty string. Exiting." << std::endl;
		}
	};

	asio::async_read_until(pipe_stream_handle, buf, '\n',
		/*
	[&pipe_stream_handle, &parser, &s, &buf]
	(boost::system::error_code ec, size_t sz) {
		if (sz) {
			std::stringstream ss;
			parser.handle_result(ss, parser.handle_command_line(s));
			asio::async_write(pipe_stream_handle, asio::buffer(ss.str()),
				[](boost::system::error_code ec, size_t sz) {});
		}
		else {
			std::cout << "S: Pipe read error " << ec << std::endl;
		}
	}*/
		handler);
#else
	asio::posix::stream_descriptor fd(pipe_ctx, pipe_native_handle);
	std::function<void(boost::system::error_code, size_t)> handler = [&fd, &buf, &parser, &handler](boost::system::error_code ec, size_t sz)
		{
			if (ec) {
				std::cout << "S: error while reading from pipe: " << ec << std::endl;
				return;
			}
			if (sz) {
				std::stringstream ss;
				auto buf_data = buf.data();
				const std::string s(asio::buffer_cast<const char*>(buf_data), buf.size());
				parser.handle_result(ss, parser.handle_command_line(s));
				asio::async_write(fd, asio::buffer(ss.str()),
					[](boost::system::error_code ec, size_t sz) {});
				asio::async_read_until(fd, buf, '\n', handler);
			}
			else {
				std::cout << "S: got empty string. Exiting." << std::endl;
			}
		};
	
		asio::async_read_until(fd, buf, '\n',
			handler);
#endif
	pipe_ctx.run();
}
