#include <cho_request/request.hpp>

#include <mstch/mstch.hpp>

#include <string>
#include <set>
#include <cassert>
#include <chrono>
#include <fstream>
#include <cstdio>
#include <cinttypes>
#include <algorithm>

static const std::string g_riot_api_schema = "http://www.mingweisamuel.com/riotapi-schema/openapi-3.0.0.json";
static const std::string g_ddragon_versions_url = "http://ddragon.leagueoflegends.com/api/versions.json";
static const std::string g_ddragon_champion_url = "https://ddragon.leagueoflegends.com/cdn/%s/data/en_US/champion.json";

namespace Hash
{
	// http://stackoverflow.com/questions/2111667/compile-time-string-Hash (tower120's solution)
	static constexpr uint32_t CRCTable[256] =
	{
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};

	template<int size, int idx = 0, class dummy = void>
	struct MM
	{
		static constexpr uint32_t crc32(const char* a_String, uint32_t a_PreviousCRC = 0xFFFFFFFF)
		{
			return MM<size, idx + 1>::crc32(a_String, (a_PreviousCRC >> 8) ^ CRCTable[(a_PreviousCRC ^ a_String[idx]) & 0xFF]);
		}

		static constexpr uint32_t crc32_lc(const char* a_String, uint32_t a_PreviousCRC = 0xFFFFFFFF)
		{
			auto t_Byte = ((a_String[idx] >= 'A' && a_String[idx] <= 'Z') ? a_String[idx] + ('a' - 'A') : a_String[idx]);
			return MM<size, idx + 1>::crc32_lc(a_String, (a_PreviousCRC >> 8) ^ CRCTable[(a_PreviousCRC ^ t_Byte) & 0xFF]);
		}
	};

	// This is the stop-recursion function
	template<int size, class dummy>
	struct MM<size, size, dummy>
	{
		static constexpr uint32_t crc32(const char* a_String, uint32_t a_PreviousCRC = 0xFFFFFFFF)
		{
			return a_PreviousCRC ^ 0xFFFFFFFF;
		}
		static constexpr uint32_t crc32_lc(const char* a_String, uint32_t a_PreviousCRC = 0xFFFFFFFF)
		{
			return a_PreviousCRC ^ 0xFFFFFFFF;
		}
	};

	static uint32_t CRC32(const char* a_String, size_t a_Length, uint32_t a_PreviousCRC = 0xFFFFFFFF)
	{
		a_PreviousCRC = 0xFFFFFFFF;
		for (size_t i = 0; i < a_Length; i++)
		{
			auto byte = a_String[i];
			a_PreviousCRC = (a_PreviousCRC >> 8) ^ CRCTable[(a_PreviousCRC ^ byte) & 0xFF];
		}
		return a_PreviousCRC ^ 0xFFFFFFFF;
	}
}

#define COMPILE_TIME_CRC32_STR(x) (Hash::MM<sizeof(x)-1>::crc32(x))
#define COMPILE_TIME_CRC32_STR_LC(x) (Hash::MM<sizeof(x)-1>::crc32_lc(x))

struct profiler
{
	using clock = std::chrono::high_resolution_clock;
	profiler(const char* a_name) : start(profiler::clock::now()), name(a_name) {}
	~profiler()
	{
		profiler::clock::time_point end = profiler::clock::now();
		double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		auto duration_name = "ns";
		if (duration > 1000)
		{
			duration /= 1000;
			duration_name = "us";

			if (duration > 1000)
			{
				duration /= 1000;
				duration_name = "ms";
			}
		}
		printf("Task '%s' took %.3f %s.\n", name, duration, duration_name);
	}

	const char* name;
	profiler::clock::time_point start;
};

struct component_data
{
	component_data(std::string a_name, cho::json::ptr a_schema, bool a_is_typedef)
	{
		name = a_name;
		schema = a_schema;
		is_typedef = a_is_typedef;
	}

	std::string name;
	std::vector<std::shared_ptr<component_data>> references;
	cho::json::ptr schema;
	bool is_typedef;
};

struct api_data
{
	std::map<std::string, std::vector<std::string>> paths;
	std::map<std::string, std::map<std::string, std::shared_ptr<component_data>>> service_components;
	std::set<std::string> services;

	std::set<std::string> std_includes;
	std::set<std::string> cho_includes;
};

std::string resolve_schema_type_name(cho::json::ptr a_ptr, api_data& a_riot_api_data, cho::json& a_json, bool a_is_parameter);

void replace(std::string& a_subject, const std::string& a_search, const std::string& a_replace)
{
	size_t pos = 0;
	while ((pos = a_subject.find(a_search, pos)) != std::string::npos)
	{
		a_subject.replace(pos, a_search.length(), a_replace);
		pos += a_replace.length();
	}
}

std::string get_c_style_service(const char* a_service_name)
{
	std::string result = a_service_name;
	replace(result, "-", "_");

	//// Remove "_v1"
	//auto res_len = result.length() - 1;
	//auto version_list = { '1', '2', '3', '4', '5', '6' };
	//for (auto v : version_list)
	//	if (result[res_len] == v)
	//		return result.substr(0, res_len - 2);

	return result;
}

void get_header(const std::string& type, api_data& a_riot_api)
{
	if (type.find("std::vector") == 0)
		return; // Already handled elsewhere

	switch (Hash::CRC32(type.c_str(), type.size()))
	{
	case COMPILE_TIME_CRC32_STR("cho::champion_key"):
		break;

	case COMPILE_TIME_CRC32_STR("const char*"):
		break;

	case COMPILE_TIME_CRC32_STR("int32_t"):
		a_riot_api.std_includes.insert("cstdint");
		break;

	case COMPILE_TIME_CRC32_STR("int64_t"):
		a_riot_api.std_includes.insert("cstdint");
		break;

	default:
		if (type.find("::") == std::string::npos)
			__debugbreak();
	}
}

std::string get_c_style_var(const char* variable)
{
	if (!variable)
		return "unknown_t";

	std::string result = "";
	auto ignore_underscore = false;

	size_t len = strlen(variable);
	for (size_t i = 0; i < len; i++)
	{
		auto character = variable[i];
		bool is_lower = character >= 'a' && character <= 'z';
		bool is_upper = character >= 'A' && character <= 'Z';
		if (is_upper)
		{
			if (!ignore_underscore)
			{
				if (i != 0) 
					result += "_";
				ignore_underscore = true;
			}

			result += tolower(character);
			continue;
		}
		else if (!is_lower && !is_upper)
		{
			ignore_underscore = true;
			result += character;
			continue;
		}

		ignore_underscore = false;
		result += character;
	}

	return result;
}

std::vector<std::shared_ptr<component_data>> add_component(const std::string& a_name, cho::json::ptr a_component, api_data& a_riot_api_data, cho::json& a_json);
std::vector<std::shared_ptr<component_data>> add_component(const std::string& a_component_name, const std::string& a_service_name, cho::json::ptr a_component, api_data& a_riot_api_data, cho::json& a_json);

std::shared_ptr<component_data> create_component(const std::string& a_comp_name_without_service, const std::string& a_final_service_name, cho::json::ptr a_component, api_data& a_riot_api_data, bool is_typedef = false)
{
	auto result_component = std::make_shared<component_data>(a_comp_name_without_service, a_component, is_typedef);
	a_riot_api_data.service_components[a_final_service_name][a_comp_name_without_service] = result_component;
	a_riot_api_data.services.insert(a_final_service_name);

	return result_component;
}

std::pair<std::string, cho::json::ptr> resolve_ref_schema(cho::json::ptr a_component, cho::json& a_json)
{
	auto component = *a_component;
	if (component["$ref"] == nullptr)
	{
		assert(false);
		return { "", a_component };
	}

	// This references the schema in the api object.
	auto schema_ref_location = component["$ref"]->to_string().substr(2);
	auto schema_ref_length = schema_ref_location.size();

	// Go down the string to get the object.
	int index = -1;
	cho::json::ptr schema = std::make_shared<cho::json>(a_json); // TODO: Get a better way to do it. We recurse from the source, but the root is not a pointer.
	auto prev_index = -1;
	std::string property = "";
	do
	{
		assert(schema != nullptr);

		prev_index = index + 1;
		index = schema_ref_location.find("/", prev_index);
		if (index == -1)
			index = schema_ref_length;

		property = schema_ref_location.substr(prev_index, index - prev_index);
		schema = (*schema)[property.c_str()];
	}
	while (index != schema_ref_length);

	assert(schema != nullptr);
	return { property, schema };
}

std::vector<std::shared_ptr<component_data>> resolve_ref_type(cho::json::ptr component, const std::string& a_service_name, api_data& a_riot_api_data, cho::json& a_json)
{
	auto cross_type = (*component)["x-type"];
	assert(cross_type != nullptr);

	// It's not a reference to something complex.
	if ((*component)["$ref"] == nullptr)
		return add_component(cross_type->to_string(), a_service_name, component, a_riot_api_data, a_json);

	auto schema = resolve_ref_schema(component, a_json);
	return add_component(schema.first, schema.second, a_riot_api_data, a_json);
}

std::pair<std::string, std::string> get_service_and_component_name(const std::string& a_name)
{
	size_t separator_index = a_name.find('.');

	if (separator_index == std::string::npos)
		return { "Global", a_name };
	else
		return { a_name.substr(0, separator_index), a_name.substr(separator_index + 1) };
}

std::vector<std::shared_ptr<component_data>> add_component(const std::string& a_name, cho::json::ptr a_component, api_data& a_riot_api_data, cho::json& a_json)
{
	auto separated_name = get_service_and_component_name(a_name);
	std::string comp_name_without_service = separated_name.second;
	std::string final_service_name = separated_name.first;

	if (final_service_name == "Global")
		return {};

	return add_component(comp_name_without_service, final_service_name, a_component, a_riot_api_data, a_json);
}

std::vector<std::shared_ptr<component_data>> add_component(const std::string& a_component_name, const std::string& a_service_name, cho::json::ptr a_component, api_data& a_riot_api_data, cho::json& a_json)
{
	cho::json& component = *a_component;

	auto type_var = component["type"];
	if (type_var == nullptr && component["x-type"] != nullptr)
	{
		auto returns = resolve_ref_type(a_component, a_service_name, a_riot_api_data, a_json);
		// The last one should be the resolved cross-reference.

		if (returns.size() > 1) 
			__debugbreak(); // Didn't actually test this..
		return returns;
	}

	auto type = type_var->to_string();
	if (type == "object")
	{
		auto cross_type = component["x-type"];

		// Hack: If it's a map with a stupid name, skip it.
		if (cross_type != nullptr && cross_type->to_string().find("Map[") == 0)
		{
			std::string keyKey = "key";
			auto& valueKey = *component["x-key"];
			if (valueKey["title"] != nullptr)
				keyKey = valueKey["title"]->to_string();
			else if (valueKey["x-type"] != nullptr)
				keyKey = valueKey["x-type"]->to_string();
			
			if (valueKey["schema"] != nullptr)
				__debugbreak();

			std::string keyValue = "value";
			auto& valueValue = *component["additionalProperties"];
			if (valueValue["title"] != nullptr)
				keyValue = valueValue["title"]->to_string();
			else if (valueValue["x-type"])
				keyValue = valueValue["x-type"]->to_string();

			if (valueKey["schema"] != nullptr)
				__debugbreak();
			//component.properties = { };
			//component.properties[keyKey] = valueKey;
			//component.properties[keyValue] = valueValue;

			//component.type = "map"; // Don't write this out.
			//delete components[service_name][newComponentName];
			return { };
		}

		auto properties = component["properties"];
		assert(properties != nullptr);

		auto result_component = create_component(a_component_name, a_service_name, a_component, a_riot_api_data);

		for (auto property_info : *properties)
		{
			auto referenced_components = add_component(property_info.first.to_string(), a_service_name, property_info.second, a_riot_api_data, a_json);

			for (size_t i = 0; i < referenced_components.size(); i++)
				result_component->references.insert(result_component->references.end(), referenced_components[i]->references.begin(), referenced_components[i]->references.end());
			result_component->references.insert(result_component->references.end(), referenced_components.begin(), referenced_components.end());
		}
		return { result_component };
	}
	else if (type == "array")
	{
		auto referenced_components = add_component(a_component_name + "Items", a_service_name, component["items"], a_riot_api_data, a_json);
		return referenced_components;
	}
	else if (type != "number" && type != "string" && type != "boolean" && type != "integer" )
	{
		__debugbreak();
	}

	return { };
}

bool fetch_components(api_data& a_riot_api_data, cho::json& a_json)
{
	cho::json::ptr components = a_json["components"];
	if (components == nullptr)
		return false;
	cho::json::ptr schemas = (*components)["schemas"];
	if (schemas == nullptr)
		return false;
	
	for (auto& component_data : *schemas)
	{
		std::string component_name = component_data.first.to_string();
		const auto& component = component_data.second;
		add_component(component_name, component, a_riot_api_data, a_json);
	}

	return true;
}

bool fetch_function_components(api_data& a_riot_api_data, cho::json& a_json)
{
	for (auto& path_infos : *a_json["paths"])
	{
		auto& path_info = *path_infos.second;

		auto&& fetch_types = { path_info["get"], path_info["post"], path_info["put"], path_info["delete"] };
		auto& service = path_info["x-endpoint"];
		auto& platforms = path_info["x-platforms"];

		for (auto& method_info_ptr : fetch_types)
		{
			if (method_info_ptr == nullptr)
				continue;
			auto& method_info = *method_info_ptr;

			auto service_name = service != nullptr ? service->to_string() : "Global";
			a_riot_api_data.services.insert(service_name);

			// Parse parameters
			auto parameters = method_info["parameters"];
			if (parameters)
			{
				for (auto& parameter_info : *parameters)
				{
					auto& parameter = (*parameter_info.second);
					auto& in = parameter["in"]->to_string();
					if (in != "query" && in != "path")
						__debugbreak();

					add_component(parameter["name"]->to_string(), service_name, parameter["schema"], a_riot_api_data, a_json);
				}
			}

			auto responses = method_info["responses"];
			if (responses == nullptr)
				continue;

			auto ok = (*responses)["200"];
			if (ok == nullptr) continue;

			auto content = (*ok)["content"];
			if (content == nullptr) continue;
			
			auto json = (*content)["application/json"];
			if (json == nullptr) continue;

			auto return_schema = (*json)["schema"];
			if (return_schema == nullptr) continue;

			assert((*return_schema)["x-type"] != nullptr); // && return_schema["$ref"])
			std::string type = (*return_schema)["x-type"]->to_string();
			if (type != "")
				add_component(type, service_name, return_schema, a_riot_api_data, a_json);
		}
	}

	return true;
}

std::string file_to_string(const char* a_file_name)
{
	std::ifstream file(a_file_name);
	return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::string resolve_array_parameter_name(cho::json::ptr a_ptr, api_data& a_riot_api_data, cho::json& a_json, bool a_is_parameter)
{
	auto item_name = resolve_schema_type_name((*a_ptr)["items"], a_riot_api_data, a_json, a_is_parameter);

	get_header(item_name, a_riot_api_data);
	a_riot_api_data.std_includes.insert("vector");

	return "std::vector<" + item_name + ">";
}

std::string resolve_sprintf_type(cho::json::ptr a_ptr, api_data& a_riot_api_data, cho::json& a_json)
{
	uint32_t type_hash;
	if ((*a_ptr)["x-enum"] != nullptr)
	{
		type_hash = COMPILE_TIME_CRC32_STR("int32_t");
	}
	else
	{
		std::string name = resolve_schema_type_name(a_ptr, a_riot_api_data, a_json, false);
		type_hash = Hash::CRC32(name.c_str(), name.size());
	}

	switch (type_hash)
	{
	case COMPILE_TIME_CRC32_STR("double"):
		return "%lf";
	case COMPILE_TIME_CRC32_STR("float"):
		return "%f";

	case COMPILE_TIME_CRC32_STR("int32_t"):
		return "%" PRId32;
	case COMPILE_TIME_CRC32_STR("int64_t"):
		return "%" PRId64;

	case COMPILE_TIME_CRC32_STR("uint32_t"):
		return "%" PRIu32;
	case COMPILE_TIME_CRC32_STR("uint64_t"):
		return "%" PRIu64;

	case COMPILE_TIME_CRC32_STR("std::string"):
		return "%s";

	default:
		__debugbreak();
	}
}

std::string resolve_schema_type_name(cho::json::ptr a_ptr, api_data& a_riot_api_data, cho::json& a_json, bool a_is_parameter)
{
	if ((*a_ptr)["$ref"] != nullptr)
	{
		auto name = resolve_ref_schema(a_ptr, a_json).first;
		auto separated_name = get_service_and_component_name(name);
		auto service = get_c_style_service(separated_name.first.c_str());
		auto type = get_c_style_var(separated_name.second.c_str());

		if (type == "lobby_event")
			__debugbreak();

		return service + "::" + type;
	}

	auto x_enum = (*a_ptr)["x-enum"] ? (*a_ptr)["x-enum"]->to_string() : "";
	if (x_enum == "champion")
		return "cho::champion_key";

	if ((*a_ptr)["format"])
	{
		auto format = (*a_ptr)["format"]->to_string();

	#define RESOLVE_CSTDINT_SCHEMA(a) case COMPILE_TIME_CRC32_STR(a): return std::string(a "_t");
		switch (Hash::CRC32(format.c_str(), format.size()))
		{
			RESOLVE_CSTDINT_SCHEMA("int32");
			RESOLVE_CSTDINT_SCHEMA("int64");

		case COMPILE_TIME_CRC32_STR("double"):
			return "double";
		case COMPILE_TIME_CRC32_STR("float"):
			return "float";

		default:
			__debugbreak();
		}
	}

	auto x_key = (*a_ptr)["x-key"];
	if (x_key != nullptr)
	{
		auto x_value = (*a_ptr)["additionalProperties"];

		auto key_name = resolve_schema_type_name(x_key, a_riot_api_data, a_json, false);
		auto value_name = resolve_schema_type_name(x_value, a_riot_api_data, a_json, false);
		return "std::map<" + key_name + ", " + value_name + ">";
	}

	auto type = (*a_ptr)["type"]->to_string();
	switch (Hash::CRC32(type.c_str(), type.size()))
	{
	case COMPILE_TIME_CRC32_STR("array"):
		return resolve_array_parameter_name(a_ptr, a_riot_api_data, a_json, a_is_parameter);

	case COMPILE_TIME_CRC32_STR("String"):
	case COMPILE_TIME_CRC32_STR("string"):
		if (a_is_parameter)
			return "const char*";
		return "std::string";

	case COMPILE_TIME_CRC32_STR("boolean"):
		return "bool";
	}

	auto x_type = (*a_ptr)["x-type"]->to_string();
	switch (Hash::CRC32(x_type.c_str(), x_type.size()))
	{
	case COMPILE_TIME_CRC32_STR("String"):
	case COMPILE_TIME_CRC32_STR("string"):
		return "const char*";

	default:
		__debugbreak();
	}
	
	__debugbreak();
}

class index_visitor : public boost::static_visitor<int>
{
public:
	int operator()(int i) const
	{
		return i;
	}

	template<typename T>
	int operator()(T i) const
	{
		return std::string::npos;
	}
};

class map_visitor : public boost::static_visitor<mstch::map>
{
public:
	mstch::map operator()(mstch::map i) const
	{
		return i;
	}

	template<typename T>
	mstch::map operator()(T i) const
	{
		return mstch::map();
	}
};

void parse_path(const char* method, const std::string& original_path, const char* service, cho::json& fetch_type, mstch::array& methods_context, api_data& a_riot_api_data, cho::json& a_json)
{
	std::string path = original_path;
	mstch::array required_params;
	mstch::array optional_params;
	mstch::array path_params;

	bool has_optional = false;
	size_t parameter_count = fetch_type["parameters"] ? fetch_type["parameters"]->size() : 0;
	auto& parameters = (*fetch_type["parameters"]);
	for (size_t i = 0; i < parameter_count; i++)
	{
		auto& param = *parameters[i];
		bool required = param["required"]->to_bool();

		std::string type_name = resolve_schema_type_name(param["schema"], a_riot_api_data, a_json, true);
		get_header(type_name, a_riot_api_data);
		auto name = param["name"]->to_string();

		auto in = param["in"]->to_string();
		if (in == "path")
		{
			required = true;

			auto param_ident = "{" + name + "}";
			auto index = original_path.find(param_ident);
			assert(path.find(param_ident) != std::string::npos);

			std::string sprintf_character = resolve_sprintf_type(param["schema"], a_riot_api_data, a_json);
			replace(path, param_ident, sprintf_character);

			auto param_name = get_c_style_var(name.c_str());

			mstch::map param =
			{
				{ "index", (int)index },
				{ "name", param_name },
			};

			// If we're a string, we need to make sure we urlencode it.
			if (sprintf_character == "%s")
			{
				param["converter_pre"] = std::string("cho::request::url_encode("); // Honestly this is disgusting
				param["converter_post"] = std::string(").c_str()");
				param["has_converter"] = true;
			}

			path_params.push_back(param);
		}

		if (required == false)
		{
			has_optional = true;
			type_name = "cho::optional<" + type_name + ">";
			a_riot_api_data.cho_includes.insert("cho/optional.hpp");
		}

		mstch::map param_context =
		{
			{ "type", type_name },
			{ "name", get_c_style_var(name.c_str()) }
		};

		if (required)
			required_params.push_back(param_context);
		else
			optional_params.push_back(param_context);
	}

	std::sort(path_params.begin(), path_params.end(), [](mstch::node a, mstch::node b)
	{
		map_visitor visitor1;
		index_visitor visitor;
		auto map_a = a.apply_visitor(visitor1);
		auto map_b = b.apply_visitor(visitor1);
		auto a_index = map_a["index"].apply_visitor(visitor);
		auto b_index = map_b["index"].apply_visitor(visitor);

		return a_index < b_index;
	});

	std::string return_type = "";
	{
		auto responses = fetch_type["responses"];
		if (responses == nullptr) goto no_response;

		auto ok = (*responses)["200"];
		if (ok == nullptr) goto no_response;

		auto content = (*ok)["content"];
		if (content == nullptr) goto no_response;

		auto json = (*content)["application/json"];
		if (json == nullptr) goto no_response;

		auto return_schema = (*json)["schema"];
		if (return_schema == nullptr) goto no_response;

		return_type = resolve_schema_type_name(return_schema, a_riot_api_data, a_json, false);
	}
no_response:

	mstch::map method_context =
	{
		{ "method", method },
		{ "path", original_path },
		{ "sprintf_path", path },
		{ "docs", (*fetch_type["externalDocs"])["url"]->to_string() },
		{ "description", fetch_type["description"]->to_string() },
		{ "method_name", get_c_style_var(get_service_and_component_name(fetch_type["operationId"]->to_string()).second.c_str()) },
		{ "params", required_params },
		{ "optional_params", optional_params },
		{ "has_optional_args", has_optional },
		{ "path_params", path_params },
		{ "is_tft", strstr(service, "tft") != nullptr }
	};

	if (return_type.empty() == false)
		method_context["return"] = mstch::map{ { "type", return_type } };
	methods_context.push_back(method_context);
}

void parse_component(const std::string& name, component_data& component, mstch::array& components_context, std::map<std::string, bool>& was_mapped, api_data& riot_api_data, cho::json& riot_api)
{
	if (was_mapped[name])
		return;
	was_mapped[name] = true;

	for (auto& reference : component.references)
		parse_component(reference->name, *reference, components_context, was_mapped, riot_api_data, riot_api);

	auto properties = (*component.schema)["properties"];
	if (properties == nullptr)
		return;

	mstch::array properties_context;
	for (auto property : *properties)
	{
		mstch::map property_context =
		{
			{ "json_var", property.first.to_string() },
			{ "name", get_c_style_var(property.first.to_string().c_str()) },
			{ "type", resolve_schema_type_name(property.second, riot_api_data, riot_api, false) },
		};
		properties_context.push_back(property_context);
	}

	mstch::map component_context =
	{
		{ "name", get_c_style_var(name.c_str()) },
		{ "properties", properties_context },
	};
	components_context.push_back(component_context);
}

int main()
{
	printf("Fetching Ddragon versions..\n");
	cho::request ddragon_version_request(g_ddragon_versions_url);
	{
		profiler t("ddragon version request");
		ddragon_version_request.perform();
	}

	auto a = ddragon_version_request.get_json();
	std::string ddragon_version = a[0]->to_string();

	char ddragon_champion_url[256];
	snprintf(ddragon_champion_url, 256, g_ddragon_champion_url.c_str(), ddragon_version.c_str());
	cho::request ddragon_champion_request(ddragon_champion_url);
	{
		profiler t("ddragon champions request");
		ddragon_champion_request.perform();
	}
	cho::json ddragon_champion = ddragon_champion_request.get_json();

	mstch::array champion_keys_context;
	{
		profiler t("ddragon champion parsing");
		for (auto& champion_info : *ddragon_champion["data"])
		{
			auto& champion = *champion_info.second;

			auto id = champion["id"]->to_string();
			auto key = champion["key"]->to_string();
			mstch::map champion_context =
			{
				{ "id", id },
				{ "key", key },
			};
			champion_keys_context.push_back(champion_context);
		}
	}

	{
		printf("Fetching Riot API schema..\n");
		cho::request riot_api_schema_request(g_riot_api_schema);
		{
			profiler t("Riot API request");
			riot_api_schema_request.perform();
		}

		cho::json riot_api = riot_api_schema_request.get_json();
		api_data riot_api_data;

		{
			profiler t2("Fetch components");
			if (!fetch_components(riot_api_data, riot_api))
				return -1;
		}

		{
			profiler t3("Fetch function components");
			if (!fetch_function_components(riot_api_data, riot_api))
				return -1;
		}

		std::string platform_template;
		std::string header_template;
		std::string source_template;
		mstch::array services_context;
		mstch::array platforms_context;
		{
			profiler t4("Preparing render");
			platform_template = file_to_string("data/templates/platform_header.mst");
			header_template = file_to_string("data/templates/header.mst");
			source_template = file_to_string("data/templates/source.mst");

			// Parse server
			auto& server = *(*riot_api["servers"])[0];
			auto url = server["url"]->to_string();
			auto& platforms = *(*(*server["variables"])["platform"])["enum"];
			for (auto& platform_info : platforms)
			{
				std::string platform = platform_info.second->to_string();
				std::string final_url = url;
				replace(final_url, "{platform}", platform);

				mstch::map platform_context =
				{
					{ "platform", platform },
					{ "value", (int)platform_info.first.to_index() },
					{ "server", final_url },
				};
				platforms_context.push_back(platform_context);
			}

			// Parse services
			for (auto service : riot_api_data.services)
			{
				mstch::map service_context;
				mstch::array components_context;
				std::map<std::string, bool> was_mapped;

				for (auto component : riot_api_data.service_components[service])
					parse_component(component.first, *component.second, components_context, was_mapped, riot_api_data, riot_api);
				service_context["components"] = components_context;

				mstch::array methods_context;
				auto& paths = *riot_api["paths"];
				for (auto& path_infos : paths)
				{
					auto& path = path_infos.first.to_string();
					auto& path_info = *path_infos.second;

					if (path_info["x-endpoint"]->to_string() != service)
						continue;

					auto fetch_types = { "get", "post", "put", "options" };
					for (auto fetch_type : fetch_types)
						if (path_info[fetch_type] != nullptr)
							parse_path(fetch_type, path, service.c_str(), *path_info[fetch_type], methods_context, riot_api_data, riot_api);
				}

				service_context["service"] = get_c_style_service(service.c_str());
				service_context["methods"] = methods_context;

				mstch::array std_includes;
				std::copy(riot_api_data.std_includes.begin(), riot_api_data.std_includes.end(), std::back_inserter(std_includes));
				service_context["std_includes"] = std_includes;

				mstch::array cho_includes;
				std::copy(riot_api_data.cho_includes.begin(), riot_api_data.cho_includes.end(), std::back_inserter(cho_includes));
				service_context["cho_includes"] = std_includes;

				services_context.push_back(service_context);
			}
		}

		mstch::map context =
		{
			{ "platforms", platforms_context },
			{ "services", services_context },
			{ "champions", champion_keys_context },
		};

		std::string output;
		{
			profiler t5("Mstch Render Header");
			output = mstch::render(header_template, context);
		}
		std::ofstream("cho/inc/cho/generated/riot_api_data.hpp") << output;

		{
			profiler t5("Mstch Render Platform Header");
			output = mstch::render(platform_template, context);
		}
		std::ofstream("cho/inc/cho/generated/riot_api_platform.hpp") << output;

		{
			profiler t5("Mstch Render Source");
			output = mstch::render(source_template, context);
		}
		std::ofstream("cho/src/generated/riot_api_data.cpp") << output;
	}
}
