#include "cho/rate_limit.hpp"
#include "cho/cache/file_cache.hpp"
#include "cho/riot_api.hpp"
#include "cho/cho.hpp"

#include <mutex>
#include <iostream>
#include <algorithm>
#include <chrono>

#if defined(_MSC_VER)
#include <direct.h>
#endif

#define RATELIMIT_REQUEST_ERROR_EPSILON			10
#define RATELIMIT_REQUEST_ERROR_EPSILON_PERC	0.1

#define RATELIMIT_TIME_ERROR_EPSILON			2000
#define RATELIMIT_TIME_ERROR_EPSILON_PERC		0.1

#define SLEEP_EPSILON 0.0000001

namespace cho
{
	const ratelimit::approach rate_limiter::default_approach = ratelimit::approach::burst;

	ratelimit::ratelimit(const ratelimit_definition& a_definition, size_t a_requests, riot::api* a_riot_api) :
		definition(a_definition), requests(a_requests), start_time(cho::time()), last_request_time(cho::time() - 100000), m_riot_api(a_riot_api)
	{}

	bool ratelimit::has_hit()
	{
		bool t_has_hit = requests >= get_real_request_limit();

		if (m_riot_api->has_verbosity_level(cho::verbosity::log))
			std::cout << "Current: " << requests << "/" << get_real_request_limit() << " (not " << definition.requests << ") so we have " << (t_has_hit ? "hit" : "not hit") << std::endl;
		return t_has_hit;
	}

	std::string to_read_time(uint64_t ms)
	{
		std::time_t t = ms / 1000;
		return std::ctime(&t);
	}

	size_t ratelimit::get_waiting_time_ms(bool a_hit_limit_only)
	{
		if (has_hit())// We hit the limit
		{
			auto t_current_time = cho::time();
			auto t_end_time = (start_time + get_real_duration());

			if (t_end_time < t_current_time) return 0; // We did not hit it

			if (m_riot_api->has_verbosity_level(cho::verbosity::warning))
				std::cout << "Exceeded ratelimit timeout: Gotta wait until " << to_read_time(t_end_time) << std::endl;

			cho::ratelimit::save_all();

			return t_end_time - t_current_time;
		}
		
		if (!a_hit_limit_only) // If we're doing spread 
		{
			auto t_real_duration = get_real_duration();
			uint64_t t_end_time = start_time + t_real_duration;
			uint64_t t_current_time = time();

			if (t_current_time > t_end_time) // Don't have to wait
				return 0;

			auto t_time_since_last_request = t_current_time - last_request_time;
			auto t_time_between_requests = t_real_duration / definition.requests;
			if (t_time_since_last_request > t_time_between_requests) return 0;

			return t_time_between_requests - t_time_since_last_request;
		}
		
		return 0; // If we're not doing burst, don't wait
	}

	size_t ratelimit::get_real_request_limit() const
	{
		// Either x% or a small limit.
		return definition.requests - std::min((double)RATELIMIT_REQUEST_ERROR_EPSILON, RATELIMIT_REQUEST_ERROR_EPSILON_PERC * definition.requests);
	}

	double ratelimit::get_real_duration() const
	{
		return definition.duration_ms + std::min((double)RATELIMIT_TIME_ERROR_EPSILON, definition.duration_ms * RATELIMIT_TIME_ERROR_EPSILON_PERC);
	}

	void ratelimit::push_wait_time()
	{
		last_request_time = cho::time();
	}

	bool rate_limiter::set_approach(ratelimit::approach a_approach)
	{
		if (a_approach == ratelimit::approach::custom)
		{
			if (m_riot_api->has_verbosity_level(cho::verbosity::error))
				std::cout << "Cannot set custom approach via cho::ratelimit::set_approach(ratelimit::approach), please use cho::ratelimit::set_approach(ratelimit::function). Ratelimit approach has not been changed!" << std::endl;
			return false;
		}

		g_approach = a_approach;
		return true;
	}

	bool rate_limiter::set_approach(ratelimit::function a_approach)
	{
		if (a_approach == nullptr) return false;
		g_approach = ratelimit::approach::custom;
		g_approach_function = a_approach;

		return true;
	}

	ratelimit::approach rate_limiter::get_approach()
	{
		return g_approach;
	}

	ratelimit_definition::ratelimit_definition(size_t a_requests, size_t a_seconds) :
		requests(a_requests), duration_ms(a_seconds * 1000)
	{

	}

	size_t ratelimit_set::get_longest_waiting_time_ms(bool a_hit_limit_only)
	{
		size_t t_wait_time = 0;

		for (size_t i = 0; i < size(); i++)
		{
			auto& t_element = (*this)[i];

			auto t_time = t_element.get_waiting_time_ms(a_hit_limit_only);
			if (t_time > t_wait_time)
				t_wait_time = t_time;
		}

		return t_wait_time;
	}

	void ratelimit_set::push_wait_time()
	{
		for (size_t i = 0; i < size(); i++)
		{
			auto& t_element = (*this)[i];
			t_element.push_wait_time();
		}
	}

	void ratelimit_set::increment()
	{
		std::lock_guard<std::mutex> t_lock(mutex);
		for (size_t i = 0; i < size(); i++)
		{
			auto& t_element = (*this)[i];
			t_element.requests++;
		}
	}

	

	// Simple function to parse these rate limit header values
	using ratelimit_numbers = std::vector<std::pair<size_t, size_t>>;
	void get_numbers_from_header(ratelimit_numbers& a_vector, const char* a_string)
	{
		a_vector.clear();
		size_t t_first = 0;

		bool parsing = false;
		size_t t_current_number = 0;
		for (const char* t_char = a_string; *t_char != '\0'; t_char++)
		{
			switch (*t_char)
			{
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				parsing = true;
				t_current_number *= 10;
				t_current_number += (size_t)((*t_char) - '0');
				break;

			case ':':
				if (parsing == false) break;

				t_first = t_current_number;
				t_current_number = 0;
				parsing = false;
				break;

			case ',':
				if (parsing == false) break;

				a_vector.push_back(std::make_pair(t_first, t_current_number));
				t_current_number = 0;
				t_first = 0;
				parsing = false;
				break;
			}
		}

		// If we were parsing, the string ended and we already had a number, we need to finish up.
		if (parsing && t_first != 0)
		{
			a_vector.push_back(std::make_pair(t_first, t_current_number));
			t_current_number = 0;
			t_first = 0;
		}
	}

	void rate_limiter::update_ratelimits(const std::string& a_method, std::vector<std::string> a_headers, ratelimit::type a_type)
	{
		static const char* t_applimit_count = "X-App-Rate-Limit-Count";
		static const char* t_applimit = "X-App-Rate-Limit";
		static const char* t_methodlimit_count = "X-App-Rate-Limit-Count";
		static const char* t_methodlimit = "X-App-Rate-Limit";

		// Get the right header type
		const char* t_limit;
		const char* t_count;
		switch (a_type)
		{
		case ratelimit::type::application:
			t_limit = t_applimit;
			t_count = t_applimit_count;
			break;

		case ratelimit::type::method:
			t_limit = t_methodlimit;
			t_count = t_methodlimit_count;
			break;

		default:
			return;
		}

		ratelimit_numbers t_current;
		ratelimit_numbers t_ratelimit;
		size_t successes = 0;

		// Find the numbers for the current count and the limit
		for (size_t i = 0; i < a_headers.size(); i++) 
		{
			const auto t_header = a_headers[i];
			if (t_header.find(t_count) == 0)
			{
				get_numbers_from_header(t_current, t_header.c_str() + strlen(t_count) + 1);

				successes++;
				if (successes == 2)
					break;
			}
			else if (t_header.find(t_limit) == 0)
			{
				get_numbers_from_header(t_ratelimit, t_header.c_str() + strlen(t_limit) + 1);

				successes++;
				if (successes == 2)
					break;
			}			
		}

		// If we have found both
		if (successes == 2)
		{
			ratelimit_set* t_ratelimit_set = nullptr;
			switch (a_type)
			{
			case ratelimit::type::application:
				t_ratelimit_set = &m_application_ratelimits;
				break;

			case ratelimit::type::method:
				if (m_method_ratelimits.find(a_method) == m_method_ratelimits.end() || m_method_ratelimits[a_method].name.length() == 0)
					m_method_ratelimits[a_method].name = a_method;
				t_ratelimit_set = &m_method_ratelimits[a_method];
				break;
			default:
				return;
			}
			
			if (t_ratelimit_set == nullptr)
				return;

			{
				std::lock_guard<std::mutex> t_lock(t_ratelimit_set->mutex);

				for (size_t i = 0; i < t_ratelimit.size(); i++)
				{
					auto* t_element = t_ratelimit_set->get_ratelimit(t_ratelimit[i].second);
					if (t_element == nullptr) // If it doesn't exist, initialise it
					{
						t_ratelimit_set->push_back(ratelimit(ratelimit_definition(t_ratelimit[i].first, t_ratelimit[i].second), t_current[i].first, m_riot_api));
						continue;
					}

					// if new batch
					if (t_current[i].first < t_element->requests)
					{
						t_element->start_time = cho::time(); // Reset timer to now
					}

					// Update request count
					t_element->requests = t_current[i].first;
				}
			}
		}
	}

	ratelimit_set::ratelimit_set(const std::string & a_name) :
		name(a_name)
	{
	}

	ratelimit_set::ratelimit_set(const ratelimit_set & a_copy) : 
		name(a_copy.name), std::vector<ratelimit>(a_copy)
	{
	}

	ratelimit * ratelimit_set::get_ratelimit(size_t a_duration)
	{
		for (size_t i = 0; i < size(); i++)
		{
			auto& t_element = (*this)[i];
			if (t_element.definition.duration_ms / 1000 == a_duration)
				return &t_element;
		}

		return nullptr;
	}

	size_t ratelimit_set::get_longest_expiry_duration() const
	{
		auto t_current_time = cho::time();
		size_t t_duration = 0;
		for (size_t i = 0; i < size(); i++)
		{
			const auto& t_element = (*this)[i];

			if (t_element.start_time + t_element.definition.duration_ms > t_current_time)
			{
				t_current_time = t_element.start_time + t_element.definition.duration_ms;
				t_duration = t_element.definition.duration_ms;
			}
		}

		return t_duration;
	}

	void ratelimit::load_all()
	{
		// TODO
	}

	void ratelimit::save_all()
	{
		// TODO
	}

	void rate_limiter::queue::add(riot_request* a_add)
	{
		std::lock_guard<std::mutex> t_lock(mutex);
		push_back(a_add);
	}

	bool rate_limiter::queue::remove(riot_request* a_request)
	{
		std::lock_guard<std::mutex> t_lock(mutex);
		erase(std::remove(begin(), end(), a_request), end());
		return empty();
	}

	rate_limiter::rate_limiter(riot::api* a_riot_api) :
		m_riot_api(a_riot_api)
	{
	}

	size_t rate_limiter::should_sleep_for_ms(const std::string& a_method) // TODO: Better place would be the rate_limit cpp file
	{
		switch (g_approach)
		{
		case ratelimit::approach::custom:
			// __debugbreak();
			return 0;

		case ratelimit::approach::burst:
		case ratelimit::approach::spread:
			bool t_is_burst = g_approach == ratelimit::approach::burst;

			auto waiting_time1 = m_application_ratelimits.get_longest_waiting_time_ms(t_is_burst);
			auto waiting_time2 = m_method_ratelimits[a_method].get_longest_waiting_time_ms(t_is_burst);

			return std::max(waiting_time1, waiting_time2);
		}

		if (m_riot_api->has_verbosity_level(cho::verbosity::warning))
			std::cout << "No valid approach found in ratelimiter!" << std::endl;
		return 0;
	}

	void rate_limiter::perform_request(riot_request* a_request, std::function<void()> a_callback)
	{
		g_application_queue.add(a_request);

		std::string t_method = static_cast<int>(a_request->get_platform_id()) + "." + a_request->get_service() + "." + a_request->get_method();

		{
			// Add the queue if missing
			{
				std::lock_guard<std::mutex> t_lock(g_method_queues_mutex);

				if (g_method_queues.find(t_method) == g_method_queues.end())
					g_method_queues[t_method];
			}

			// Lock up the specific queue, and add this request
			std::lock_guard<std::mutex> t_lock(g_method_queues[t_method].mutex);
			g_method_queues[t_method].push_back(a_request);
		}

		m_application_ratelimits.increment();
		m_method_ratelimits[t_method].increment();

		auto t_sleep_ms = should_sleep_for_ms(t_method);
		if (t_sleep_ms > SLEEP_EPSILON)
		{

			if (m_riot_api->has_verbosity_level(cho::verbosity::log))
			{
				std::string t_time = (t_sleep_ms > 10000) ? std::to_string((size_t)(t_sleep_ms / 1000)) + " seconds" : (std::to_string(t_sleep_ms) + " milliseconds");
				std::cout << "Sleeping for " << t_time << "." << std::endl;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds((size_t)t_sleep_ms));
		}

		a_callback();

		bool is_empty = g_application_queue.remove(a_request);
		if (is_empty)
		{
			update_ratelimits(t_method, a_request->get_headers_received(), ratelimit::type::application);
		}

		{
			std::lock_guard<std::mutex> t_lock(g_method_queues[t_method].mutex);
			g_method_queues[t_method].erase(std::remove(g_method_queues[t_method].begin(), g_method_queues[t_method].end(), a_request), g_method_queues[t_method].end());
			if (g_method_queues[t_method].empty())
			{
				update_ratelimits(t_method, a_request->get_headers_received(), ratelimit::type::method);
			}
		}

		m_application_ratelimits.push_wait_time();
		m_method_ratelimits[t_method].push_wait_time();
	}
}