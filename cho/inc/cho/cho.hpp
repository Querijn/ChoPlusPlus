#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

#include <cho/riot_api.hpp>

#include <cho/generated/riot_api_data.hpp>
#include <cho/generated/riot_api_platform.hpp>

#if defined(major)
#undef major
#undef minor
#endif

namespace cho
{
	struct version_t
	{
		version_t() : major(0), minor(0), patch(0) {}
		version_t(uint16_t a_major, uint16_t a_minor, uint16_t a_patch) : major(a_major), minor(a_minor), patch(a_patch) {}

		uint16_t major;
		uint16_t minor;
		uint16_t patch;

		operator std::string() const;

		bool operator ==(const version_t& a_version) const;
		bool operator !=(const version_t& a_version) const;

		bool operator <(const version_t& a_version) const;
		bool operator >(const version_t& a_version) const;
		bool operator >=(const version_t& a_version) const;
		bool operator <=(const version_t& a_version) const;

		bool is_null() const;
		static const version_t null;
	};

	uint64_t time();
	
	extern version_t version;
	extern std::string useragent;

	void set_thread_affinity(std::thread& a_thread, size_t a_core_id);

	template<typename Func>
	void main(Func a_function, riot::api* a_api = riot::api::get_global(), size_t a_thread_count = 0)
	{
		bool aligned = false;
		if (a_thread_count < 1)
		{
			a_thread_count = std::thread::hardware_concurrency();
			aligned = true;
		}

		std::vector<std::thread> threads(a_thread_count);

		for (size_t i = 0; i < a_thread_count; ++i)
		{
			set_thread_affinity(threads[i], i);
			threads[i] = std::thread(a_function);
		}

		for (size_t i = 0; i < a_thread_count; ++i)
			threads[i].join();
	}
}