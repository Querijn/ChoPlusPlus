#include "cho/cho.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace cho
{
	version_t version = version_t(1, 0, 1);
	std::string useragent = std::string("ChoPlusPlus v") + std::string(version);

	const version_t version_t::null = version_t(0, 0, 0);

	version_t::operator std::string() const
	{
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
	}

	bool version_t::operator==(const version_t & a_version) const
	{
		return major == a_version.major && minor == a_version.minor && patch == a_version.patch;
	}

	bool version_t::operator!=(const version_t & a_version) const
	{
		return operator==(a_version) == false;
	}

	bool version_t::operator<(const version_t & a_version) const
	{
		if (major < a_version.major)
			return true;

		if (major > a_version.major)
			return false;

		// At this point `major` can only be equal to `a_version.major`

		if (minor < a_version.minor)
			return true;

		if (minor > a_version.minor)
			return false;

		// At this point `minor` can only be equal to `a_version.minor`
		return patch < a_version.patch;
	}

	bool version_t::operator>(const version_t & a_version) const
	{
		if (major > a_version.major)
			return true;

		if (major < a_version.major)
			return false;

		// At this point `major` can only be equal to `a_version.major`

		if (minor > a_version.minor)
			return true;

		if (minor < a_version.minor)
			return false;

		// At this point `minor` can only be equal to `a_version.minor`
		return patch > a_version.patch;
	}

	bool version_t::operator>=(const version_t & a_version) const
	{
		return operator<(a_version) == false;
	}

	bool version_t::operator<=(const version_t & a_version) const
	{
		return operator>(a_version) == false;
	}

	bool version_t::is_null() const
	{
		return *this == version_t::null;
	}

	uint64_t time()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	void set_thread_affinity(std::thread& a_thread, size_t a_core_id)
	{
	#if defined(_WIN32)
		SetThreadAffinityMask(a_thread.native_handle(), (static_cast<DWORD_PTR>(1) << a_core_id));
	#else
		#pragma warning "Cannot determine how to assign a thread to a core. We need a custom implementation; it won't do this for now!"
	#endif
	}
}