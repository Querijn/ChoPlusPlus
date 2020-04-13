#include "cho/cache/file_cache.hpp"
#include "cho/cho.hpp"
#include "cho/hash.hpp"

#if defined(_MSC_VER)
#include <direct.h>
#endif

#include <fstream>
#include <vector>

namespace cho
{
	std::shared_ptr<base_cache> base_cache::get(const std::string & a_key, const cache_miss_function& a_on_miss) { return nullptr; }
	bool base_cache::exists(const std::string & a_key) { return false; }

	base_cache::base_cache(const std::string& a_key, size_t a_expiry_time_seconds, const std::string& a_contents) :
		expiry_date(cho::time() + a_expiry_time_seconds), contents(a_contents), key(a_key)
	{ }
	
	file_cache::file_cache(const std::string& a_key, size_t a_expiry_time_seconds, const std::string& a_contents) :
		base_cache(a_key, a_expiry_time_seconds, a_contents)
	{
	}

	file_cache::file_cache(const std::string & a_key, const std::string& a_file_location) :
		base_cache(a_key, 0, "")
	{
		read_from_file(a_file_location);
	}

	std::string file_cache::get_location() const
	{
		return file_cache::get_location_by_key(key);
	}

	std::string file_cache::get_location_by_key(const std::string& a_key)
	{
		auto t_key = a_key;
		for (auto& t_char : t_key)
		{
			switch (t_char)
			{
			#if _WIN32
				case '\\':
				case '/':
				case ':':
				case '*':
				case '"':
				case '<':
				case '>':
				case '|':
			#elif __OSX__
				case ':':
				case '/':
			#elif __linux__
				case '/':
			#else
			#pragma error "undefined platform!"
			#endif
					t_char = '_';
					break;

				default: 
					continue;
			}
		}

		return "cho_cache/" + t_key;
	}

	void file_cache::write_to_file(const std::string & a_file)
	{
		std::ofstream t_file(a_file, std::ofstream::binary);

		// write time
		t_file.write(reinterpret_cast<char*>(&expiry_date), sizeof(expiry_date));

		// write object
		t_file << contents;
		t_file.close();
	}

	void file_cache::read_from_file(const std::string & a_file)
	{
		std::ifstream t_file(a_file, std::ofstream::binary);

		// read time
		t_file.read(reinterpret_cast<char*>(&expiry_date), sizeof(expiry_date));

		// get file size
		t_file.seekg(0, t_file.end);
		auto t_file_size = t_file.tellg();

		// read json
		// TODO: Can I do this without the copy?
		t_file.seekg(sizeof(expiry_date));
		std::vector<char> t_buffer((size_t)t_file_size);
		t_file.read(t_buffer.data(), t_file_size);
		contents = t_buffer.data();

		t_file.close();
	}

	std::shared_ptr<file_cache> file_cache::get(const std::string & a_key, const cache_miss_function& a_on_miss)
	{
		auto t_file_name = get_location_by_key(a_key);

		// If cache folder does not exist
		struct stat t_stat_buffer;
		if (stat("cho_cache", &t_stat_buffer) != 0)
		{
#if defined _MSC_VER
			_mkdir("cho_cache");
#elif defined __GNUC__
			mkdir("cho_cache", 0777);
#endif
		}

		// If file does not exist
		if (stat(t_file_name.c_str(), &t_stat_buffer) != 0)
		{
			std::string t_contents;
			size_t expiry_time_seconds = 60;
			if (a_on_miss(a_key, expiry_time_seconds, t_contents) == false)
				return nullptr;

			file_cache t_cache(a_key, expiry_time_seconds, t_contents);
			t_cache.write_to_file(t_file_name);
			return std::shared_ptr<file_cache>(new file_cache(t_cache));
		}

		// If expired
		file_cache t_cache(a_key, t_file_name);
		auto t_time = cho::time();
		if (t_cache.expiry_date <= t_time)
		{
			std::string t_element;
			size_t expiry_time_seconds = 60;
			if (a_on_miss(a_key, expiry_time_seconds, t_cache.contents) == false)
				return nullptr;

			t_cache.expiry_date = cho::time() + expiry_time_seconds;
			t_cache.write_to_file(t_file_name);
			return std::shared_ptr<file_cache>(new file_cache(t_cache));
		}

		return std::shared_ptr<file_cache>(new file_cache(t_cache));
	}

	bool file_cache::exists(const std::string & a_key)
	{
		auto t_file_name = get_location_by_key(a_key);

		// If cache folder does not exist
		struct stat t_stat_buffer;
		if (stat("cho_cache", &t_stat_buffer) != 0)
			return false;

		// If file does not exist
		if (stat(t_file_name.c_str(), &t_stat_buffer) != 0)
			return false;

		return true;
	}

	void file_cache::store()
	{
		write_to_file(get_location_by_key(key));
	}

	void file_cache::invalidate()
	{
		expiry_date = cho::time() - 1;
		contents = "null";
		store();
	}
}
