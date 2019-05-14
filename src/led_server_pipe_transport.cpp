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
#include <fcntl.h>


namespace utils { namespace pipes {

using platform_stream_handle = ::boost::asio::posix::stream_descriptor;

/// @brief Pair of fds for read and write.
///        Created because open(3) doesn't guarantee that
///        O_RDWR flag will work for FIFO files.
class asio_pipe_stream_pair {

	boost::asio::posix::stream_descriptor rd;
	boost::asio::posix::stream_descriptor wr;
public:
	asio_pipe_stream_pair(boost::asio::io_context &context,
									 int rd_fd, int wr_fd)
		: rd(context, rd_fd), wr(context, wr_fd) {}

	boost::asio::posix::stream_descriptor &read_stream()
	{
		return rd;
	}

	boost::asio::posix::stream_descriptor &write_stream()
	{
		return wr;
	}
};

/// @brief Helper function to get read stream
decltype(auto) get_asio_read_stream(asio_pipe_stream_pair &p)
{
	return p.read_stream();
}

/// @brief Helper function to get read stream
decltype(auto) get_asio_write_stream(asio_pipe_stream_pair &p)
{
	return p.write_stream();
}

class asio_named_pipe_stream_pair : public asio_pipe_stream_pair {
public:

};

// TODO: raii.
auto create_pipe(::boost::asio::io_context &io_ctx, const utils::string_view &name)
{
	remove(name.data());	
	int err = mkfifo(name.data(), 0666);
	if (err != 0) {
		std::cout << "S: can't create named pipe: ";
		perror("mkfifo");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}
	int fd_rd = open(name.data(), O_RDONLY | O_NONBLOCK);
	if (fd_rd == -1) {
		std::cout << "S: can't open named pipe for reading: " << std::flush;
		perror("open");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}

	int fd_wr = open(name.data(), O_WRONLY);
	if (fd_wr == -1) {
		std::cout << "S: can't open named pipe for writing: " << std::flush;
		perror("open");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}
	return asio_pipe_stream_pair{io_ctx, fd_rd, fd_wr};
}


asio_pipe_stream_pair create_pipe_pair(::boost::asio::io_context &io_ctx, const utils::string_view &name)
{
	std::string str = name.data();

	// client pipe: client would write, server can read.
	str.append("_client");
	remove(str.c_str());

	int err = mkfifo(str.c_str(), 0666);
	if (err != 0) {
		std::cout << "S: can't create named pipe " << str << " for reading: ";
		perror("mkfifo");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}

	int fd_rd = open(str.data(), O_RDONLY | O_NONBLOCK);
	if (fd_rd == -1) {
		std::cout << "S: can't open named pipe for reading: ";
		perror("open");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}

	// server pipe: server could write, client would read.
	str = name.to_string() + "_server";
	std::cout << str;
	remove(str.c_str());

	err = mkfifo(str.c_str(), 0666);
	if (err != 0) {
		std::cout << "S: can't create named pipe " << str << " for reading: " << std::flush;
		perror("mkfifo");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}

	// NOTE about O_NONBLOCK (from fifo(7) manual):
	// A process can open a FIFO in nonblocking mode.  In this  case,  opening
	// for read-only succeeds even if no one has opened on the write side yet
	// and opening for write-only fails with ENXIO (no such device or address)
	// unless the other end has already been opened.

	// NOTE about using O_RDWR (from fifo(7) manual):
	// Under  Linux,  opening  a  FIFO for read and write will succeed both in
	// blocking and nonblocking mode.  POSIX leaves this  behavior  undefined.
	// This  can be used to open a FIFO for writing while there are no readers
	// available.  A process that uses both ends of the connection in order to
	// communicate with itself should be very careful to avoid deadlocks.

	int fd_wr = open(str.c_str(), O_WRONLY);
	if (fd_wr == -1) {
		std::cout << "S: can't open named pipe " << str << " for writing: " << std::flush;
		perror("open");
		return asio_pipe_stream_pair{io_ctx, -1, -1};
	}
	return asio_pipe_stream_pair{io_ctx, fd_rd, fd_wr};
}

} // namespace pipes
} // namespace utils
#endif

/// @brief Transport-agnostic handler logic.
class server_protocol_handler {
	led_server::protocol_parser &parser;

public:
	server_protocol_handler(led_server::protocol_parser &parser)
		:parser(parser) {}

	std::string handle_request(const std::string &data)
	{
		std::stringstream ss;
		parser.handle_result(ss, parser.handle_command_line(data));
		return ss.str();
	}
};

std::string handle_server_request(const std::string &request, led_server::protocol_parser &parser)
{
	std::stringstream ss;
	parser.handle_result(ss, parser.handle_command_line(request));
	return ss.str();
}

template <typename PlatformStreamHandle>
class TransportHandler {
	using asio_streambuf = ::boost::asio::streambuf;
	using boost_error_code = ::boost::system::error_code;
	
	asio_streambuf buf;
	led_server::protocol_parser &parser;
	PlatformStreamHandle &platform_stream_handle;
public:
	TransportHandler(led_server::protocol_parser &parser,
						  PlatformStreamHandle &descr) 
		: parser(parser), platform_stream_handle(descr) {}
	
	asio_streambuf &get_asio_buffer()
	{
		return buf;
	}
	
	std::string get_request_data(size_t request_size)
	{
		std::string s{::boost::asio::buffer_cast<const char *>(buf.data()), buf.size()};
		buf.consume(request_size);
		return s;
	}
	
	void handle(const boost_error_code &ec, std::size_t sz)
	{
		namespace asio = ::boost::asio;
		if (ec) {
			std::cout << "S: error while reading from pipe: " << ec << std::endl;
			return;
		}
		if (sz == 0) {
			std::cout << "S: got empty string. Exiting." << std::endl;
			return;
		}
		std::cout << "S: read " << sz << " bytes from pipota" << std::endl;
		auto data = get_request_data(sz);
		std::cout << "S: incoming string: " << data << std::endl;
		auto result = handle_server_request(data, parser);
		std::cout << "S: after handling we got this: " << result << std::endl;
		asio::async_write(
					utils::pipes::get_asio_write_stream(platform_stream_handle), asio::buffer(result),
						  [](const boost_error_code &ec, size_t /* sz */) {
			if (ec) {
				std::cout << "S: can't send response: error " << ec << std::endl;
			}
		});
		
		asio::async_read_until(
					utils::pipes::get_asio_read_stream(platform_stream_handle), buf, '\n',
							   boost::bind(&TransportHandler<PlatformStreamHandle>::handle, this,
										   asio::placeholders::error, asio::placeholders::bytes_transferred)
							   );
	}
};

void start_pipe_server_transport(led_server::protocol_parser &parser)
{
	namespace asio = ::boost::asio;
	std::string pipe_name = "test3";
	std::cout << "S: opening pipe " << pipe_name << std::endl;
	asio::io_context pipe_ctx;
	auto pipe = utils::pipes::create_pipe(pipe_ctx, pipe_name);
#ifdef _WIN32
	using platrform_stream_handle = asio::windows::stream_handle;
	platform_stream_handle pipe_stream_handle(pipe_ctx, p);
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
#endif
	using platform_stream_handle = utils::pipes::platform_stream_handle;
	
	try {		
		// platform_stream_handle fd(pipe_ctx, pipe_native_handle);
		TransportHandler<utils::pipes::asio_pipe_stream_pair> handler{parser, pipe};
		if (false) {
			std::cout << "Waiting connections to pipe " << pipe_name
					  << "_client. Answer will be posted to "
					  << pipe_name << "_server.\n";
		}
		pipe.read_stream().async_wait(asio::posix::stream_descriptor::wait_read,
					  [&pipe, &handler](const boost::system::error_code &ec) {
			using namespace std::placeholders;
			if (ec != 0) {
				std::cout << "S: failed to wait for pipe object: error " << ec << "(" << ec.message() << ")" << std::endl;
			}
			asio::async_read_until(pipe.read_stream(), handler.get_asio_buffer(), '\n',
								   boost::bind(&decltype(handler)::handle, &handler, asio::placeholders::error,
											   asio::placeholders::bytes_transferred));
		});
		pipe_ctx.run();
	} catch (std::runtime_error &e) {
		std::cerr << "Error opening pipe fd: " << e.what();
	}
}
