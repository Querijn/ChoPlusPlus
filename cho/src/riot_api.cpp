#include "cho/riot_api.hpp"
#include "cho/rate_limit.hpp"

#include <iostream>

namespace cho
{
	const verbosity default_verbosity = verbosity::errors;

	namespace riot
	{
		api* api::m_global;

		api::api(const std::string& a_key, ratelimit::approach a_approach, verbosity a_verbosity) :
			m_key(a_key),
			m_verbosity(a_verbosity),
			m_ratelimiter(this)
		{ 
			m_global = this;
		}

		std::shared_ptr<riot_request> api::make_request(cho::platform_id a_platform, const char* a_service, const char* a_method, const char* a_path)
		{
			return std::make_shared<riot_request>(a_platform, a_service, a_method, a_path, this);
		}

		bool api::has_verbosity_level(cho::verbosity a_verbosity)
		{
			return m_verbosity & a_verbosity;
		}

		api::~api()
		{
			if (this == m_global)
				m_global = nullptr;
		}

		api* api::get_global()
		{
			return m_global;
		}
	}
}