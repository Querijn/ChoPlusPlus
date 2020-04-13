#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace cho
{
	using int32_t = int32_t;
	using int64_t = int64_t;
	using uint32_t = uint32_t;
	using uint64_t = uint64_t;
	using double_t = double;
	using string_t = std::string;
	using bool_t = bool;

	using time_t = cho::uint64_t;

	using uint_t = unsigned int;
	using int_t = signed int;
	using long_t = long;

	template<typename T>
	class optional
	{
	public:
		optional() {}
		optional(const T& t) : value(t), filled(true) {}

		operator T() const { return value; }
		operator T&() { return value; }

		cho::bool_t filled = false;
		T value;
	};
};
