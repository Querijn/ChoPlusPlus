#include "cho_request/base_request.hpp"

#include <string>

namespace cho
{
	base_request::base_request(const char* a_url) :
		m_request_url(a_url)
	{
	}

	base_request::base_request(const std::string& a_url) :
		m_request_url(a_url)
	{
	}

	cho::json base_request::get_json()
	{
		return cho::json(m_data_received.c_str());
	}

	std::string base_request::get_text()
	{
		return m_data_received;
	}

	void base_request::append_url(const std::string & a_url)
	{
		m_request_url += a_url;
	}

	void base_request::set_url(const std::string & a_url)
	{
		m_request_url = a_url;
	}

	std::string base_request::get_url() const
	{
		return m_request_url;
	}

	size_t base_request::get_response_code() const
	{
		return m_response_code;
	}

	std::vector<std::string> base_request::get_headers_received() const
	{
		return m_headers_received;
	}
}