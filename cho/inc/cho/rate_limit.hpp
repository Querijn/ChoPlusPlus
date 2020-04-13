#pragma once

#include <ctime>
#include <functional>
#include <mutex>
#include <vector>
#include <map>

namespace cho
{
	namespace riot { class api; class request; }

	struct ratelimit_definition
	{
		ratelimit_definition(size_t a_requests, size_t a_seconds);

		size_t requests;
		size_t duration_ms;
	};

	struct ratelimit
	{
		using function = std::function<unsigned int(std::vector<ratelimit>)>;
		enum class approach
		{
			burst,
			spread,
			custom
		};

		enum class type
		{
			application,
			method,
			service
		};

		explicit ratelimit(bool a_dont_use) : definition(0, 0) {}
		ratelimit(const ratelimit_definition& a_definition, size_t a_requests, riot::api* a_riot_api);

		bool has_hit();
		size_t get_waiting_time_ms(bool a_hit_limit_only);
		size_t get_real_request_limit() const;
		double get_real_duration() const;
		void push_wait_time();

		size_t requests = 0;
		uint64_t start_time = 0;
		uint64_t last_request_time = 0;
		ratelimit_definition definition;
		riot::api* m_riot_api = nullptr;

		void load_all();
		void save_all();
	};

	struct ratelimit_set : public std::vector<ratelimit>
	{
		ratelimit_set() {}
		ratelimit_set(const std::string& a_name);
		ratelimit_set(const ratelimit_set& a_copy);
		ratelimit_set& operator=(const ratelimit_set& a_copy);

		ratelimit* get_ratelimit(size_t a_duration);
		size_t get_longest_waiting_time_ms(bool a_hit_limit_only);

		std::mutex mutex;
		std::string name;

		void push_wait_time();
		void increment();

		size_t get_longest_expiry_duration() const;
	};

	class rate_limiter
	{
	public:
		static const ratelimit::approach default_approach;
		rate_limiter(riot::api* a_riot_api);

		friend class riot_request;
	private:
		size_t should_sleep_for_ms(const std::string& a_method);
		void perform_request(riot_request* a_request, std::function<void()> a_callback);
		void update_ratelimits(const std::string& a_method, std::vector<std::string> a_headers, ratelimit::type a_type);
		bool set_approach(ratelimit::approach a_approach);
		bool set_approach(ratelimit::function a_approach);
		ratelimit::approach get_approach();

		struct queue : public std::vector<riot_request*>
		{
			queue() {}
			std::mutex mutex;

			void add(riot_request* a_add);
			bool remove(riot_request* a_remove);
		};

		riot::api* m_riot_api;
		queue g_application_queue;
		std::map<std::string, queue> g_method_queues;
		std::mutex g_method_queues_mutex;

		std::map<std::string, ratelimit_set> m_method_ratelimits;
		ratelimit_set m_application_ratelimits;
		ratelimit::function g_approach_function = nullptr;

		ratelimit::approach g_approach = default_approach;
	};
}