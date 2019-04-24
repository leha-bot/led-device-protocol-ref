#ifndef UTILS_FILESYSTEM_H_
#define UTILS_FILESYSTEM_H_

#ifndef __cplusplus
#error This Header is designed for use with C++ compiler.
#endif

#if __cplusplus <= 201703L || USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace utils {
namespace filesystem = ::boost::filesystem;
}
#else
#include <filesystem>
namespace utils {
namespace filesystem = ::std::filesystem;
}

#endif


#endif
