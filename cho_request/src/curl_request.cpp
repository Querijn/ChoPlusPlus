#include <cho_request/curl_request.hpp>

#include <curl/curl.h>
#include <string>

#define CURL_REQUEST_BUFFER_SIZE 32768

namespace cho
{
	struct curl_request_impl
	{
		static CURL* m_static_curl;

		CURL* m_curl = nullptr;
		curl_slist* m_headers = nullptr;
	};

	CURL* curl_request_impl::m_static_curl = nullptr;

	bool curl_request::m_curl_initialised = false;

	curl_request::curl_request(const std::string& a_url) :
		base_request(a_url), m_impl(std::make_unique<curl_request_impl>())
	{
		if (m_curl_initialised == false)
			m_curl_initialised = curl_global_init(CURL_GLOBAL_ALL) == CURLcode::CURLE_OK;
	}

	curl_request::~curl_request()
	{
		if (m_impl->m_curl != nullptr)
		{
			curl_easy_cleanup(m_impl->m_curl);
			m_impl->m_curl = nullptr;
		}

		if (m_impl->m_headers != nullptr)
		{
			curl_slist_free_all(m_impl->m_headers);
			m_impl->m_headers = nullptr;
		}
	}

	void curl_request::add_header(const std::string & a_header, const std::string & a_value)
	{
		m_impl->m_headers = curl_slist_append(m_impl->m_headers, (a_header + ": " + a_value).c_str());
	}

	void curl_request::perform()
	{
		m_impl->m_curl = curl_easy_init();
		
		auto t_error = curl_easy_setopt(m_impl->m_curl, CURLOPT_FOLLOWLOCATION, 1L);
		// This should not be a big issue, don't throw
		//if (t_error != CURLE_OK) throw cho::request_exception("Curl: Cannot add follow location request. ("+ curl_easy_strerror(t_error) +")");

		curl_easy_setopt(m_impl->m_curl, CURLOPT_URL, m_request_url.c_str());
		curl_easy_setopt(m_impl->m_curl, CURLOPT_BUFFERSIZE, CURL_REQUEST_BUFFER_SIZE);

		curl_easy_setopt(m_impl->m_curl, CURLOPT_WRITEFUNCTION, handle_result);
		curl_easy_setopt(m_impl->m_curl, CURLOPT_WRITEDATA, (void *)this);

		curl_easy_setopt(m_impl->m_curl, CURLOPT_HEADERFUNCTION, handle_header);
		curl_easy_setopt(m_impl->m_curl, CURLOPT_HEADERDATA, (void *)this);
		
		if (m_impl->m_headers != nullptr)
			curl_easy_setopt(m_impl->m_curl, CURLOPT_HTTPHEADER, m_impl->m_headers);

		m_headers_received.clear();
		m_data_received.clear();

		t_error = curl_easy_perform(m_impl->m_curl);

		long t_response_code;
		t_error = curl_easy_getinfo(m_impl->m_curl, CURLINFO_RESPONSE_CODE, &t_response_code);
		m_response_code = t_error == CURLE_OK ? t_response_code : 0; // Maybe throw?
	}

	std::string curl_request::url_encode(const std::string & a_string)
	{
		if (m_curl_initialised == false)
			m_curl_initialised = curl_global_init(CURL_GLOBAL_ALL) == CURLcode::CURLE_OK;

		if (curl_request_impl::m_static_curl == nullptr)
			curl_request_impl::m_static_curl = curl_easy_init();

		char* t_output = curl_easy_escape(curl_request_impl::m_static_curl, a_string.c_str(), (int)a_string.length());
		if (t_output == nullptr)
			return "";
		
		std::string t_result = t_output;
		curl_free(t_output);

		return t_result;
	}

	double curl_request::get_response_time() const
	{
		double total;
		auto res = curl_easy_getinfo(m_impl->m_curl, CURLINFO_TOTAL_TIME, &total);
		if (res != CURLE_OK)
			return -1;
		return total;
	}
	
	size_t curl_request::handle_result(void * a_content, size_t a_size, size_t a_nmemb, void * a_user_data)
	{
		curl_request* t_requester = reinterpret_cast<curl_request*>(a_user_data);
		if (t_requester == nullptr)
			return 0;

		size_t t_size = a_size * a_nmemb;
		t_requester->m_data_received.append(reinterpret_cast<char*>(a_content), t_size);

		return t_size;
	}

	size_t curl_request::handle_header(char* a_buffer, size_t a_size, size_t a_item_count, void * a_user_data)
	{
		curl_request* t_requester = reinterpret_cast<curl_request*>(a_user_data);
		if (t_requester == nullptr)
			return 0;

		size_t t_size = a_item_count * a_size;
		t_requester->m_headers_received.push_back(std::string(a_buffer, t_size));
		return t_size;
	}
}