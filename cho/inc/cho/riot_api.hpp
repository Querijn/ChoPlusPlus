#pragma once

#include <string>
#include <memory>

#include <cho/rate_limit.hpp>
#include <cho/riot_api_request.hpp>

namespace cho
{
	enum verbosity // TODO: put this in a nice place.
	{
		none	= 0b0000,
		log		= 0b1000,
		warning	= 0b0100,
		error	= 0b0010,
		fatal	= 0b0001,

		everything = log | warning | error | fatal,
		warnings = warning | error | fatal,
		errors = error | fatal,
	};

	extern const verbosity default_verbosity;

	namespace riot
	{
		class api
		{
		public:
			api(const std::string& a_key, ratelimit::approach a_approach = rate_limiter::default_approach, verbosity a_verbosity = default_verbosity);
			~api();

			static api* get_global();

			std::shared_ptr<riot_request> make_request(cho::platform_id a_platform, const char* a_service, const char* a_method, const char* a_path);

			bool has_verbosity_level(cho::verbosity a_verbosity);

			friend class riot_request;
		private:
			static api* m_global;
			std::string m_key;
			verbosity m_verbosity;
			rate_limiter m_ratelimiter;
		};
	}
}