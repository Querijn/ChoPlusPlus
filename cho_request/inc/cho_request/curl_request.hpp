#pragma once

#include <cho_request/base_request.hpp>

#include <string>
#include <memory>

namespace cho
{
	struct curl_request_impl;
	class curl_request : public base_request
	{
	public:
		curl_request(const std::string& a_url);
		~curl_request();

		virtual void						add_header(const std::string& a_header, const std::string& a_value);

		virtual void						perform();

		static std::string					url_encode(const std::string& a_string);

		double								get_response_time() const;
	private:
		static size_t handle_result(void* a_content, size_t a_size, size_t a_nmemb, void* a_user_data);
		static size_t handle_header(char* a_buffer, size_t a_size, size_t a_item_count, void* a_user_data);

		std::unique_ptr<curl_request_impl> m_impl;
		
		static bool m_curl_initialised;
	};
}