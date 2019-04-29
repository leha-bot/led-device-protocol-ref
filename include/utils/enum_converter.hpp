#ifndef UTILS_ENUM_STRING_H_
#define UTILS_ENUM_STRING_H_

#ifndef __cplusplus
#error This Header is designed for use with C++ compiler.
#endif

#include "utils/string_view.hpp"
#include <boost/bimap.hpp>

namespace utils {

/// @brief Converts enum value to string view.
template <typename Enum>
class enum_converter {
	boost::bimap<Enum, utils::string_view> enum_map;
public:
	enum_converter(std::initializer_list<std::pair<Enum, utils::string_view>> il)
	{
		for (auto elem : il) {
			enum_map.left.insert(elem);
		}
	}

	auto &enum_to_string() const
	{
		return enum_map.left;
	}

	auto &string_to_enum() const
	{
		return enum_map.right;
	}

	auto find(Enum val) const
	{
		return enum_map.left.find(val);
	}

	auto find(const utils::string_view &val) const
	{
		return enum_map.right.find(val);
	}

	Enum operator[](const utils::string_view &enum_str) const
	{
		return enum_map[enum_str];
	}

	const utils::string_view & operator[](Enum val) const
	{
		return enum_map[val];
	}
};

}

/// @brief Helper macro for getting string representation of enum's value in map.
#define MAKE_ENUM_MAP_ENTRY(value) {value, #value}

/// @brief Helper macro which also skips namespace name.
#define MAKE_ENUM_MAP_ENTRY_SKIP_NS(value, ns) {value, #value + sizeof(#ns) + 2}
#define MAKE_ENUM_MAP_ENTRY_WITH_NS(ns, value) {ns :: value, #value}

#endif