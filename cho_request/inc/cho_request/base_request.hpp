#pragma once

#include <cho_json/json.hpp>

#include <future>
#include <string>
#include <vector>

namespace cho
{
	class base_request
	{
	public:
		base_request() = delete;

		base_request(const char* a_url);
		base_request(const std::string& a_url);

		virtual cho::json					get_json();
		virtual std::string					get_text();

		void								append_url(const std::string& a_url);
		void								set_url(const std::string& a_url);
		std::string							get_url() const;

		virtual void						add_header(const std::string& a_header, const std::string& a_value) = 0;

		virtual void						perform() = 0;

		size_t								get_response_code() const;
		std::vector<std::string>			get_headers_received() const;
	protected:
		std::string m_data_received;
		std::vector<std::string> m_headers_received;

		std::string m_request_url;
		size_t m_response_code = 0;
	};
}