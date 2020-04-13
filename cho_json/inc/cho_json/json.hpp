#pragma once

#include <memory>
#include <string>
#include <map>

namespace cho
{
	namespace json_parser { struct parser_data; }
	class json
	{
	public:
		using ptr = std::shared_ptr<json>;
		class key
		{
		public:
			static const size_t string_max_length = 256;
			enum class key_type
			{
				is_string = 0,
				is_number = 1
			};

			key(const char* a_value, size_t a_length = 0);
			key(const key& a_key);
			key(const int& a_value);
			~key();

			bool operator<(const key& a_right_key) const;
			bool operator==(const char* a_string) const;
			bool operator==(int a_number) const;
			char operator[](size_t i) const;

			std::string to_string() const;
			size_t to_index() const;

		private:
			key_type type;

			union
			{
				struct
				{
					char string_value[string_max_length];
					size_t string_byte_length;
				};

				int number_value;
			};
		};

		enum class type
		{
			unset_type,
			null_type,
			boolean_type,
			object_type,
			array_type,
			string_type,
			number_type,
		};

		static json null;

		json(const char* a_value, size_t a_length = 0, std::shared_ptr<char> a_owner = nullptr);

		json::ptr operator[](const char* a_key);
		json::ptr operator[](int a_index);

		class iterator
		{
		public:
			using value = std::pair<json::key, json::ptr>;

			iterator::value operator*() const;
			void operator++();
			bool operator!=(const iterator& a_iterator) const;

			friend class json;
		private:
			iterator(std::map<json::key, json::ptr>::iterator a_iterator);

			std::map<json::key, json::ptr>::iterator m_iterator;
		};

		iterator begin();
		iterator end();
		size_t size();

		bool is_null() const;
		bool is_bool() const;
		bool is_object() const;
		bool is_array() const;
		bool is_string() const;
		bool is_number() const;

		std::string to_string();
		bool to_bool();
		int64_t to_integer();
		uint64_t to_unsigned_integer();
		double to_double();

	private:
		std::shared_ptr<json_parser::parser_data> m_parser_data;
		type m_type;
	};
}