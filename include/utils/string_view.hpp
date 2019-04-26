#ifndef UTILS_STRING_VIEW_H_
#define UTILS_STRING_VIEW_H_

#ifndef __cplusplus
#error This Header is designed for use with C++ compiler.
#endif

#if __cplusplus <= 201703L || USE_BOOST_STRING_VIEW
#include <boost/utility/string_view.hpp>
namespace utils {
using string_view = ::boost::string_view;
}
#else
#include <string_view>
namespace utils {
using string_view = ::std::string_view;
}

#endif


#endif
