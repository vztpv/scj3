
#include "claujson.h"

#include "_simdjson.h" // modified simdjson // using simdjson 3.3.0

#include <future>

#include <set>
#include <execution>

#include "ThreadPool.h"

#include "rapidjson/writer.h"


#define claujson_inline _simdjson_inline


#define ERROR(msg) \
	do { \
		throw msg; \
		/* error.make(__LINE__, StringView(msg)); */ \
	} while (false) 


#if __cpp_lib_string_view
#else

claujson::StringView operator""sv(const char* str, size_t sz) {
	return claujson::StringView(str, sz);
}

uint64_t claujson::StringView::npos = -1;

bool operator==(const std::string& str, claujson::StringView sv) {
	return strcmp(str.data(), sv.data()) == 0;
}

#endif

namespace claujson {

	Value Structured::data_null{ nullptr, false }; // valid is false..
	uint64_t Structured::npos = -1; // 

	Log log;

	static _simdjson::dom::parser_for_claujson test_;
	_simdjson::internal::dom_parser_implementation* simdjson_imple = nullptr;
	static std::unique_ptr<ThreadPool> pool;

	// class PartialJson, only used in class LoadData.
	class PartialJson : public Structured {
	protected:
		std::vector<Value> arr_vec;
		//
		std::vector<Value> obj_key_vec;
		std::vector<Value> obj_val_vec;

		Value virtualJson;
	public:
		virtual ~PartialJson();

	private:
		friend class LoadData;
		friend class LoadData2;
		friend class Object;
		friend class Array;

		PartialJson();

	public:
		virtual bool is_partial_json() const;

		virtual bool is_object() const;

		virtual bool is_array() const;

		virtual uint64_t get_data_size() const;

		virtual Value& get_value_list(uint64_t idx);


	private:
		virtual Value& get_key_list(uint64_t idx);		
	public:

		virtual const Value& get_value_list(uint64_t idx) const;


		virtual const Value& get_key_list(uint64_t idx) const;

		virtual const Value& get_const_key_list(uint64_t idx);

		virtual const Value& get_const_key_list(uint64_t idx) const;

		virtual void clear(uint64_t idx);

		virtual bool is_virtual() const;

		virtual void clear();

		virtual void reserve_data_list(uint64_t len);



		virtual bool add_object_element(Value key, Value val);
		virtual bool add_array_element(Value val);

		virtual bool add_array(Ptr<Structured> arr);
		virtual bool add_object(Ptr<Structured> obj);


		virtual bool add_array(Value key, Ptr<Structured> arr);
		virtual bool add_object(Value key, Ptr<Structured> obj);

		virtual bool assign_value_element(uint64_t idx, Value val);
		virtual bool assign_key_element(uint64_t idx, Value key);

		virtual void erase(StringView key, bool real = false);
#if __cplusplus >= 202002L
		virtual void erase(std::u8string_view key, bool real = false);
#endif
		virtual void erase(uint64_t idx, bool real = false);


	private:

		virtual void MergeWith(PtrWeak<Structured> j, int start_offset);

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			ValueType type, uint64_t key_token_idx);

		virtual void add_user_type(ValueType type);

		virtual bool add_user_type(Value key, Ptr<Structured> j);

		virtual bool add_user_type(Ptr<Structured> j);


	};

	class VirtualObject : public Object {
	public:

		virtual bool is_virtual() const;

		virtual ~VirtualObject();
	};

	class VirtualArray : public Array {
	public:

		virtual bool is_virtual() const;

		virtual ~VirtualArray();
	};



	std::ostream& operator<<(std::ostream& stream, const claujson::Value& data) {

		if (false == data.is_valid()) {
			stream << "--not valid\n";
			return stream;
		}

		switch (data._type) {
		case claujson::ValueType::INT:
			stream << data._int_val;
			break;
		case claujson::ValueType::UINT:
			stream << data._uint_val;
			break;
		case claujson::ValueType::FLOAT:
			stream << data._float_val;
			break;
		case claujson::ValueType::STRING:
			stream << "\"" << (*data._str_val) << "\"";
			break;
		case claujson::ValueType::BOOL:
			stream << data._bool_val;
			break;
		case claujson::ValueType::NULL_:
			stream << "null";
			break;
		case claujson::ValueType::ARRAY:
		case claujson::ValueType::OBJECT:
		{
			//	stream << "array_or_object";
			auto* x = data.as_structured_ptr();
			if (x->is_array()) {
				stream << "[ ";
				uint64_t sz = x->get_data_size();
				for (uint64_t i = 0; i < sz; ++i) {
					stream << x->get_value_list(i) << " ";
					if (i < sz - 1) {
						stream << " , ";
					}
				}
				stream << "]\n";
			}
			else if (x->is_object()) {
				stream << "{ ";
				uint64_t sz = x->get_data_size();
				for (uint64_t i = 0; i < sz; ++i) {
					stream << x->get_key_list(i) << " : ";
					stream << x->get_value_list(i) << " ";
					if (i < sz - 1) {
						stream << " , ";
					}
				}
				stream << "}\n";
			}
		}
		break;
		}

		return stream;
	}
}

namespace claujson {


	Value Value::clone() const {
		if (!is_valid()) {
			return Value(nullptr, false);
		}

		Value x;

		x._type = this->_type;

		if (x._type == ValueType::STRING) {
			x._str_val = new std::string(this->_str_val->data(), this->_str_val->size());

		}
		else {
			x._int_val = this->_int_val;
		}

		if (x.is_structured()) {
			x._array_or_object_ptr = this->as_structured_ptr()->clone();
		}

		return x;
	}
	Value::operator bool() const {
		return this->_valid;
	}

	Value::Value(Structured* x) {
		set_ptr(x);
	}
	Value::Value(int x) {
		set_int(x);
	}

	Value::Value(unsigned int x) {
		set_uint(x);
	}

	Value::Value(int64_t x) {
		set_int(x);
	}
	Value::Value(uint64_t x) {
		set_uint(x);
	}
	Value::Value(double x) {
		set_float(x);
	}
	Value::Value(StringView x, bool convert) {
		if (!set_str(x.data(), x.size(), convert)) {
			this->_valid = false;
		}
	}

#if __cplusplus >= 202002L
	// C++20~
	Value::Value(std::u8string_view x, bool convert) {
		if (!set_str(reinterpret_cast<const char*>(x.data()), x.size(), convert)) {
			this->_valid = false;
		}
	}

	Value::Value(const char8_t* x, bool convert) {
		std::u8string_view sv(x);

		if (!set_str(reinterpret_cast<const char*>(sv.data()), sv.size(), convert)) {
			this->_valid = false;
		}
	}

#endif

	Value::Value(const char* x, bool convert) {
		if (!set_str(x, strlen(x), convert)) {
			this->_valid = false;
		}
	}

	Value::Value(bool x) {
		set_bool(x);
	}
	Value::Value(nullptr_t x) {
		set_type(ValueType::NULL_);
	}

	Value::Value(nullptr_t, bool valid) {
		set_type(ValueType::NULL_);
		this->_valid = valid;
	}

	ValueType Value::type() const {
		return _type;
	}

	bool Value::is_valid() const {
		return this->_valid;
	}

	bool Value::is_null() const {
		return is_valid() && type() == ValueType::NULL_;
	}

	bool Value::is_primitive() const {
		return is_valid() && !is_structured();
	}

	bool Value::is_structured() const {
		return is_valid() && (type() == ValueType::ARRAY || type() == ValueType::OBJECT);
	}

	bool Value::is_array() const {
		auto* x = this->as_structured_ptr();

		if (x) {
			return x->is_array();
		}
		return false;
	}

	bool Value::is_object() const {
		auto* x = this->as_structured_ptr();

		if (x) {
			return x->is_object();
		}
		return false;
	}



	bool Value::is_int() const {
		return is_valid() && type() == ValueType::INT;
	}

	bool Value::is_uint() const {
		return is_valid() && type() == ValueType::UINT;
	}

	bool Value::is_float() const {
		return is_valid() && type() == ValueType::FLOAT;
	}

	bool Value::is_bool() const {
		return is_valid() && (type() == ValueType::BOOL);
	}

	bool Value::is_str() const {
		return is_valid() && type() == ValueType::STRING;
	}

	int64_t Value::int_val() const {
		return _int_val;
	}

	uint64_t Value::uint_val() const {
		return _uint_val;
	}

	double Value::float_val() const {
		return _float_val;
	}

	int64_t& Value::int_val() {
		return _int_val;
	}

	uint64_t& Value::uint_val() {
		return _uint_val;
	}

	double& Value::float_val() {
		return _float_val;
	}

	bool Value::bool_val() const {
		if (!is_bool()) {
			return false;
		}
		return _bool_val;
	}


	bool& Value::bool_val() {
		return _bool_val;
	}



	claujson_inline StringView sub_route(StringView route, uint64_t found_idx, uint64_t new_idx) {
		if (found_idx + 1 == new_idx) {
			return ""sv;
		}
		return route.substr(found_idx + 1, new_idx - found_idx - 1);
	}
#if __cplusplus >= 202002L
	claujson_inline std::u8string_view sub_route(std::u8string_view route, uint64_t found_idx, uint64_t new_idx) {
		if (found_idx + 1 == new_idx) {
			return u8""sv;
		}
		return route.substr(found_idx + 1, new_idx - found_idx - 1);
	}
#endif
	bool to_uint_for_json_pointer(StringView x, uint64_t* val, _simdjson::internal::dom_parser_implementation* simdjson_imple) {
		const char* buf = x.data();
		uint64_t idx = 0;
		uint64_t idx2 = x.size();

		switch (x[0]) {
		case '0':
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			std::unique_ptr<uint8_t[]> copy;

			uint64_t temp[2] = { 0 };

			const uint8_t* value = reinterpret_cast<const uint8_t*>(buf + idx);

			{ // chk code...
				copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[idx2 - idx + _simdjson::_SIMDJSON_PADDING]); // x.size() + padding
				if (copy.get() == nullptr) { return false; } // cf) new Json?
				std::memcpy(copy.get(), &buf[idx], idx2 - idx);
				std::memset(copy.get() + idx2 - idx, ' ', _simdjson::_SIMDJSON_PADDING);
				value = copy.get();
			}
			auto x = simdjson_imple->parse_number(value, temp);

			if (x != _simdjson::SUCCESS) {
				log << warn << "parse number error. " << x << "\n";
				return false;
			}

			long long int_val = 0;
			unsigned long long uint_val = 0;
			//double float_val = 0;

			switch (static_cast<_simdjson::internal::tape_type>(temp[0] >> 56)) {
			case _simdjson::internal::tape_type::INT64:
				memcpy(&int_val, &temp[1], sizeof(uint64_t));
				*val = int_val;

				return true;
				break;
			case _simdjson::internal::tape_type::UINT64:
				memcpy(&uint_val, &temp[1], sizeof(uint64_t));
				*val = uint_val;

				return true;
				break;
			case _simdjson::internal::tape_type::DOUBLE:
				// error.
				return false;
				break;
			}
		}
		break;
		}

		return false;
	}
#if __cplusplus >= 202002L
	bool to_uint_for_json_pointer(std::u8string_view x, uint64_t* val, _simdjson::internal::dom_parser_implementation* simdjson_imple) {
		auto* buf = x.data();
		uint64_t idx = 0;
		uint64_t idx2 = x.size();

		switch (x[0]) {
		case '0':
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			std::unique_ptr<uint8_t[]> copy;

			uint64_t temp[2] = { 0 };

			const uint8_t* value = reinterpret_cast<const uint8_t*>(buf + idx);

			{ // chk code...
				copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[idx2 - idx + _simdjson::_SIMDJSON_PADDING]); // x.size() + padding
				if (copy.get() == nullptr) { return false; } // cf) new Json?
				std::memcpy(copy.get(), &buf[idx], idx2 - idx);
				std::memset(copy.get() + idx2 - idx, ' ', _simdjson::_SIMDJSON_PADDING);
				value = copy.get();
			}

			if (auto x = simdjson_imple->parse_number(value, temp)
				; x != _simdjson::SUCCESS) {
				log << warn << "parse number error. " << x << "\n";
				return false;
			}

			long long int_val = 0;
			unsigned long long uint_val = 0;
			//double float_val = 0;

			switch (static_cast<_simdjson::internal::tape_type>(temp[0] >> 56)) {
			case _simdjson::internal::tape_type::INT64:
				memcpy(&int_val, &temp[1], sizeof(uint64_t));
				*val = int_val;

				return true;
				break;
			case _simdjson::internal::tape_type::UINT64:
				memcpy(&uint_val, &temp[1], sizeof(uint64_t));
				*val = uint_val;

				return true;
				break;
			case _simdjson::internal::tape_type::DOUBLE:
				// error.
				return false;
				break;
			}
		}
		break;
		}

		return false;
	}
#endif
	bool Value::json_pointerA(StringView route, std::vector<Value>& vec, bool convert) {
		std::vector<StringView> routeVec;
		std::vector<Value> routeDataVec;


		if (route.empty()) {
			vec.clear();
			return true;
		}

		routeVec.reserve(route.size());
		routeDataVec.reserve(route.size());

		if (route[0] != '/') {
			return false;
		}

		// 1. route -> split with '/'  to routeVec.
		uint64_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != StringView::npos) {
			uint64_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == StringView::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else {... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Value temp(x, convert); // do 2, 3.

			if (temp.is_valid()) {
				routeDataVec.push_back(std::move(temp));
			}
			else {
				return false;
			}
		}

		vec = std::move(routeDataVec);

		return true;
	}
#if __cplusplus >= 202002L
	bool Value::json_pointerA(std::u8string_view route, std::vector<Value>& vec, bool convert) {
		std::vector<std::u8string_view> routeVec;
		std::vector<Value> routeDataVec;


		if (route.empty()) {
			vec.clear();
			return true;
		}

		routeVec.reserve(route.size());
		routeDataVec.reserve(route.size());

		if (route[0] != '/') {
			return false;
		}

		// 1. route -> split with '/'  to routeVec.
		uint64_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != StringView::npos) {
			uint64_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == StringView::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else {... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Value temp(x, convert); // do 2, 3.

			if (temp.is_valid()) {
				routeDataVec.push_back(std::move(temp));
			}
			else {
				return false;
			}
		}

		vec = std::move(routeDataVec);

		return true;
	}
#endif


	Value& Value::json_pointerB(const std::vector<Value>& routeDataVec) { // option-> StringView route?
		static Value unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (routeDataVec.empty()) {
			return *this;
		}

		// 4. find Data with route. and return
		Value* data = this;

		for (uint64_t i = 0; i < routeDataVec.size(); ++i) {
			const Value& x = routeDataVec[i];

			if (data->is_primitive()) {
				if (i == routeDataVec.size() - 1) {
					return *data;
				}
				else {
					return unvalid_data;
				}
			}

			Structured* j = data->as_structured_ptr();

			if (j->is_array()) { // array -> with idx
				uint64_t idx = 0;
				//bool found = false;
				//uint64_t arr_size = j->get_data_size();

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx, simdjson_imple);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				StringView str = x.str_val();
				std::string result(str.data(), str.size());

				uint64_t count = 0;

				// chk ~0 -> ~, ~1 -> /
				uint64_t idx = 0;

				idx = str.find('~');

				while (idx != std::string::npos) {
					uint64_t k = idx;

					if (k + 1 < str.size()) {
						if (str[k + 1] == '0') {
							result[k] = '~';
							result.erase(result.begin() + k + 1);
							count++;
						}
						else if (str[k + 1] == '1') {
							result[k] = '/';
							result.erase(result.begin() + k + 1);
							count++;
						}
						else {
							return unvalid_data;
						}
					}
					else {
						return unvalid_data;
					}

					idx = str.find('~');
				}

				result.resize(result.size() - count);
				data = &((*j)[result]);
			}
		}

		return *data;
	}

	// race condition..? not support multi-thread  access...
	Value& Value::json_pointer(StringView route, bool convert) {
		static Value unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (route.empty()) {
			return *this;
		}

		std::vector<StringView> routeVec;
		std::vector<Value> routeDataVec;

		routeVec.reserve(route.size());
		routeDataVec.reserve(route.size());

		if (route[0] != '/') {
			return unvalid_data;
		}

		// 1. route -> split with '/'  to routeVec.
		uint64_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != StringView::npos) {
			uint64_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == StringView::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else {... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Value temp(x, convert);

			if (temp.is_valid()) {
				routeDataVec.push_back(std::move(temp));
			}
			else {
				return unvalid_data;
			}
		}

		// 4. find Data with route. and return
		Value* data = this;

		for (uint64_t i = 0; i < routeDataVec.size(); ++i) {
			Value& x = routeDataVec[i];

			if (data->is_primitive()) {
				if (i == routeDataVec.size() - 1) {
					return *data;
				}
				else {
					return unvalid_data;
				}
			}

			Structured* j = data->as_structured_ptr();

			if (j->is_array()) { // array -> with idx
				uint64_t idx = 0;

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx, simdjson_imple);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				StringView str = x.str_val();
				std::string result;

				result.reserve(str.size());

				// chk ~0 -> ~, ~1 -> /
				for (uint64_t k = 0; k < str.size(); ++k) {
					if (str[k] == '~') {
						if (k + 1 < str.size()) {
							if (str[k + 1] == '0') {
								result.push_back('~');
								++k;
							}
							else if (str[k + 1] == '1') {
								result.push_back('/');
								++k;
							}
							else {
								return unvalid_data;
							}
						}
						else {
							return unvalid_data;
						}
					}
					else {
						result.push_back(str[k]);
					}
				}

				data = &j->operator[](result);
			}
		}

		return *data;
	}
	const Value& Value::json_pointer(StringView route, bool convert) const {
		static const Value unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (route.empty()) {
			return *this;
		}

		std::vector<StringView> routeVec;
		std::vector<Value> routeDataVec;

		routeVec.reserve(route.size());
		routeDataVec.reserve(route.size());

		if (route[0] != '/') {
			return unvalid_data;
		}

		// 1. route -> split with '/'  to routeVec.
		uint64_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != StringView::npos) {
			uint64_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == StringView::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else {... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Value temp(x, convert);

			if (temp.is_valid()) {
				routeDataVec.push_back(std::move(temp));
			}
			else {
				return unvalid_data;
			}
		}

		// 4. find Data with route. and return
		const Value* data = this;

		for (uint64_t i = 0; i < routeDataVec.size(); ++i) {
			Value& x = routeDataVec[i];

			if (data->is_primitive()) {
				if (i == routeDataVec.size() - 1) {
					return *data;
				}
				else {
					return unvalid_data;
				}
			}

			const Structured* j = data->as_structured_ptr();

			if (j->is_array()) { // array -> with idx
				uint64_t idx = 0;
				//bool found = false;
				//uint64_t arr_size = j->get_data_size();

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx, simdjson_imple);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				StringView str = x.str_val();
				std::string result;

				result.reserve(str.size());

				// chk ~0 -> ~, ~1 -> /
				for (uint64_t k = 0; k < str.size(); ++k) {
					if (str[k] == '~') {
						if (k + 1 < str.size()) {
							if (str[k + 1] == '0') {
								result.push_back('~');
								++k;
							}
							else if (str[k + 1] == '1') {
								result.push_back('/');
								++k;
							}
							else {
								return unvalid_data;
							}
						}
						else {
							return unvalid_data;
						}
					}
					else {
						result.push_back(str[k]);
					}
				}

				data = &((*j)[result]); // chk at vs []
			}
		}

		return *data;
	}

	// need refactoring...
	Value& Value::json_pointer(const Value& route) {
		return json_pointer(route.get_string(), false);
	}
	const Value& Value::json_pointer(const Value& route) const {
		return json_pointer(route.get_string(), false);
	}

#if __cplusplus >= 202002L
	Value& Value::json_pointer(std::u8string_view route, bool convert) {
		return json_pointer(StringView(reinterpret_cast<const char*>(route.data(), route.size()), convert));
	}
	const Value& Value::json_pointer(std::u8string_view route, bool convert) const {
		return json_pointer(StringView(reinterpret_cast<const char*>(route.data(), route.size()), convert));
	}


#endif


	void Value::clear(bool real) {

		if (real && _type == ValueType::STRING) {
			delete _str_val; _str_val = nullptr;
			_type = ValueType::NONE;
		}
		else if (_type == ValueType::STRING) {
			//
		}
		else {
			_int_val = 0;

			_type = ValueType::NONE;
		}
	}

	std::string& Value::str_val() {
		// type check...
		return *_str_val;
	}

	const std::string& Value::str_val() const {
		// type check...
		return *_str_val;
	}

	void Value::set_ptr(Structured* x) {
		if (!is_valid()) {
			return;
		}

		if (_type == ValueType::STRING) {
			delete _str_val;
		}

		_array_or_object_ptr = x;

		// ValueType::ERROR -> ValueType::NOT_ARRAY_AND_OBJECT ?
		_type = x->is_array() ? ValueType::ARRAY : (x->is_object() ? ValueType::OBJECT : ValueType::ERROR); // chk change DataType:: ~~ -> DataType:: ~~
	}
	void Value::set_int(long long x) {
		if (!is_valid()) {
			return;
		}

		if (_type == ValueType::STRING) {
			delete _str_val;
		}
		_int_val = x;
		_type = ValueType::INT;
	}

	void Value::set_uint(unsigned long long x) {
		if (!is_valid()) {
			return;
		}
		if (_type == ValueType::STRING) {
			delete _str_val;
		}
		_uint_val = x;
		_type = ValueType::UINT;
	}

	void Value::set_float(double x) {
		if (!is_valid()) {
			return;
		}
		if (_type == ValueType::STRING) {
			delete _str_val;
		}
		_float_val = x;

		_type = ValueType::FLOAT;
	}

	bool Value::set_str(const char* str, uint64_t len, bool convert) {
		if (!is_valid()) {
			return false;
		}

		if (!convert) {
			if (_type != ValueType::STRING) {
				_str_val = new std::string(str, len);
			}
			else {
				_str_val->assign(str, len);
			}

			_type = ValueType::STRING;
			return true;
		}

		const uint64_t block_size = 1024;


		uint8_t buf_src[block_size + _simdjson::_SIMDJSON_PADDING];
		uint8_t buf_dest[block_size + _simdjson::_SIMDJSON_PADDING];


		if (len >= block_size) {
			uint8_t* buf_src = (uint8_t*)calloc(len + 1 + _simdjson::_SIMDJSON_PADDING, sizeof(uint8_t));
			uint8_t* buf_dest = (uint8_t*)calloc(len + 1 + _simdjson::_SIMDJSON_PADDING, sizeof(uint8_t));
			if (!buf_src || !buf_dest) {
				if (buf_src) { free(buf_src); }
				if (buf_dest) { free(buf_dest); }

				return false;
			}
			memset(buf_src, '"', len + 1 + _simdjson::_SIMDJSON_PADDING);
			memset(buf_dest, '"', len + 1 + _simdjson::_SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			// chk... fallback..
			{
				bool valid = _simdjson::String::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					free(buf_src);
					free(buf_dest);

					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return false;
				}
			}
			auto* x = simdjson_imple->parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				free(buf_src);
				free(buf_dest);

				log << warn << "Error in Convert for string";
				return false;
			}
			else {
				*x = '\0';
				uint64_t string_length = uint32_t(x - buf_dest);

				if (_type != ValueType::STRING) {
					_str_val = new std::string((char*)buf_dest, string_length);
				}
				else {
					_str_val->assign((char*)buf_dest, string_length);
				}
			}

			free(buf_src);
			free(buf_dest);
		}
		else {
			memset(buf_src, '"', block_size + _simdjson::_SIMDJSON_PADDING);
			memset(buf_dest, '"', block_size + _simdjson::_SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			{
				bool valid = _simdjson::String::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return false;
				}
			}
			auto* x = simdjson_imple->parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				log << warn << "Error in Convert for string";
				return false;
			}
			else {
				*x = '\0';
				uint64_t string_length = uint32_t(x - buf_dest);

				if (_type != ValueType::STRING) {
					_str_val = new std::string((char*)buf_dest, string_length);
				}
				else {
					_str_val->assign((char*)buf_dest, string_length);
				}
			}
		}

		_type = ValueType::STRING;

		return true;
	}

	void Value::set_str_in_parse(const char* str, uint64_t len) {
		if (_type != ValueType::STRING) {
			_str_val = new std::string(str, len);
		}
		else {
			_str_val->assign(str, len);
		}
		_type = ValueType::STRING;
	}

	void Value::set_bool(bool x) {
		if (!is_valid()) {
			return;
		}
		if (_type == ValueType::STRING) {
			delete _str_val;
		}

		_bool_val = x;

		{
			set_type(ValueType::BOOL);
		}
	}

	void Value::set_null() {
		if (!is_valid()) {
			return;
		}
		if (_type == ValueType::STRING) {
			delete _str_val;
		}

		set_type(ValueType::NULL_);
	}

	void Value::set_type(ValueType type) {
		this->_type = type;
	}

	Value::~Value() {
		if (_type == ValueType::STRING && _str_val) {
			//log << warn  << "chk";
			delete _str_val;
			_str_val = nullptr;
		}
	}

	Value::Value(Value&& other) noexcept
		: _type(other._type), _valid(other._valid)
	{
		if (!other.is_valid()) {
			return;
		}

		if (_type == ValueType::STRING) {
			_str_val = other._str_val;
			other._str_val = nullptr;
			other._type = ValueType::NONE;
		}
		else {
			std::swap(_int_val, other._int_val);
			other._type = ValueType::NONE;
		}
	}

	Value::Value() : _int_val(0), _valid(true), _type(ValueType::NONE) { }

	bool Value::operator==(const Value& other) const { // chk array or object?
		if (this->_type == other._type) {
			switch (this->_type) {
			case ValueType::STRING:
				return (*this->_str_val) == (*other._str_val);
				break;
			case ValueType::INT:
				return this->_int_val == other._int_val;
				break;
			case ValueType::UINT:
				return this->_uint_val == other._uint_val;
				break;
			case ValueType::FLOAT:
				return this->_float_val == other._float_val;
				break;
			case ValueType::BOOL:
				return this->_bool_val == other._bool_val;
				break;
			case ValueType::ARRAY:
			case ValueType::OBJECT:
			{
				const Structured* j = this->as_structured_ptr();
				const Structured* k = other.as_structured_ptr();

				const uint64_t sz_j = j->get_data_size();
				const uint64_t sz_k = k->get_data_size();

				if (sz_j != sz_k) { return false; }

				const uint64_t sz = sz_j;

				if (j->is_array() && k->is_array()) {
					for (uint64_t i = 0; i < sz; ++i) {
						if (j->get_value_list(i) != k->get_value_list(i)) {
							return false;
						}
					}
				}
				else if (j->is_object() && k->is_object()) {
					for (uint64_t i = 0; i < sz; ++i) {
						if (j->get_value_list(i) != k->get_value_list(i)) {
							return false;
						}
						if (j->get_key_list(i) != k->get_key_list(i)) {
							return false;
						}
					}
				}
				else {
					return false;
				}
			}
			break;
			}
			return true;
		}
		return false;
	}

	bool Value::operator!=(const Value& other) const {
		return !((*this) == other);
	}

	bool Value::operator<(const Value& other) const {
		if (this->_type == other._type) {
			switch (this->_type) {
			case ValueType::STRING:
				return (*this->_str_val) < (*other._str_val);
				break;
			case ValueType::INT:
				return this->_int_val < other._int_val;
				break;
			case ValueType::UINT:
				return this->_uint_val < other._uint_val;
				break;
			case ValueType::FLOAT:
				return this->_float_val < other._float_val;
				break;
			case ValueType::BOOL:
				return this->_bool_val < other._bool_val;
				break;
			}

		}
		return false;
	}


	Value& Value::operator=(Value&& other) noexcept {
		if (this == &other) {
			return *this;
		}

		if (!is_valid()) {
			return *this;
		}

		std::swap(this->_type, other._type);
		std::swap(this->_int_val, other._int_val);

		clean(other);

		return *this;
	}

	claujson_inline bool ConvertString(claujson::Value& data, char* text, uint64_t len) {
		uint8_t sbuf[1024 + 1 + _simdjson::_SIMDJSON_PADDING];
		std::unique_ptr<uint8_t[]> ubuf;
		uint8_t* string_buf = nullptr;

		if (len <= 1024) {
			string_buf = sbuf;
		}
		else {
			ubuf = std::make_unique<uint8_t[]>(len + _simdjson::_SIMDJSON_PADDING);
			string_buf = &ubuf[0];
		}
		auto* x = simdjson_imple->parse_string((uint8_t*)text + 1, string_buf, false);
		if (x == nullptr) {
			return false; // ERROR("Error in Convert for string");
		}
		else {
			*x = '\0';
			auto string_length = uint32_t(x - string_buf);
			data.set_str_in_parse(reinterpret_cast<char*>(string_buf), string_length);
		}
		return true;
	}

	claujson_inline bool ConvertNumber(claujson::Value& data, char* text, uint64_t len, bool isFirst) {

		std::unique_ptr<uint8_t[]> copy;

		uint64_t temp[2] = { 0 };

		uint8_t* value = reinterpret_cast<uint8_t*>(text);

		if (isFirst) { // if this case may be root number -> chk.. visit_root_number. in tape_builder in simdjson.cpp
			copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[len + _simdjson::_SIMDJSON_PADDING]);
			if (copy.get() == nullptr) { return false; } // ERROR("Error in Convert for new"); } // cf) new Json?
			std::memcpy(copy.get(), text, len);
			std::memset(copy.get() + len, ' ', _simdjson::_SIMDJSON_PADDING);
			value = copy.get();
		}

		auto x = simdjson_imple->parse_number(value, temp);

		if (x != _simdjson::SUCCESS) {
			log << warn << StringView((char*)value, 20) << "\n";
			log << warn << "parse number error. " << x << "\n";
			return false; //ERROR("parse number error. ");
			//throw "Error in Convert to parse number";
		}

		long long int_val = 0;
		unsigned long long uint_val = 0;
		double float_val = 0;

		switch (static_cast<_simdjson::internal::tape_type>(temp[0] >> 56)) {
		case _simdjson::internal::tape_type::INT64:
			memcpy(&int_val, &temp[1], sizeof(uint64_t));

			data.set_int(int_val);
			break;
		case _simdjson::internal::tape_type::UINT64:
			memcpy(&uint_val, &temp[1], sizeof(uint64_t));

			data.set_uint(uint_val);
			break;
		case _simdjson::internal::tape_type::DOUBLE:
			memcpy(&float_val, &temp[1], sizeof(uint64_t));

			data.set_float(float_val);
			break;
		}

		return true;
	}

	claujson_inline claujson::Value& Convert(claujson::Value& data, uint64_t buf_idx, uint64_t next_buf_idx, bool key,
		char* buf, uint64_t token_idx, bool& err) {

		//try {
		data.clear(false);

		switch (buf[buf_idx]) {
		case '"':
			if (ConvertString(data, &buf[buf_idx], next_buf_idx - buf_idx)) {}
			else {
				goto ERR;
			}
			break;
		case '-':
		case '0':
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			if (ConvertNumber(data, &buf[buf_idx], next_buf_idx - buf_idx, token_idx == 0)) {}
			else {
				goto ERR;
			}

			break;
		}
		case 't':
		{
			if (!simdjson_imple->is_valid_true_atom(reinterpret_cast<uint8_t*>(&buf[buf_idx]), next_buf_idx - buf_idx)) {
				goto ERR; //ERROR("Error in Convert for true");
			}

			data.set_bool(true);
		}
		break;
		case 'f':
			if (!simdjson_imple->is_valid_false_atom(reinterpret_cast<uint8_t*>(&buf[buf_idx]), next_buf_idx - buf_idx)) {
				goto ERR; //ERROR("Error in Convert for false");
			}

			data.set_bool(false);
			break;
		case 'n':
			if (!simdjson_imple->is_valid_null_atom(reinterpret_cast<uint8_t*>(&buf[buf_idx]), next_buf_idx - buf_idx)) {
				goto ERR; //ERROR("Error in Convert for null");
			}

			data.set_null();
			break;

		default:
			log << warn << "convert error : " << (int)buf[buf_idx] << " " << buf[buf_idx] << "\n";
			goto ERR; //ERROR("convert Error");
			//throw "Error in Convert : not expected";
		}
		return data;
		//}

		//catch (const char* str) {
	ERR:
		//log << warn << str << "\n";
		//ERROR(str);
		err = true;
		return data;
	}

	//bool Structured::is_valid() const {
	//	return valid;
	//}


	Structured* Structured::clone() const {
		if (this->is_array()) {
			return ((Array*)this)->clone();
		}
		else if (this->is_object()) {
			return ((Object*)this)->clone();
		}
		else {
			return nullptr;
		}
	}

	//Structured::Structured(bool valid) : valid(valid) { }

	Structured::Structured() { }


	Structured::~Structured() {

	}



	const Value& Structured::at(StringView key) const {
		if (!is_object()) {
			return data_null;
		}

		// is valid utf-8 and no \\~ ?? //
		auto key_in_json = claujson::convert_to_string_in_json(key);
		if (!key_in_json.first) {
			return data_null;
		}

		uint64_t len = get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (get_key_list(i).str_val().compare(key_in_json.second) == 0) {
				return get_value_list(i);
			}
		}

		return data_null;
	}

	Value& Structured::at(StringView key) {
		if (!is_object()) { //} || !is_valid()) {
			return data_null;
		}

		// is valid utf-8 and no \\~ ?? //
		auto key_in_json = claujson::convert_to_string_in_json(key);
		if (!key_in_json.first) {
			return data_null;
		}

		uint64_t len = get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (get_key_list(i).str_val().compare(key_in_json.second) == 0) {
				return get_value_list(i);
			}
		}

		return data_null;
	}

	uint64_t Structured::find(StringView key) const {
		if (!is_object()) { //} || !is_valid()) {
			return npos;
		}

		// is valid utf-8 and no \\~ ?? //
		auto key_in_json = claujson::convert_to_string_in_json(key);
		if (!key_in_json.first) {
			return npos;
		}

		uint64_t len = get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (get_key_list(i).str_val().compare(key_in_json.second) == 0) {
				return i;
			}
		}

		return -1;
	}

	uint64_t Structured::find_(StringView key) const {
		if (!is_object()) { // } || !is_valid()) {
			return npos;
		}

		uint64_t len = get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (get_key_list(i).str_val() == key) {
				return i;
			}
		}

		return -1;
	}

#if __cplusplus >= 202002L
	const Value& Structured::at(std::u8string_view key) const {
		return at(StringView(reinterpret_cast<const char*>(key.data()), key.size()));
	}

	Value& Structured::at(std::u8string_view key) {
		return at(StringView(reinterpret_cast<const char*>(key.data()), key.size()));
	}

	uint64_t Structured::find(std::u8string_view key) const { // chk (uint64_t)-1 == (maximum)....-> eof?
		return find(StringView(reinterpret_cast<const char*>(key.data()), key.size()));
	}

	uint64_t Structured::find_(std::u8string_view key) const { // chk (uint64_t)-1 == (maximum)....-> eof?
		return find_(StringView(reinterpret_cast<const char*>(key.data()), key.size()));
	}

	Value& Structured::operator[](std::u8string_view key) { // if not exist key, then nothing.
		return operator[](StringView(reinterpret_cast<const char*>(key.data()), key.size()));
	}
	const Value& Structured::operator[](std::u8string_view key) const { // if not exist key, then nothing.
		return operator[](StringView(reinterpret_cast<const char*>(key.data()), key.size()));
	}
#endif

	Value& Structured::operator[](uint64_t idx) {
		if (idx >= get_data_size()) {
			return data_null;
		}
		return get_value_list(idx);
	}

	const Value& Structured::operator[](uint64_t idx) const {
		if (idx >= get_data_size()) {
			return data_null;
		}
		return get_value_list(idx);
	}


	Value& Structured::operator[](StringView key) { // if not exist key, then nothing.
		uint64_t idx = npos;
		if ((idx = find_(key)) == npos) {
			return data_null;
		}

		return get_value_list(idx);
	}
	const Value& Structured::operator[](StringView key) const { // if not exist key, then nothing.
		uint64_t idx = npos;
		if ((idx = find_(key)) == npos) {
			return data_null;
		}

		return get_value_list(idx);
	}



	PtrWeak<Structured> Structured::get_parent() const {
		return parent;
	}

	bool Structured::change_key(const Value& key, const Value& new_key) { // chk test...
		if (this->is_object() && key.is_str() && new_key.is_str()) {
			auto idx = find(key.str_val());
			if (idx == npos) {
				return false;
			}

			// is valid utf-8 and no \\~ ?? //
			auto new_key_in_json = claujson::convert_to_string_in_json(new_key.get_string());
			if (!new_key_in_json.first) {
				return npos;
			}

			get_key_list(idx) = Value(new_key_in_json.second);

			return true;
		}
		return false;
	}

	Value& Structured::get_value() {
		return data_null;
	}

	bool Structured::is_partial_json() const { return false; }

	bool Structured::is_user_type() const {
		return is_object() || is_array() || is_partial_json();
	}

	void Structured::set_parent(PtrWeak<Structured> j) {
		parent = j;
	}


	//Object::Object(bool valid) : Structured(valid) { }


	class CompKey {
	private:
		const std::vector<Value>* vec;
	public:

		CompKey(const std::vector<Value>* vec) : vec(vec) {
			//
		}

		bool operator()(uint64_t x, uint64_t y) const {
			return (*vec)[x] < (*vec)[y];
		}
	};

	Structured* Object::clone() const {
		Structured* result = new Object();

		uint64_t sz = this->get_data_size();

		for (uint64_t i = 0; i < sz; ++i) {
			result->add_object_element(this->get_key_list(i).clone(), this->get_value_list(i).clone());
		}

		return result;
	}

	bool Object::chk_key_dup(uint64_t* idx) const {
		bool has_dup = false;
		std::vector<uint64_t> copy_(obj_key_vec.size(), 0);

		for (uint64_t i = 0; i < copy_.size(); ++i) {
			copy_[i] = i;
		}

		CompKey comp(&obj_key_vec);

		std::stable_sort(copy_.begin(), copy_.end(), comp);

		for (uint64_t i = 1; i < copy_.size(); ++i) {
			if (obj_key_vec[copy_[i]] == obj_key_vec[copy_[i - 1]]) {
				has_dup = true;
				if (idx) {
					*idx = copy_[i - 1]; //
				}
				break;
			}
		}

		return has_dup;
	}

	Value Object::Make() {
		return Value(new Object());
	}

	Object::Object() { }

	Object::~Object() {
		for (auto& x : obj_val_vec) {
			if (x.is_structured()) {
				delete x.as_structured_ptr();
			}
		}
	}

	bool Object::is_object() const {
		return true;
	}
	bool Object::is_array() const {
		return false;
	}

	claujson_inline uint64_t Object::get_data_size() const {
		return obj_val_vec.size();
	}

	Value& Object::get_value_list(uint64_t idx) {
		return obj_val_vec[idx];
	}

	Value& Object::get_key_list(uint64_t idx) { // if key change then also obj_val_vec[idx].key? change??
		return obj_key_vec[idx];
	}

	const Value& Object::get_const_key_list(uint64_t idx) {
		return obj_key_vec[idx];
	}
	const Value& Object::get_const_key_list(uint64_t idx) const {
		return obj_key_vec[idx];
	}
	const Value& Object::get_value_list(uint64_t idx) const {
		return obj_val_vec[idx];
	}

	const Value& Object::get_key_list(uint64_t idx) const {
		return obj_key_vec[idx];
	}

	void Object::clear(uint64_t idx) {
		obj_val_vec[idx].clear(false);
	}

	bool Object::is_virtual() const {
		return false;
	}

	void Object::clear() {
		obj_val_vec.clear();
		obj_key_vec.clear();
	}


	std::vector<Value>::iterator Object::begin() {
		return obj_val_vec.begin();
	}

	std::vector<Value>::iterator Object::end() {
		return obj_val_vec.end();
	}

	void Object::reserve_data_list(uint64_t len) {
		obj_val_vec.reserve(len);
		obj_key_vec.reserve(len);
	}

	bool Object::add_object_element(Value key, Value val) {
		if (!key.is_str()) {
			return false;
		}

		if (val.is_structured()) {
			auto* x = (Structured*)val.as_structured_ptr();
			x->set_parent(this);
		}
		obj_key_vec.push_back(std::move(key));
		obj_val_vec.push_back(std::move(val));

		return true;
	}

	bool Object::add_array_element(Value val) { return false; }
	bool Object::add_array(Ptr<Structured> arr) {
		return false;
	}

	bool Object::add_object(Ptr<Structured> obj) {

		return false;
	}

	bool Object::add_array(Value key, Ptr<Structured> arr) {

		if (key.is_str() && arr->is_array()) {
			arr->set_parent(this);
			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(Value(arr.release()));
		}
		else {
			return false;
		}

		return true;
	}

	bool Object::add_object(Value key, Ptr<Structured> obj) {
		if (key.is_str() && obj->is_object()) {
			obj->set_parent(this);
			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(Value(obj.release()));
		}
		else {
			return false;
		}

		return false;
	}

	bool Object::assign_value_element(uint64_t idx, Value val) { this->obj_val_vec[idx] = std::move(val); return true; }
	bool Object::assign_key_element(uint64_t idx, Value key) {
		if (!key || !key.is_str()) {
			return false;
		}
		this->obj_key_vec[idx] = std::move(key);
		return true;
	}

	void Object::erase(StringView key, bool real) {
		uint64_t idx = this->find(key);
		erase(idx, real);
	}
#if __cplusplus >= 202002L
	void Object::erase(std::u8string_view key, bool real) {
		uint64_t idx = this->find(key);
		erase(idx, real);
	}
#endif
	void Object::erase(uint64_t idx, bool real) {

		if (real) {
			clean(obj_key_vec[idx]);
			clean(obj_val_vec[idx]);
		}

		obj_key_vec.erase(obj_key_vec.begin() + idx);
		obj_val_vec.erase(obj_val_vec.begin() + idx);
	}


	void Object::MergeWith(PtrWeak<Structured> j, int start_offset) {

		if (j->is_object()) {
			auto* x = dynamic_cast<Object*>(j);

			uint64_t len = j->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (j->get_value_list(i).is_structured()) {
					j->get_value_list(i).as_structured_ptr()->set_parent(this);
				}
			}

			if (x->obj_key_vec.empty() == false) {
				obj_key_vec.insert(obj_key_vec.end(), std::make_move_iterator(x->obj_key_vec.begin()) + start_offset,
					std::make_move_iterator(x->obj_key_vec.end()));
				obj_val_vec.insert(obj_val_vec.end(), std::make_move_iterator(x->obj_val_vec.begin()) + start_offset,
					std::make_move_iterator(x->obj_val_vec.end()));
			}
			else {
				log << info << "test1";
			}
		}
		else if (j->is_partial_json()) {
			auto* x = dynamic_cast<PartialJson*>(j);

			if (x->arr_vec.empty() == false) { // not object?
				ERROR("partial json is not object");
			}

			uint64_t len = j->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (j->get_value_list(i).is_structured()) {
					j->get_value_list(i).as_structured_ptr()->set_parent(this);
				}
			}

			if (x->virtualJson.is_structured()) {
				start_offset = 0;
			}

			if (x->obj_key_vec.empty() == false) {
				obj_key_vec.insert(obj_key_vec.end(), std::make_move_iterator(x->obj_key_vec.begin()) + start_offset,
					std::make_move_iterator(x->obj_key_vec.end()));
				obj_val_vec.insert(obj_val_vec.end(), std::make_move_iterator(x->obj_val_vec.begin()) + start_offset,
					std::make_move_iterator(x->obj_val_vec.end()));
			}
			else {
				log << info << "test2";
			}
		}
		else {
			ERROR("Object::MergeWith Error");
		}
	}

	claujson_inline void Object::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t key_token_idx, uint64_t val_token_idx) {

			{
				Value temp;// key
				Value temp2;

				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, key_token_idx, e);

				if (e) {
					ERROR("Error in add_item_type");
				}
				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, val_token_idx, e);
				if (e) {
					ERROR("Error in add_item_type");
				}

				if (temp.type() != ValueType::STRING) {
					ERROR("Error in add_item_type, key is not string");
				}

				obj_key_vec.push_back(std::move(temp));
				obj_val_vec.push_back(std::move(temp2));
			}
	}

	void Object::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t val_token_idx) {
		// error

		log << warn << "errr..";
		ERROR("Error Object::add_item_type");
	}

	void Object::add_user_type(ValueType type) {
		// error

		log << warn << "errr..";
		ERROR("Error Object::add_user_type");
		return;
	}

	bool Object::add_user_type(Ptr<Structured> j) {
		if (j->is_virtual()) {
			j->set_parent(this);

			obj_key_vec.push_back(Value());
			obj_val_vec.emplace_back(j.release());
			return true;
		}

		return false;
	}

	bool Object::add_user_type(Value key, Ptr<Structured> j) {

		if (key.is_str() || j->is_virtual()) {
			j->set_parent(this);

			obj_key_vec.push_back(std::move(key));
			obj_val_vec.emplace_back(j.release());
		}
		else {
			log << warn << "chk..";
			return false;
		}

		return true;
	}

	Structured* Array::clone() const {
		Structured* result = new Array();

		uint64_t sz = this->get_data_size();
		for (uint64_t i = 0; i < sz; ++i) {
			result->add_array_element(this->get_value_list(i).clone());
		}

		return result;
	}

	Value Array::Make() {
		return Value(new Array());
	}

	Array::Array() { }

	Array::~Array() {
		for (auto& x : arr_vec) {
			if (x.is_structured()) {
				delete x.as_structured_ptr();
			}
		}
	}

	bool Array::is_object() const {
		return false;
	}
	bool Array::is_array() const {
		return true;
	}

	claujson_inline uint64_t Array::get_data_size() const {
		return arr_vec.size();
	}

	Value& Array::get_value_list(uint64_t idx) {
		return arr_vec[idx];
	}

	Value& Array::get_key_list(uint64_t idx) {
		return data_null;
	}

	const Value& Array::get_const_key_list(uint64_t idx) {
		return data_null;
	}

	const Value& Array::get_const_key_list(uint64_t idx) const {
		return data_null;
	}


	const Value& Array::get_value_list(uint64_t idx) const {
		return arr_vec[idx];
	}

	const Value& Array::get_key_list(uint64_t idx) const {
		return data_null;
	}

	void Array::clear(uint64_t idx) {
		arr_vec[idx].clear(false);
	}

	bool Array::is_virtual() const {
		return false;
	}
	void Array::clear() {
		arr_vec.clear();
	}

	void Array::reserve_data_list(uint64_t len) {
		arr_vec.reserve(len);
	}


	std::vector<Value>::iterator Array::begin() {
		return arr_vec.begin();
	}

	std::vector<Value>::iterator Array::end() {
		return arr_vec.end();
	}

	bool Array::add_object_element(Value key, Value val) {
		return false;
	}

	bool Array::add_array_element(Value val) {

		if (val.is_structured()) {
			val.as_structured_ptr()->set_parent(this);
		}

		arr_vec.push_back(std::move(val));

		return true;
	}

	bool Array::add_array(Ptr<Structured> arr) {
		arr->set_parent(this);
		arr_vec.push_back(Value(arr.release()));

		return true;
	}
	bool Array::add_array(Value key, Ptr<Structured> arr) {
		return false;
	}

	bool Array::add_object(Ptr<Structured> obj) {
		obj->set_parent(this);
		arr_vec.push_back(Value(obj.release()));
		return true;
	}
	bool Array::add_object(Value key, Ptr<Structured> obj) {
		return false;
	}
	bool Array::assign_value_element(uint64_t idx, Value val) {

		arr_vec[idx] = std::move(val);

		return true;
	}

	bool Array::assign_key_element(uint64_t idx, Value key) {
		return false;
	}

	void Array::erase(StringView key, bool real) {
		uint64_t idx = this->find(key);
		erase(idx, real);
	}
#if __cplusplus >= 202002L
	void Array::erase(std::u8string_view key, bool real) {
		uint64_t idx = this->find(key);
		erase(idx, real);
	}
#endif

	void Array::erase(uint64_t idx, bool real) {

		if (real) {
			clean(arr_vec[idx]);
		}

		arr_vec.erase(arr_vec.begin() + idx);
	}

	void Array::MergeWith(PtrWeak<Structured> j, int start_offset) {

		if (j->is_array()) {
			auto* x = dynamic_cast<Array*>(j);

			uint64_t len = j->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (j->get_value_list(i).is_structured()) {
					j->get_value_list(i).as_structured_ptr()->set_parent(this);
				}
			}

			if (x->arr_vec.empty() == false) {
				arr_vec.insert(arr_vec.end(), std::make_move_iterator(x->arr_vec.begin()) + start_offset,
					std::make_move_iterator(x->arr_vec.end()));
			}
			else {

				log << info << "test3";
			}
		}
		else if (j->is_partial_json()) {
			auto* x = dynamic_cast<PartialJson*>(j);

			if (x->obj_key_vec.empty() == false) { // not object?
				ERROR("partial json is not array");
			}

			uint64_t len = j->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (j->get_value_list(i).is_structured()) {
					j->get_value_list(i).as_structured_ptr()->set_parent(this);
				}
			}


			if (x->virtualJson.is_structured()) {
				start_offset = 0;
			}

			if (x->arr_vec.empty() == false) {
				arr_vec.insert(arr_vec.end(), std::make_move_iterator(x->arr_vec.begin()) + start_offset,
					std::make_move_iterator(x->arr_vec.end()));
			}
			else {

				log << info << "test4";
			}
		}
		else {
			ERROR("Array::MergeWith Error");
		}
	}


	void Array::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t key_token_idx, uint64_t val_token_idx) {

		// error
		log << warn << "error..";
		ERROR("Error Array::add_item_type");
	}

	claujson_inline void Array::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t val_token_idx) {

			{
				Value temp2;
				bool e = false;
				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, val_token_idx, e);
				if (e) {

					ERROR("Error in add_item_type");
				}
				arr_vec.push_back(std::move(temp2));
			}
	}

	void Array::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		ValueType type, uint64_t key_token_idx) {
		log << warn << "error";
		ERROR("Array::add_user_type1");
	}

	bool Array::add_user_type(Ptr<Structured> j) {

		if (j->is_virtual()) {
			j->set_parent(this);
			arr_vec.emplace_back(j.release());
		}
		else {
			j->set_parent(this);
			arr_vec.emplace_back(j.release());
		}

		return true;
	}
	bool Array::add_user_type(Value key, Ptr<Structured> j) {
		return false;
	}

	// class PartialJson, only used in class LoadData2.
		// todo - rename? PartialNode ?

	PartialJson::~PartialJson() {
		for (auto& x : obj_val_vec) {
			if (x.is_structured()) {
				delete ((Structured*)x.as_structured_ptr());
			}
		}

		for (auto& x : arr_vec) {
			if (x.is_structured()) {
				delete ((Structured*)x.as_structured_ptr());
			}
		}

		if (virtualJson.is_structured()) {
			delete ((Structured*)virtualJson.as_structured_ptr());
		}
	}

	PartialJson::PartialJson() {

	}

	bool PartialJson::is_partial_json() const { return true; }

	bool PartialJson::is_object() const {
		return false;
	}
	bool PartialJson::is_array() const {
		return false;
	}

	claujson_inline uint64_t PartialJson::get_data_size() const {
		int count = 0;

		if (virtualJson.is_structured()) {
			count = 1;
		}

		return arr_vec.size() + obj_val_vec.size() + count;
	}

	Value& PartialJson::get_value_list(uint64_t idx) {

		if (virtualJson.is_structured() && idx == 0) {
			return virtualJson;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return arr_vec[idx];
		}
		else {
			return obj_val_vec[idx];
		}
	}


	Value& PartialJson::get_key_list(uint64_t idx) {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_key_vec[idx];
		}
	}

	const Value& PartialJson::get_const_key_list(uint64_t idx) {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_key_vec[idx];
		}
	}
	const Value& PartialJson::get_const_key_list(uint64_t idx) const {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_key_vec[idx];
		}
	}
	const Value& PartialJson::get_value_list(uint64_t idx) const {
		if (virtualJson.is_structured() && idx == 0) {
			return virtualJson;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return arr_vec[idx];
		}
		else {
			return obj_val_vec[idx];
		}
	}


	const Value& PartialJson::get_key_list(uint64_t idx) const {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_key_vec[idx];
		}
	}


	void PartialJson::clear(uint64_t idx) { // use carefully..
		if (virtualJson.is_structured() && idx == 0) {
			virtualJson.clear(false);
			return;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}
		if (!arr_vec.empty()) {
			arr_vec[idx].clear(false);
		}
		else {
			obj_val_vec[idx].clear(false);
		}
	}

	bool PartialJson::is_virtual() const {
		return false;
	}

	void PartialJson::clear() {
		arr_vec.clear();
		obj_key_vec.clear();
		obj_val_vec.clear();
		virtualJson.clear(false);
	}

	void PartialJson::reserve_data_list(uint64_t len) {
		if (!arr_vec.empty()) {
			arr_vec.reserve(len);
		}
		if (!obj_val_vec.empty()) {
			obj_val_vec.reserve(len);
			obj_key_vec.reserve(len);
		}
	}



	bool PartialJson::add_object_element(Value key, Value val) {
		if (!key.is_str()) {
			return false;
		}
		if (!arr_vec.empty()) {
			ERROR("partialJson is array or object.");
			return false;
		}

		if (val.is_structured()) {
			auto* x = (Structured*)val.as_structured_ptr();
			x->set_parent(this);
		}

		obj_key_vec.push_back(std::move(key));
		obj_val_vec.push_back(std::move(val));

		return true;
	}
	bool PartialJson::add_array_element(Value val) {
		if (!obj_key_vec.empty()) {
			ERROR("partialJson is array or object.");
			return false;
		}

		if (val.is_structured()) {
			val.as_structured_ptr()->set_parent(this);
		}

		arr_vec.push_back(std::move(val));

		return true;
	}


	bool PartialJson::add_array(Ptr<Structured> arr) {
		log << warn << "not used..";
		ERROR("NOT USED");
		return false;
	}
	bool PartialJson::add_object(Ptr<Structured> obj) {
		log << warn << "not used..";
		ERROR("NOT USED");
		return false;
	}

	bool PartialJson::add_array(Value key, Ptr<Structured> arr) {
		log << warn << "not used..";
		ERROR("NOT USED");
		return false;
	}
	bool PartialJson::add_object(Value key, Ptr<Structured> obj) {
		log << warn << "not used..";
		ERROR("NOT USED");
		return false;
	}

	bool PartialJson::assign_value_element(uint64_t idx, Value val) {
		log << warn << "not used..";
		ERROR("NOT USED");
		return false;
	}

	bool PartialJson::assign_key_element(uint64_t idx, Value key) {
		log << warn << "not used..";
		ERROR("NOT USED");
		return false;
	}

	void PartialJson::erase(StringView key, bool real) {
		log << warn << "not used..";
		ERROR("NOT USED");
	}
#if __cplusplus >= 202002L
	void PartialJson::erase(std::u8string_view key, bool real) {
		log << warn << "not used..";
		ERROR("NOT USED");
	}
#endif
	void PartialJson::erase(uint64_t idx, bool real) {
		log << warn << "not used..";
		ERROR("NOT USED");
	}

	void PartialJson::MergeWith(PtrWeak<Structured> j, int start_offset) {

		if (j->is_array()) {
			auto* x = dynamic_cast<Array*>(j);

			uint64_t len = j->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (j->get_value_list(i).is_structured()) {
					j->get_value_list(i).as_structured_ptr()->set_parent(this);
				}
			}
			if (x->arr_vec.empty() == false) {
				arr_vec.insert(arr_vec.end(), std::make_move_iterator(x->arr_vec.begin()) + start_offset,
					std::make_move_iterator(x->arr_vec.end()));
			}
			else {

				log << info << "test5";
			}
		}
		else if (j->is_partial_json()) {
			auto* x = dynamic_cast<PartialJson*>(j);

			uint64_t len = j->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (j->get_value_list(i).is_structured()) {
					j->get_value_list(i).as_structured_ptr()->set_parent(this);
				}
			}

			if (x->virtualJson.is_structured()) {
				start_offset = 0;
			}

			if (x->arr_vec.empty() == false) {
				arr_vec.insert(arr_vec.end(), std::make_move_iterator(x->arr_vec.begin()) + start_offset,
					std::make_move_iterator(x->arr_vec.end()));
			}
			else {

				log << info << "test6";
			}
		}
		else {
			ERROR("PartialJson::MergeWith Error");
		}
	}

	void PartialJson::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t key_token_idx, uint64_t val_token_idx) {

			{
				Value temp;
				Value temp2;

				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, key_token_idx, e);

				if (e) {
					ERROR("Error in add_item_type");
				}

				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, val_token_idx, e);

				if (e) {
					ERROR("Error in add_item_type");
				}

				if (temp.type() != ValueType::STRING) {
					ERROR("Error in add_item_type, key is not string");
				}


				if (!arr_vec.empty()) {
					ERROR("partialJson is array or object.");
				}

				obj_key_vec.push_back(std::move(temp));
				obj_val_vec.push_back(std::move(temp2));
			}
	}

	void PartialJson::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t val_token_idx) {

			{
				Value temp2;
				bool e = false;

				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, val_token_idx, e);

				if (e) {

					ERROR("Error in add_item_type");
				}

				if (!obj_key_vec.empty()) {
					ERROR("partialJson is array or object.");
				}

				arr_vec.push_back(std::move(temp2));
			}
	}

	bool PartialJson::add_user_type(Ptr<Structured> j) {

		j->set_parent(this);

		if (j->is_virtual()) {
			virtualJson = Value(j.release());
		}
		else {
			if (!obj_key_vec.empty()) {
				return false; //ERROR("partialJson is array or object.");
			}

			arr_vec.emplace_back(j.release());
		}

		return true;
	}

	bool PartialJson::add_user_type(Value key, Ptr<Structured> j) {

		j->set_parent(this);

		{

			if (!arr_vec.empty()) {
				return false; //ERROR("partialJson is array or object.");
			}

			if (key.is_str()) {
				obj_key_vec.push_back(std::move(key));
				obj_val_vec.emplace_back(j.release());
			}
			else {
				log << warn << "ERROR";
				return false; //ERROR("PartialJson::add_user_type");
			}
		}

		return true;

	}


	bool VirtualObject::is_virtual() const {
		return true;
	}

	VirtualObject::~VirtualObject() {
		//
	}

	bool VirtualArray::is_virtual() const {
		return true;
	}

	VirtualArray::~VirtualArray() {
		//
	}


	claujson_inline void Object::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		ValueType type, uint64_t key_token_idx) {

			{
				Value temp;
				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, key_token_idx, e);
				if (e) {
					ERROR("Error in add_user_type");
				}

				if (temp.type() != ValueType::STRING) {
					ERROR("Error in add_item_type, key is not string");
				}

				Structured* json = nullptr;

				if (type == ValueType::OBJECT) {
					json = new Object();
				}
				else if (type == ValueType::ARRAY) {
					json = new Array();
				}

				obj_key_vec.push_back(std::move(temp));
				obj_val_vec.push_back(Value(json));

				json->set_parent(this);
			}
	}
	claujson_inline void Array::add_user_type(ValueType type) {

		{
			Structured* json = nullptr;

			if (type == ValueType::OBJECT) {
				json = new Object();
			}
			else if (type == ValueType::ARRAY) {
				json = new Array();
			}


			arr_vec.push_back(Value(json));


			json->set_parent(this);
		}
	}

	claujson_inline void PartialJson::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		ValueType type, uint64_t key_token_idx) {
			{
				if (!arr_vec.empty()) {
					ERROR("partialJson is array or object.");
				}

				Value temp;
				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, key_token_idx, e);

				if (e) {
					ERROR("Error in add_user_type");
				}

				if (temp.type() != ValueType::STRING) {
					ERROR("Error in add_item_type, key is not string");
				}


				Structured* json = nullptr;

				if (type == ValueType::OBJECT) {
					json = new Object();
				}
				else if (type == ValueType::ARRAY) {
					json = new Array();
				}



				obj_key_vec.push_back(std::move(temp));
				obj_val_vec.push_back(Value(json));

				json->set_parent(this);
			}
	}
	claujson_inline void PartialJson::add_user_type(ValueType type) {
		{
			if (!obj_key_vec.empty()) {
				ERROR("PartialJson is array or object.");
			}

			Structured* json = nullptr;

			if (type == ValueType::OBJECT) {
				json = new Object();
			}
			else if (type == ValueType::ARRAY) {
				json = new Array();
			}

			arr_vec.push_back(Value(json));

			json->set_parent(this);
		}
	}

	Array* Value::as_array() {
		if (is_structured() && as_structured_ptr()->is_array()) {
			return static_cast<Array*>(_array_or_object_ptr);
		}
		return nullptr;
	}

	Object* Value::as_object() {
		if (is_structured() && as_structured_ptr()->is_object()) {
			return static_cast<Object*>(_array_or_object_ptr);
		}
		return nullptr;
	}


	const Array* Value::as_array() const {
		if (is_structured() && as_structured_ptr()->is_array()) {
			return static_cast<Array*>(_array_or_object_ptr);
		}
		return nullptr;
	}

	const Object* Value::as_object() const {
		if (is_structured() && as_structured_ptr()->is_object()) {
			return static_cast<Object*>(_array_or_object_ptr);
		}
		return nullptr;
	}

	Structured* Value::as_structured_ptr() {
		if (!is_structured()) {
			return nullptr;
		}
		return (_array_or_object_ptr);
	}

	const Structured* Value::as_structured_ptr() const {
		if (!is_structured()) {
			return nullptr;
		}
		return (_array_or_object_ptr);
	}

	Value& Value::operator[](uint64_t idx) {
		static Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->operator[](idx);
		}

		return empty_value;
	}

	const Value& Value::operator[](uint64_t idx) const {
		static const Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->operator[](idx);
		}

		return empty_value;
	}

	Value& Value::at(StringView x) {
		static Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->at(x);
		}

		return empty_value;
	}

	const Value& Value::at(StringView x) const {
		static const Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->at(x);
		}

		return empty_value;
	}
#if __cplusplus >= 202002L
	Value& Value::at(std::u8string_view x) {
		static Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->at(x);
		}

		return empty_value;
	}
	const Value& Value::at(std::u8string_view x) const {
		static const Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->at(x);
		}

		return empty_value;
	}
#endif

	uint64_t Value::find(StringView key) const // find with key`s convert.
	{

		if (is_structured()) {
			return as_structured_ptr()->find(key);
		}

		return Structured::npos;
	}

	uint64_t Value::find_(StringView key) const { // find without key`s converting?
		if (is_structured()) {
			return as_structured_ptr()->find_(key);
		}

		return Structured::npos;
	}
#if __cplusplus >= 202002L

	uint64_t Value::find(std::u8string_view key) const {
		return find(StringView(reinterpret_cast<const char*>(key.data(), key.size())));
	}

	uint64_t Value::find_(std::u8string_view key) const { // find without key`s converting?
		return find(StringView(reinterpret_cast<const char*>(key.data(), key.size())));
	}
	// without covnert
	Value& Value::operator[](std::u8string_view key) { // if not exist key, then nothing.
		return operator[](StringView(reinterpret_cast<const char*>(key.data(), key.size())));
	}
	const Value& Value::operator[](std::u8string_view key) const { // if not exist key, then nothing.
		return operator[](StringView(reinterpret_cast<const char*>(key.data(), key.size())));
	}
#endif

	// at vs [] (at <- convert key to key_in_json.) ([] <- do not convert.)
	Value& Value::operator[](StringView key) { // if not exist key, then nothing.
		static Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->operator[](key);
		}

		return empty_value;
	}
	const Value& Value::operator[](StringView key) const { // if not exist key, then nothing.
		static Value empty_value(nullptr, false);

		if (is_structured()) {
			return as_structured_ptr()->operator[](key);
		}

		return empty_value;
	}
	// todo - as_array, as_object.
// as_array
// if (valid && ((Json*)_array_or_object_ptr)->is_array()) { return ~~ } return 

	unsigned char __type_arr[256] = {
59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 34  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 44  , 100  , 59  , 59  , 100  , 100
, 100  , 100  , 100  , 100  , 100  , 100  , 100  , 100  , 58  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 91  , 59  , 93  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 102  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 110  , 59  , 59  , 59  , 59  , 59  , 116  , 59  , 59  , 59
, 59  , 59  , 59  , 123  , 59  , 125  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59  , 59
, 59  , 59  , 59  , 59  , 59  , 59
	};

	/*claujson_inline unsigned char __comma_chk_table[2][256] = {{0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 1  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 1
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 1  , 0  , 0  , 0  , 0  , 0  , 0
, 1  , 0  , 1  , 0  , 0  , 0  , 0  , 0  , 1  , 0
, 1  , 0  , 0  , 0  , 0  , 0  , 1  , 1  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 1  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
, 0  , 0  , 0  , 0  , 0  , 0  } ,
{ 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1
 , 1  , 1  , 1  , 1  , 1  , 1  } }; */

 // not exact type! for int, uint, float. ( int, uint, float -> float )
	claujson_inline _simdjson::internal::tape_type get_type(unsigned char x) {
		
		return _simdjson::internal::tape_type(x);
		
		//return (_simdjson::internal::tape_type)__type_arr[x]; // more fast version..
		/*
		switch (x) {
		case '-':
		case '0':
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return _simdjson::internal::tape_type::DOUBLE; // number?
			break;
		case '"':
		case 't':
		case 'f':
		case 'n':
		case '{':
		case '[':
		case '}':
		case ']':
			return	(_simdjson::internal::tape_type)(x);
			break;
		case ':':
		case ',':

			return	(_simdjson::internal::tape_type)(x);
			break;
		}
		return _simdjson::internal::tape_type::NONE;*/
	}

	class LoadData;

	class LoadData2 {
	public:

		friend class LoadData;

		static uint64_t Size(Structured* root) {
			return _Size(root) + 1;
		}

		static uint64_t _Size(Structured* root) {
			if (root == nullptr) {
				return 0;
			}
			

			uint64_t len = root->get_data_size();
			uint64_t x = 0;

			for (uint64_t i = 0; i < len; ++i) {
				auto* ptr = root->get_value_list(i).as_structured_ptr();
				if (ptr) {
					x += Size(ptr);
				}
			}

			return x;

		}

		// find n node.. , need rename..
		static void Find2(Structured* root, const uint64_t n, uint64_t& idx, bool chk_hint, uint64_t& _len, std::vector<uint64_t>& offset, std::vector<uint64_t>& offset2, std::vector<Structured*>& out, std::vector<int>& hint) {
			if (idx >= n) {
				return;
			}

			offset[idx]--; // intial offset must >= 1

			if (offset[idx] == 0) {

				if (!out[idx]) {
					out[idx] = root;

					if (chk_hint) {
						hint[idx] = 1;
					}

					++idx;
					if (idx >= n) {
						return;
					}

					uint64_t sz = Size(out[idx - 1]);

					if (_len < offset2[idx - 1] + sz - 1) {
						return;
					}

					_len = _len - (offset2[idx - 1] + sz - 1); // chk...

					if (_len <= 0) {
						return;
					}

					for (uint64_t k = idx; k < n; ++k) {
						offset[k] = _len / (n - idx + 1);
					}
					offset[n - 1] = _len - _len / (n - idx + 1) * (n - idx);

					for (uint64_t k = idx; k < n; ++k) {
						offset2[k] = offset[k];
					}
				}

				return;
			}

			uint64_t len = root->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (root->get_value_list(i).is_structured()) {

					Find2(root->get_value_list(i).as_structured_ptr(), n, idx, i < len - 1, _len, offset, offset2, out, hint);

					if (idx >= n) {
						return;
					}
				}
			}
		}


		static void Divide(Structured* pos, Structured*& result) { // after pos.. -> to result.
			if (pos == nullptr) {
				return;
			}


			Structured* pos_ = pos;
			Structured* parent = pos_->get_parent();

			Structured* out = new PartialJson(); //


			while (parent && parent->is_partial_json() == false) {
				long long idx = 0;
				uint64_t len = parent->get_data_size();
				for (uint64_t i = 0; i < len; ++i) {
					if (parent->get_value_list(i).is_structured() && parent->get_value_list(i).as_structured_ptr() == pos_) {
						idx = i;
						break;
					}
				}

				for (uint64_t i = idx + 1; i < len; ++i) {
					if (parent->get_value_list(i).is_structured()) {
						if (parent->is_array()) {
							out->add_user_type(Ptr<Structured>(parent->get_value_list(i).as_structured_ptr()));
						}
						else {
							out->add_user_type(std::move(parent->get_key_list(i)), Ptr<Structured>(parent->get_value_list(i).as_structured_ptr()));
						}
					}

					else {
						if (parent->get_key_list(i).is_str() == false) {
							out->add_array_element(std::move(parent->get_value_list(i)));
						}
						else { // out->is_object
							out->add_object_element(std::move(parent->get_key_list(i)), std::move(parent->get_value_list(i)));
						}
					}
				}

				{
					Structured* temp = nullptr;
					if (parent->is_object()) {
						temp = new VirtualObject();
					}
					else if (parent->is_array()) { // parent->is_array()
						temp = new VirtualArray();
					}
					else { // PartialJson
						// none
					}

					uint64_t len = out->get_data_size();

					for (uint64_t i = 0; i < len; ++i) {
						if (out->get_value_list(i).is_structured()) {

							if (temp->is_array()) {
								temp->add_user_type(Ptr<Structured>((Structured*)out->get_value_list(i).as_structured_ptr()));
							}
							else {
								if (out->get_value_list(i).as_structured_ptr()->is_virtual()) {
									temp->add_user_type(Value(), Ptr<Structured>((Structured*)out->get_value_list(i).as_structured_ptr()));
								}
								else {
									temp->add_user_type(std::move(out->get_key_list(i)), Ptr<Structured>((Structured*)out->get_value_list(i).as_structured_ptr()));
								}
							}
						}
						else {
							if (temp->is_object()) {
								temp->add_object_element(std::move(out->get_key_list(i)),
									std::move(out->get_value_list(i)));
							}
							else {
								temp->add_array_element(std::move(out->get_value_list(i)));
							}
						}
					}

					out->clear();
					out->add_user_type(Ptr<Structured>(temp));
				}

				for (long long i = (long long)parent->get_data_size() - 1; i > idx; --i) {
					parent->erase(i);
				}

				pos_ = parent;
				parent = parent->get_parent();
			}

			result = out;
		}

		static std::vector<claujson::Structured*> Divide2(uint64_t n, claujson::Value& j, std::vector<claujson::Structured*>& result, std::vector<int>& hint) {
			if (j.is_structured() == false) {
				return { nullptr };
			}

			if (n == 0) {
				return { nullptr };
			}
			auto a = std::chrono::steady_clock::now();
			uint64_t len = claujson::LoadData2::Size(j.as_structured_ptr());
			auto b= std::chrono::steady_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a); 
			log << info << "size is " << dur.count() << "ms\n";
			if (len / n == 0) {
				return { nullptr };
			}

			std::vector<uint64_t> offset(n - 1, 0);

			for (uint64_t i = 0; i < offset.size(); ++i) {
				offset[i] = len / n;
			}
			offset.back() = len - len / n * (n - 1);

			hint = std::vector<int>(n - 1, 0);

			std::vector<claujson::Structured*> pos(n, nullptr);

			a = std::chrono::steady_clock::now();
			{
				uint64_t idx = 0;
				auto offset2 = offset;

				claujson::LoadData2::Find2(j.as_structured_ptr(), n - 1, idx, false, len, offset, offset2, pos, hint);
			}
			b = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
			log << info << "Find2 is " << dur.count() << "ms\n";

			for (uint64_t i = 0; i < n - 1; ++i) {
				if (!pos[i]) {
					return { nullptr };
				}
			}


			std::vector<Structured*> temp_parent(n, nullptr);
			{
				uint64_t i = 0;
				for (; i < n - 1; ++i) {

					if (i > 0 && temp_parent[i - 1] == nullptr) {
						for (uint64_t j = 0; j < i; ++j) {
							int op = 0;
							
							claujson::LoadData2::Merge2(temp_parent[j], result[j], &temp_parent[j + 1], op);

						}

						for (uint64_t j = 0; j < result.size(); ++j) {
							if (result[i]) {
								delete result[i];
								result[i] = nullptr;
							}
						}

						return { nullptr };
					}

					claujson::LoadData2::Divide(pos[i], result[i]);

					temp_parent[i] = pos[i]->get_parent();

				}

				if (i > 0 && temp_parent[i - 1] == nullptr) {
					for (uint64_t j = 0; j < i; ++j) {
						int op = 0;
						
						claujson::LoadData2::Merge2(temp_parent[j], result[j], &temp_parent[j + 1], op);
					}

					for (uint64_t j = 0; j < result.size(); ++j) {
						if (result[i]) {
							delete result[i];
							result[i] = nullptr;
						}
					}

					return { nullptr };
				}
			}


			return pos;
		}

		static int Merge(Structured* next, Structured* ut, Structured** ut_next)
		{

			// check!!
			while (ut->get_data_size() >= 1
				&& ut->get_value_list(0).is_structured() && ((Structured*)ut->get_value_list(0).as_structured_ptr())->is_virtual())
			{
				ut = (Structured*)ut->get_value_list(0).as_structured_ptr();
			}

			while (true) {

				class Structured* _ut = ut;
				class Structured* _next = next;

				//log << warn  << "chk\n";
				if (ut_next && _ut == *ut_next) { // chk_next_ut
					*ut_next = _next;

					//log << info << "chked in merge...\n"; // special case!
				}

				if (_next->is_array() && _ut->is_object()) {
					ERROR("Error in Merge, next is array but child? is object");
				}
				if (_next->is_object() && _ut->is_array()) {
					ERROR("Error in Merge, next is object but child? is array");
				}

				int start_offset = 0;
				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_structured() && _ut->get_value_list(0).as_structured_ptr()->is_virtual()) {
					++start_offset;
				}

				_next->MergeWith(_ut, start_offset);

				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_structured() && _ut->get_value_list(0).as_structured_ptr()->is_virtual()) {
					clean(_ut->get_value_list(0));
				}

				_ut->clear();

				ut = ut->get_parent();
				next = next->get_parent();


				if (next && ut) {
					//
				}
				else {
					// right_depth > left_depth
					if (!next && ut) {
						return -1;
					}
					else if (next && !ut) {
						return 1;
					}

					return 0;
				}
			}
		}

		static int Merge2(Structured* next, Structured* ut, Structured** ut_next, int& op)
		{

			if (!ut) {
				*ut_next = next;
				return 0;
			}

			// check!!
			while (ut->get_data_size() >= 1
				&& ut->get_value_list(0).is_structured() && ((Structured*)ut->get_value_list(0).as_structured_ptr())->is_virtual())
			{
				ut = (Structured*)ut->get_value_list(0).as_structured_ptr();
			}


			while (true) {
				//log << warn  << "chk\n";

				class Structured* _ut = ut;
				class Structured* _next = next;

				if (_next->is_array() && _ut->is_object()) {
					ERROR("Error in Merge, next is array but child? is object");
				}
				if (_next->is_object() && _ut->is_array()) {
					ERROR("Error in Merge, next is object but child? is array");
				}

				if (ut_next && _ut == (*ut_next)) {
					*ut_next = _next;
					log << info << "chked in merge2...\n";
				}

				int start_offset = 0;
				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_structured() && _ut->get_value_list(0).as_structured_ptr()->is_virtual()) {
					++start_offset;
				}

				_next->MergeWith(_ut, start_offset);

				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_structured() && _ut->get_value_list(0).as_structured_ptr()->is_virtual()) {
					clean(_ut->get_value_list(0));
				}

				_ut->clear();

				ut = ut->get_parent();
				next = next->get_parent();


				if (next && ut) {
					//
				}
				else {
					// right_depth > left_depth
					if (!next && ut) {
						return -1;
					}
					else if (next && !ut) {
						return 1;
					}

					return 0;
				}
			}
		}


		struct TokenTemp { // need to rename.
			// 
			int64_t buf_idx;  // buf_idx?
			int64_t next_buf_idx; // next_buf_idx?
			//Json
			uint64_t token_idx; // token_idx?
			//
			bool is_key = false;
		};

		static bool __LoadData(char* buf, uint64_t buf_len,
			_simdjson::internal::dom_parser_implementation* imple,
			int64_t token_arr_start, uint64_t token_arr_len, Ptr<Structured>& _global,
			int start_state, int last_state, // this line : now not used..
			class Structured** next, uint64_t* count_vec, int* err, uint64_t no)
		{
			try {
				if (token_arr_len <= 0) {
					*next = nullptr;
					return false;
				}

				// token_arr_len >= 1


				class Structured* global = _global.get();

				uint64_t braceNum = 0;

				Structured* nowUT = global; // use get_parent(), not std::vector<Structured*>

				TokenTemp key; int state = 0;

				for (uint64_t i = 0; i < token_arr_len; ++i) {

					const _simdjson::internal::tape_type type = get_type(buf[imple->structural_indexes[token_arr_start + i]]);

					if (type == _simdjson::internal::tape_type::COMMA) {
						continue;
					}

					// Left 1
					if (type == _simdjson::internal::tape_type::START_OBJECT ||
						type == _simdjson::internal::tape_type::START_ARRAY) { // object start, array start


						state = 0;

						if (key.is_key) {
							nowUT->add_user_type(key.buf_idx, key.next_buf_idx, buf,
								type == _simdjson::internal::tape_type::START_OBJECT ? ValueType::OBJECT : ValueType::ARRAY, key.token_idx); // object vs array
							key.is_key = false;
						}
						else {
							nowUT->add_user_type(type == _simdjson::internal::tape_type::START_OBJECT ? ValueType::OBJECT : ValueType::ARRAY);
						}


						class Structured* pTemp = (Structured*)nowUT->get_value_list(nowUT->get_data_size() - 1).as_structured_ptr();

						braceNum++;

						/// initial new nestedUT.
						nowUT = pTemp;

						
						if (count_vec[token_arr_start + i] > 0) {
							nowUT->reserve_data_list(nowUT->get_data_size() + count_vec[token_arr_start + i]);
						}
						else {
							log << info << "chk";
						}
					}
					// Right 2
					else if (type == _simdjson::internal::tape_type::END_OBJECT ||
						type == _simdjson::internal::tape_type::END_ARRAY) {

						state = 0;
						if (braceNum == 0) {

							Ptr<Structured> ut; // is v_array or v_object.

							if (type == _simdjson::internal::tape_type::END_OBJECT) {
								ut = Ptr<Structured>(new VirtualObject());
							}
							else {
								ut = Ptr<Structured>(new VirtualArray());
							}

							uint64_t len = nowUT->get_data_size();

							for (uint64_t i = 0; i < len; ++i) {
								if (nowUT->get_value_list(i).is_structured()) {
									if (ut->is_array()) {
										ut->add_user_type(Ptr<Structured>((Structured*)nowUT->get_value_list(i).as_structured_ptr()));
									}
									else { // ut->is_object()
										if (nowUT->is_virtual()) {
											ut->add_user_type(Value(), Ptr<Structured>((Structured*)nowUT->get_value_list(i).as_structured_ptr()));
										}
										else {
											ut->add_user_type(std::move(nowUT->get_key_list(i)), Ptr<Structured>((Structured*)nowUT->get_value_list(i).as_structured_ptr()));
										}
									}
								}
								else {
									if (ut->is_object()) {
										ut->add_object_element(std::move(nowUT->get_key_list(i)),
											std::move(nowUT->get_value_list(i)));
									}
									else {
										ut->add_array_element(std::move(nowUT->get_value_list(i)));
									}
								}
							}

							nowUT->clear();
							nowUT->add_user_type(std::move(ut)); // this nowUT is always PartialJson?
						}
						else {
							braceNum--;

							nowUT = nowUT->get_parent();
						}
					}
					else {

						bool is_key = token_arr_start + i + 1 < imple->n_structural_indexes && buf[imple->structural_indexes[token_arr_start + i + 1]] == ':';
									

						{
							TokenTemp data;

							data.buf_idx = imple->structural_indexes[token_arr_start + i];
							data.token_idx = token_arr_start + i;

							if (token_arr_start + i + 1 < imple->n_structural_indexes) {
								data.next_buf_idx = imple->structural_indexes[token_arr_start + i + 1];
							}
							else {
								data.next_buf_idx = buf_len;
							}


							if (is_key) {
								data.is_key = true;

								//if (token_arr_start + i + 2 < imple->n_structural_indexes) { // if valid string?
									key = std::move(data);
								//}
								++i; // pass key
							}
							else {

								if (state == 0 && braceNum == 0) {
									if (count_vec[token_arr_start + i] > 0) {
										nowUT->reserve_data_list(nowUT->get_data_size() + count_vec[token_arr_start + i]);
									}
									else {
										log << info << "chk..";
									}
									state = 1;
								}

								if (key.is_key) {
									nowUT->add_item_type(key.buf_idx, key.next_buf_idx, data.buf_idx, data.next_buf_idx, buf, key.token_idx, data.token_idx);
									key.is_key = false;
								}
								else {
									nowUT->add_item_type(data.buf_idx, data.next_buf_idx, buf, data.token_idx);
								}
							}
						}
					}

				}

				if (next) {
					*next = nowUT;
				}

				return true;
			}
			catch (const char* _err) {
				*err = -10;

				log << warn << _err << "\n";

				return false;
			}
			catch (...) {
				*err = -11;

				return false;
			}
		}

		static int64_t FindDivisionPlace(char* buf, _simdjson::internal::dom_parser_implementation* imple, int64_t start, int64_t last)
		{
			for (int64_t a = start; a <= last; ++a) {
				auto& x = imple->structural_indexes[a]; //  token_arr[a];
				const _simdjson::internal::tape_type type = (_simdjson::internal::tape_type)buf[x];
				//bool key = false;
				//bool next_is_valid = false;

				switch ((int)type) {
				case ',':
					return a ;
				default:
					break;
				}
			}
			return -1;
		}

		static bool _LoadData(Value& global, char* buf, uint64_t buf_len,

			_simdjson::internal::dom_parser_implementation* imple, int64_t& length,
			std::vector<int64_t>& start, uint64_t* count_vec, uint64_t parse_num) // first, strVec.empty() must be true!!
		{
			Ptr<Structured> _global = Ptr<Structured>(new PartialJson());
			std::vector<Ptr<Structured>> __global;

			try {
				{
					uint64_t pivot_num = parse_num;
					
					{ 
					std::set<int64_t> _pivots;
					std::vector<int64_t> pivots;
					//const int64_t num = token_arr_len; //

					if (pivot_num > 0) {
						std::vector<int64_t> pivot;
						pivots.reserve(pivot_num + 1);
						pivot.reserve(pivot_num);

						pivot.push_back(start[0]);

						for (int i = 1; i < parse_num; ++i) {
							pivot.push_back(FindDivisionPlace(buf, imple, start[i], start[i + 1] - 1));
						}

						for (uint64_t i = 0; i < pivot.size(); ++i) {
							if (pivot[i] != -1) {
								_pivots.insert(pivot[i]);
							}
						}

						for (auto& x : _pivots) {
							pivots.push_back(x);
						}

						pivots.push_back(length);
					}
					else {
						pivots.push_back(start[0]);
						pivots.push_back(length);
					}

					std::vector<class Structured*> next(pivots.size() - 1, nullptr);
					{

						__global = std::vector<Ptr<Structured>>(pivots.size() - 1);
						for (uint64_t i = 0; i < __global.size(); ++i) {
							__global[i] = Ptr<Structured>(new PartialJson());
						}

						std::vector<std::future<bool>> result(pivots.size() - 1);
						std::vector<int> err(pivots.size() - 1, 0);

						{
							int64_t idx = pivots[1] - pivots[0];
							int64_t _token_arr_len = idx;


							result[0] = pool->enqueue(__LoadData, (buf), buf_len, (imple), start[0], _token_arr_len, std::ref(__global[0]), 0, 0,
								&next[0], count_vec, &err[0], 0);
						}

						auto a = std::chrono::steady_clock::now();

						for (uint64_t i = 1; i < pivots.size() - 1; ++i) {
							int64_t _token_arr_len = pivots[i + 1] - pivots[i];

							result[i] = pool->enqueue(__LoadData, (buf), buf_len, (imple), pivots[i], _token_arr_len, std::ref(__global[i]), 0, 0,
								&next[i], count_vec, &err[i], i);

						}


						// wait
						for (uint64_t i = 0; i < result.size(); ++i) {
							result[i].get();
						}

						auto b = std::chrono::steady_clock::now();
						auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
						log << info << "parse1 " << dur.count() << "ms\n";

						// check..
						for (uint64_t i = 0; i < err.size(); ++i) {
							switch (err[i]) {
							case 0:
								break;
							case -10:
							case -11:
								return false;
								break;
							case -1:
							case -4:
								log << warn << "Syntax Error\n"; return false;
								break;
							case -2:
								log << warn << "error final state is not last_state!\n"; return false;
								break;
							case -3:
								log << warn << "error x > buffer + buffer_len:\n"; return false;
								break;
							default:
								log << warn << "unknown parser error\n"; return false;
								break;
							}
						}

						// Merge

						{
							int i = 0;
							std::vector<int> chk(parse_num, 0);
							auto x = next.begin();
							auto y = __global.begin();
							while (true) {
								if ((*y)->get_data_size() == 0) {
									chk[i] = 1;
								}

								++x;
								++y;
								++i;

								if (x == next.end()) {
									break;
								}
							}

							uint64_t start = 0;
							uint64_t last = pivots.size() - 1 - 1;

							for (uint64_t i = 0; i < pivots.size() - 1; ++i) {
								if (chk[i] == 0) {
									start = i;
									break;
								}
							}

							for (uint64_t i = pivots.size() - 1 - 1; i >= 0; --i) {
								if (chk[i] == 0) {
									last = i;
									break;
								}
							}

							if (__global[start]->get_data_size() > 0 && __global[start]->get_value_list(0).is_structured()
								&& ((Structured*)__global[start]->get_value_list(0).as_structured_ptr())->is_virtual()) {
								log << warn << "not valid file1\n";
								throw 1;
							}
							if (next[last] && next[last]->get_parent() != nullptr) {
								log << warn << "not valid file2\n";
								throw 2;
							}


							int err = Merge(_global.get(), __global[start].get(), &next[start]);
							if (-1 == err || (pivots.size() == 0 && 1 == err)) {
								log << warn << "not valid file3\n";
								throw 3;
							}

							for (uint64_t i = start + 1; i <= last; ++i) {

								if (chk[i]) {
									continue;
								}

								// linearly merge and error check...
								uint64_t before = i - 1;
								for (uint64_t k = i; k > 0; --k) {
									if (chk[k - 1] == 0) {
										before = k - 1;
										break;
									}
								}

								int err = Merge(next[before], __global[i].get(), &next[i]);

								if (-1 == err) {
									log << warn << "chk " << i << " " << __global.size() << "\n";
									log << warn << "not valid file4\n";
									throw 4;
								}
								else if (i == last && 1 == err) {
									log << warn << "n]ot valid file5\n";
									throw 5;
								}
							}
						}
						//catch (...) {
							//throw "in Merge, error";
						//	return false;
						//}
						//

						if (_global->get_data_size() > 1) { // bug fix..
							log << warn << "not valid file6\n";
							throw 6;
						}

						auto c = std::chrono::steady_clock::now();
						auto dur2 = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
						log << info << "parse2 " << dur2.count() << "ms\n";
					}
					}
					auto a = std::chrono::steady_clock::now();


					if (_global->get_value_list(0).is_structured()) {
						_global->get_value_list(0).as_structured_ptr()->set_parent(nullptr);
					}

					global = std::move(_global->get_value_list(0));


					auto b = std::chrono::steady_clock::now();
					auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

					log << info << "chk " << dur.count() << "ms\n";

					//	log << warn  << std::chrono::steady_clock::now() - a__ << "ms\n";
				}
				//	log << warn  << std::chrono::steady_clock::now() - a__ << "ms\n";
				return true;
			}
			catch (int err) {

				log << warn << "merge error " << err << "\n";
				//ERROR("Merge Error"sv);
				return false;
			}
			catch (const char* err) {

				log << warn << err << "\n";
				//ERROR("Merge Error"sv);

				return false;
			}
			//catch (...) {

			//	log << warn  << "internal error\n";
				//ERROR("Internal Error"sv);
			//	return false;
			//}

		}
		static bool parse(Value& global, char* buf, uint64_t buf_len,

			_simdjson::internal::dom_parser_implementation* imple,
			int64_t length, std::vector<int64_t>& start, uint64_t* count_vec, uint64_t thr_num) {

			return LoadData2::_LoadData(global, buf, buf_len, imple, length, start, count_vec, thr_num);
		}
	};

	
	// using rapidjson, for speed.
	class StrStream {
	private:
		rapidjson::StringBuffer m_buffer;
		rapidjson::Writer<rapidjson::StringBuffer> m_writer{m_buffer};

	public:

		const char* buf() const {
			return m_buffer.GetString();
		}
		uint64_t buf_size() const {
			return m_buffer.GetSize();
		}

		StrStream& add_1(const char* x, uint64_t sz) {
			m_writer.String(x, sz); // fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& add_char(char x) {
			m_buffer.Put(x);
			return *this;
		}

		StrStream& add_float(double x) {
			m_writer.Double(x); // fmt::format_to(std::back_inserter(out), "{:f}", x);
			return *this;
		}

		StrStream& add_int(int64_t x) {
			m_writer.Int64(x); // fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& add_uint(uint64_t x) {
			m_writer.Uint64(x); // fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& add_2(const char* str) {
			while (*str != '\0') {
				add_char(*str);
				++str;
			}

			return *this;
		}
	};


	class LoadData // Save? -> Write?
	{
	private:
		//                         
		static void _save(StrStream& stream, const Value& data, std::vector<Structured*>& chk_list, const int depth, bool pretty);
		static void _save(StrStream& stream, const Value& data, const int depth, bool pretty);

		static void save_(StrStream& stream, const Value& global, Structured* temp, bool pretty, bool hint);

	public:
		// test?... just Data has one element 
		static void save(const std::string& fileName, const Value& global, bool pretty, bool hint = false);

		static void save(std::ostream& stream, const Value& data, bool pretty);

		static std::string save_to_str(const Value& data, bool pretty);

		static void save_parallel(const std::string& fileName, Value& j, uint64_t thr_num, bool pretty);

	};

	// precated?
	claujson_inline void _write_string(StrStream& stream, char ch) {
		switch (ch) {
		case '\\':
			stream.add_2("\\\\");
			break;
		case '\"':
			stream.add_2("\\\"");
			break;
		case '\n':
			stream.add_2("\\n");
			break;
		case '\b':
			stream.add_2("\\b");
			break;
		case '\f':
			stream.add_2("\\f");
			break;
		case '\r':
			stream.add_2("\\r");
			break;
		case '\t':
			stream.add_2("\\t");
			break;
		default:
		{
			int code = ch;
			if ((code >= 0 && code < 0x20) || code == 0x7F)
			{
				char buf[] = "\\uDDDD";
				snprintf(buf + 2, 5, "%04X", code);
				stream.add_2(buf);
			}
			else {
				char buf[] = { ch, '\0' };
				stream.add_2(buf);
			}
		}
		}
	}

	claujson_inline void write_string(StrStream& stream, const std::string& str) {
		stream.add_1(str.data(), str.size());
	}

	static  const char* str_open_array[] = { "[", " [ \n",  };
	static   const  char* str_open_object[] = { "{", " { \n",  };
	static   const  char* str_close_array[] = { "]", " ] \n", };
	static   const  char* str_close_object[] = { "}", " } \n", };
	static   const  char* str_comma[] = { ",", " , " };
	static   const  char* str_colon[] = { ":", " : " };
	static   const  char* str_space[] = { "", " " };

	claujson_inline void save_primitive(StrStream& stream, const Value& x) {
		if (x.type() == ValueType::STRING) {

			write_string(stream, x.str_val());

		}
		else if (x.type() == ValueType::BOOL) {
			stream.add_2(x.bool_val() ? "true" : "false");
		}
		else if (x.type() == ValueType::FLOAT) {
			stream.add_float(x.float_val());
		}
		else if (x.type() == ValueType::INT) {
			stream.add_int(x.int_val());
		}
		else if (x.type() == ValueType::UINT) {
			stream.add_uint(x.uint_val());
		}
		else if (x.type() == ValueType::NULL_) {
			stream.add_2("null");
		}
	}
	std::string LoadData::save_to_str(const Value& global, bool pretty) {
		StrStream stream;

		if (global.is_structured()) {
			bool is_arr = global.as_structured_ptr()->is_array();

			if (is_arr) {
				stream.add_2(str_open_array[pretty ? 1 : 0]);
			}
			else {
				stream.add_2(str_open_object[pretty ? 1 : 0]);
			}

			_save(stream, global, 0, pretty);

			if (is_arr) {
				stream.add_2(str_close_array[pretty ? 1 : 0]);
			}
			else {
				stream.add_2(str_close_object[pretty ? 1 : 0]);
			}

		}
		else {
			auto& x = global;
			save_primitive(stream, x);
		}

		return std::string(stream.buf(), stream.buf_size());
	}

	//                           
	void LoadData::_save(StrStream& stream, const Value& data, std::vector<Structured*>& chk_list, const int depth, bool pretty) {
		const Structured* ut = nullptr;

		if (data.is_structured()) {
			ut = data.as_structured_ptr();
		}

		if (ut && ut->is_object()) {
			uint64_t len = ut->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_structured()) {
					auto& x = ut->get_key_list(i);

					if (x.type() == ValueType::STRING) {

						write_string(stream, x.str_val());

						{
							stream.add_2(str_colon[pretty ? 1 : 0]);
						}
					}
					else {
						//log << warn  << "Error : no key\n";
					}

					auto* y = ((Structured*)(ut->get_value_list(i).as_structured_ptr()));

					if (y->is_object() && y->is_virtual() == false) {
						stream.add_2(str_open_object[pretty ? 1 : 0]); // "{";
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream.add_2(str_open_array[pretty ? 1 : 0]); // "[";
					}

					_save(stream, ut->get_value_list(i), chk_list, depth + 1, pretty);

					if (y->is_object() && std::find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream.add_2(str_close_object[pretty ? 1 : 0]); // "}";
					}
					else if (y->is_array() && std::find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream.add_2(str_close_array[pretty ? 1 : 0]); // "]";
					}
				}
				else {
					auto& x = ut->get_key_list(i);

					if (x.type() == ValueType::STRING) {
						write_string(stream, x.str_val());

						{
							stream.add_2(str_colon[pretty ? 1 : 0]); // " : ";
						}
					}

					{
						auto& x = ut->get_value_list(i);

						save_primitive(stream, x);
					}
				}

				if (i < ut->get_data_size() - 1) {
					stream.add_2(str_comma[pretty ? 1 : 0]); // ",";
				}
			}
		}
		else if (ut && ut->is_array()) {
			uint64_t len = ut->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_structured()) {

					auto* y = ((Structured*)(ut->get_value_list(i).as_structured_ptr()));

					if (y->is_object() && y->is_virtual() == false) {
						stream.add_2(str_open_object[pretty ? 1 : 0]); // "{";
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream.add_2(str_open_array[pretty ? 1 : 0]); // "[";
					}

					_save(stream, ut->get_value_list(i), chk_list, depth + 1, pretty);

					y = ((Structured*)(ut->get_value_list(i).as_structured_ptr()));


					if (y->is_object() && std::find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream.add_2(str_close_object[pretty ? 1 : 0]); // "}";
					}
					else if (y->is_array() && std::find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream.add_2(str_close_array[pretty ? 1 : 0]); // "]";
					}

				}
				else {
					auto& x = ut->get_value_list(i);


					save_primitive(stream, x);
				}

				if (i < ut->get_data_size() - 1) {
					stream.add_2(str_comma[pretty ? 1 : 0]); // ",";
				}
			}
		}
		else if (data) { // valid
			auto& x = data;

			save_primitive(stream, x);
		}
	}

	void LoadData::_save(StrStream& stream, const Value& data, const int depth, bool pretty) {
		const Structured* ut = nullptr;

		if (data.is_structured()) {
			ut = data.as_structured_ptr();
		}

		if (ut && ut->is_object()) {
			uint64_t len = ut->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_structured()) {
					auto& x = ut->get_key_list(i);

					if (x.type() == ValueType::STRING) {


						uint64_t len = x.str_val().size();

						write_string(stream, x.str_val());


						{
							stream.add_2(str_colon[pretty ? 1 : 0]); // " : ";
						}
					}
					else {
						//log << warn  << "Error : no key\n"; // chk...
					}

					auto* y = ((Structured*)(ut->get_value_list(i).as_structured_ptr()));

					if (y->is_object() && y->is_virtual() == false) {
						stream.add_2(str_open_object[pretty ? 1 : 0]);
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream.add_2(str_open_array[pretty ? 1 : 0]);
					}

					_save(stream, ut->get_value_list(i), depth + 1, pretty);

					if (y->is_object()) {
						stream.add_2(str_close_object[pretty ? 1 : 0]);
					}
					else if (y->is_array()) {
						stream.add_2(str_close_array[pretty ? 1 : 0]);
					}
				}
				else {
					auto& x = ut->get_key_list(i);

					if (x.type() == ValueType::STRING) {
				

						uint64_t len = x.str_val().size();
						write_string(stream, x.str_val());




						{
							stream.add_2(str_colon[pretty ? 1 : 0]);
						}
					}

					{
						auto& x = ut->get_value_list(i);

						save_primitive(stream, x);
					}
				}

				if (i < ut->get_data_size() - 1) {
					stream.add_2(str_comma[pretty ? 1 : 0]);
				}
			}
		}
		else if (ut && ut->is_array()) {
			uint64_t len = ut->get_data_size();
			for (uint64_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_structured()) {

					auto* y = ((Structured*)(ut->get_value_list(i).as_structured_ptr()));

					if (y->is_object() && y->is_virtual() == false) {
						stream.add_2(str_open_object[pretty ? 1 : 0]);
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream.add_2(str_open_array[pretty ? 1 : 0]);
					}

					_save(stream, ut->get_value_list(i), depth + 1, pretty);

					y = ((Structured*)(ut->get_value_list(i).as_structured_ptr()));


					if (y->is_object()) {
						stream.add_2(str_close_object[pretty ? 1 : 0]);
					}
					else if (y->is_array()) {
						stream.add_2(str_close_array[pretty ? 1 : 0]);
					}

				}
				else {
					auto& x = ut->get_value_list(i);

					save_primitive(stream, x);
				}

				if (i < ut->get_data_size() - 1) {
					stream.add_2(str_comma[pretty? 1 : 0]);
				}
			}
		}
		else if (data) { // valid
			auto& x = data;

			save_primitive(stream, x);
		}
	}

	// todo... just Data has one element 
	void LoadData::save(const std::string& fileName, const Value& global, bool pretty, bool hint) {
		StrStream stream;

		if (global.is_structured()) {
			if (hint) {
				stream.add_2(str_comma[pretty ? 1 : 0]);
			}
			bool is_arr = global.as_structured_ptr()->is_array();

			if (is_arr) {
				stream.add_2(str_open_array[pretty ? 1 : 0]);
			}
			else {
				stream.add_2(str_open_object[pretty ? 1 : 0]);
			}

			_save(stream, global, 1, pretty);

			if (is_arr) {
				stream.add_2(str_close_array[pretty? 1 : 0]);
			}
			else {
				stream.add_2(str_close_object[pretty ? 1 : 0]);
			}

		}
		else {
			if (hint) {
				stream.add_2(str_comma[pretty ? 1 : 0]);
			}
			auto& x = global;

			save_primitive(stream, x);
		}

		std::ofstream outFile;
		outFile.open(fileName, std::ios::binary); // binary!
		outFile.write(stream.buf(), stream.buf_size());
		outFile.close();
	}

	void LoadData::save(std::ostream& stream, const Value& data, bool pretty) {
		StrStream str_stream;
		_save(str_stream, data, 0, pretty);
		stream << StringView(str_stream.buf(), str_stream.buf_size());
	}

	void LoadData::save_(StrStream& stream, const Value& global, Structured* temp, bool pretty, bool hint) {

		std::vector<Structured*> chk_list; // point for division?, virtual nodes? }}}?

		{
			while (temp) {
				chk_list.push_back(temp);
				temp = temp->get_parent();
			}
		}

		if (global.is_structured()) {
			if (hint) {
				stream.add_2(str_comma[pretty ? 1 : 0]); // stream << ",";
			}

			auto* j = global.as_structured_ptr();


			if (j->is_array() && j->is_virtual() == false) {
				stream.add_2(str_open_array[pretty ? 1 : 0]);
			}
			else if (j->is_object() && j->is_virtual() == false) {
				stream.add_2(str_open_object[pretty ? 1 : 0]);
			}

			_save(stream, global, chk_list, 1, pretty);

			if (j->is_array() && std::find(chk_list.begin(), chk_list.end(), j) == chk_list.end()) {
				stream.add_2(str_close_array[pretty ? 1 : 0]);
			}
			else if (j->is_object() && std::find(chk_list.begin(), chk_list.end(), j) == chk_list.end()) {
				stream.add_2(str_close_object[pretty ? 1 : 0]);
			}
		}
		else {
			if (hint) {
				stream.add_2(str_comma[pretty ? 1 : 0]);
			}

			auto& x = global;

			save_primitive(stream, x);
		}
	}


	void LoadData::save_parallel(const std::string& fileName, Value& j, uint64_t thr_num, bool pretty) {

		if (!j.is_structured()) {
			save(fileName, j, pretty, false);
			return;
		}

		if (thr_num <= 0) {
			thr_num = std::max((int)std::thread::hardware_concurrency() - 2, 1);

		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		if (thr_num == 1) {
			save(fileName, j, pretty, false);
			return;
		}

		std::vector<claujson::Structured*> temp(thr_num, nullptr); //
		std::vector<claujson::Structured*> temp_parent(thr_num, nullptr);
		{
			std::vector<claujson::Structured*> result(temp.size() - 1, nullptr);

			std::vector<int> hint(temp.size() - 1, false);

			auto a = std::chrono::steady_clock::now();
			temp = LoadData2::Divide2(thr_num, j, result, hint);
			auto b = std::chrono::steady_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

			log << info << "divide... " << dur.count() << "ms\n";
			if (temp.size() == 1 && temp[0] == nullptr) {
				save(fileName, j, pretty, false);
				return;
			}

			for (uint64_t i = 0; i < temp.size() - 1; ++i) {
				temp_parent[i] = temp[i]->get_parent();
			}

			a = std::chrono::steady_clock::now();

			std::vector<claujson::StrStream> stream(thr_num);

			//std::vector<std::thread> thr(thr_num);
			std::vector<std::future<void>> thr_result(thr_num);


			thr_result[0] = pool->enqueue(save_, std::ref(stream[0]), std::cref(j), temp_parent[0], pretty, (false));


			for (uint64_t i = 1; i < thr_num; ++i) {
				thr_result[i] = pool->enqueue(save_, std::ref(stream[i]), std::cref(result[i - 1]->get_value_list(0)), temp_parent[i], pretty, (hint[i - 1]));
			}

			for (uint64_t i = 0; i < thr_num; ++i) {
				thr_result[i].get();
			}

			b = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

			log << info << "save_ " << dur.count() << "ms\n";
			a = std::chrono::steady_clock::now();
			int op = 0;
			
			claujson::LoadData2::Merge2(temp_parent[0], result[0], &temp_parent[1], op);
			
			for (uint64_t i = 1; i < temp.size() - 1; ++i) {
				claujson::LoadData2::Merge2(temp_parent[i], result[i], &temp_parent[i + 1], op);
				op = 0;
			}
			b = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

			log << info << "merge " << dur.count() << "ms\n";

			for (uint64_t i = 0; i < result.size(); ++i) {
				if (result[i]) {
					delete result[i];
					result[i] = nullptr;
				}
			}
			a = std::chrono::steady_clock::now();
			std::ofstream outFile(fileName, std::ios::binary);
			if (outFile) {
				for (uint64_t i = 0; i < stream.size(); ++i) {
					outFile.write(stream[i].buf(), stream[i].buf_size());
				}

				outFile.close();
			}
			b = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
			log << info << "write to file " << dur.count() << "ms\n";
		}
	}

	bool is_valid2(_simdjson::dom::parser_for_claujson& dom_parser, uint64_t start, uint64_t last,
		int* _start_state, int* _last_state,
		std::vector<int>* _is_array = nullptr, std::vector<int>* _is_virtual_array = nullptr, uint64_t* count = nullptr,
		int* err = nullptr) {

		const auto& buf = dom_parser.raw_buf();

		auto* simdjson_imple = dom_parser.raw_implementation().get();
		uint64_t idx = start;
		uint64_t depth = 0;
		std::vector<int> is_array;
		std::vector<int> is_virtual_array;
		std::vector<uint64_t> _stack; _stack.reserve(1024);
		uint64_t virtual_count = 0;
		uint64_t virtual_idx = 0;

		is_array.reserve(1024);
		is_virtual_array.reserve(1024);

		int state = 0;


		if (start > last) {
			return false;
		}

		if (start > 0 && start == last) {
			return false;
		}
		//
// Start the document
//
		///if (at_eof()) { return EMPTY; }

		if (simdjson_imple->n_structural_indexes == 0) {
			return true; // chk?
		}

		//log_start_value("document");
		//SIMDJSON_TRY(visitor.visit_document_start(*this));

		//
		// Read first value
		//
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]]; //advance();

			// Make sure the outer object or array is closed before continuing; otherwise, there are ways we
			// could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
			//if (!STREAMING) {
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[simdjson_imple->n_structural_indexes - 1]] != '}') {
				log << warn << ("starting brace unmatched");

				if (err) {
					*err = 1;
				}

				return false;
			}
					break;
			case '[': if (buf[simdjson_imple->structural_indexes[simdjson_imple->n_structural_indexes - 1]] != ']') {
				log << warn << ("starting bracket unmatched");
				if (err) {
					*err = 1;
				}
				return false;
			}
					break;
			}
			//	}

			switch (value) { // start == 0
			case '{': { if (buf[simdjson_imple->structural_indexes[idx]] == '}') {
				++idx; log << warn << ("empty object"); break;
			} *_start_state = 0; _stack.push_back(idx - 1); goto object_begin;  }
			case '[': { if (buf[simdjson_imple->structural_indexes[idx]] == ']') {
				++idx; log << warn << ("empty array"); break;
			} *_start_state = 4;  _stack.push_back(idx - 1); goto array_begin; }

			default: break;
			}


			if (start > 0 && value == ',') {
				if (idx < simdjson_imple->n_structural_indexes - 1) {
					if (buf[simdjson_imple->structural_indexes[idx + 1]] == ':') {
						--idx;
						*_start_state = 2;
						goto object_continue;
					}
					else {
						--idx;
						*_start_state = 6;
						goto array_continue;
					}
				}
				else { // idx >= n_~~  // error
					*err = true;
					return false;
				}
			}

			switch (value) {
			case ':':
			case ',':
			case '}':
			case ']':
			{ log << warn << "not primitive"; return false; } break;
			}
		}
		goto document_end;

		//
		// Object parser states
		//
	object_begin:
		//log_start_value("object");
		depth++;

		{
			if (idx > last) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}
		state = 0;
		if (is_array.size() < depth) {
			is_array.push_back(0);
		}

		_stack.push_back(idx - 1); 
		//dom_parser.is_array[depth] = false;
		is_array[depth - 1] = 0;
		//SIMDJSON_TRY(visitor.visit_object_start(*this));

		{
			auto key = buf[simdjson_imple->structural_indexes[idx++]]; // advance();
			if (key != '"') {
				log << warn << ("Object does not start with a key");
				if (err) {
					*err = 1;
				}
				return false;
			}
			//SIMDJSON_TRY(visitor.increment_count(*this));
			//SIMDJSON_TRY(visitor.visit_key(*this, key));
		}

	object_field:

		{
			if (idx > last) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		state = 1;

		if (_simdjson_unlikely(buf[simdjson_imple->structural_indexes[idx++]] != ':')) { log << warn << ("Missing colon after key in object"); return false; }
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]];
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[idx]] == '}') { ++idx; break; } goto object_begin;
			case '[': if (buf[simdjson_imple->structural_indexes[idx]] == ']') { ++idx; break; } goto array_begin;
			case ',': { log << warn << "wrong comma.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ':': { log << warn << "wrong colon.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '}': { log << warn << "wrong }.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ']': { log << warn << "wrong ].";
				if (err) {
					*err = 1;
				}
				return false; }
			default: //SIMDJSON_TRY(visitor.visit_primitive(*this, value)); 
				break;
			}
		}

	object_continue:


		if (!_stack.empty()) {
			count[_stack.back()]++; 
		}
		else {
			if (virtual_count == 0) {
				virtual_idx = idx - 1;
			}
			virtual_count++;
		}

		{
			if (idx > last) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}
		state = 2;
		switch (buf[simdjson_imple->structural_indexes[idx++]]) {
		case ',':
			//SIMDJSON_TRY(visitor.increment_count(*this));
		{
			auto key = buf[simdjson_imple->structural_indexes[idx++]]; // advance();
			if (_simdjson_unlikely(key != '"')) {
				log << warn << ("Key string missing at beginning of field in object");
				if (err) {
					*err = 1;
				}
				return false;
			}
			//SIMDJSON_TRY(visitor.visit_key(*this, key));
		}
		goto object_field;
		case '}': goto scope_end;
		case ':': { log << warn << "wrong colon.";
			if (err) {
				*err = 1;
			}
			return false; }
		default: log << warn << ("No comma between object fields"); return false;
		}

	scope_end:
		{
			state = 3;
			if (depth > 0) {
				depth--; is_array.pop_back(); _stack.pop_back(); if (_stack.empty()) { virtual_count = 0; }
			}
			else {
				// depth <= 0.. virtual array or virtual object..
				switch (buf[simdjson_imple->structural_indexes[idx - 1]]) {
				case ']': 
					is_virtual_array.push_back(1); count[virtual_idx] = virtual_count; virtual_count = 0;
					break;
				case '}':
					is_virtual_array.push_back(0); count[virtual_idx] = virtual_count;  virtual_count = 0;
					break;
				}
			}

			if (idx > last) { // depth == 0) {
				goto document_end;
			}

			if (depth == 0) {
				// is in array or object ?
				if (buf[simdjson_imple->structural_indexes[idx]] == ',') {
					++idx;
					if (idx < simdjson_imple->n_structural_indexes - 1) {
						if (buf[simdjson_imple->structural_indexes[idx + 1]] == ':') {
							--idx;
							goto object_continue;
						}
						else {
							--idx;
							goto array_continue;
						}
					}
					else { // idx >= n_~~  // error
						*err = true;
						return false;
					}
				}
				else {
					switch (buf[simdjson_imple->structural_indexes[idx]]) {
					case ']':
					case '}':
						++idx;
						goto scope_end;
						break;
					default:
						// error
						*err = true;
						return false;
					}
				}
			}

			if (is_array[depth - 1]) { goto array_continue; }
			goto object_continue;
		}

		//
		// Array parser states
		//
	array_begin:

		{
			if (idx > last) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}
		state = 4;
		//log_start_value("array");
		depth++;
		if (is_array.size() < depth) { is_array.push_back(1); }
		is_array[depth - 1] = 1;

		_stack.push_back(idx - 1);

		//SIMDJSON_TRY(visitor.visit_array_start(*this));
	//	SIMDJSON_TRY(visitor.increment_count(*this));

	array_value:
		{
			if (idx > last) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		state = 5;
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]];
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[idx]] == '}') { ++idx; break; } goto object_begin;
			case '[': if (buf[simdjson_imple->structural_indexes[idx]] == ']') { ++idx; break; } goto array_begin;
			case ',': { log << warn << "wrong comma.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ':': { log << warn << "wrong colon.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '}': { log << warn << "wrong }.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ']': { log << warn << "wrong ].";
				if (err) {
					*err = 1;
				}
				return false; }
			default: break;
			}
		}

	array_continue:
		if (!_stack.empty()) {
			count[_stack.back()]++;
		}
		else {
			if (virtual_count == 0) {
				virtual_idx = idx - 1;
			}
			virtual_count++;
		}

		{
			if (idx > last) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		state = 6;
		switch (buf[simdjson_imple->structural_indexes[idx++]]) {
		case ',': goto array_value;
		case ']': goto scope_end;
		case ':': { log << warn << "wrong colon.";
			if (err) {
				*err = 1;
			}
			return false; }
		default: log << warn << ("Missing comma between array values");
			if (err) {
				*err = 1;
			}
			return false;
		}

	document_end:

		*_last_state = state;

		// If we didn't make it to the end, it's an error
		if (idx <= last) {
			log << warn << ("More than one JSON value at the root of the document, or extra characters at the end of the JSON!"); // chk...

			if (err) {
				*err = 1;
			}
			return false;
		}

		if (_is_array) {
			*_is_array = std::move(is_array);
		}

		if (_is_virtual_array) {
			*_is_virtual_array = std::move(is_virtual_array);
		}

		return true;
	}

	bool is_valid(_simdjson::dom::parser_for_claujson& dom_parser, uint64_t middle, std::vector<int>* _is_array = nullptr, int* err = nullptr) {

		const auto& buf = dom_parser.raw_buf();
		const auto buf_len = dom_parser.raw_len();

		auto* simdjson_imple = dom_parser.raw_implementation().get();
		uint64_t idx = 0;
		uint64_t depth = 0;
		std::vector<int> is_array;

		is_array.reserve(1024);

		//
// Start the document
//
		///if (at_eof()) { return EMPTY; }

		if (simdjson_imple->n_structural_indexes == 0) {
			return true; // chk?
		}

		//log_start_value("document");
		//SIMDJSON_TRY(visitor.visit_document_start(*this));

		//
		// Read first value
		//
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]]; //advance();

			// Make sure the outer object or array is closed before continuing; otherwise, there are ways we
			// could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
			//if (!STREAMING) {
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[simdjson_imple->n_structural_indexes - 1]] != '}') {
				log << warn << ("starting brace unmatched");

				if (err) {
					*err = 1;
				}

				return false;
			}
					break;
			case '[': if (buf[simdjson_imple->structural_indexes[simdjson_imple->n_structural_indexes - 1]] != ']') {
				log << warn << ("starting bracket unmatched");
				if (err) {
					*err = 1;
				}
				return false;
			}
					break;
			}
			//	}

			switch (value) {
			case '{': { if (buf[simdjson_imple->structural_indexes[idx]] == '}') {
				++idx; log << warn << ("empty object"); break;
			} goto object_begin; }
			case '[': { if (buf[simdjson_imple->structural_indexes[idx]] == ']') {
				++idx; log << warn << ("empty array"); break;
			} goto array_begin; }

			default: break;
			}


			switch (value) {
			case ':':
			case ',':
			case '}':
			case ']':
			{ log << warn << "not primitive"; return false; } break;
			}
		}
		goto document_end;

		//
		// Object parser states
		//
	object_begin:
		//log_start_value("object");
		depth++;
		if (is_array.size() < depth) {
			is_array.push_back(0);
		}
		
		//dom_parser.is_array[depth] = false;
		is_array[depth - 1] = 0;
		//SIMDJSON_TRY(visitor.visit_object_start(*this));

		{
			if (idx > middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}


		{
			auto key = buf[simdjson_imple->structural_indexes[idx++]]; // advance();
			if (key != '"') {
				log << warn << ("Object does not start with a key");
				if (err) {
					*err = 1;
				}
				return false;
			}
			//SIMDJSON_TRY(visitor.increment_count(*this));
			//SIMDJSON_TRY(visitor.visit_key(*this, key));
		}

	object_field:

		{
			if (idx > middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		if (_simdjson_unlikely(buf[simdjson_imple->structural_indexes[idx++]] != ':')) { log << warn << ("Missing colon after key in object"); return false; }
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]];
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[idx]] == '}') { ++idx; break; } goto object_begin;
			case '[': if (buf[simdjson_imple->structural_indexes[idx]] == ']') { ++idx; break; } goto array_begin;
			case ',': { log << warn << "wrong comma.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ':': { log << warn << "wrong colon.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '}': { log << warn << "wrong }.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ']': { log << warn << "wrong ].";
				if (err) {
					*err = 1;
				}
				return false; }
			default: //SIMDJSON_TRY(visitor.visit_primitive(*this, value)); 
				break;
			}
		}

	object_continue:
		{
			if (idx > middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		switch (buf[simdjson_imple->structural_indexes[idx++]]) {
		case ',':
			//SIMDJSON_TRY(visitor.increment_count(*this));
		{
			auto key = buf[simdjson_imple->structural_indexes[idx++]]; // advance();
			if (_simdjson_unlikely(key != '"')) {
				log << warn << ("Key string missing at beginning of field in object");
				if (err) {
					*err = 1;
				}
				return false;
			}
			//SIMDJSON_TRY(visitor.visit_key(*this, key));
		}
		goto object_field;
		case '}': goto scope_end;
		case ':': { log << warn << "wrong colon.";
			if (err) {
				*err = 1;
			}
			return false; }
		default: log << warn << ("No comma between object fields"); return false;
		}

	scope_end:
		depth--; is_array.pop_back();
		if (depth == 0) {
			goto document_end;
		}
		if (is_array[depth - 1]) { goto array_continue; }
		goto object_continue;

		//
		// Array parser states
		//
	array_begin:
		//log_start_value("array");
		depth++;
		if (is_array.size() < depth) { is_array.push_back(1); }
		is_array[depth - 1] = 1;

		{
			if (idx > middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		//SIMDJSON_TRY(visitor.visit_array_start(*this));
	//	SIMDJSON_TRY(visitor.increment_count(*this));

	array_value:
		{
			if (idx > middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]];
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[idx]] == '}') { ++idx; break; } goto object_begin;
			case '[': if (buf[simdjson_imple->structural_indexes[idx]] == ']') { ++idx; break; } goto array_begin;
			case ',': { log << warn << "wrong comma.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ':': { log << warn << "wrong colon.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '}': { log << warn << "wrong }.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ']': { log << warn << "wrong ].";
				if (err) {
					*err = 1;
				}
				return false; }
			default: break;
			}
		}

	array_continue:
		{
			if (idx > middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}
		}

		switch (buf[simdjson_imple->structural_indexes[idx++]]) {
		case ',': goto array_value;
		case ']': goto scope_end;
		case ':': { log << warn << "wrong colon.";
			if (err) {
				*err = 1;
			}
			return false; }
		default: log << warn << ("Missing comma between array values");
			if (err) {
				*err = 1;
			}
			return false;
		}

	document_end:

		// If we didn't make it to the end, it's an error
		if (idx < middle) {
			log << warn << ("More than one JSON value at the root of the document, or extra characters at the end of the JSON!"); // chk...

			if (err) {
				*err = 1;
			}
			return false;
		}

		if (_is_array) {
			*_is_array = std::move(is_array);
		}

		return true;
	}
	bool is_valid_reverse(_simdjson::dom::parser_for_claujson& dom_parser, int64_t middle, std::vector<int>* _is_array = nullptr, int* err = nullptr) { // str[middle] == ','

		const auto& buf = dom_parser.raw_buf();

		auto* simdjson_imple = dom_parser.raw_implementation().get();
		int64_t idx = simdjson_imple->n_structural_indexes - 1;
		uint64_t depth = 0;
		std::vector<int> is_array;

		is_array.reserve(1024);

		//
// Start the document
//
		///if (at_eof()) { return EMPTY; }

		//if (idx == middle) { // , (nothing) <- error
		//	return false; 
		//}
		//else 
		if (idx < middle) {
			return true; // chk?
		}

		//log_start_value("document");
		//SIMDJSON_TRY(visitor.visit_document_start(*this));

		//
		// Read first value
		//
		{
			auto value = buf[simdjson_imple->structural_indexes[idx--]]; //advance();

			// Make sure the outer object or array is closed before continuing; otherwise, there are ways we
			// could get into memory corruption. See https://github.com/simdjson/simdjson/issues/906
			//if (!STREAMING) {
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[0]] != '}') {
				log << warn << ("starting brace unmatched");
				if (err) {
					*err = 1;
				}
				return false;
			}
					break;
			case '[': if (buf[simdjson_imple->structural_indexes[0]] != ']') {
				log << warn << ("starting bracket unmatched");
				if (err) {
					*err = 1;
				}
				return false;
			}
					break;
			}
			//	}

			switch (value) {
			case '}': { if (idx >= 0 && buf[simdjson_imple->structural_indexes[idx]] == '{') {
				--idx; log << warn << ("empty object"); break;
			} goto scope_end; }
			case ']': { if (idx >= 0 && buf[simdjson_imple->structural_indexes[idx]] == '[') {
				--idx; log << warn << ("empty array"); break;
			} goto scope_end; }

			default: break;
			}


			switch (value) {
			case ':':
			case ',':
			case '{':
			case '[':
			{ log << warn << "not primitive";
			if (err) {
				*err = 1;
			}
			return false; } break;
			}
		}
		if (idx < middle) {
			goto document_end;
		}
		goto scope_end;

		//
		// Object parser states
		//
	object_begin:
		depth--; is_array.pop_back();
		if (depth == 0) {
			goto document_end;
		}

		{
			auto value = buf[simdjson_imple->structural_indexes[idx + 1]];

			if (is_array[depth - 1]) {
				goto array_continue;
			}
			goto object_key;

		}



	object_value:
		{
			if (idx < middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}

			auto value = buf[simdjson_imple->structural_indexes[idx--]];

			switch (value) {
			case '}':
				if (idx >= 0 && buf[simdjson_imple->structural_indexes[idx]] == '{') { --idx; break; } goto scope_end;
			case ']':
				if (idx >= 0 && buf[simdjson_imple->structural_indexes[idx]] == '[') { --idx; break; } goto scope_end;
			case ',': { log << warn << "wrong comma.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ':': { log << warn << "wrong colon.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '{': { log << warn << "wrong {.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '[': { log << warn << "wrong [.";
				if (err) {
					*err = 1;
				}
				return false; }
			default: //SIMDJSON_TRY(visitor.visit_primitive(*this, value)); 
				break;
			}
		}

	object_key:
		if (idx < middle) {
			goto document_end;
		}
		if (err && *err) {
			return false;
		}


		if (_simdjson_unlikely(buf[simdjson_imple->structural_indexes[idx--]] != ':')) {
			log << warn << ("Missing colon after key in object");
			if (err) {
				*err = 1;
			}
			return false;
		}

		//log_start_value("object");
		{
			if (idx < middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}


			auto key = buf[simdjson_imple->structural_indexes[idx--]]; // advance();
			if (key != '"') {
				log << warn << ("Object does not start with a key");
				if (err) {
					*err = 1;
				}
				return false;
			}
			//SIMDJSON_TRY(visitor.increment_count(*this));
			//SIMDJSON_TRY(visitor.visit_key(*this, key));

			if (idx < middle) {
				goto document_end;
			}
			if (err && *err) {
				return false;
			}

		}

	object_continue:
		if (idx < middle) {
			goto document_end;
		}
		if (err && *err) {
			return false;
		}


		switch (buf[simdjson_imple->structural_indexes[idx--]]) {
		case ',': goto object_value;
		case '{': goto object_begin;
		case ':': { log << warn << "wrong colon.";
			if (err) {
				*err = 1;
			}
			return false; }
		default: log << warn << ("No comma between object fields");
			if (err) {
				*err = 1;
			}
			return false;
		}

	scope_end:
		{
			depth++;
			if (is_array.size() < depth) {
				is_array.push_back(0);
			}
		
			if (idx < middle) {
				goto document_end;
			}

			if (err && *err) {
				return false;
			}

			auto value = buf[simdjson_imple->structural_indexes[idx + 1]];

			switch (value) {
			case '}':
				is_array[depth - 1] = 0;
				goto object_value;
			case ']':
				is_array[depth - 1] = 1;
				goto array_value;
			}

			log << warn << "not } or ]";
			if (err) {
				*err = 1;
			}

			return false;
		}
		//depth--;
		//if (depth == 0) {
		//	goto document_end;
		//}
		//if (is_array[depth - 1]) { goto array_continue; }
		//goto object_continue;

		//
		// Array parser states
		//
	array_begin:
		//log_start_value("array");
		depth--; is_array.pop_back();
		if (depth == 0) {
			goto document_end;
		}

		if (idx < middle) {
			goto document_end;
		}

		if (err && *err) {
			return false;
		}

		{
			auto value = buf[simdjson_imple->structural_indexes[idx + 1]];

			if (is_array[depth - 1]) {
				goto array_continue;
			}
			goto object_key;
		}

		//SIMDJSON_TRY(visitor.visit_array_start(*this));
		//SIMDJSON_TRY(visitor.increment_count(*this));

	array_value:

		if (idx < middle) {
			goto document_end;
		}
		if (err && *err) {
			return false;
		}


		{
			auto value = buf[simdjson_imple->structural_indexes[idx--]];
			switch (value) {
			case '}': if (idx >= 0 && buf[simdjson_imple->structural_indexes[idx]] == '{') { --idx; break; } goto scope_end;
			case ']': if (idx >= 0 && buf[simdjson_imple->structural_indexes[idx]] == '[') { --idx; break; } goto scope_end;
			case ',': { log << warn << "wrong comma.";
				if (err) {
					*err = 1;
				}
				return false; }
			case ':': { log << warn << "wrong colon.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '{': { log << warn << "wrong {.";
				if (err) {
					*err = 1;
				}
				return false; }
			case '[': { log << warn << "wrong [.";
				if (err) {
					*err = 1;
				}
				return false; }
			default: break;
			}
		}

	array_continue:
		if (idx < middle) {
			goto document_end;
		}
		if (err && *err) {
			return false;
		}


		switch (buf[simdjson_imple->structural_indexes[idx--]]) {
		case ',': goto array_value;
		case '[': goto array_begin;
		case ':': { log << warn << "wrong colon.";
			if (err) {
				*err = 1;
			}
			return false; }
		default: log << warn << ("Missing comma between array values");
			if (err) {
				*err = 1;
			}
			return false;
		}

	document_end:

		if (idx >= middle) {
			log << warn << ("json is not valid");
			if (err) {
				*err = 1;
			}

			return false;
		}

		log << info << "test end\n";

		if (_is_array) {
			*_is_array = std::move(is_array);
		}

		return true;
	}

	std::pair<bool, uint64_t> parse(const std::string& fileName, Value& ut, uint64_t thr_num, bool use_all_function)
	{
		if (thr_num <= 0) {
			thr_num = std::max((int)std::thread::hardware_concurrency() - 2, 1);
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		uint64_t length = 0;

		auto _ = std::chrono::steady_clock::now();

		uint64_t* count_vec = nullptr;
		{

			log << info << "simdjson-stage1 start\n";
			// not static??
			static _simdjson::dom::parser_for_claujson test;

			auto x = test.load(fileName);

			if (x.error() != _simdjson::error_code::SUCCESS) {
				log << warn << "stage1 error : ";
				log << warn << x.error() << "\n";

				//ERROR(_simdjson::error_message(x.error()));

				return { false, 0 };
			}

			const auto& buf = test.raw_buf();
			const auto buf_len = test.raw_len();

			auto* simdjson_imple_ = test.raw_implementation().get();

			std::vector<int64_t> start(thr_num + 1, 0);
			//std::vector<int> key;

			auto a = std::chrono::steady_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(a - _);
			log << info << dur.count() << "ms\n";


			{
				uint64_t how_many = simdjson_imple_->n_structural_indexes;
				length = how_many;

				start[0] = 0;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}
			}

			if (length == 0) {
				log << warn << "empty string is not valid json";
				return { false, 0 };
			}

			auto b = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
			log << info << "valid1 " << dur.count() << "ms\n";

			b = std::chrono::steady_clock::now();

			//if (!is_valid(test, length - 1)) {
			//	return { false, 0 };
			//}
			// find ,
			{

				auto a = std::chrono::steady_clock::now();

				if (use_all_function) {

					std::set<uint64_t> _set;

					std::vector<uint64_t> start(thr_num + 1);
					std::vector<uint64_t> last(thr_num);

					std::vector<int> start_state(thr_num, -1);
					std::vector<int> last_state(thr_num, -1);

					for (uint64_t i = 1; i < thr_num; ++i) {
						uint64_t middle = length / thr_num * i;
						for (uint64_t i = middle; i < length; ++i) {
							if (buf[simdjson_imple_->structural_indexes[i]] == ',') {
								middle = i; _set.insert(i); break;
							}

							if (i == length - 1) {
								middle = length;
							}
						}
					}

					_set.insert(0);

					start.resize(1 + _set.size());
					last.resize(_set.size());

					int count = 0;
					for (auto x : _set) {
						start[count] = x;
						++count;
					}
					start[_set.size()] = length - 1;

					for (uint64_t i = 0; i < _set.size(); ++i) {
						last[i] = start[i + 1];
					}

					std::vector<std::vector<int>> is_array(_set.size()), is_virtual_array(_set.size());
					std::vector<std::future<bool>> thr_result(_set.size());
					int err = 0;

					count_vec = (uint64_t*)calloc(length, sizeof(uint64_t));

					for (int i = 0; i < _set.size(); ++i) {
						thr_result[i] = pool->enqueue(is_valid2, std::ref(test), start[i], last[i], &start_state[i], &last_state[i], &is_array[i], &is_virtual_array[i], count_vec, &err);
					}
					std::vector<int> result(_set.size());

					for (uint64_t i = 0; i < _set.size(); ++i) {
						result[i] = static_cast<int>(thr_result[i].get());
					}

					for (uint64_t i = 0; i < result.size(); ++i) {
						if (result[i] == false) {
							free(count_vec);
							return { false, -1 };
						}
					}

					for (uint64_t i = 0; i < _set.size() - 1; ++i) {
						if (start_state[i + 1] != last_state[i]) { // need more tests.
							free(count_vec); return { false, -2 };
						}
					}

					if (is_virtual_array[0].empty() == false) { // first block has no virtual array or virtual object.!
						free(count_vec); return { false, -3 };
					}

					for (uint64_t i = 1; i < _set.size(); ++i) {
						if (false == is_virtual_array[i].empty()) {
							// remove? matched is_array(or object) and is_virtual_array(or object)
							if (is_array[0].size() >= is_virtual_array[i].size()) {
								for (uint64_t j = 0; j < is_virtual_array[i].size(); ++j) {
									if (is_array[0].back() != is_virtual_array[i][j]) {
										free(count_vec); return { false, -3 };
									}
									is_array[0].pop_back();
								}
							}
							else {
								free(count_vec); return { false, -3 };
							}
						}
						// added...
						is_array[0].insert(is_array[0].end(), is_array[i].begin(), is_array[i].end());
					}

					if (false == is_array[0].empty()) {
						free(count_vec); return { false, -4 };
					}
				}
				else {
					if (!is_valid(test, length)) {
						return { false, 0 };
					}
				}

				dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - a);
				log << info << "test time " << dur.count() << "ms\n";
			}
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - b);
			log << info << dur.count() << "ms\n";

			b = std::chrono::steady_clock::now();

			start[thr_num] = length;
			if (false == claujson::LoadData2::parse(ut, buf.get(), buf_len, simdjson_imple_, length, start, count_vec, thr_num)) // 0 : use all thread..
			{
				free(count_vec);
				return { false, 0 };
			}
			auto c = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);

			log << info << dur.count() << "ms\n";
		}
		auto c = std::chrono::steady_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - _);
		log << info << dur.count() << "ms\n";

		free(count_vec);
		return  { true, length };
	}
	std::pair<bool, uint64_t> parse_str(StringView str, Value& ut, uint64_t thr_num, bool use_all_function)
	{


		log << info << str << "\n";

		if (thr_num <= 0) {
			thr_num = std::max((int)std::thread::hardware_concurrency() - 2, 1);

		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		uint64_t length = 0;

		auto _ = std::chrono::steady_clock::now();
		uint64_t* count_vec = nullptr;
		{
		
			// not static?
			static _simdjson::dom::parser_for_claujson test;

			auto x = test.parse(str.data(), str.length());

			if (x.error() != _simdjson::error_code::SUCCESS) {
				log << warn << "stage1 error : ";
				log << warn << x.error() << "\n";

				return { false, 0 };
			}
			const auto& buf = test.raw_buf();
			const auto buf_len = test.raw_len();
			auto* simdjson_imple_ = test.raw_implementation().get();

			std::vector<int64_t> start(thr_num + 1, 0);
			//std::vector<int> key;

			auto a = std::chrono::steady_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(a - _);
			log << info << dur.count() << "ms\n";


			{
				uint64_t how_many = simdjson_imple_->n_structural_indexes;
				length = how_many;

				start[0] = 0;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}
			}

			if (length == 0) {
				log << warn << "empty string is not valid json";
				return { false, 0 };
			}

			auto b = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

			log << info << dur.count() << "ms\n";
			b = std::chrono::steady_clock::now();

			if (use_all_function) {

				std::set<uint64_t> _set;

				std::vector<uint64_t> start(thr_num + 1);
				std::vector<uint64_t> last(thr_num);

				std::vector<int> start_state(thr_num, -1);
				std::vector<int> last_state(thr_num, -1);

				for (uint64_t i = 1; i < thr_num; ++i) {
					uint64_t middle = length / thr_num * i;
					for (uint64_t i = middle; i < length; ++i) {
						if (buf[simdjson_imple_->structural_indexes[i]] == ',') {
							middle = i; _set.insert(i); break;
						}

						if (i == length - 1) {
							middle = length;
						}
					}
				}

				_set.insert(0);

				start.resize(1 + _set.size());
				last.resize(_set.size());

				int count = 0;
				for (auto x : _set) {
					start[count] = x;
					++count;
				}
				start[_set.size()] = length - 1;

				for (uint64_t i = 0; i < _set.size(); ++i) {
					last[i] = start[i + 1];
				}

				std::vector<std::vector<int>> is_array(_set.size()), is_virtual_array(_set.size());
				std::vector<std::future<bool>> thr_result(_set.size());
				int err = 0;
				count_vec = (uint64_t*)calloc(length, sizeof(uint64_t));
				for (int i = 0; i < _set.size(); ++i) {
					thr_result[i] = pool->enqueue(is_valid2, std::ref(test), start[i], last[i], &start_state[i], &last_state[i], &is_array[i], &is_virtual_array[i], count_vec, &err);
				}
				std::vector<int> vec(_set.size());

				for (uint64_t i = 0; i < _set.size(); ++i) {
					vec[i] = (int)thr_result[i].get();
				}

				bool result = true;

				for (uint64_t i = 0; i < vec.size(); ++i) {
					if (vec[i] == false) {
						free(count_vec); return { false, -1 };
					}
				}

				for (uint64_t i = 0; i < _set.size() - 1; ++i) {
					if (start_state[i + 1] != last_state[i]) {
						free(count_vec); return { false, -2 };
					}
				}

				if (is_virtual_array[0].empty() == false) { // first block has no virtual array or virtual object.!
					free(count_vec); return { false, -3 };
				}

				for (uint64_t i = 1; i < _set.size(); ++i) {
					if (false == is_virtual_array[i].empty()) {
						//  
						if (is_array[0].size() >= is_virtual_array[i].size()) {
							for (uint64_t j = 0; j < is_virtual_array[i].size(); ++j) {
								if (is_array[0].back() != is_virtual_array[i][j]) {
									free(count_vec); return { false, -3 };
								}
								is_array[0].pop_back();
							}
						}
						else {
							free(count_vec); return { false, -3 };
						}
					}

					is_array[0].insert(is_array[0].end(), is_array[i].begin(), is_array[i].end());
				}

				if (false == is_array[0].empty()) {
					free(count_vec); return { false, -4 };

				}
			}
			else {
				if (!is_valid(test, length)) {
					free(count_vec); return { false, 0 };
				}
			}

			dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - b);
			log << info << dur.count() << "ms\n";

			b = std::chrono::steady_clock::now();

			start[thr_num] = length;

			if (false == claujson::LoadData2::parse(ut, buf.get(), buf_len, simdjson_imple_, length, start, count_vec, thr_num)) // 0 : use all thread..
			{
				free(count_vec);
				return { false, 0 };
			}
			auto c = std::chrono::steady_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
			log << info << dur.count() << "ms\n";
		}
		auto c = std::chrono::steady_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(c - _);
		log << info << dur.count() << "ms\n";

		free(count_vec);
		return  { true, length };
	}

#if __cplusplus >= 202002L
	// C++20~
	std::pair<bool, uint64_t> parse_str(std::u8string_view str, Value& ut, uint64_t thr_num, bool use_all_function) {
		return parse_str(StringView(reinterpret_cast<const char*>(str.data()), str.size()), ut, thr_num, use_all_function);
	}
#endif

	std::string save_to_str(const Value& global, bool pretty) {
		return LoadData::save_to_str(global, pretty);
	}


	void save(const std::string& fileName, const Value& global, bool pretty) {
		LoadData::save(fileName, global, pretty, false);
	}

	void save_parallel(const std::string& fileName, Value& j, uint64_t thr_num, bool pretty) {
		LoadData::save_parallel(fileName, j, thr_num, pretty);
	}

	static std::string escape_for_json_pointer(std::string str) {
		// 1. ~ -> ~0
		// 2. / -> ~1
		// 1->2 ok, 2->1 no ok.
		{
			uint64_t idx = 0;
			idx = str.find('~');
			while (idx != std::string::npos) {
				str = str.replace(str.begin() + idx, str.begin() + idx + 1, "~0");
				idx = str.find("~", idx + 2);
			}
		}

		{
			uint64_t idx = 0;
			idx = str.find('/');
			while (idx != std::string::npos) {
				str = str.replace(str.begin() + idx, str.begin() + idx + 1, "~1");
				idx = str.find('/', idx + 2);
			}
		}

		return str;
	}


	// cf) /- -> / in array.
	static Value _diff(const Value& x, const Value& y, std::string route) {
		Value result(new Array());
		Structured* j = result.as_structured_ptr();

		if (x == y) {
			return result;
		}

		if (x.type() != y.type()) {
			Object* obj = new Object();

			obj->add_object_element(Value("op"sv), Value("replace"sv));
			obj->add_object_element(Value("path"sv), Value(route));
			obj->add_object_element(Value("value"sv), Value(y.clone()));

			j->add_object(Ptr<Structured>(obj));
			return result;
		}

		switch (x.type()) {
		case ValueType::ARRAY:
		case ValueType::OBJECT:
		{
			const Structured* jx = x.as_structured_ptr();
			const Structured* jy = y.as_structured_ptr();

			if (jx->is_array()) {
				uint64_t i = 0;
				uint64_t sz_x = jx->get_data_size();
				uint64_t sz_y = jy->get_data_size();

				for (; i < sz_x && i < sz_y; ++i) {
					std::string new_route = route;
					new_route += '/';
					new_route += std::to_string(i); // change? fmt?

					Value inner_diff = _diff(jx->get_value_list(i), jy->get_value_list(i), std::move(new_route));

					{
						Structured* w = inner_diff.as_structured_ptr();
						uint64_t sz_w = w->get_data_size();

						for (uint64_t t = 0; t < sz_w; ++t) {
							Value temp = std::move(w->get_value_list(t));
							j->add_object(Ptr<Structured>(temp.as_structured_ptr()));
						}

						clean(inner_diff);
					}
				}

				if (i < sz_x) {
					for (uint64_t _i = sz_x; _i > i; --i) {
						Object* obj = new Object();

						obj->add_object_element(Value("op"sv), Value("remove"sv));

						obj->add_object_element(Value("path"sv), Value(route));
						obj->add_object_element(Value("last_idx"sv), Value(_i - 1));

						j->add_object(Ptr<Structured>(obj));
					}
				}
				else {
					for (; i < sz_y; ++i) {
						Object* obj = new Object();

						obj->add_object_element(Value("op"sv), Value("add"sv));

						obj->add_object_element(Value("path"sv), Value(route));

						obj->add_object_element(Value("value"sv), Value(jy->get_value_list(i).clone()));

						j->add_object(Ptr<Structured>(obj));
					}
				}
			}
			else if (jx->is_object()) {
				uint64_t sz_x = jx->get_data_size();
				uint64_t sz_y = jy->get_data_size();

				for (uint64_t i = sz_x; i > 0; --i) {
					std::string key = jx->get_key_list(i - 1).str_val();
					std::string new_route = route;
					new_route += '/';
					new_route += escape_for_json_pointer(key);
					uint64_t idx = jy->find_(key);
					if (idx != Structured::npos) {
						Value inner_diff = _diff((jx->get_value_list(i - 1)), jy->get_value_list(idx), new_route);

						{
							Structured* w = inner_diff.as_structured_ptr();
							uint64_t sz_w = w->get_data_size();

							for (uint64_t t = 0; t < sz_w; ++t) {
								Value temp = std::move(w->get_value_list(t));
								j->add_object(Ptr<Structured>(temp.as_structured_ptr()));
							}

							clean(inner_diff);
						}
					}
					else {
						Object* obj = new Object();

						obj->add_object_element(Value("op"sv), Value("remove"sv));
						obj->add_object_element(Value("path"sv), Value(route));
						obj->add_object_element(Value("last_key"sv), Value(key));

						j->add_object(Ptr<Structured>(obj));
					}
				}

				for (uint64_t i = 0; i < sz_y; ++i) {
					std::string key = jy->get_key_list(i).str_val();
					uint64_t idx = jx->find_(key);
					if (idx == Structured::npos) {
						Object* obj = new Object();

						obj->add_object_element(Value("op"sv), Value("add"sv));
						obj->add_object_element(Value("path"sv), Value(route));
						obj->add_object_element(Value("key"sv), Value(jy->get_key_list(i).clone()));
						obj->add_object_element(Value("value"sv), Value(jy->get_value_list(i).clone()));

						j->add_object(Ptr<Structured>(obj));
					}
				}
			}
		}
		break;

		case ValueType::BOOL:
		case ValueType::NULL_:
		case ValueType::FLOAT:
		case ValueType::INT:
		case ValueType::UINT:
		case ValueType::STRING:
		{
			Object* obj = new Object();

			obj->add_object_element(Value("op"sv), Value("replace"sv));
			obj->add_object_element(Value("path"sv), Value(route));
			obj->add_object_element(Value("value"sv), Value(y.clone()));

			j->add_object(Ptr<Structured>(obj));
			break;
		}
		}

		return result;
	}


	Value diff(const Value& x, const Value& y) {
		return _diff(x, y, "");
	}

	Value patch(const Value& x, const Value& diff) {
		Value unvalid_data(nullptr, false);

		const Structured* j_diff = diff.as_structured_ptr();

		if (!j_diff->is_array()) {
			return unvalid_data;
		}

		Value result = x.clone();

		uint64_t sz_diff = j_diff->get_data_size();

		for (uint64_t i = 0; i < sz_diff; ++i) {
			const Object* obj = (const Object*)j_diff->get_value_list(i).as_structured_ptr();
			uint64_t op_idx = obj->find_("op"sv);
			uint64_t path_idx = obj->find_("path"sv);
			uint64_t value_idx = obj->find_("value"sv);
			uint64_t key_idx = obj->find_("key"sv);

			if (op_idx == Structured::npos) {
				clean(result);
				return unvalid_data;
			}

			if (path_idx == Structured::npos) {
				clean(result);
				return unvalid_data;
			}

			if (obj->get_value_list(op_idx).str_val() == "replace"sv) {
				if (value_idx == Structured::npos) {
					clean(result);
					return unvalid_data;
				}

				Value& value = result.json_pointer(obj->get_value_list(path_idx));
				{
					if (value.is_structured()) {
						clean(value);
						value.clear(false);
					}
				}
				value = obj->get_value_list(value_idx).clone();
			}
			else if (obj->get_value_list(op_idx).str_val() == "remove"sv) {
				Structured* parent = result.json_pointer(obj->get_value_list(path_idx)).as_structured_ptr();

				// case : result.json_pointer returns root?
				if (!parent) {
					if (result.is_structured()) {
						clean(result);
					}
					result.clear(false);
				}
				else if (parent->is_array()) {
					uint64_t last_idx_idx = obj->find_("last_idx"sv);
					if (last_idx_idx == Structured::npos) {
						clean(result);
						return unvalid_data;
					}

					uint64_t last_idx = obj->get_value_list(last_idx_idx).uint_val();

					delete parent->get_value_list(last_idx).as_structured_ptr(); // chk!
					parent->erase(last_idx);
				}
				else {
					uint64_t last_key_idx = obj->find_("last_key"sv);
					if (last_key_idx == Structured::npos) {
						clean(result);
						return unvalid_data;
					}

					std::string last_key = obj->get_value_list(last_key_idx).str_val();
					uint64_t _idx = parent->find_(last_key);
					delete parent->get_value_list(_idx).as_structured_ptr();
					parent->erase(_idx);
				}
			}
			else if (obj->get_value_list(op_idx).str_val() == "add"sv) {
				if (value_idx == Structured::npos) {
					clean(result);
					return unvalid_data;
				}

				Value& _ = result.json_pointer(obj->get_value_list(path_idx));

				Structured* parent = _.as_structured_ptr();

				// case : result.json_pointer returns root?
				if (!parent) {
					result = obj->get_value_list(value_idx).clone();
				}
				else if (parent->is_array()) {
					parent->add_array_element(obj->get_value_list(value_idx).clone());
				}
				else if (parent->is_object()) {
					if (key_idx == Structured::npos) {
						clean(result);
						return unvalid_data;
					}
					parent->add_object_element(obj->get_value_list(key_idx).clone(), obj->get_value_list(value_idx).clone());
				}
			}
		}

		return result;
	}

	void clean(Value& x) {
		if (x.is_structured()) {
			delete x.as_structured_ptr();
			x.set_null();
		}
	}


	void init(int thr_num) {
		if (!simdjson_imple) {
			log.no_print();

			StringView str = "{}"sv;

			auto x = test_.parse(str.data(), str.length());
			simdjson_imple = test_.raw_implementation().get();
		}

		if (!pool) {
			if (thr_num <= 0) {
				thr_num = std::max((int)std::thread::hardware_concurrency() - 2, 1);

			}
			if (thr_num <= 0) {
				thr_num = 1;
			}

			pool = std::make_unique<ThreadPool>(thr_num);
		}
	}


	bool is_valid_string_in_json(StringView x) {
		const char* str = x.data();
		uint64_t len = x.size();
		const uint64_t block_size = 1024;


		uint8_t buf_src[block_size + _simdjson::_SIMDJSON_PADDING];
		uint8_t buf_dest[block_size + _simdjson::_SIMDJSON_PADDING];


		if (len >= block_size) {
			uint8_t* buf_src = (uint8_t*)calloc(len + 1 + _simdjson::_SIMDJSON_PADDING, sizeof(uint8_t));
			uint8_t* buf_dest = (uint8_t*)calloc(len + 1 + _simdjson::_SIMDJSON_PADDING, sizeof(uint8_t));
			if (!buf_src || !buf_dest) {
				if (buf_src) { free(buf_src); }
				if (buf_dest) { free(buf_dest); }

				return false;
			}
			memset(buf_src, '"', len + 1 + _simdjson::_SIMDJSON_PADDING);
			memset(buf_dest, '"', len + 1 + _simdjson::_SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			// chk... fallback..
			{
				bool valid = _simdjson::String::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					free(buf_src);
					free(buf_dest);

					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return false;
				}
			}
			auto* x = simdjson_imple->parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				free(buf_src);
				free(buf_dest);

				log << warn << "Error in Convert for string";
				return false;
			}
			else {
				//
			}

			free(buf_src);
			free(buf_dest);
		}
		else {
			memset(buf_src, '"', block_size + _simdjson::_SIMDJSON_PADDING);
			memset(buf_dest, '"', block_size + _simdjson::_SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			{
				bool valid = _simdjson::String::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return false;
				}
			}
			auto* x = simdjson_imple->parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				log << warn << "Error in Convert for string";
				return false;
			}
			else {
				//
			}
		}

		return true;
	}

	std::pair<bool, std::string> convert_to_string_in_json(StringView x) {
		const char* str = x.data();
		uint64_t len = x.size();
		const uint64_t block_size = 1024;


		uint8_t buf_src[block_size + _simdjson::_SIMDJSON_PADDING];
		uint8_t buf_dest[block_size + _simdjson::_SIMDJSON_PADDING];

		uint64_t string_length = 0;

		if (len >= block_size) {
			uint8_t* buf_src = (uint8_t*)calloc(len + 1 + _simdjson::_SIMDJSON_PADDING, sizeof(uint8_t));
			uint8_t* buf_dest = (uint8_t*)calloc(len + 1 + _simdjson::_SIMDJSON_PADDING, sizeof(uint8_t));
			if (!buf_src || !buf_dest) {
				if (buf_src) { free(buf_src); }
				if (buf_dest) { free(buf_dest); }

				return { false, "" };
			}
			memset(buf_src, '"', len + 1 + _simdjson::_SIMDJSON_PADDING);
			memset(buf_dest, '"', len + 1 + _simdjson::_SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			// chk... fallback..
			{
				bool valid = _simdjson::String::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					free(buf_src);
					free(buf_dest);

					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return { false, "" };
				}
			}
			auto* x = simdjson_imple->parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				free(buf_src);
				free(buf_dest);

				log << warn << "Error in Convert for string";
				return { false, "" };
			}
			else {
				*x = '\0';
				string_length = uint32_t(x - buf_dest);
			}

			std::string result(reinterpret_cast<char*>(buf_dest), string_length);

			free(buf_src);
			free(buf_dest);

			return { true, result };
		}
		else {
			memset(buf_src, '"', block_size + _simdjson::_SIMDJSON_PADDING);
			memset(buf_dest, '"', block_size + _simdjson::_SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			{
				bool valid = _simdjson::String::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return { false, "" };
				}
			}
			auto* x = simdjson_imple->parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				log << warn << "Error in Convert for string";
				return { false, "" };
			}
			else {
				*x = '\0';
				string_length = uint32_t(x - buf_dest);
			}

			return { true, std::string(reinterpret_cast<char*>(buf_dest), string_length) };
		}

	}

#if __cplusplus >= 202002L
	bool is_valid_string_in_json(std::u8string_view x) {
		return is_valid_string_in_json(StringView((const char*)x.data(), x.size()));
	}

	std::pair<bool, std::string> convert_to_string_in_json(std::u8string_view x) {
		return convert_to_string_in_json(StringView(reinterpret_cast<const char*>(x.data()), x.size()));
	}
#endif

}

