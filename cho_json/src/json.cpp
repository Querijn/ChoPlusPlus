#include <cho_json/json.hpp>

#include <cstdint>
#include <vector>
#include <string>
#include <cassert>
#include <map>
#include <algorithm>

namespace cho
{
	json json::null("null");

	namespace json_parser
	{
		enum class parser_state : int
		{
			expecting_key,
			expecting_value,
			at_end
		};

		enum class parser_location : int
		{
			unknown,
			in_object,
			in_array
		};

		struct parser_data
		{
			parser_data(const char* a_data, size_t a_length, std::shared_ptr<char> a_owner)
			{
				end_offset = a_length == 0 ? strlen(a_data) + 1 : a_length;
				if (a_owner)
				{
					data = a_owner;
					start_offset = a_data - data.get();

				#if _DEBUG
					debug_data = a_data;
				#endif
				}
				else
				{
					start_offset = 0;
					data = std::shared_ptr<char>(new char[end_offset], std::default_delete<char[]>());
					memcpy(data.get(), a_data, end_offset);

				#if _DEBUG
					debug_data = data.get();
				#endif
				}

			}

			parser_data(const parser_data&) = delete;

			std::shared_ptr<char> data;
		#if _DEBUG
			const char* debug_data;
		#endif
			size_t start_offset;
			size_t end_offset;

			size_t parse_offset = 0;
			int depth = 0;
			parser_state state = parser_state::expecting_value;
			parser_location location = parser_location::unknown;

			uint64_t unsigned_num_value;
			int64_t num_value;
			double double_value;
			std::string string_value;
			bool bool_value;

			std::map<json::key, json::ptr> children;
			size_t children_count = 0;
		};

		inline void eat_depth(std::shared_ptr<parser_data>& a_parser, const char* a_data, size_t& i, size_t a_length, size_t a_desired_depth)
		{
			i++; // Make sure we don't process the current char
			size_t start = i;
			bool in_string = false;
			for (; i < a_length; i++)
			{
				if (in_string == false)
				{
					switch (a_data[i])
					{
					case ']':
					case '}':
						a_parser->depth--;
						if (a_parser->depth == a_desired_depth)
							return;
						break;
					case '[':
					case '{':
						a_parser->depth++;
						break;
					}
				}
				else if (/*in_string &&*/ a_data[i] == '"')
				{
					if (a_data[i - 1] != '\\') // If not \"
						in_string = false;
				}
			}

			// __debugbreak();
		}

		inline void eat_whitespace(const char* a_data, size_t& i, size_t a_length)
		{
			for (; i < a_length; i++)
			{
				switch (a_data[i])
				{
				case 0x20: // Space
				case 0x09:
				case 0x0A:
				case 0x0D:
					break;
				default:
					return;
				}
			}
		}

		inline size_t eat_string(const char* a_data, size_t& i, size_t a_length)
		{
			assert(a_data[i] == '"');
			i++;

			size_t start = i;
			for (; i < a_length; i++)
			{
				if (a_data[i] == '"' && a_data[i - 1] != '\\')
					return i - start;
			}

			return a_length - start;
		}

		inline size_t eat_number(const char* a_data, size_t& i, size_t a_length)
		{
			size_t start = i;
			for (; i < a_length; i++)
			{
				if (a_data[i] == '-' || a_data[i] == '+' || a_data[i] == '.' || a_data[i] == 'e' || a_data[i] == 'E')
					continue;

				if (a_data[i] >= '0' && a_data[i] <= '9')
					continue;

				return i - start;
			}

			return a_length - start;
		}

		inline void eat_until_key_value_separator(const char* a_data, size_t& i, size_t a_length)
		{
			for (; i < a_length; i++)
				if (a_data[i] == ':')
					return;
		}

		inline void eat_until_value_separator(const char* a_data, size_t& i, size_t a_length, int& a_depth)
		{
			for (; i < a_length; i++)
			{
				switch (a_data[i])
				{
				case '}':
				case ']':
					a_depth--;

				case ',':
					return;
				}
			}
		}

		inline void parse_value(std::shared_ptr<parser_data>& a_parser, const char* a_data, size_t& i, size_t a_length, json::type* a_type)
		{
			switch (a_data[i])
			{
			case '[':
				a_parser->depth++;
				if (a_parser->depth != 1) // Some value we don't care about (other parsers will take care of it)
				{
					eat_depth(a_parser, a_data, i, a_length, 1);
				}
				else // we're parsing the current json, and it's an array
				{
					a_parser->location = parser_location::in_array;
					a_parser->state = parser_state::expecting_value;

					if (a_type && *a_type == json::type::unset_type) // If we just wanted the type, return now
					{
						i++;
						*a_type = json::type::array_type;
						return;
					}
				}
				break;

			case '{':
				a_parser->depth++;
				if (a_parser->depth != 1) // Some value we don't care about (other parsers will take care of it)
				{
					auto start = i;
					eat_depth(a_parser, a_data, i, a_length, 1);
					if (a_parser->location == parser_location::in_array)
						a_parser->children[a_parser->children_count++] = std::make_shared<json>(a_data + start, i + 1, a_parser->data);
				}
				else // we're parsing the current json, and it's an object
				{
					a_parser->location = parser_location::in_object;
					a_parser->state = parser_state::expecting_key;

					if (a_type && *a_type == json::type::unset_type) // If we just wanted the type, return now
					{
						i++;
						*a_type = json::type::object_type;
						return;
					}
				}
				break;

			case '"':
			{
				size_t start = i + 1;
				size_t end = eat_string(a_data, i, a_length) + start; // we don't care about the contents (yet)
				// if (a_data[end] == '\"' && a_data[end - 1] != '\\')
				//	// __debugbreak();

				if (a_parser->location == parser_location::in_array)
				{
					a_parser->children[a_parser->children_count++] = std::make_shared<json>(a_data + start - 1, end + 1, a_parser->data);
				}

				eat_until_value_separator(a_data, i, a_length, a_parser->depth);
				if (a_parser->depth > 0) // If not done
				{
					if (a_parser->location == parser_location::in_object)
					{
						a_parser->state = parser_state::expecting_key;
					}
				}
				else // if (a_parser->depth <= 0) // We're at our end
				{
					if (a_type && *a_type == json::type::unset_type)
					{
						a_parser->string_value = std::string(a_data + start, a_data + end);
						*a_type = json::type::string_type;
					}

					a_parser->state = parser_state::at_end; // Done! Can't go deeper.
					return;
				}
				break;
			}

			case 't': // true
				assert(memcmp("true", a_data + i, 4) == 0);

				if (a_parser->location == parser_location::in_array)
					a_parser->children[a_parser->children_count++] = std::make_shared<json>(a_data + i, i + 4, a_parser->data);
				i += 3;

				if (a_type && a_parser->depth <= 0) // If we just wanted the type, return now
				{
					i++;
					*a_type = json::type::boolean_type;
					a_parser->bool_value = true;

					a_parser->state = parser_state::at_end; // Done! Can't go deeper.
					return;
				}
				break;

			case 'f': // false
				assert(memcmp("false", a_data + i, 5) == 0);

				if (a_parser->location == parser_location::in_array)
					a_parser->children[a_parser->children_count++] = std::make_shared<json>(a_data + i, i + 5, a_parser->data);
				i += 4;

				if (a_type && a_parser->depth <= 0) // If we just wanted the type, return now
				{
					i++;
					*a_type = json::type::boolean_type;
					a_parser->bool_value = false;

					a_parser->state = parser_state::at_end; // Done! Can't go deeper.
					return;
				}
				break;

			case 'n': // null
				assert(memcmp("null", a_data + i, 4) == 0);

				if (a_parser->location == parser_location::in_array)
					a_parser->children[a_parser->children_count++] = std::make_shared<json>(a_data + i, i + 4, a_parser->data);
				i += 3;

				if (a_type && *a_type == json::type::unset_type && a_parser->depth <= 0) // If we just wanted the type, return now
				{
					i++;
					*a_type = json::type::null_type;

					a_parser->state = parser_state::at_end; // Done! Can't go deeper.
					return;
				}
				break;

			case ':':
				assert(a_parser->state == parser_state::expecting_value);
				assert(a_parser->location == parser_location::in_object);
				a_parser->state = parser_state::expecting_value;
				break;

			case ',':
				assert(a_parser->state == parser_state::expecting_value);

				if (a_parser->location == parser_location::in_object)
					a_parser->state = parser_state::expecting_key;
				else
					a_parser->state = parser_state::expecting_value;
				break;

			case '}':
			case ']':
				a_parser->depth--;
				if (a_parser->depth <= 0)
					a_parser->state = parser_state::at_end; // End of object/array
				return; // value not found

			case '0': case '1': case '2':
			case '3': case '4': case '5': 
			case '6': case '7':	case '8':
			case '9': case '-':
			{
				size_t start = i;
				size_t end = eat_number(a_data, i, a_length) + start; // we don't care about the contents (yet)
				// if (a_data[end] == '\"' && a_data[end - 1] != '\\')
				//	// __debugbreak();

				if (a_parser->location == parser_location::in_array)
				{
					a_parser->children[a_parser->children_count++] = std::make_shared<json>(a_data + start - 1, end + 1, a_parser->data);
				}

				eat_until_value_separator(a_data, i, a_length, a_parser->depth);
				if (a_parser->depth > 0) // If not done
				{
					if (a_parser->location == parser_location::in_object)
					{
						a_parser->state = parser_state::expecting_key;
					}
				}
				else // if (a_parser->depth <= 0) // We're at our end
				{
					if (a_type && *a_type == json::type::unset_type)
					{
						a_parser->string_value = std::string(a_data + start, a_data + end);
						a_parser->double_value = atof(a_parser->string_value.c_str());
						a_parser->num_value = atoll(a_parser->string_value.c_str());
						if (a_parser->num_value > 0 && a_parser->num_value <= INT64_MAX)
							a_parser->unsigned_num_value = a_parser->num_value;
						else 
							strtoull(a_parser->string_value.c_str(), nullptr, 10);
						*a_type = json::type::number_type;
					}

					a_parser->state = parser_state::at_end; // Done! Can't go deeper.
					return;
				}
				break;
			}

			case '\0':
				a_parser->state = parser_state::at_end; // Done! Can't go deeper.
				return;

			default:
				// __debugbreak();
				break;
			}
		}

		inline const char* parse_key(std::shared_ptr<parser_data>& a_parser, const char* a_data, size_t& i, size_t a_length, size_t& a_key_length)
		{
			switch (a_data[i])
			{
			case '\"':
			{
				a_key_length = eat_string(a_data, i, a_length);
				return a_data + i - a_key_length;
			}

			case '}':
				break;

			default:
				// __debugbreak();
				break;
			}

			return nullptr;
		}

		inline void parse(std::shared_ptr<parser_data>& a_parser, json::ptr* a_result = nullptr, const char* a_key_to_find = nullptr, size_t a_key_length = 0, json::type* a_type = nullptr, const int* a_index_to_find = nullptr)
		{
		#define parse_return(a) do { if (a_result) *a_result = (a); return; } while(0);
			const char* data = a_parser->data.get() + a_parser->start_offset;
			size_t length = a_parser->end_offset;

			// for each character
			for (size_t i = a_parser->parse_offset; i < length; i++)
			{
				eat_whitespace(data, i, length);

				switch (a_parser->state)
				{
				case parser_state::expecting_value:
					parse_value(a_parser, data, i, length, a_type);
					if (a_type && *a_type != json::type::unset_type)
					{
						a_parser->parse_offset = i;
						parse_return(nullptr);
					}
					else if (a_index_to_find != nullptr && *a_index_to_find >= 0 && *a_index_to_find < a_parser->children_count) // Stop parser if we were looking for this index (and it's not end-based)
					{
						assert(a_parser->location == parser_location::in_array);
						a_parser->parse_offset = i;
						parse_return(a_parser->children.find(*a_index_to_find)->second);
					}
					break;

				case parser_state::expecting_key:
				{
					size_t key_length;
					const char* key = parse_key(a_parser, data, i, length, key_length);
					if (key == nullptr)
					{
						if (data[i] == '}' && a_parser->depth == 1)
							a_parser->state = parser_state::at_end;
						parse_return(nullptr);
					}

					eat_until_key_value_separator(data, i, length);
					a_parser->state = parser_state::expecting_value;

					auto child_key = json::key(key, key_length);
					a_parser->children[child_key] = std::make_shared<json>(data + i + 1, length - i - 1, a_parser->data); // TODO: Make the length here shorter. We could eat_depth and get the distance?
					a_parser->children_count++;

					if (a_key_to_find && child_key == a_key_to_find) // Stop parser if we were looking for this key
					{
						a_parser->parse_offset = i;
						parse_return(a_parser->children.find(child_key)->second);
					}
					break;
				}

				case parser_state::at_end: // Force end
					a_parser->parse_offset = length;

					if (a_index_to_find != nullptr) // If we were looking for an end-based index, return that instead.
					{
						assert(a_parser->location == parser_location::in_array);

						int index_num = *a_index_to_find < 0 ? (int)a_parser->children_count + *a_index_to_find : *a_index_to_find; // correct the index if negative
						if (index_num < 0) parse_return(nullptr); // if we have an incorrect index after correction, return invalid.

						auto index = a_parser->children.find(index_num);
						parse_return(index != a_parser->children.end() ? index->second : nullptr);
					}
					parse_return(nullptr);
				}
			}

			a_parser->parse_offset = length;
			parse_return(nullptr);
		}
	#undef parse_return
	}

	json::json(const char* a_data, size_t a_length, std::shared_ptr<char> a_owner)
	{
		m_parser_data = std::make_shared<json_parser::parser_data>(a_data, a_length, a_owner);
		m_type = json::type::unset_type;
		json_parser::parse(m_parser_data, nullptr, nullptr, 0, &m_type);
	}

	json::ptr json::operator[](const char* a_key)
	{
		assert(m_type == type::object_type);
		auto index = m_parser_data->children.find(a_key);
		if (index != m_parser_data->children.end())
			return index->second;

		json::ptr t_result;
		json_parser::parse(m_parser_data, &t_result, a_key, strlen(a_key));
		return t_result;
	}

	json::ptr json::operator[](int a_index)
	{
		assert(m_type == type::array_type);
		auto index = m_parser_data->children.find(a_index);
		if (index != m_parser_data->children.end())
			return index->second;

		json::ptr t_result;
		json_parser::parse(m_parser_data, &t_result, nullptr, 0, nullptr, &a_index);
		return t_result;
	}

	bool json::is_object() const { return m_type == type::object_type; }

	json::iterator json::begin()
	{
		json_parser::parse(m_parser_data);
		return json::iterator(m_parser_data->children.begin());
	}

	json::iterator json::end()
	{
		json_parser::parse(m_parser_data);
		return json::iterator(m_parser_data->children.end());
	}

	size_t json::size()
	{
		json_parser::parse(m_parser_data);

		assert(m_type == json::type::array_type);
		return m_parser_data->children_count;
	}

	bool json::is_null() const { return this == nullptr || m_type == json::type::null_type; }
	bool json::is_bool() const { return m_type == json::type::boolean_type; }
	bool json::is_array() const { return m_type == json::type::array_type; }
	bool json::is_string() const { return m_type == json::type::string_type; }
	bool json::is_number() const { return m_type == json::type::number_type; }

	std::string json::to_string()
	{
		json_parser::parse(m_parser_data);
		return m_parser_data->string_value;
	}

	bool json::to_bool()
	{
		json_parser::parse(m_parser_data);
		return m_parser_data->bool_value;
	}

	int64_t json::to_integer()
	{
		json_parser::parse(m_parser_data);
		return m_parser_data->num_value;
	}

	uint64_t json::to_unsigned_integer()
	{
		json_parser::parse(m_parser_data);
		return m_parser_data->unsigned_num_value;
	}

	double json::to_double()
	{
		json_parser::parse(m_parser_data);
		return m_parser_data->double_value;
	}

	json::key::key(const char* a_key, size_t a_length) :
		type(key_type::is_string)
	{
		string_byte_length = std::min(string_max_length - 1, a_length ? a_length : strlen(a_key));
		memcpy(string_value, a_key, string_byte_length);
		string_value[string_byte_length] = 0;
	}

	json::key::key(const key& a_key)
	{
		type = a_key.type;
		switch (type)
		{
		case key_type::is_number:
			number_value = a_key.number_value;
			break;

		case key_type::is_string:
			string_byte_length = a_key.string_byte_length;
			memcpy(string_value, a_key.string_value, string_byte_length);
			string_value[string_byte_length] = 0;
			break;
		}
	}

	json::key::key(const int& a_value) :
		type(key_type::is_number)
	{
		number_value = a_value;
	}

	json::key::~key()
	{
		if (type == key_type::is_number)
			number_value = 0;
	}


	size_t json::key::to_index() const
	{
		assert(type == key_type::is_number);
		return number_value;
	}
	
	bool json::key::operator<(const key & a_right_key) const
	{
		if (type != a_right_key.type)
			return type < a_right_key.type;

		switch (type)
		{
		default:
			// __debugbreak();
		case key_type::is_number:
			return number_value < a_right_key.number_value;

		case key_type::is_string:
			if (string_byte_length != a_right_key.string_byte_length)
				return string_byte_length < a_right_key.string_byte_length;
			return std::string(string_value) < std::string(a_right_key.string_value); // TODO: optimise
		}
	}

	bool json::key::operator==(const char* a_string) const
	{
		switch (type)
		{
		case key_type::is_string:
			return a_string == std::string(string_value); // TODO: optimise

		case key_type::is_number:
			return false;
		};
	}

	bool json::key::operator==(int a_number) const
	{
		switch (type)
		{
		case key_type::is_number:
			return a_number == number_value;

		case key_type::is_string:
			return false;
		};
	}

	std::string json::key::to_string() const
	{
		switch (type)
		{
		case key_type::is_string:
			return string_value;
		case key_type::is_number:
			return std::to_string(number_value);
		};

		assert(false);
		return "UNKNOWN";
	}

	char json::key::operator[](size_t i) const
	{
		assert(type == key_type::is_string);
		return string_value[i];
	}
	
	json::iterator::value json::iterator::operator*() const
	{
		return *m_iterator;
	}

	void json::iterator::operator++()
	{
		m_iterator++;
	}

	bool json::iterator::operator!=(const iterator& a_iterator) const
	{
		return m_iterator != a_iterator.m_iterator; 
	}

	json::iterator::iterator(std::map<json::key, json::ptr>::iterator a_iterator)
	{
		m_iterator = a_iterator;
	}
}