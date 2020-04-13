#include "cho/riot_api_request.hpp"
#include "cho/settings.hpp"
#include "cho/cache/file_cache.hpp"
#include "cho/riot_api.hpp"
#include "cho/hash.hpp"

#include <chrono>
#include <iostream>

namespace cho
{
	riot_request::riot_request(const platform_id& a_platform_id, const std::string & a_service, const std::string & a_method, const std::string & a_url, riot::api* a_riot_api) :
		cho::request(a_url), m_platform_id(a_platform_id), m_service(a_service), m_method(a_method), m_riot_api(a_riot_api)
	{
		add_header("Accept-Charset", "charset=UTF-8");
		add_header("X-Riot-Token", m_riot_api->m_key);
	}

	void riot_request::perform()
	{
		if (m_riot_api->has_verbosity_level(cho::verbosity::log))
			std::cout << "Requesting " << m_request_url << std::endl;

		// Load cache if not loaded
		/*if (!cho::settings::cache) cho::settings::cache = std::unique_ptr<cho::settings::cache_settings>(new cho::settings::cache_settings());*/

		// The cache type. Usually service, but we want to deviate from it when we get the matchlist.
		std::string t_cache_type = m_method.find("matchlist") != std::string::npos ? "matchlist" : m_service;
		auto t_cache_time = 3600000; // cho::settings::cache->get_duration(t_cache_type);
		if (t_cache_time != 0)
		{
			if (m_riot_api->has_verbosity_level(cho::verbosity::log))
				std::cout << "Requesting " << m_service << "::" << m_method << " from cache." << std::endl;

			bool t_needs_copy = true; // If we're fetching from cache, the data needs to be copied from the cache

			auto t_cache = cho::file_cache::get(m_request_url, [&](const std::string& a_key, size_t& a_seconds_to_cache, std::string& a_contents) -> bool
			{
				if (m_riot_api->has_verbosity_level(cho::verbosity::log))
					std::cout << m_service << "::" << m_method << " was not in the cache." << std::endl;

				t_needs_copy = false;
				uncached_perform();

				if (get_response_code() != 200) return false;

				if (m_riot_api->has_verbosity_level(cho::verbosity::log))
					std::cout << "Succesfully requested " << m_service << "::" << m_method << ", saving to cache for " << t_cache_time / 1000.0 << " seconds." << std::endl;

				a_seconds_to_cache = t_cache_time;
				a_contents = get_text();
				return true;
			});

			if (t_needs_copy && t_cache != nullptr)
			{
				m_data_received = t_cache->contents;
				m_response_code = 200;
			}
			else if (t_cache == nullptr) // The data failed.
			{
				m_response_code = 412;
				m_data_received = "null";
			}

			return;
		}
		
		uncached_perform();
	}

	void riot_request::uncached_perform()
	{
		m_riot_api->m_ratelimiter.perform_request(this, [&]()
		{
			auto t_tries_total = 3;
			for (int tries = 0; tries < t_tries_total; tries++)
			{
				cho::request::perform();
				if (!should_retry())
					return;

				// If it was bad, we would've not been here
				// Retrying after 3 seconds
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(3s);
			}

			if (m_riot_api->has_verbosity_level(cho::verbosity::error))
				std::cout << "After " << t_tries_total << " tries, still haven't got a valid response from server for " << m_request_url << std::endl;
		});
	}

	bool riot_request::should_retry() const
	{
		if (m_response_code >= 200 && m_response_code < 300)
			return false;

		if (m_response_code >= 500 && m_response_code < 600)
			return true;

		switch (m_response_code)
		{
		default:
		case 403:
		case 404:
			return false;

		case 429:
		case 412:
			return true;
		}
	}

	bool riot_request::should_throw_on_response_code() const
	{
		if (m_response_code == 429)
		{
			for (size_t i = 0; i < m_headers_received.size(); i++)
			{
				const std::string& t_header = m_headers_received[i];

				// If we find a Retry-After or the type is method or application, we messed up.
				if ((t_header.find("Retry-After") == std::string::npos) ||
					(t_header.find("X-Rate-Limit-Type") == 0 && (t_header.find("method") != std::string::npos || t_header.find("application") != std::string::npos)))
					return true;

				if (m_riot_api->has_verbosity_level(cho::verbosity::warning))
					std::cout << "Service '" << m_service << "' rate limited! This is unlikely to be your fault, but this was the method name: " << m_method << std::endl;
				return false;
			}
		}
		else if (m_response_code >= 500)
		{
			if (m_riot_api->has_verbosity_level(cho::verbosity::warning))
				std::cout << "Server sent " << m_response_code << " error from the server. " << std::endl;
			return false;
		}

		return m_response_code != 200;
	}

	platform_id riot_request::get_platform_id() const
	{
		return m_platform_id;
	}

	std::string riot_request::get_service() const
	{
		return m_service;
	}

	std::string riot_request::get_method() const
	{
		return m_method;
	}
}
