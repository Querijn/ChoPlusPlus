#pragma once 

#include <cho_request/request.hpp>
#include <cho/generated/riot_api_platform.hpp>

namespace cho
{
	namespace riot { class api; }
	class riot_request : public cho::request
	{
	public:
		riot_request(const platform_id& a_platform_id, const std::string& a_service, const std::string& a_method, const std::string& a_url, riot::api* a_riot_api);

		virtual void				perform();

		bool						should_retry() const;
		virtual bool				should_throw_on_response_code() const;

		platform_id get_platform_id() const;
		std::string get_service() const;
		std::string get_method() const;

		// TODO:
		/*
		template<typename cache_type, typename = std::is_base_of<cache_type, cho::base_cache>::value> // also, make sure this is correct
		static void add_cache();
		*/

	private:
		void uncached_perform();

		platform_id m_platform_id;
		std::string m_service;
		std::string m_method;
		riot::api* m_riot_api;
	};
}