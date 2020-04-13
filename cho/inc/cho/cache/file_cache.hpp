#pragma once

#include <functional>
#include <exception>
#include <string>
#include <ctime>
#include <map>
#include <memory>

namespace cho
{
	using cache_miss_function = std::function<bool(const std::string&, size_t&, std::string&)>;

	class base_cache
	{
	public:
		static std::shared_ptr<base_cache> get(const std::string& a_key, const cache_miss_function& a_on_miss);
		static bool exists(const std::string& a_key);
		
		virtual void store() = 0;
		virtual void invalidate() = 0;

		uint64_t expiry_date;
		std::string contents;
		std::string key;
	
	protected: 
		base_cache(const std::string& a_key, size_t a_expiry_time_seconds, const std::string& a_contents);

	private:
		base_cache() = delete;
	};

	class file_cache : public base_cache
	{
	public:
		static std::shared_ptr<file_cache> get(const std::string& a_key, const cache_miss_function& a_on_miss);
		static bool exists(const std::string& a_key);

		virtual void store() override;
		virtual void invalidate() override;

	private:
		file_cache(const std::string & a_key, const std::string& a_file_location);
		file_cache(const std::string& a_key, size_t a_expiry_time_seconds, const std::string& a_contents);

		std::string get_location() const;
		static std::string get_location_by_key(const std::string& a_key);
		void write_to_file(const std::string& a_file);
		void read_from_file(const std::string& a_file);

		static std::map<std::string, file_cache> m_file_caches;
	};
}