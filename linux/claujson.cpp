
#include <cstdio>

#include "claujson.h"

#include "simdjson.h" // modified simdjson // using simdjson 2.2.2 

// for fast save
//#include "fmt/format.h"
//#include "fmt/compile.h"

#include <string_view>
using namespace std::string_view_literals;


namespace claujson {
	
	inline static _simdjson::dom::parser_for_claujson test_;
	inline static _simdjson::internal::dom_parser_implementation* simdjson_imple = nullptr;
	
	// class PartialJson, only used in class LoadData.
	class PartialJson : public Json {
	protected:
		std::vector<Data> arr_vec; 
		//
		std::vector<Data> obj_key_vec;
		std::vector<Data> obj_val_vec;

		Data virtualJson;
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

		virtual size_t get_data_size() const;

		virtual Data& get_value_list(size_t idx);


		virtual Data& get_key_list(size_t idx);


		virtual const Data& get_value_list(size_t idx) const;


		virtual const Data& get_key_list(size_t idx) const;

		virtual void clear(size_t idx);

		virtual bool is_virtual() const;

		virtual void clear();

		virtual void reserve_data_list(size_t len);



		virtual bool add_object_element(Data key, Data val);
		virtual bool add_array_element(Data val);

		virtual bool add_array(Ptr<Json> arr);
		virtual bool add_object(Ptr<Json> obj);

		virtual bool insert_array_element(size_t idx, Data val);

		virtual void erase(std::string_view key, bool real = false);

		virtual void erase(size_t idx, bool real = false);


	private:

		virtual void MergeWith(PtrWeak<Json> j, int start_offset);

		virtual void Link(Ptr<Json> j);

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint8_t* string_buf, uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint8_t* string_buf, uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			uint8_t* string_buf, int ut_type, uint64_t key_token_idx);

		virtual void add_user_type(int type);

		virtual bool add_user_type(Ptr<Json> j);


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



	std::ostream& operator<<(std::ostream& stream, const claujson::Data& data) {

		if (false == data.is_valid()) {
			stream << "--not valid\n";
			return stream;
		}

		switch (data._type) {
		case claujson::DataType::INT:
			stream << data._int_val;
			break;
		case claujson::DataType::UINT:
			stream << data._uint_val;
			break;
		case claujson::DataType::FLOAT:
			stream << data._float_val;
			break;
		case claujson::DataType::STRING:
			stream << "\"" << (*data._str_val) << "\"";
			break;
		case claujson::DataType::BOOL:
			stream << data._bool_val;
			break;
		case claujson::DataType::NULL_:
			stream << "null";
			break;
		case claujson::DataType::ARRAY_OR_OBJECT:
		{
		//	stream << "array_or_object";
			auto* x = data.as_json_ptr();
			if (x->is_array()) {
				stream << "[ ";
				size_t sz = x->get_data_size();
				for (size_t i = 0; i < sz; ++i) {
					stream << x->get_value_list(i) << " ";
					if (i < sz - 1) {
						stream << " , ";
					}
				}
				stream << "]\n";
			}
			else if (x->is_object()) {
				stream << "{ ";
				size_t sz = x->get_data_size();
				for (size_t i = 0; i < sz; ++i) {
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


	Data Data::clone() const {
		if (!is_valid()) {
			return Data(nullptr, false);
		}

		Data x;
		
		x._type = this->_type; 

		if (x._type == DataType::STRING) {
			x._str_val = new std::string(this->_str_val->data(), this->_str_val->size());

		}
		else {
			x._int_val = this->_int_val;
		}

		if (x.is_ptr()) {
			x._ptr_val = this->as_json_ptr()->clone();
		}

		return x;
	}
	Data::operator bool() const {
		return this->_valid;
	}

	/*
	Data::Data(const Data& other)
		: _type(other._type) 
	{
		if (_type == DataType::STRING) {
			_str_val = new std::string(other._str_val->data(), other._str_val->size());

		}
		else {
			_int_val = other._int_val;
		}
	}
	*/

	Data::Data(Json* x) {
		set_ptr(x);
	}
	Data::Data(int x) {
		set_int(x);
	}

	Data::Data(unsigned int x) {
		set_uint(x);
	}

	Data::Data(int64_t x) {
		set_int(x);
	}
	Data::Data(uint64_t x) {
		set_uint(x);
	}
	Data::Data(double x) {
		set_float(x);
	}
	Data::Data(std::string_view x) {
		if (!set_str(x.data(), x.size())) {
			this->_valid = false;
		}
	}
	//explicit Data(const char* x) {
	//	set_str(x, strlen(x));
	//}
	// C++20~
	//explicit Data(const char8_t* x) {
	//	set_str((const char*)x, strlen((const char*)x));
	//}
	Data::Data(bool x) {
		set_bool(x);
	}
	Data::Data(nullptr_t x) {
		set_type(DataType::NULL_);
	}

	Data::Data(nullptr_t, bool valid) {
		set_type(DataType::NULL_);
		this->_valid = valid;
	}

	DataType Data::type() const {
		return _type;
	}

	bool Data::is_valid() const {
		return this->_valid;
	}

	bool Data::is_null() const {
		return is_valid() && type() == DataType::NULL_;
	}

	bool Data::is_primitive() const {
		return is_valid() && !is_ptr();
	}

	bool Data::is_structured() const {
		return is_valid() && is_ptr();
	}

	bool Data::is_int() const {
		return is_valid() && type() == DataType::INT;
	}

	bool Data::is_uint() const {
		return is_valid() && type() == DataType::UINT;
	}

	bool Data::is_float() const {
		return is_valid() && type() == DataType::FLOAT;
	}

	bool Data::is_bool() const {
		return is_valid() && (type() == DataType::BOOL);
	}

	bool Data::is_str() const {
		return is_valid() && type() == DataType::STRING;
	}

	bool Data::is_ptr() const {
		return is_valid() && (type() == DataType::ARRAY_OR_OBJECT);
	}

	int64_t Data::int_val() const {
		return _int_val;
	}

	uint64_t Data::uint_val() const {
		return _uint_val;
	}

	double Data::float_val() const {
		return _float_val;
	}

	int64_t& Data::int_val() {
		return _int_val;
	}

	uint64_t& Data::uint_val() {
		return _uint_val;
	}

	double& Data::float_val() {
		return _float_val;
	}

	bool Data::bool_val() const {
		if (!is_bool()) {
			return false;
		}
		return _bool_val;
	}

	Json* Data::ptr_val() const {
		if (!is_ptr()) {
			return nullptr;
		}
		return _ptr_val;
	}


	inline std::string_view sub_route(std::string_view route, size_t found_idx, size_t new_idx) {
		if (found_idx + 1 == new_idx) {
			return ""sv;
		}
		return route.substr(found_idx + 1, new_idx - found_idx - 1);
	}


	bool to_uint_for_json_pointer(std::string_view x, size_t* val, _simdjson::internal::dom_parser_implementation* simdjson_imple) {
		const char* buf = x.data();
		size_t idx = 0;
		size_t idx2 = x.size();

		switch (x[0]) {
			case '0':
			case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			{
				std::unique_ptr<uint8_t[]> copy;

				uint64_t temp[2] = { 0 };

				const uint8_t* value = reinterpret_cast<const uint8_t*>(buf + idx);

				{ // chk code...
					copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[idx2 - idx + _simdjson::SIMDJSON_PADDING]); // x.size() + padding
					if (copy.get() == nullptr) { return false; } // cf) new Json?
					std::memcpy(copy.get(), &buf[idx], idx2 - idx);
					std::memset(copy.get() + idx2 - idx, ' ', _simdjson::SIMDJSON_PADDING);
					value = copy.get();
				}

				if (auto x = simdjson_imple->parse_number(value, temp)
					; x != _simdjson::SUCCESS) {
					log << warn  << "parse number error. " << x << "\n";
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

	bool Data::json_pointerA(std::string_view route, std::vector<Data>& vec) {
		std::vector<std::string_view> routeVec;
		std::vector<Data> routeDataVec;

			
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
		size_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != std::string_view::npos) {
			size_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == std::string_view::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else { ... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Data temp(x); // do 2, 3.

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

	Data& Data::json_pointerB(const std::vector<Data>& routeDataVec) { // option-> std::string_view route?
		static Data unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (routeDataVec.empty()) {
			return *this;
		}

		// 4. find Data with route. and return
		Data* data = this;

		for (size_t i = 0; i < routeDataVec.size(); ++i) {
			const Data& x = routeDataVec[i];

			if (data->is_primitive()) {
				if (i == routeDataVec.size() - 1) {
					return *data;
				}
				else {
					return unvalid_data;
				}
			}

			Json* j = data->as_json_ptr();

			if (j->is_array()) { // array -> with idx
				size_t idx = 0;
				//bool found = false;
				//size_t arr_size = j->get_data_size();

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx, simdjson_imple);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				std::string_view str = x.str_val();
				std::string result(str);

				size_t count = 0;

				// chk ~0 -> ~, ~1 -> /
				size_t idx = 0;

				idx = str.find('~');

				while (idx != std::string::npos) {
					size_t k = idx;

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
				data = &j->at(result);
			}
		}

		return *data;
	}


	// think.. Data vs Data& vs Data* ? 
	// race condition..? not support multi-thread  access...
	Data& Data::json_pointer(std::string_view route) {
		static Data unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}
		
		// the whole document.
		if (route.empty()) {
			return *this;
		}

		std::vector<std::string_view> routeVec;
		std::vector<Data> routeDataVec;

		routeVec.reserve(route.size());
		routeDataVec.reserve(route.size());

		if (route[0] != '/') {
			return unvalid_data;
		}

		// 1. route -> split with '/'  to routeVec.
		size_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != std::string_view::npos) {
			size_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == std::string_view::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else { ... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Data temp(x); 
			
			if (temp.is_valid()) {
				routeDataVec.push_back(std::move(temp));
			}
			else {
				return unvalid_data;
			}
		}

		// 4. find Data with route. and return
		Data* data = this;

		for (size_t i = 0; i < routeDataVec.size(); ++i) {
			Data& x = routeDataVec[i];

			if (data->is_primitive()) {
				if (i == routeDataVec.size() - 1) {
					return *data;
				}
				else {
					return unvalid_data;
				}
			}

			Json* j = data->as_json_ptr();

			if (j->is_array()) { // array -> with idx
				size_t idx = 0;
				bool found = false;
				size_t arr_size = j->get_data_size();

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx, simdjson_imple);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				std::string_view str = x.str_val();
				std::string result;

				result.reserve(str.size());

				// chk ~0 -> ~, ~1 -> /
				for (size_t k = 0; k < str.size(); ++k) {
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

				data = &j->at(result);
			}
		}

		return *data;
	}
	const Data& Data::json_pointer(std::string_view route) const {
		static const Data unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (route.empty()) {
			return *this;
		}

		std::vector<std::string_view> routeVec;
		std::vector<Data> routeDataVec;

		routeVec.reserve(route.size());
		routeDataVec.reserve(route.size());

		if (route[0] != '/') {
			return unvalid_data;
		}

		// 1. route -> split with '/'  to routeVec.
		size_t found_idx = 0; // first found_idx is 0, found '/'

		while (found_idx != std::string_view::npos) {
			size_t new_idx = route.find('/', found_idx + 1);

			if (new_idx == std::string_view::npos) {
				routeVec.push_back(sub_route(route, found_idx, route.size()));
				break;
			}
			// else { ... }
			routeVec.push_back(sub_route(route, found_idx, new_idx));

			found_idx = new_idx;
		}

		// 2. using simdjson util, check utf-8 for string in routeVec.
		// 3. using simdjson util, check valid for string in routeVec.

		for (auto& x : routeVec) {
			Data temp(x);

			if (temp.is_valid()) {
				routeDataVec.push_back(std::move(temp));
			}
			else {
				return unvalid_data;
			}
		}

		// 4. find Data with route. and return
		const Data* data = this;

		for (size_t i = 0; i < routeDataVec.size(); ++i) {
			Data& x = routeDataVec[i];

			if (data->is_primitive()) {
				if (i == routeDataVec.size() - 1) {
					return *data;
				}
				else {
					return unvalid_data;
				}
			}

			const Json* j = data->as_json_ptr();

			if (j->is_array()) { // array -> with idx
				size_t idx = 0;
				//bool found = false;
				//size_t arr_size = j->get_data_size();

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx, simdjson_imple);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				std::string_view str = x.str_val();
				std::string result;

				result.reserve(str.size());

				// chk ~0 -> ~, ~1 -> /
				for (size_t k = 0; k < str.size(); ++k) {
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

				data = &j->at(result);
			}
		}

		return *data;
	}



	void Data::clear() {

		if (_type == DataType::STRING) {
			delete _str_val; _str_val = nullptr;
		}
		else {
			_int_val = 0;
		}

		//valid = true;

		_type = DataType::NONE;
	}

	std::string& Data::str_val() {
		// type check...
		return *_str_val;
	}

	const std::string& Data::str_val() const {
		// type check...
		return *_str_val;
	}

	void Data::set_ptr(Json* x) {
		if (!is_valid()) {
			return;
		}

		if (_type == DataType::STRING) {
			delete _str_val;
		}

		_ptr_val = x;

		_type = DataType::ARRAY_OR_OBJECT; // chk change DataType:: ~~ -> DataType:: ~~
	}
	void Data::set_int(long long x) {
		if (!is_valid()) {
			return;
		}

		if (_type == DataType::STRING) {
			delete _str_val;
		}
		_int_val = x;
		_type = DataType::INT;
	}

	void Data::set_uint(unsigned long long x) {
		if (!is_valid()) {
			return;
		}
		if (_type == DataType::STRING) {
			delete _str_val;
		}
		_uint_val = x;
		_type = DataType::UINT;
	}

	void Data::set_float(double x) {
		if (!is_valid()) {
			return;
		}
		if (_type == DataType::STRING) {
			delete _str_val;
		}
		_float_val = x;

		_type = DataType::FLOAT;
	}

	bool Data::set_str(const char* str, size_t len) {
		if (!is_valid()) {
			return false;
		}

		const size_t block_size = 1024;


		uint8_t buf_src[block_size + _simdjson::SIMDJSON_PADDING];
		uint8_t buf_dest[block_size + _simdjson::SIMDJSON_PADDING];


		if (len >= block_size) {
			uint8_t* buf_src = (uint8_t*)calloc(len + _simdjson::SIMDJSON_PADDING, sizeof(uint8_t));
			uint8_t* buf_dest = (uint8_t*)calloc(len + _simdjson::SIMDJSON_PADDING, sizeof(uint8_t));
			if (!buf_src || !buf_dest) {
				if (buf_src) { free(buf_src); }
				if (buf_dest) { free(buf_dest); }

				return false;
			}
			memset(buf_src, '"', len + _simdjson::SIMDJSON_PADDING);
			memset(buf_dest, '"', len + _simdjson::SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			// chk... fallback..
			{
				bool valid = _simdjson::validate_utf8(reinterpret_cast<char*>(buf_src), len); 

				if (!valid) {
					free(buf_src);
					free(buf_dest);

					log << warn  << "not valid utf8" << "\n";
					log << warn  << "Error in Convert for string, validate...";
					return false;
				}
			}

			if (auto* x = simdjson_imple->parse_string(buf_src, buf_dest); x == nullptr) {
				free(buf_src);
				free(buf_dest);

				log << warn  << "Error in Convert for string";
				return false;
			}
			else {
				*x = '\0';
				size_t string_length = uint32_t(x - buf_dest);

				if (_type != DataType::STRING) {
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
			memset(buf_src, '"', block_size + _simdjson::SIMDJSON_PADDING);
			memset(buf_dest, '"', block_size + _simdjson::SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			{
				bool valid = _simdjson::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					log << warn  << "not valid utf8" << "\n";
					log << warn  << "Error in Convert for string, validate...";
					return false;
				}
			}

			if (auto* x = simdjson_imple->parse_string(buf_src, buf_dest); x == nullptr) {
				log << warn  << "Error in Convert for string";
				return false;
			}
			else {
				*x = '\0';
				size_t string_length = uint32_t(x - buf_dest);

				if (_type != DataType::STRING) {
					_str_val = new std::string((char*)buf_dest, string_length);
				}
				else {
					_str_val->assign((char*)buf_dest, string_length);
				}
			}
		}

		_type = DataType::STRING;

		return true;
	}

	void Data::set_str_in_parse(const char* str, size_t len) {
		if (_type != DataType::STRING) {
			_str_val = new std::string(str, len);
		}
		else {
			_str_val->assign(str, len);
		}
		_type = DataType::STRING;
	}

	void Data::set_bool(bool x) {
		if (!is_valid()) {
			return;
		}
		if (_type == DataType::STRING) {
			delete _str_val;
		}

		_bool_val = x;

		{
			set_type(DataType::BOOL);
		}
	}

	void Data::set_null() {
		if (!is_valid()) {
			return;
		}
		if (_type == DataType::STRING) {
			delete _str_val;
		}

		set_type(DataType::NULL_);
	}

	void Data::set_type(DataType type) {
		this->_type = type;
	}
		
	Data::~Data() {
		if (_type == DataType::STRING && _str_val) {
			//log << warn  << "chk";
			delete _str_val;
			_str_val = nullptr;
		}
	}

	Data::Data(Data&& other) noexcept
		: _type(other._type), _valid(other._valid)
	{
		if (!other.is_valid()) {
			return;
		}

		if (_type == DataType::STRING) {
			_str_val = other._str_val;
			other._str_val = nullptr;
			other._type = DataType::NONE;
		}
		else {
			std::swap(_int_val, other._int_val);
		}

		clean(other);
	}

	Data::Data() : _int_val(0), _valid(true), _type(DataType::NONE) { }

	bool Data::operator==(const Data& other) const { // chk array or object?
		if (this->_type == other._type) {
			switch (this->_type) {
			case DataType::STRING:
				return (*this->_str_val) == (*other._str_val);
				break;
			case DataType::INT:
				return this->_int_val == other._int_val;
				break;
			case DataType::UINT:
				return this->_uint_val == other._uint_val;
				break;
			case DataType::FLOAT:
				return this->_float_val == other._float_val;
				break;
			case DataType::BOOL:
				return this->_bool_val == other._bool_val;
				break;
			case DataType::ARRAY_OR_OBJECT:
			{
				const Json* j = this->as_json_ptr();
				const Json* k = other.as_json_ptr();

				const size_t sz_j = j->get_data_size();
				const size_t sz_k = k->get_data_size();

				if (sz_j != sz_k) { return false; }

				const size_t sz = sz_j;

				if (j->is_array() && k->is_array()) {
					for (size_t i = 0; i < sz; ++i) {
						if (j->get_value_list(i) != k->get_value_list(i)) {
							return false;
						}
					}
				}
				else if (j->is_object() && k->is_object()) {
					for (size_t i = 0; i < sz; ++i) {
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

	bool Data::operator!=(const Data& other) const {
		return !((*this) == other);
	}

	bool Data::operator<(const Data& other) const {
		if (this->_type == other._type) {
			switch (this->_type) {
			case DataType::STRING:
				return (*this->_str_val) < (*other._str_val);
				break;
			case DataType::INT:
				return this->_int_val < other._int_val;
				break;
			case DataType::UINT:
				return this->_uint_val < other._uint_val;
				break;
			case DataType::FLOAT:
				return this->_float_val < other._float_val;
				break;
			case DataType::BOOL:
				return this->_bool_val < other._bool_val;
				break;
			}
		
		}
		return false;
	}


	Data& Data::operator=(Data&& other) noexcept {
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


	inline claujson::Data& Convert(claujson::Data& data, uint64_t buf_idx, uint64_t next_buf_idx, bool key,
		char* buf, uint8_t* string_buf, uint64_t token_idx, bool& err) {

		try {
			data.clear();

			uint32_t string_length;

			switch (buf[buf_idx]) {
			case '"':
			{
				if (auto* x = simdjson_imple->parse_string((uint8_t*)&buf[buf_idx] + 1,
					&string_buf[buf_idx]); x == nullptr) {
					ERROR("Error in Convert for string");
				}
				else {
					*x = '\0';
					string_length = uint32_t(x - &string_buf[buf_idx]);
				}

				// chk token_arr_start + i + 1 >= imple->n_structural_indexes...
				data.set_str_in_parse(reinterpret_cast<char*>(&string_buf[buf_idx]), string_length);
			}
			break;
			case 't':
			{
				if (!simdjson_imple->is_valid_true_atom(reinterpret_cast<uint8_t*>(&buf[buf_idx]), next_buf_idx - buf_idx)) {
					ERROR("Error in Convert for true");
				}

				data.set_bool(true);
			}
			break;
			case 'f':
				if (!simdjson_imple->is_valid_false_atom(reinterpret_cast<uint8_t*>(&buf[buf_idx]), next_buf_idx - buf_idx)) {
					ERROR("Error in Convert for false");
				}

				data.set_bool(false);
				break;
			case 'n':
				if (!simdjson_imple->is_valid_null_atom(reinterpret_cast<uint8_t*>(&buf[buf_idx]), next_buf_idx - buf_idx)) {
					ERROR("Error in Convert for null");
				}

				data.set_null();
				break;
			case '-':
			case '0':
			case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			{
				std::unique_ptr<uint8_t[]> copy;

				uint64_t temp[2] = { 0 };

				uint8_t* value = reinterpret_cast<uint8_t*>(buf + buf_idx);

				if (token_idx == 0) { // if this case may be root number -> chk.. visit_root_number. in tape_builder in simdjson.cpp
					copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[next_buf_idx - buf_idx + _simdjson::SIMDJSON_PADDING]);
					if (copy.get() == nullptr) { ERROR("Error in Convert for new"); } // cf) new Json?
					std::memcpy(copy.get(), &buf[buf_idx], next_buf_idx - buf_idx);
					std::memset(copy.get() + next_buf_idx - buf_idx, ' ', _simdjson::SIMDJSON_PADDING);
					value = copy.get();
				}

				if (auto x = simdjson_imple->parse_number(value, temp)
					; x != _simdjson::SUCCESS) {
					log << warn  << "parse number error. " << x << "\n";
					ERROR("parse number error. ");
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

				break;
			}
			default:
				log << warn  << "convert error : " << (int)buf[buf_idx] << " " << buf[buf_idx] << "\n";
				ERROR("convert Error");
				//throw "Error in Convert : not expected";
			}
			return data;
		}
		catch (const char* str) {
			log << warn  << str << "\n";
			//ERROR(str);
			err = true;
			return data;
		}
	}

	bool Json::is_valid() const {
		return valid;
	}


	Json* Json::clone() const {
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

	Json::Json(bool valid) : valid(valid) { }

	Json::Json() { }


		Json::~Json() {

		}



		const Data& Json::at(std::string_view key) const {
			if (!is_object() || !is_valid()) {
				return data_null;
			}

			size_t len = get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (get_key_list(i).str_val().compare(key) == 0) {
					return get_value_list(i);
				}
			}

			return data_null;
		}

		Data& Json::at(std::string_view key) {
			if (!is_object() || !is_valid()) {
				return data_null;
			}

			size_t len = get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (get_key_list(i).str_val().compare(key) == 0) {
					return get_value_list(i);
				}
			}

			return data_null;
		}

		size_t Json::find(std::string_view key) const { // chk (uint64_t)-1 == (maximum)....-> eof?
			if (!is_object() || !is_valid()) {
				return -1;
			}

			size_t len = get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (get_key_list(i).str_val().compare(key) == 0) {
					return i;
				}
			}

			return -1;
		}


		Data& Json::operator[](size_t idx) {
			if (idx >= get_data_size() || !is_valid()) {
				return data_null;
			}
			return get_value_list(idx);
		}

		const Data& Json::operator[](size_t idx) const {
			if (idx >= get_data_size() || !is_valid()) {
				return data_null;
			}
			return get_value_list(idx);
		}

		bool Json::has_key() const {
			return key.is_str();
		}

		PtrWeak<Json> Json::get_parent() const {
			return parent;
		}


		const Data& Json::get_key() const {
			return key;
		}

		bool Json::set_key(Data key) {
			if (key.is_str()) {
				this->key = std::move(key);
				return true;
			}
			return false; // no change..
		}

		bool Json::change_key(const Data& key, const Data& new_key) { // chk test...
			if (this->is_object() && key.is_str() && new_key.is_str()) {
				auto idx = find(key.str_val());
				if (idx == -1) {
					return false;
				}

				get_key_list(idx) = new_key.clone();

				//auto val = std::move(get_value_list(idx));
				//erase(idx);
				//add_object_element(new_key.clone(), std::move(val));
				return true;
			}
			return false;
		}

		Data& Json::get_value() {
			return data_null;
		}

		bool Json::is_partial_json() const { return false; }

		bool Json::is_user_type() const {
			return is_object() || is_array() || is_partial_json();
		}

		void Json::set_parent(PtrWeak<Json> j) {
			parent = j;
		}


		Object::Object(bool valid) : Json(valid) { }


		class CompKey {
		private:
			const std::vector<Data>* vec;
		public:

			CompKey(const std::vector<Data>* vec) : vec(vec) {
				//
			}

			bool operator()(size_t x, size_t y) const {
				return (*vec)[x] < (*vec)[y];
			}
		};

		Json* Object::clone() const {
			Json* result = new Object();

			size_t sz = this->get_data_size();

			for (size_t i = 0; i < sz; ++i) {
				result->add_object_element(this->get_key_list(i).clone(), this->get_value_list(i).clone());
			}

			result->key = this->key.clone();

			return result;
		}

		bool Object::chk_key_dup(size_t* idx) const {
			bool has_dup = false;
			std::vector<size_t> copy_(obj_key_vec.size(), 0);

			for (size_t i = 0; i < copy_.size(); ++i) {
				copy_[i] = i;
			}

			CompKey comp(&obj_key_vec);

			std::stable_sort(copy_.begin(), copy_.end(), comp);

			for (size_t i = 1; i < copy_.size(); ++i) {
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

		Data Object::Make() {
			return Data(new Object());
		}

		Object::Object() { }

		Object::~Object() {
			for (auto& x : obj_val_vec) {
				if (x.is_ptr()) {
					delete ((Json*)x.ptr_val());
				}
			}
		}

		bool Object::is_object() const {
			return true;
		}
		bool Object::is_array() const {
			return false;
		}
		
		size_t Object::get_data_size() const {
			return obj_val_vec.size();
		}

		Data& Object::get_value_list(size_t idx) {
			return obj_val_vec[idx];
		}

		Data& Object::get_key_list(size_t idx) { // if key change then also obj_val_vec[idx].key? change??
			return obj_key_vec[idx];
		}


		const Data& Object::get_value_list(size_t idx) const {
			return obj_val_vec[idx];
		}

		const Data& Object::get_key_list(size_t idx) const {
			return obj_key_vec[idx];
		}

		void Object::clear(size_t idx) {
			obj_val_vec[idx].clear();
		}

		bool Object::is_virtual() const {
			return false;
		}

		void Object::clear() {
			obj_val_vec.clear();
			obj_key_vec.clear();
		}

		void Object::reserve_data_list(size_t len) {
			obj_val_vec.reserve(len);
			obj_key_vec.reserve(len);
		}

		bool Object::add_object_element(Data key, Data val) {
			if (!is_valid() || !key.is_str()) {
				return false;
			}

			if (val.is_ptr()) {
				auto* x = (Json*)val.ptr_val();
				x->set_key(key.clone()); // no need?
			}
			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(std::move(val));
			
			return true;
		}

		bool Object::add_array_element(Data val) { return false; }
		bool Object::add_array(Ptr<Json> arr) {
			if (!is_valid()) {
				return false;
			}

			if (arr->has_key()) {
				obj_key_vec.push_back(arr->get_key().clone());
				obj_val_vec.push_back(Data(arr.release()));
			}
			else {
				return false;
			}

			return true;
		}

		bool Object::add_object(Ptr<Json> obj) {
			if (!is_valid()) {
				return false;
			}

			if (obj->has_key()) {
				obj_key_vec.push_back(obj->get_key().clone());
				obj_val_vec.push_back(Data(obj.release()));
			}
			else {
				return false;
			}

			return true;
		}

		bool Object::insert_array_element(size_t idx, Data val) { return false; }

		void Object::erase(std::string_view key, bool real) {
			size_t idx = this->find(key);
			erase(idx, real);
		}

		void Object::erase(size_t idx, bool real) {
			if (!is_valid()) {
				return;
			}

			if (real) {
				clean(obj_key_vec[idx]);
				clean(obj_val_vec[idx]);
			}

			obj_key_vec.erase(obj_key_vec.begin() + idx);
			obj_val_vec.erase(obj_val_vec.begin() + idx);
		}


		void Object::MergeWith(PtrWeak<Json> j, int start_offset) {
			if (!is_valid()) {
				return;
			}

			if (j->is_object()) {
				auto* x = dynamic_cast<Object*>(j);

				size_t len = j->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (j->get_value_list(i).is_ptr()) {
						j->get_value_list(i).as_json_ptr()->set_parent(this);
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

				size_t len = j->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (j->get_value_list(i).is_ptr()) {
						j->get_value_list(i).as_json_ptr()->set_parent(this);
					}
				}

				if (x->virtualJson.is_ptr()) {
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

		void Object::Link(Ptr<Json> j) {
			if (!is_valid()) {
				return;
			}

			if (j->has_key()) {
				//
			}
			else {
				// error...
				log << warn  << "Link errr1";
				ERROR("Link Error");
				return;
			}

			j->set_parent(this);

			obj_key_vec.push_back(j->get_key().clone());
			obj_val_vec.push_back(Data(j.release()));
		}

		void Object::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint8_t* string_buf, uint64_t key_token_idx, uint64_t val_token_idx) {
			if (!is_valid()) {
				return;
			}
			{
				Data temp;// key
				Data temp2;

				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, string_buf, key_token_idx, e);

				if (e) {
					ERROR("Error in add_item_type");
				}
				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, string_buf, val_token_idx, e);
				if (e) {
					ERROR("Error in add_item_type");
				}

				if (temp.type() != DataType::STRING) {
					ERROR("Error in add_item_type, key is not string");
				}

				obj_key_vec.push_back(std::move(temp));
				obj_val_vec.push_back(std::move(temp2));
			}
		}

		void Object::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint8_t* string_buf, uint64_t val_token_idx) {
			// error

			log << warn  << "errr..";
			ERROR("Error Object::add_item_type");
		}

		void Object::add_user_type(int type) {
			// error

			log << warn  << "errr..";
			ERROR("Error Object::add_user_type");
			return;
		}

		bool Object::add_user_type(Ptr<Json> j) {
			if (!is_valid()) {
				return false;
			} 

			if (j->is_virtual()) {
				j->set_parent(this);

				obj_key_vec.push_back(Data());
				obj_val_vec.emplace_back(j.release());
			}
			else if (j->has_key()) {
				j->set_parent(this);

				obj_key_vec.push_back(j->get_key().clone());
				obj_val_vec.emplace_back(j.release());
			}
			else {
				log << warn  << "chk..";
				return false;
			}

			return true;
		}

		 Array::Array(bool valid) : Json(valid) { }


		 Json* Array::clone() const {
			 Json* result = new Array();

			 size_t sz = this->get_data_size();
			 for (size_t i = 0; i < sz; ++i) {
				 result->add_array_element(this->get_value_list(i).clone());
			 }

			 result->key = this->key.clone();

			 return result;
		 }

		Data Array::Make() {
			return Data(new Array());
		}

		Array::Array() { }

		Array::~Array() {
			for (auto& x : arr_vec) {
				if (x.is_ptr()) {
					delete ((Json*)x.ptr_val());
				}
			}
		}

		bool Array::is_object() const {
			return false;
		}
		bool Array::is_array() const {
			return true;
		}

		size_t Array::get_data_size() const {
			return arr_vec.size();
		}

		Data& Array::get_value_list(size_t idx) {
			return arr_vec[idx];
		}

		Data& Array::get_key_list(size_t idx) {
			return data_null;
		}

		const Data& Array::get_value_list(size_t idx) const {
			return arr_vec[idx];
		}

		const Data& Array::get_key_list(size_t idx) const {
			return data_null;
		}

		void Array::clear(size_t idx) {
			arr_vec[idx].clear();
		}

		bool Array::is_virtual() const {
			return false;
		}
		void Array::clear() {
			arr_vec.clear();
		}

		void Array::reserve_data_list(size_t len) {
			arr_vec.reserve(len);
		}


		std::vector<Data>::iterator Array::begin() {
			return arr_vec.begin();
		}

		std::vector<Data>::iterator Array::end() {
			return arr_vec.end();
		}

		bool Array::add_object_element(Data key, Data val) {
			return false;
		}

		bool Array::add_array_element(Data val) {
			if (!is_valid()) {
				return false;
			}

			arr_vec.push_back(std::move(val));

			return true;
		}
		
		bool Array::add_array(Ptr<Json> arr) {
			if (!is_valid()) {
				return false;
			}

			if (!arr->has_key()) {
				arr_vec.push_back(Data(arr.release()));
			}
			else {
				return false;
			}
			return true;
		}
		
		bool Array::add_object(Ptr<Json> obj) {
			if (!is_valid()) {
				return false;
			}

			if (!obj->has_key()) {
				arr_vec.push_back(Data(obj.release()));
			}
			else {
				return false;
			}
			return true;
		}

		bool Array::insert_array_element(size_t idx, Data val) {
			if (!is_valid()) {
				return false;
			}

			arr_vec.insert(arr_vec.begin() + idx, std::move(val));

			return true;
		}

		void Array::erase(std::string_view key, bool real) {
			size_t idx = this->find(key);
			erase(idx, real);
		}

		void Array::erase(size_t idx, bool real) {
			if (!is_valid()) {
				return;
			}

			if (real) {
				clean(arr_vec[idx]);
			}

			arr_vec.erase(arr_vec.begin() + idx);
		}
		
		void Array::MergeWith(PtrWeak<Json> j, int start_offset) {
			if (!is_valid()) {
				return;
			}

			if (j->is_array()) {
				auto* x = dynamic_cast<Array*>(j);

				size_t len = j->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (j->get_value_list(i).is_ptr()) {
						j->get_value_list(i).as_json_ptr()->set_parent(this);
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

				size_t len = j->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (j->get_value_list(i).is_ptr()) {
						j->get_value_list(i).as_json_ptr()->set_parent(this);
					}
				}


				if (x->virtualJson.is_ptr()) {
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

		void Array::Link(Ptr<Json> j) {
			if (!is_valid()) {
				return;
			}

			if (!j->has_key()) {
				//
			}
			else {
				// error...

				log << warn  << "Link error2";
				ERROR("Link Error");
				return;
			}

			j->set_parent(this);

			arr_vec.push_back(Data(j.release()));
		}

		void Array::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint8_t* string_buf, uint64_t key_token_idx, uint64_t val_token_idx) {

			// error
			log << warn  << "error..";
			ERROR("Error Array::add_item_type");
		}

		void Array::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint8_t* string_buf, uint64_t val_token_idx) {
			if (!is_valid()) {
				return;
			}


			{
				Data temp2;
				bool e = false;
				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, true, buf, string_buf, val_token_idx, e);
				if (e) {

					ERROR("Error in add_item_type");
				}
				arr_vec.push_back(std::move(temp2));
			}
		}

		void Array::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			uint8_t* string_buf, int type, uint64_t key_token_idx) {
			log << warn  << "error";
			ERROR("Array::add_user_type1");
		}

		bool Array::add_user_type(Ptr<Json> j) {
			if (!is_valid()) {
				return false;
			}


			if (j->is_virtual()) {
				j->set_parent(this);
				arr_vec.emplace_back(j.release());
			}
			else if (j->has_key() == false) {
				j->set_parent(this);
				arr_vec.emplace_back(j.release());
			}
			else {
				// error..
				log << warn  << "error..";
				return false;
			}

			return true;
		}

	// class PartialJson, only used in class LoadData2.
		// todo - rename? PartialNode ?
	
		PartialJson::~PartialJson() {
			for (auto& x : obj_val_vec) {
				if (x.is_ptr()) {
					delete ((Json*)x.ptr_val());
				}
			}

			for (auto& x : arr_vec) {
				if (x.is_ptr()) {
					delete ((Json*)x.ptr_val());
				}
			}

			if (virtualJson.is_ptr()) {
				delete ((Json*)virtualJson.ptr_val());
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
		
		size_t PartialJson::get_data_size() const {
			int count = 0;

			if (virtualJson.is_ptr()) {
				count = 1;
			}

			return arr_vec.size() + obj_val_vec.size() + count;
		}

		Data& PartialJson::get_value_list(size_t idx) {

			if (virtualJson.is_ptr() && idx == 0) {
				return virtualJson;
			}
			if (virtualJson.is_ptr()) {
				--idx;
			}

			if (!arr_vec.empty()) {
				return arr_vec[idx];
			}
			else {
				return obj_val_vec[idx];
			}
		}


		Data& PartialJson::get_key_list(size_t idx) {
			if (virtualJson.is_ptr() && idx == 0) {
				return data_null;
			}
			if (virtualJson.is_ptr()) {
				--idx;
			}

			if (!arr_vec.empty()) {
				return data_null;
			}
			else {
				return obj_key_vec[idx];
			}
		}


		const Data& PartialJson::get_value_list(size_t idx) const {
			if (virtualJson.is_ptr() && idx == 0) {
				return virtualJson;
			}
			if (virtualJson.is_ptr()) {
				--idx;
			}

			if (!arr_vec.empty()) {
				return arr_vec[idx];
			}
			else {
				return obj_val_vec[idx];
			}
		}


		const Data& PartialJson::get_key_list(size_t idx) const {
			if (virtualJson.is_ptr() && idx == 0) {
				return data_null;
			}
			if (virtualJson.is_ptr()) {
				--idx;
			}

			if (!arr_vec.empty()) {
				return data_null;
			}
			else {
				return obj_key_vec[idx];
			}
		}


		void PartialJson::clear(size_t idx) { // use carefully..
			if (virtualJson.is_ptr() && idx == 0) {
				virtualJson.clear();
				return;
			}
			if (virtualJson.is_ptr()) {
				--idx;
			}
			if (!arr_vec.empty()) {
				arr_vec[idx].clear();
			}
			else {
				obj_val_vec[idx].clear();
			}
		}

		bool PartialJson::is_virtual() const {
			return false;
		}
		
		void PartialJson::clear() {
			arr_vec.clear();
			obj_key_vec.clear();
			obj_val_vec.clear();
			virtualJson.clear();
		}

		void PartialJson::reserve_data_list(size_t len) {
			if (!arr_vec.empty()) {
				arr_vec.reserve(len);
			}
			if (!obj_val_vec.empty()) {
				obj_val_vec.reserve(len);
				obj_key_vec.reserve(len);
			}
		}



		bool PartialJson::add_object_element(Data key, Data val) {
			if (!is_valid() || !key.is_str()) {
				return false;
			}
			if (!arr_vec.empty()) {
				ERROR("partialJson is array or object.");
				return false;
			}

			if (val.is_ptr()) {
				auto* x = (Json*)val.ptr_val();
				x->set_key(key.clone()); // no need?
			}

			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(std::move(val));

			return true;
		}
		bool PartialJson::add_array_element(Data val) {
			if (!obj_key_vec.empty()) {
				ERROR("partialJson is array or object.");
				return false;
			}

			arr_vec.push_back(std::move(val));

			return true;
		}


		bool PartialJson::add_array(Ptr<Json> arr) {
			log << warn  << "not used..";
			ERROR("NOT USED");
			return false;
		}
		bool PartialJson::add_object(Ptr<Json> obj) {
			log << warn  << "not used..";
			ERROR("NOT USED");
			return false;
		}
		bool PartialJson::insert_array_element(size_t idx, Data val) {
			log << warn  << "not used.."; 
			ERROR("NOT USED");
			return false;
		}

		void PartialJson::erase(std::string_view key, bool real) {
			log << warn  << "not used..";
			ERROR("NOT USED");
		}

		void PartialJson::erase(size_t idx, bool real) {
			log << warn  << "not used..";
			ERROR("NOT USED");
		}

		void PartialJson::MergeWith(PtrWeak<Json> j, int start_offset) {
			if (!is_valid()) {
				return;
			}

			if (j->is_array()) {
				auto* x = dynamic_cast<Array*>(j);

				size_t len = j->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (j->get_value_list(i).is_ptr()) {
						j->get_value_list(i).as_json_ptr()->set_parent(this);
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

				size_t len = j->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (j->get_value_list(i).is_ptr()) {
						j->get_value_list(i).as_json_ptr()->set_parent(this);
					}
				}

				if (x->virtualJson.is_ptr()) {
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

		void PartialJson::Link(Ptr<Json> j) { // use carefully...

			j->set_parent(this);

			if (!j->has_key()) {
				arr_vec.push_back(Data(j.release()));
			}
			else {
				obj_key_vec.push_back(j->get_key().clone());
				obj_val_vec.push_back(Data(j.release()));
			}
		}


		void PartialJson::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx, 
			char* buf, uint8_t* string_buf, uint64_t key_token_idx, uint64_t val_token_idx) {

				{
					Data temp;
					Data temp2;

					bool e = false;

					claujson::Convert(temp, key_buf_idx, key_next_buf_idx,  true, buf, string_buf, key_token_idx, e);

					if (e) {
						ERROR("Error in add_item_type");
					}

					claujson::Convert(temp2, val_buf_idx, val_next_buf_idx,  false, buf, string_buf, val_token_idx, e);

					if (e) {
						ERROR("Error in add_item_type");
					}

					if (temp.type() != DataType::STRING) {
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
			char* buf, uint8_t* string_buf, uint64_t val_token_idx) {

				{
					Data temp2;
					bool e = false;

					claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, true, buf, string_buf, val_token_idx, e);

					if (e) {

						ERROR("Error in add_item_type");
					}

					if (!obj_key_vec.empty()) {
						ERROR("partialJson is array or object.");
					}

					arr_vec.push_back(std::move(temp2));
				}
		}

		bool PartialJson::add_user_type(Ptr<Json> j) {

			j->set_parent(this);

			if (j->is_virtual()) {
				virtualJson = Data(j.release());
			}
			else if (j->has_key() == false) {
				if (!obj_key_vec.empty()) {
					return false; //ERROR("partialJson is array or object.");
				}

				arr_vec.emplace_back(j.release());
			}
			else {
				
				if (!arr_vec.empty()) {
					return false; //ERROR("partialJson is array or object.");
				}

				if (j->has_key()) {
					obj_key_vec.push_back(j->get_key().clone());
					obj_val_vec.emplace_back(j.release());
				}
				else {
					log << warn  << "ERROR";
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
	

	inline void Object::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		uint8_t* string_buf, int type, uint64_t key_token_idx) {
		if (!is_valid()) {
			return;
		}

		{
			Data temp;
			bool e = false;

			claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, string_buf, key_token_idx, e);
			if (e) {
				ERROR("Error in add_user_type");
			}

			if (temp.type() != DataType::STRING) {
				ERROR("Error in add_item_type, key is not string");
			}

			Json* json = nullptr;

			if (type == 0) {
				json = new Object();
			}
			else if (type == 1) {
				json = new Array();
			}

			obj_key_vec.push_back(temp.clone());
			obj_val_vec.push_back(Data(json));

			json->set_key(std::move(temp));
			json->set_parent(this);
		}
	}
	inline void Array::add_user_type(int type) {
		if (!is_valid()) {
			return;
		}

		{
			Json* json = nullptr;

			if (type == 0) {
				json = new Object();
			}
			else if (type == 1) {
				json = new Array();
			}


			if (type == 0) {
				arr_vec.push_back(Data(json));
			}
			else if (type == 1) {
				arr_vec.push_back(Data(json));
			}

			json->set_parent(this);
		}
	}

	inline void PartialJson::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		uint8_t* string_buf, int type, uint64_t key_token_idx) {
			{
				if (!arr_vec.empty()) {
					ERROR("partialJson is array or object.");
				}

				Data temp;
				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, string_buf, key_token_idx, e);

				if (e) {
					ERROR("Error in add_user_type");
				}

				if (temp.type() != DataType::STRING) {
					ERROR("Error in add_item_type, key is not string");
				}


				Json* json = nullptr;

				if (type == 0) {
					json = new Object();
				}
				else if (type == 1) {
					json = new Array();
				}



				obj_key_vec.push_back(temp.clone());
				obj_val_vec.push_back(Data(json));
				

				json->set_key(std::move(temp));
				json->set_parent(this);
			}
	}
	inline void PartialJson::add_user_type(int type) {
		{
			if (!obj_key_vec.empty()) {
				ERROR("PartialJson is array or object.");
			}

			Json* json = nullptr;

			if (type == 0) {
				json = new Object();
			}
			else if (type == 1) {
				json = new Array();
			}


			if (type == 0) {
				arr_vec.push_back(Data(json));
			}
			else if (type == 1) {
				arr_vec.push_back(Data(json));
			}


			json->set_parent(this);
		}
	}

	Array& Data::as_array() {
		if (is_ptr() && as_json_ptr()->is_array()) {
			return *static_cast<Array*>(_ptr_val);
		}
		static Array empty_arr{ false };
		return empty_arr;
	}

	Object& Data::as_object() {
		if (is_ptr() && as_json_ptr()->is_object()) {
			return *static_cast<Object*>(_ptr_val);
		}
		static Object empty_obj{ false };
		return empty_obj;
	}


	const Array& Data::as_array() const {
		if (is_ptr() && as_json_ptr()->is_array()) {
			return *static_cast<Array*>(_ptr_val);
		}
		static const Array empty_arr{ false };
		return empty_arr;
	}

	const Object& Data::as_object() const {
		if (is_ptr() && as_json_ptr()->is_object()) {
			return *static_cast<Object*>(_ptr_val);
		}
		static const Object empty_obj{ false };
		return empty_obj;
	}

	Json* Data::as_json_ptr() {
		if (!is_ptr()) {
			return nullptr;
		}
		return (_ptr_val);
	}

	const Json* Data::as_json_ptr() const {
		if (!is_ptr()) {
			return nullptr;
		}
		return (_ptr_val);
	}


	// todo - as_array, as_object.
// as_array
// if (valid && ((Json*)_ptr_val)->is_array()) { return ~~ } return 

	inline unsigned char __type_arr[256] = {
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

	inline unsigned char __comma_chk_table[2][256] = { { 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
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
 , 1  , 1  , 1  , 1  , 1  , 1  } };

	// not exact type! for int, uint, float. ( int, uint, float -> float )
	inline _simdjson::internal::tape_type get_type(unsigned char x) {
		return (_simdjson::internal::tape_type)__type_arr[x]; // more fast version..
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

		static size_t Size(Json* root) {
			return _Size(root) + 1;
		}

		 static size_t _Size(Json* root) {
			 if (root == nullptr) {
				return 0;
			}

			size_t len = root->get_data_size();
			size_t x = 0;

			for (size_t i = 0; i < len; ++i) {
				if (root->get_value_list(i).is_ptr()) {
					x += Size(root->get_value_list(i).as_json_ptr());
				}
			}

			return x;

		}

		 // find n node.. 
		 static void Find2(Json* root, const size_t n, size_t& idx, bool chk_hint, size_t& _len, std::vector<size_t>& offset, std::vector<size_t>& offset2, std::vector<Json*>& out, std::vector<int>& hint) {
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

					 size_t sz = Size(out[idx - 1]);

					 if (_len < offset2[idx - 1] + sz - 1) {
						 return;
					 }

					 _len = _len - (offset2[idx - 1] + sz - 1); // chk...

					 if (_len <= 0) {
						 return;
					 }

					 for (size_t k = idx; k < n; ++k) {
						 offset[k] = _len / (n - idx + 1);
					 }
					 offset[n - 1] = _len - _len / (n - idx + 1) * (n - idx);

					 for (size_t k = idx; k < n; ++k) {
						 offset2[k] = offset[k];
					 }
				 }

				 return;
			 }

			 size_t len = root->get_data_size();
			 for (size_t i = 0; i < len; ++i) {
				 if (root->get_value_list(i).is_ptr()) {

					 Find2(root->get_value_list(i).as_json_ptr(), n, idx, i < len - 1, _len, offset, offset2, out, hint);
					 
					 if (idx >= n) {
						 return;
					 }
				 }
			 }
		 }


		 static void Divide(Json* pos, Json*& result) { // after pos.. -> to result.
			if (pos == nullptr) {
				return;
			}


			Json* pos_ = pos;
			Json* parent = pos_->get_parent();

			Json* out = new PartialJson(); //


			while (parent && parent->is_partial_json() == false) {
				long long idx = 0;
				size_t len = parent->get_data_size();
				for (size_t i = 0; i < len; ++i) {
					if (parent->get_value_list(i).is_ptr() && parent->get_value_list(i).ptr_val() == pos_) {
						idx = i;
						break;
					}
				}

				for (size_t i = idx + 1; i < len; ++i) {
					if (parent->get_value_list(i).is_ptr()) {
						out->add_user_type(Ptr<Json>((Json*)parent->get_value_list(i).ptr_val()));
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
					Json* temp = nullptr;
					if (parent->is_object()) {
						temp = new VirtualObject();
					}
					else if (parent->is_array()) { // parent->is_array()
						temp = new VirtualArray();
					}
					else { // PartialJson
						// none
					}

					size_t len = out->get_data_size();

					for (size_t i = 0; i < len; ++i) {
						if (out->get_value_list(i).is_ptr()) {
							temp->add_user_type(Ptr<Json>((Json*)out->get_value_list(i).ptr_val()));
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
					out->add_user_type(Ptr<Json>(temp));
				}

				for (long long i = (long long)parent->get_data_size() - 1; i > idx; --i) {
					parent->erase(i);
				}

				pos_ = parent;
				parent = parent->get_parent();
			}

			result = out;
		}

		static std::vector<claujson::Json*> Divide2(size_t n, claujson::Data& j, std::vector<claujson::Json*>& result, std::vector<int>& hint) {
			if (j.is_ptr() == false) {
				return { nullptr };
			}

			if (n == 0) {
				return { nullptr };
			}

			size_t len = claujson::LoadData2::Size(j.as_json_ptr());

			if (len / n == 0) {
				return { nullptr };
			}

			std::vector<size_t> offset(n - 1, 0);

			for (size_t i = 0; i < offset.size(); ++i) {
offset[i] = len / n;
			}
			offset.back() = len - len / n * (n - 1);

			hint = std::vector<int>(n - 1, 0);

			std::vector<claujson::Json*> out(n, nullptr);

			{
				size_t idx = 0;
				auto offset2 = offset;

				claujson::LoadData2::Find2(j.as_json_ptr(), n - 1, idx, false, len, offset, offset2, out, hint);
			}

			for (size_t i = 0; i < n - 1; ++i) {
				if (!out[i]) {
					return { nullptr };
				}
			}


			std::vector<Json*> temp_parent(n, nullptr);
			{
				size_t i = 0;
				for (; i < n - 1; ++i) {

					if (i > 0 && temp_parent[i - 1] == nullptr) {
						for (size_t j = 0; j < i; ++j) {
							int op = 0;
							int ret = claujson::LoadData2::Merge2(temp_parent[j], result[j], &temp_parent[j + 1], op);

							for (size_t i = 1; i < result.size(); ++i) {
								claujson::Ptr<claujson::Json> clean2(result[i]);
							}
						}

						return { nullptr };
					}

					claujson::LoadData2::Divide(out[i], result[i]);

					temp_parent[i] = out[i]->get_parent();

				}

				if (i > 0 && temp_parent[i - 1] == nullptr) {
					for (size_t j = 0; j < i; ++j) {
						int op = 0;
						int ret = claujson::LoadData2::Merge2(temp_parent[j], result[j], &temp_parent[j + 1], op);

						for (size_t i = 1; i < result.size(); ++i) {
							claujson::Ptr<claujson::Json> clean2(result[i]);
						}
					}

					return { nullptr };
				}
			}


			return out;
		}

		static int Merge(Json* next, Json* ut, Json** ut_next)
		{

			// check!!
			while (ut->get_data_size() >= 1
				&& ut->get_value_list(0).is_ptr() && ((Json*)ut->get_value_list(0).ptr_val())->is_virtual())
			{
				ut = (Json*)ut->get_value_list(0).ptr_val();
			}

			bool chk_ut_next = false;



			while (true) {

				class Json* _ut = ut;
				class Json* _next = next;

				//log << warn  << "chk\n";
				if (ut_next && _ut == *ut_next) {
					*ut_next = _next;
					chk_ut_next = true;

					log << info << "chked in merge...\n";
				}

				if (_next->is_array() && _ut->is_object()) {
					ERROR("Error in Merge, next is array but child? is object");
				}
				if (_next->is_object() && _ut->is_array()) {
					ERROR("Error in Merge, next is object but child? is array");
				}

				if (_next->get_parent() == nullptr && _ut->get_data_size() > 0 && _ut->get_key_list(0).is_str()) {
					ERROR("Error in Merge, root must have not key");
				}
				if (_next->get_parent() == nullptr && _ut->get_data_size() > 1) {
					ERROR("Error in Merge, root must have one element");
				}

				int start_offset = 0;
				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_ptr() && _ut->get_value_list(0).ptr_val()->is_virtual()) {
					++start_offset;
				}

				_next->MergeWith(_ut, start_offset);

				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_ptr() && _ut->get_value_list(0).ptr_val()->is_virtual()) {
					clean(_ut->get_value_list(0));
				}

				/*
				size_t _size = _ut->get_data_size();

				for (size_t i = 0; i < _size; ++i) {

					if (_ut->get_value_list(i).is_ptr()) { // partial json, array, object
						if (((Json*)(_ut->get_value_list(i).ptr_val()))->is_virtual() == false) {
							// root
							if (_next->get_parent() == nullptr && _ut->get_key_list(i).is_str()) {
								ERROR("Error in Merge, root must have not key");
							}

							_next->Link(Ptr<Json>(((Json*)(_ut->get_value_list(i).ptr_val()))));
							_ut->clear(i);
						}
					}
					else { // item type.
						
						// root
						if (_next->get_parent() == nullptr && _ut->get_key_list(i).is_str()) {
							ERROR("Error in Merge, root must have not key");
						}

						if (_next->is_array() || _next->is_partial_json()) {
							_next->add_array_element(std::move(_ut->get_value_list(i)));
						}
						else {
							_next->add_object_element(std::move(_ut->get_key_list(i)), std::move(_ut->get_value_list(i)));
						}
						_ut->clear(i);
					}
				}
				*/
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

		static int Merge2(Json* next, Json* ut, Json** ut_next, int& op)
		{

			if (!ut) {
				*ut_next = next;
				return 0;
			}

			// check!!
			while (ut->get_data_size() >= 1
				&& ut->get_value_list(0).is_ptr() && ((Json*)ut->get_value_list(0).ptr_val())->is_virtual())
			{
				ut = (Json*)ut->get_value_list(0).ptr_val();
			}


			while (true) {
				//log << warn  << "chk\n";

				class Json* _ut = ut;
				class Json* _next = next;

				if (_next->is_array() && _ut->is_object()) {
					ERROR("Error in Merge, next is array but child? is object");
				}
				if (_next->is_object() && _ut->is_array()) {
					ERROR("Error in Merge, next is object but child? is array");
				}


				if (ut_next && _ut == (*ut_next)) {
					*ut_next = _next;
					log << info  << "chked in merge...\n";
				}


				if (_next->get_parent() == nullptr && _ut->get_data_size() > 0 && _ut->get_key_list(0).is_str()) {
					ERROR("Error in Merge, root must have not key");
				}
				if (_next->get_parent() == nullptr && _ut->get_data_size() > 1) {
					ERROR("Error in Merge, root must have one element");
				}

				int start_offset = 0;
				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_ptr() && _ut->get_value_list(0).ptr_val()->is_virtual()) {
					++start_offset;
				}

				_next->MergeWith(_ut, start_offset);

				if (_ut->get_data_size() > 0 && _ut->get_value_list(0).is_ptr() && _ut->get_value_list(0).ptr_val()->is_virtual()) {
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

		static bool __LoadData(char* buf, size_t buf_len,
			uint8_t* string_buf,
			_simdjson::internal::dom_parser_implementation* imple,
			int64_t token_arr_start, size_t token_arr_len, Ptr<Json>& _global,
			int start_state, int last_state, class Json** next, int* err, uint64_t no)
		{
			try {
				int a = clock();

				std::vector<TokenTemp> Vec;

				if (token_arr_len <= 0) {
					*next = nullptr;
					return false;
				}

				class Json* global = _global.get();

				int state = start_state;
				size_t braceNum = 0;
				std::vector< class Json* > nestedUT;

				nestedUT.reserve(10);
				nestedUT.push_back(global);

				int64_t count = 0;

				TokenTemp key; //bool is_before_comma = false;
				//bool is_now_comma = false;
				//bool is_next_comma = false;

				//if (token_arr_start > 0) {
				//	const _simdjson::internal::tape_type before_type =
					//	get_type(buf[imple->structural_indexes[token_arr_start - 1]]);

				//	is_before_comma = before_type == _simdjson::internal::tape_type::COMMA;
			//	}


				for (uint64_t i = 0; i < token_arr_len; ++i) {

					//is_now_comma = is_next_comma;

					const _simdjson::internal::tape_type type = get_type(buf[imple->structural_indexes[token_arr_start + i]]);

					if (type == _simdjson::internal::tape_type::COMMA) {
						continue;
					}

				//	if (is_before_comma && type == _simdjson::internal::tape_type::COMMA) {
					//	log << warn  << "before is comma\n";

					//	ERROR("Error in __Load... and case : , ,");
						//
					//}


					//if (token_arr_start + i > 0) {
					//	const _simdjson::internal::tape_type before_type =
					//		get_type(buf[imple->structural_indexes[token_arr_start + i - 1]]);
					//
					//	if (before_type == _simdjson::internal::tape_type::START_ARRAY || before_type == _simdjson::internal::tape_type::START_OBJECT) {
					//		is_now_comma = false; //log << warn  << "2-i " << i << "\n";
					//	}
					//}

					//if (is_before_comma) {
					//	is_now_comma = false;
				//	}
				//
					//if (!is_now_comma && type == _simdjson::internal::tape_type::COMMA) {
					//	log << warn  << "now is not comma\n";

					//	ERROR("Error in __Load.., now is comma but, no expect.");							//
					//}
					//if (is_now_comma && type != _simdjson::internal::tape_type::COMMA) {
					//	log << warn  << "is now comma... but not..\n";

					//	ERROR("Error in __Load..., comma is expected but, is not");
					//}


					//is_before_comma = type == _simdjson::internal::tape_type::COMMA;

					//if (type == _simdjson::internal::tape_type::COMMA) {
					//	if (token_arr_start + i + 1 < imple->n_structural_indexes) {
					//		const _simdjson::internal::tape_type _type = // next_type
					//			get_type(buf[imple->structural_indexes[token_arr_start + i + 1]]);

					//		if (_type == _simdjson::internal::tape_type::END_ARRAY || _type == _simdjson::internal::tape_type::END_OBJECT) {
					//			ERROR("Error in __Load..,  case : , } or , ]");
					//			//
					//		}
					/////		else if (_type == _simdjson::internal::tape_type::COLON) {
					//			ERROR("Error in __Load... case :    , : ");
					//		}

					//		continue;
					//	}
					//	else {
					//		ERROR("Error in __Load..., last valid char? is , ");
					//	}
				//	}

				//	if (type == _simdjson::internal::tape_type::COLON) {
				//		ERROR("Error in __Load..., checked colon..");
				//		//
				//	}


					//is_next_comma = __comma_chk_table[(int)is_now_comma][(unsigned char)type]; // comma_chk_table
					/*switch (type) {
					case _simdjson::internal::tape_type::END_ARRAY:
					case _simdjson::internal::tape_type::END_OBJECT:
					case _simdjson::internal::tape_type::STRING:
					case _simdjson::internal::tape_type::INT:
					case _simdjson::internal::tape_type::UINT:
					case _simdjson::internal::tape_type::DOUBLE:
					case _simdjson::internal::tape_type::TRUE_VALUE:
					case _simdjson::internal::tape_type::FALSE_VALUE:
					case _simdjson::internal::tape_type::NULL_VALUE:
					case _simdjson::internal::tape_type::NONE: //
						is_now_comma = true;
						break;
					} */

					//if (token_arr_start + i + 1 < imple->n_structural_indexes) {
					//	const _simdjson::internal::tape_type _type = // next_type
					//		get_type(buf[imple->structural_indexes[token_arr_start + i + 1]]);

					////	if (_type == _simdjson::internal::tape_type::END_ARRAY || _type == _simdjson::internal::tape_type::END_OBJECT) {
					//		is_next_comma = false;
					//	}
					//}
					//else {
					//	is_next_comma = false;
					//}

					// Left 1
					if (type == _simdjson::internal::tape_type::START_OBJECT ||
						type == _simdjson::internal::tape_type::START_ARRAY) { // object start, array start

						if (!Vec.empty()) {

							if (Vec[0].is_key) {
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size() / 2);

								//if (Vec.size() % 2 == 1) {
								//	log << warn  << "Vec.size()%2==1\n";
								//	ERROR("Error in __Load..., key : value  error");
								//}

								for (size_t x = 0; x < Vec.size(); x += 2) {
									//if (!Vec[x].is_key) {
									//	log << warn  << "vec[x].is not key\n";
									//	ERROR("Error in __Load..., key : value  error");
									//}
									//if (Vec[x + 1].is_key) {
									//	log << warn  << "vec[x].is key\n";
									//	ERROR("Error in __Load..., key : value  error");
									//}
									nestedUT[braceNum]->add_item_type((Vec[x].buf_idx), Vec[x].next_buf_idx, 
										(Vec[x + 1].buf_idx), Vec[x + 1].next_buf_idx, 
										buf, string_buf, Vec[x].token_idx, Vec[x + 1].token_idx);
								}
							}
							else {
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size());

								for (size_t x = 0; x < Vec.size(); x += 1) {
									//if (Vec[x].is_key) {
									//	log << warn  << "Vec[x].iskey\n";

									//	ERROR("Error in __Load..., key : value  error");
									//}
									nestedUT[braceNum]->add_item_type((Vec[x].buf_idx), Vec[x].next_buf_idx, buf, string_buf, Vec[x].token_idx);
								}
							}

							Vec.clear();
						}


						if (key.is_key) {
							nestedUT[braceNum]->add_user_type(key.buf_idx, key.next_buf_idx, buf, string_buf,
								type == _simdjson::internal::tape_type::START_OBJECT ? 0 : 1, key.token_idx); // object vs array
							key.is_key = false;
						}
						else {
							nestedUT[braceNum]->add_user_type(type == _simdjson::internal::tape_type::START_OBJECT ? 0 : 1);
						}


						class Json* pTemp = (Json*)nestedUT[braceNum]->get_value_list(nestedUT[braceNum]->get_data_size() - 1).ptr_val();

						braceNum++;

						/// new nestedUT
						if (nestedUT.size() == braceNum) {
							nestedUT.push_back(nullptr);
						}

						/// initial new nestedUT.
						nestedUT[braceNum] = pTemp;

						state = 0;

					}
					// Right 2
					else if (type == _simdjson::internal::tape_type::END_OBJECT ||
						type == _simdjson::internal::tape_type::END_ARRAY) {

						//if (type == _simdjson::internal::tape_type::END_ARRAY && nestedUT[braceNum]->is_object()) {
						////	log << warn  << "{]";
						//	ERROR("Error in __Load.., case : {]?");
						//}

						//if (type == _simdjson::internal::tape_type::END_OBJECT && nestedUT[braceNum]->is_array()) {
						//	log << warn  << "[}";


						//	ERROR("Error in __Load.., case : [}?");
						//}

						state = 0;

						if (!Vec.empty()) {
							if (type == _simdjson::internal::tape_type::END_OBJECT) {
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size() / 2);


							//	if (Vec.size() % 2 == 1) {
							//		log << warn  << "Vec.size() is odd\n";
							//		ERROR("Error in __Load..., key : value  error");
							//	}


								for (size_t x = 0; x < Vec.size(); x += 2) {
							//		if (!Vec[x].is_key) {
							//			log << warn  << "is not key\n";
							//			ERROR("Error in __Load..., key : value  error");
							//		}
							//		if (Vec[x + 1].is_key) {
							//			log << warn  << "is key\n";
							//			ERROR("Error in __Load..., key : value  error");
							//		}

									nestedUT[braceNum]->add_item_type(Vec[x].buf_idx, Vec[x].next_buf_idx,
										Vec[x + 1].buf_idx, Vec[x + 1].next_buf_idx, buf, string_buf, Vec[x].token_idx, Vec[x + 1].token_idx);
								}
							}
							else { // END_ARRAY
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size());

								for (auto& x : Vec) {
									//if (x.is_key) {
								//		ERROR("Error in __Load.., expect no key but has key...");
									//}

									nestedUT[braceNum]->add_item_type((x.buf_idx), x.next_buf_idx, buf, string_buf, x.token_idx);
								}
							}

							Vec.clear();
						}


						if (braceNum == 0) {

							Ptr<Json> ut; // is v_array or v_object.

							if (type == _simdjson::internal::tape_type::END_OBJECT) {
								ut = Ptr<Json>(new VirtualObject());
							}
							else {
								ut = Ptr<Json>(new VirtualArray());
							}

							bool chk = false;
							size_t len = nestedUT[braceNum]->get_data_size();

							for (size_t i = 0; i < len; ++i) {
								if (nestedUT[braceNum]->get_value_list(i).is_ptr()) {
									ut->add_user_type(Ptr<Json>((Json*)nestedUT[braceNum]->get_value_list(i).ptr_val()));
								}
								else {
									if (ut->is_object()) { 
										ut->add_object_element(std::move(nestedUT[braceNum]->get_key_list(i)),
											std::move(nestedUT[braceNum]->get_value_list(i)));
									}
									else { 
										ut->add_array_element(std::move(nestedUT[braceNum]->get_value_list(i)));
									}
								}
							}

							nestedUT[braceNum]->clear();
							nestedUT[braceNum]->add_user_type(std::move(ut));

							braceNum++;
						}

						{
							if (braceNum < nestedUT.size()) {
								nestedUT[braceNum] = nullptr;
							}

							braceNum--;
						}
					}
					else {
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

							bool is_key = false;
							if (token_arr_start + i + 1 < imple->n_structural_indexes && buf[imple->structural_indexes[token_arr_start + i + 1]] == ':') {
								is_key = true;
							}

							if (is_key) {
								data.is_key = true;

								if (token_arr_start + i + 2 < imple->n_structural_indexes) {
									const _simdjson::internal::tape_type _type = (_simdjson::internal::tape_type)buf[imple->structural_indexes[token_arr_start + i + 2]];

									if (_type == _simdjson::internal::tape_type::START_ARRAY || _type == _simdjson::internal::tape_type::START_OBJECT) {
										key = std::move(data);
									}
									else {
										Vec.push_back(std::move(data));
									}
								}
								else {
									Vec.push_back(std::move(data));
								}
								++i;

							//	is_next_comma = false;
							//	is_before_comma = false;
							}
							else {
								Vec.push_back(std::move(data));
							}

							state = 0;
						}
					}

				}


				if (next) {
					*next = nestedUT[braceNum];
				}

				if (Vec.empty() == false) {
					if (Vec[0].is_key) {
						for (size_t x = 0; x < Vec.size(); x += 2) {
							//if (!Vec[x].is_key) {
							//	ERROR("Error in __Load..., key : value  error");
							//}

							//if (Vec.size() % 2 == 1) {
							//	ERROR("Error in __Load..., key : value  error");
							//}


							//if (Vec[x + 1].is_key) {
							//	ERROR("Error in __Load..., key : value  error");
							//}

							nestedUT[braceNum]->add_item_type(Vec[x].buf_idx, Vec[x].next_buf_idx, Vec[x + 1].buf_idx, Vec[x + 1].next_buf_idx, 
								buf, string_buf, Vec[x].token_idx, Vec[x + 1].token_idx);
						}
					}
					else {
						for (size_t x = 0; x < Vec.size(); x += 1) {
							//if (Vec[x].is_key) {
							//	ERROR("Error in __Load..., array element has key..");
							//}

							nestedUT[braceNum]->add_item_type(Vec[x].buf_idx, Vec[x].next_buf_idx, buf, string_buf, Vec[x].token_idx);
						}
					}

					Vec.clear();
				}

				if (state != last_state) {
					*err = -2;
					return false;
					// throw STRING("error final state is not last_state!  : ") + toStr(state);
				}

				int b = clock();

				return true;
			}
			catch (const char* _err) {
				*err = -10;

				log << warn  << _err << "\n";

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
					return a + 1;
				default:
					break;
				}
			}
			return -1;
		}

		static bool _LoadData(Data& global, char* buf, size_t buf_len,
			uint8_t* string_buf,
			_simdjson::internal::dom_parser_implementation* imple, int64_t& length,
			std::vector<int64_t>& start, size_t parse_num) // first, strVec.empty() must be true!!
		{
			Ptr<Json> _global = Ptr<Json>(new PartialJson());
			std::vector<Ptr<Json>> __global;

			try {
				int a__ = clock();
				
				{
					// chk clear?

					const int pivot_num = static_cast<int>(parse_num) - 1;
					//size_t token_arr_len = length; // size?


					bool first = true;
					int64_t sum = 0;

					{ int a_ = clock();
					std::set<int64_t> _pivots;
					std::vector<int64_t> pivots;
					//const int64_t num = token_arr_len; //

					if (pivot_num > 0) {
						std::vector<int64_t> pivot;
						pivots.reserve(pivot_num);
						pivot.reserve(pivot_num);

						pivot.push_back(start[0]);

						for (int i = 1; i < parse_num; ++i) {
							pivot.push_back(FindDivisionPlace(buf, imple, start[i], start[i + 1] - 1));
						}

						for (size_t i = 0; i < pivot.size(); ++i) {
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
					int b_ = clock();
					//log << warn  << "pivots.. " << b_ - a_ << "ms\n";
					std::vector<class Json*> next(pivots.size() - 1, nullptr);
					{

						__global = std::vector<Ptr<Json>>(pivots.size() - 1);
						for (size_t i = 0; i < __global.size(); ++i) {
							__global[i] = Ptr<Json>(new PartialJson());
						}

						std::vector<std::thread> thr(pivots.size() - 1);


						std::vector<int> err(pivots.size() - 1, 0);
						
						auto a = std::chrono::steady_clock::now();

						{
							int64_t idx = pivots[1] - pivots[0];
							int64_t _token_arr_len = idx;


							thr[0] = std::thread(__LoadData, (buf), buf_len, (string_buf), (imple), start[0], _token_arr_len, std::ref(__global[0]), 0, 0,
								&next[0], &err[0], 0);
						}

						

						for (size_t i = 1; i < pivots.size() - 1; ++i) {
							int64_t _token_arr_len = pivots[i + 1] - pivots[i];

							thr[i] = std::thread(__LoadData, (buf), buf_len, (string_buf), (imple), pivots[i], _token_arr_len, std::ref(__global[i]), 0, 0,
								&next[i], &err[i], i);

						}


						// wait
						for (size_t i = 0; i < thr.size(); ++i) {
							thr[i].join();
						}

						auto b = std::chrono::steady_clock::now();
						auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
						log << info  << "parse1 " << dur.count() << "ms\n";

						// check..
						for (size_t i = 0; i < err.size(); ++i) {
							switch (err[i]) {
							case 0:
								break;
							case -10:
							case -11:
								return false;
								break;
							case -1:
							case -4:
								log << warn  << "Syntax Error\n"; return false;
								break;
							case -2:
								log << warn  << "error final state is not last_state!\n"; return false;
								break;
							case -3:
								log << warn  << "error x > buffer + buffer_len:\n"; return false;
								break;
							default:
								log << warn  << "unknown parser error\n"; return false;
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

							for (int64_t i = 0; i < pivots.size() - 1; ++i) {
								if (chk[i] == 0) {
									start = i;
									break;
								}
							}

							for (int64_t i = pivots.size() - 1 - 1; i >= 0; --i) {
								if (chk[i] == 0) {
									last = i;
									break;
								}
							}

							if (__global[start]->get_data_size() > 0 && __global[start]->get_value_list(0).is_ptr()
								&& ((Json*)__global[start]->get_value_list(0).ptr_val())->is_virtual()) {
								log << warn  << "not valid file1\n";
								throw 1;
							}
							if (next[last] && next[last]->get_parent() != nullptr) {
								log << warn  << "not valid file2\n";
								throw 2;
							}

							
							int err = Merge(_global.get(), __global[start].get(), &next[start]);
							if (-1 == err || (pivots.size() == 0 && 1 == err)) {
								log << warn  << "not valid file3\n";
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
									log << warn  << "chk " << i << " " << __global.size() << "\n";
									log << warn  << "not valid file4\n";
									throw 4;
								}
								else if (i == last && 1 == err) {
									log << warn  << "n]ot valid file5\n";
									throw 5;
								}
							}
						}
						//catch (...) {
							//throw "in Merge, error";
						//	return false;
						//}
						//

						if ( _global->get_data_size() > 1) { // bug fix..
							log << warn  << "not valid file6\n";
							throw 6;
						}

						auto c = std::chrono::steady_clock::now();
						auto dur2 = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
						log << info  << "parse2 " << dur2.count() << "ms\n";
					}
					}
					int a = clock();


					if (_global->get_value_list(0).is_ptr()) {
						_global->get_value_list(0).as_json_ptr()->set_parent(nullptr);
					}

					global = std::move(_global->get_value_list(0));


					int b = clock();
					log << info  << "chk " << b - a << "ms\n";

					//	log << warn  << clock() - a__ << "ms\n";
				}
				//	log << warn  << clock() - a__ << "ms\n";
				return true;
			}
			catch (int err) {

				log << warn  << "merge error " << err << "\n";
				//ERROR("Merge Error"sv);
				return false;
			}
			catch (const char* err) {

				log << warn  << err << "\n";
				//ERROR("Merge Error"sv);

				return false;
			}
			//catch (...) {

			//	log << warn  << "internal error\n";
				//ERROR("Internal Error"sv);
			//	return false;
			//}

		}
		static bool parse(Data& global, char* buf, size_t buf_len,
			uint8_t* string_buf,
			_simdjson::internal::dom_parser_implementation* imple,
			int64_t length, std::vector<int64_t>& start, size_t thr_num) {

			return LoadData2::_LoadData(global, buf, buf_len, string_buf, imple, length, start, thr_num);
		}
	};

	// using fmt, for speed.
	class StrStream {
	private:
		//fmt::memory_buffer out;
	public:

		const char* buf() const {
			return "";// out.data();
		}
		size_t buf_size() const {
			return 0; // out.size();
		}

		StrStream& operator<<(const char* x) {
			//fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(double x) {
			//fmt::format_to(std::back_inserter(out), "{}", x); // FMT_COMPILE("{:.10f}"), x);
			return *this;
		}

		StrStream& operator<<(int64_t x) {
			//fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(uint64_t x) {
			//fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(char ch) {
			//fmt::format_to(std::back_inserter(out), "{}", ch);
			return *this;
		}
	};


	class LoadData // Save?
	{
	private:
		//                         
		static void _save(StrStream& stream, const Data& data, std::vector<Json*>& chk_list, const int depth);
		static void _save(StrStream& stream, const Data& data, const int depth);

		static void save_(StrStream& stream, const Data& global, Json* temp, bool hint);

	public:
		// test?... just Data has one element 
		static void save(const std::string& fileName, const Data& global, bool hint = false);

		static void save(std::ostream& stream, const Data& data);

		static std::string save_to_str(const Data& data);

		static void save_parallel(const std::string& fileName, Data& j, size_t thr_num);

	};


	std::string LoadData::save_to_str(const Data& global) {
		StrStream stream;

		if (global.is_ptr()) {
			bool is_arr = global.as_json_ptr()->is_array();

			if (is_arr) {
				stream << " [ ";
			}
			else {
				stream << " { ";
			}

			_save(stream, global, 0);

			if (is_arr) {
				stream << " ] ";
			}
			else {
				stream << " } ";
			}

		}
		else {
			auto& x = global;
			if (x.type() == DataType::STRING) {
				stream << "\"";

				size_t len = x.str_val().size();
				for (uint64_t j = 0; j < len; ++j) {
					switch ((x.str_val())[j]) {
					case '\\':
						stream << "\\\\";
						break;
					case '\"':
						stream << "\\\"";
						break;
					case '\n':
						stream << "\\n";
						break;

					default:

						int code = (x.str_val())[j];
						if (code > 0 && (code < 0x20 || code == 0x7F))
						{
							char buf[] = "\\uDDDD";
							sprintf(buf + 2, "%04X", code);
							stream << buf;
						}
						else {
							stream << (x.str_val())[j];
						}

					}
				}stream << "\"";
			}
			else if (x.type() == DataType::BOOL) {
				stream << (x.bool_val() ? "true" : "false");
			}
			else if (x.type() == DataType::FLOAT) {
				stream << (x.float_val());
			}
			else if (x.type() == DataType::INT) {
				stream << x.int_val();
			}
			else if (x.type() == DataType::UINT) {
				stream << x.uint_val();
			}
			else if (x.type() == DataType::NULL_) {
				stream << "null";
			}
		}

		return std::string(stream.buf(), stream.buf_size());
	}

	//                            todo - change Json* ut to Data& data ?
	void LoadData::_save(StrStream& stream, const Data& data, std::vector<Json*>& chk_list, const int depth) {
		const Json* ut = nullptr;

		if (data.is_ptr()) {
			ut = data.as_json_ptr();
		}

		if (ut && ut->is_object()) {
			size_t len = ut->get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_ptr()) {
					auto& x = ut->get_key_list(i);

					if (x.type() == DataType::STRING) {
						stream << "\"";

						size_t len = x.str_val().size();
						for (uint64_t j = 0; j < len; ++j) {
							switch ((x.str_val())[j]) {
							case '\\':
								stream << "\\\\";
								break;
							case '\"':
								stream << "\\\"";
								break;
							case '\n':
								stream << "\\n";
								break;

							default:

								int code = (x.str_val())[j];
								if (code > 0 && (code < 0x20 || code == 0x7F))
								{
									char buf[] = "\\uDDDD";
									sprintf(buf + 2, "%04X", code);
									stream << buf;
								}
								else {
									stream << (x.str_val())[j];
								}

							}
						}stream << "\"";

						{
							stream << " : ";
						}
					}
					else {
						//log << warn  << "Error : no key\n";
					}
					stream << " ";

					auto* y = ((Json*)(ut->get_value_list(i).ptr_val()));

					if (y->is_object() && y->is_virtual() == false) {
						stream << " { \n";
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream << " [ \n";
					}

					_save(stream, ut->get_value_list(i), chk_list, depth + 1);

					if (y->is_object() && find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream << " } \n";
					}
					else if (y->is_array() && find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream << " ] \n";
					}
				}
				else {
					auto& x = ut->get_key_list(i);

					if (x.type() == DataType::STRING) {
						stream << "\"";

						size_t len = x.str_val().size();
						for (uint64_t j = 0; j < len; ++j) {
							switch ((x.str_val())[j]) {
							case '\\':
								stream << "\\\\";
								break;
							case '\"':
								stream << "\\\"";
								break;
							case '\n':
								stream << "\\n";
								break;

							default:

								int code = (x.str_val())[j];
								if (code > 0 && (code < 0x20 || code == 0x7F))
								{
									char buf[] = "\\uDDDD";
									sprintf(buf +2, "%04X", code);
									stream << buf;
								}
								else {
									stream << (x.str_val())[j];
								}

							}
						}

						stream << "\"";

						{
							stream << " : ";
						}
					}

					{
						auto& x = ut->get_value_list(i);

						if (x.type() == DataType::STRING) {
							stream << "\"";

							size_t len = x.str_val().size();
							for (uint64_t j = 0; j < len; ++j) {
								switch ((x.str_val())[j]) {
								case '\\':
									stream << "\\\\";
									break;
								case '\"':
									stream << "\\\"";
									break;
								case '\n':
									stream << "\\n";
									break;

								default:

									int code = (x.str_val())[j];
									if (code > 0 && (code < 0x20 || code == 0x7F))
									{
										char buf[] = "\\uDDDD";
										sprintf(buf +2, "%04X", code);
										stream << buf;
									}
									else {
										stream << (x.str_val())[j];
									}

								}
							}
							stream << "\"";

						}
						else if (x.type() == DataType::BOOL) {
							stream << (x.bool_val() ? "true" : "false");
						}
						else if (x.type() == DataType::FLOAT) {
							stream << (x.float_val());
						}
						else if (x.type() == DataType::INT) {
							stream << x.int_val();
						}
						else if (x.type() == DataType::UINT) {
							stream << x.uint_val();
						}
						else if (x.type() == DataType::NULL_) {
							stream << "null";
						}
					}
				}

				if (i < ut->get_data_size() - 1) {
					stream << ", ";
				}
			}
		}
		else if (ut && ut->is_array()) {
			size_t len = ut->get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_ptr()) {


					auto* y = ((Json*)(ut->get_value_list(i).ptr_val()));

					if (y->is_object() && y->is_virtual() == false) {
						stream << " { \n";
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream << " [ \n";
					}



					_save(stream, ut->get_value_list(i), chk_list, depth + 1);

					y = ((Json*)(ut->get_value_list(i).ptr_val()));


					if (y->is_object() && find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream << " } \n";
					}
					else if (y->is_array() && find(chk_list.begin(), chk_list.end(), y) == chk_list.end()) {
						stream << " ] \n";
					}

				}
				else {

					auto& x = ut->get_value_list(i);

					if (x.type() == DataType::STRING) {
						stream << "\"";

						size_t len = x.str_val().size();
						for (uint64_t j = 0; j < len; ++j) {
							switch ((x.str_val())[j]) {
							case '\\':
								stream << "\\\\";
								break;
							case '\"':
								stream << "\\\"";
								break;
							case '\n':
								stream << "\\n";
								break;

							default:

								int code = (x.str_val())[j];
								if (code > 0 && (code < 0x20 || code == 0x7F))
								{
									char buf[] = "\\uDDDD";
									sprintf(buf +2, "%04X", code);
									stream << buf;
								}
								else {
									stream << (x.str_val())[j];
								}

							}
						}stream << "\"";
					}
					else if (x.type() == DataType::BOOL) {
						stream << (x.bool_val() ? "true" : "false");
					}
					else if (x.type() == DataType::FLOAT) {
						stream << (x.float_val());
					}
					else if (x.type() == DataType::INT) {
						stream << x.int_val();
					}
					else if (x.type() == DataType::UINT) {
						stream << x.uint_val();
					}
					else if (x.type() == DataType::NULL_) {
						stream << "null";
					}


					stream << " ";
				}

				if (i < ut->get_data_size() - 1) {
					stream << ", ";
				}
			}
		}
		else if (data) { // valid
			auto& x = data;

			if (x.type() == DataType::STRING) {
				stream << "\"";

				size_t len = x.str_val().size();
				for (uint64_t j = 0; j < len; ++j) {
					switch ((x.str_val())[j]) {
					case '\\':
						stream << "\\\\";
						break;
					case '\"':
						stream << "\\\"";
						break;
					case '\n':
						stream << "\\n";
						break;

					default:

						int code = (x.str_val())[j];
						if (code > 0 && (code < 0x20 || code == 0x7F))
						{
							char buf[] = "\\uDDDD";
							sprintf(buf +2, "%04X", code);
							stream << buf;
						}
						else {
							stream << (x.str_val())[j];
						}

					}
				}
				stream << "\"";

			}
			else if (x.type() == DataType::BOOL) {
				stream << (x.bool_val() ? "true" : "false");
			}
			else if (x.type() == DataType::FLOAT) {
				stream << (x.float_val());
			}
			else if (x.type() == DataType::INT) {
				stream << x.int_val();
			}
			else if (x.type() == DataType::UINT) {
				stream << x.uint_val();
			}
			else if (x.type() == DataType::NULL_) {
				stream << "null";
			}
		}
	}
	
	void LoadData::_save(StrStream& stream, const Data& data, const int depth) {
		const Json* ut = nullptr;

		if (data.is_ptr()) {
			ut = data.as_json_ptr();
		}


		if (ut && ut->is_object()) {
			size_t len = ut->get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_ptr()) {
					auto& x = ut->get_key_list(i);

					if (x.type() == DataType::STRING) {
						stream << "\"";

						size_t len = x.str_val().size();
						for (uint64_t j = 0; j < len; ++j) {
							switch ((x.str_val())[j]) {
							case '\\':
								stream << "\\\\";
								break;
							case '\"':
								stream << "\\\"";
								break;
							case '\n':
								stream << "\\n";
								break;

							default:

								int code = (x.str_val())[j];
								if (code > 0 && (code < 0x20 || code == 0x7F)) // chk this... with validate_string function. from simdjson..
								{
									char buf[] = "\\uDDDD";
									sprintf(buf +2, "%04X", code);
									stream << buf;
								}
								else {
									stream << (x.str_val())[j];
								}

							}
						}stream << "\"";

						{
							stream << " : ";
						}
					}
					else {
						//log << warn  << "Error : no key\n";
					}
					stream << " ";

					auto* y = ((Json*)(ut->get_value_list(i).ptr_val()));

					if (y->is_object() && y->is_virtual() == false) {
						stream << " { \n";
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream << " [ \n";
					}

					_save(stream, ut->get_value_list(i), depth + 1);

					if (y->is_object()) {
						stream << " } \n";
					}
					else if (y->is_array()) {
						stream << " ] \n";
					}
				}
				else {
					auto& x = ut->get_key_list(i);

					if (x.type() == DataType::STRING) {
						stream << "\"";

						size_t len = x.str_val().size();
						for (uint64_t j = 0; j < len; ++j) {
							switch ((x.str_val())[j]) {
							case '\\':
								stream << "\\\\";
								break;
							case '\"':
								stream << "\\\"";
								break;
							case '\n':
								stream << "\\n";
								break;

							default:

								int code = (x.str_val())[j];
								if (code > 0 && (code < 0x20 || code == 0x7F))
								{
									char buf[] = "\\uDDDD";
									sprintf(buf +2, "%04X", code);
									stream << buf;
								}
								else {
									stream << (x.str_val())[j];
								}

							}
						}

						stream << "\"";

						{
							stream << " : ";
						}
					}

					{
						auto& x = ut->get_value_list(i);

						if (x.type() == DataType::STRING) {
							stream << "\"";

							size_t len = x.str_val().size();
							for (uint64_t j = 0; j < len; ++j) {
								switch ((x.str_val())[j]) {
								case '\\':
									stream << "\\\\";
									break;
								case '\"':
									stream << "\\\"";
									break;
								case '\n':
									stream << "\\n";
									break;

								default:

									int code = (x.str_val())[j];
									if (code > 0 && (code < 0x20 || code == 0x7F))
									{
										char buf[] = "\\uDDDD";
										sprintf(buf +2, "%04X", code);
										stream << buf;
									}
									else {
										stream << (x.str_val())[j];
									}

								}
							}
							stream << "\"";

						}
						else if (x.type() == DataType::BOOL) {
							stream << (x.bool_val() ? "true" : "false");
						}
						else if (x.type() == DataType::FLOAT) {
							stream << (x.float_val());
						}
						else if (x.type() == DataType::INT) {
							stream << x.int_val();
						}
						else if (x.type() == DataType::UINT) {
							stream << x.uint_val();
						}
						else if (x.type() == DataType::NULL_) {
							stream << "null";
						}
					}
				}

				if (i < ut->get_data_size() - 1) {
					stream << ", ";
				}
			}
		}
		else if (ut && ut->is_array()) {
			size_t len = ut->get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (ut->get_value_list(i).is_ptr()) {


					auto* y = ((Json*)(ut->get_value_list(i).ptr_val()));

					if (y->is_object() && y->is_virtual() == false) {
						stream << " { \n";
					}
					else if (y->is_array() && y->is_virtual() == false) {
						stream << " [ \n";
					}



					_save(stream, ut->get_value_list(i), depth + 1);

					y = ((Json*)(ut->get_value_list(i).ptr_val()));


					if (y->is_object()) {
						stream << " } \n";
					}
					else if (y->is_array()) {
						stream << " ] \n";
					}

				}
				else {

					auto& x = ut->get_value_list(i);

					if (x.type() == DataType::STRING) {
						stream << "\"";

						size_t len = x.str_val().size();
						for (uint64_t j = 0; j < len; ++j) {
							switch ((x.str_val())[j]) {
							case '\\':
								stream << "\\\\";
								break;
							case '\"':
								stream << "\\\"";
								break;
							case '\n':
								stream << "\\n";
								break;

							default:

								int code = (x.str_val())[j];
								if (code > 0 && (code < 0x20 || code == 0x7F))
								{
									char buf[] = "\\uDDDD";
									sprintf(buf +2, "%04X", code);
									stream << buf;
								}
								else {
									stream << (x.str_val())[j];
								}

							}
						}stream << "\"";
					}
					else if (x.type() == DataType::BOOL) {
						stream << (x.bool_val() ? "true" : "false");
					}
					else if (x.type() == DataType::FLOAT) {
						stream << (x.float_val());
					}
					else if (x.type() == DataType::INT) {
						stream << x.int_val();
					}
					else if (x.type() == DataType::UINT) {
						stream << x.uint_val();
					}
					else if (x.type() == DataType::NULL_) {
						stream << "null";
					}


					stream << " ";
				}

				if (i < ut->get_data_size() - 1) {
					stream << ", ";
				}
			}
		}
		else if (data) { // valid
			auto& x = data;

			if (x.type() == DataType::STRING) {
				stream << "\"";

				size_t len = x.str_val().size();
				for (uint64_t j = 0; j < len; ++j) {
					switch ((x.str_val())[j]) {
					case '\\':
						stream << "\\\\";
						break;
					case '\"':
						stream << "\\\"";
						break;
					case '\n':
						stream << "\\n";
						break;

					default:

						int code = (x.str_val())[j];
						if (code > 0 && (code < 0x20 || code == 0x7F))
						{
							char buf[] = "\\uDDDD";
							sprintf(buf +2, "%04X", code);
							stream << buf;
						}
						else {
							stream << (x.str_val())[j];
						}

					}
				}
				stream << "\"";

			}
			else if (x.type() == DataType::BOOL) {
				stream << (x.bool_val() ? "true" : "false");
			}
			else if (x.type() == DataType::FLOAT) {
				stream << (x.float_val());
			}
			else if (x.type() == DataType::INT) {
				stream << x.int_val();
			}
			else if (x.type() == DataType::UINT) {
				stream << x.uint_val();
			}
			else if (x.type() == DataType::NULL_) {
				stream << "null";
			}
		}
	}

	// todo... just Data has one element 
	void LoadData::save(const std::string& fileName, const Data& global, bool hint) {
		StrStream stream;

		if (global.is_ptr()) {
			if (hint) {
				stream << " , ";
			}
			bool is_arr = global.as_json_ptr()->is_array();

			if (is_arr) {
				stream << " [ ";
			}
			else {
				stream << " { ";
			}

			_save(stream, global, 0);

			if (is_arr) {
				stream << " ] ";
			}
			else {
				stream << " } ";
			}

		}
		else {
			if (hint) {
				stream << " , ";
			}
			auto& x = global;
			if (x.type() == DataType::STRING) {
				stream << "\"";

				size_t len = x.str_val().size();
				for (uint64_t j = 0; j < len; ++j) {
					switch ((x.str_val())[j]) {
					case '\\':
						stream << "\\\\";
						break;
					case '\"':
						stream << "\\\"";
						break;
					case '\n':
						stream << "\\n";
						break;

					default:

						int code = (x.str_val())[j];
						if (code > 0 && (code < 0x20 || code == 0x7F))
						{
							char buf[] = "\\uDDDD";
							sprintf(buf +2, "%04X", code);
							stream << buf;
						}
						else {
							stream << (x.str_val())[j];
						}

					}
				}stream << "\"";
			}
			else if (x.type() == DataType::BOOL) {
				stream << (x.bool_val() ? "true" : "false");
			}
			else if (x.type() == DataType::FLOAT) {
				stream << (x.float_val());
			}
			else if (x.type() == DataType::INT) {
				stream << x.int_val();
			}
			else if (x.type() == DataType::UINT) {
				stream << x.uint_val();
			}
			else if (x.type() == DataType::NULL_) {
				stream << "null";
			}
		}

		std::ofstream outFile;
		outFile.open(fileName, std::ios::binary); // binary!
		outFile.write(stream.buf(), stream.buf_size());
		outFile.close();
	}

	void LoadData::save(std::ostream& stream, const Data& data) {
		StrStream str_stream;
		_save(str_stream, data, 0);
		stream << std::string_view(str_stream.buf(), str_stream.buf_size());
	}

	void LoadData::save_(StrStream& stream, const Data& global, Json* temp, bool hint) {

		std::vector<Json*> chk_list; // point for division?, virtual nodes? }}}?

		{
			while (temp) {
				chk_list.push_back(temp);
				temp = temp->get_parent();
			}
		}

		if (global.is_ptr()) {
			if (hint) {
				stream << " , ";
			}

			auto* j = global.as_json_ptr();


			if (j->is_array() && j->is_virtual() == false) {
				stream << " [ ";
			}
			else if (j->is_object() && j->is_virtual() == false) {
				stream << " { ";
			}

			_save(stream, global, chk_list, 1);

			if (j->is_array() && find(chk_list.begin(), chk_list.end(), j) == chk_list.end()) {
				stream << " ] ";
			}
			else if (j->is_object() && find(chk_list.begin(), chk_list.end(), j) == chk_list.end()) {
				stream << " } ";
			}
		}
		else {
			if (hint) {
				stream << " , ";
			}

			auto& x = global;
			if (x.type() == DataType::STRING) {
				stream << "\"";

				size_t len = x.str_val().size();
				for (uint64_t j = 0; j < len; ++j) {
					switch ((x.str_val())[j]) {
					case '\\':
						stream << "\\\\";
						break;
					case '\"':
						stream << "\\\"";
						break;
					case '\n':
						stream << "\\n";
						break;

					default:

						int code = (x.str_val())[j];
						if (code > 0 && (code < 0x20 || code == 0x7F))
						{
							char buf[] = "\\uDDDD";
							sprintf(buf +2, "%04X", code);
							stream << buf;
						}
						else {
							stream << (x.str_val())[j];
						}

					}
				}stream << "\"";
			}
			else if (x.type() == DataType::BOOL) {
				stream << (x.bool_val() ? "true" : "false");
			}
			else if (x.type() == DataType::FLOAT) {
				stream << (x.float_val());
			}
			else if (x.type() == DataType::INT) {
				stream << x.int_val();
			}
			else if (x.type() == DataType::UINT) {
				stream << x.uint_val();
			}
			else if (x.type() == DataType::NULL_) {
				stream << "null";
			}
		}
	}


	void LoadData::save_parallel(const std::string& fileName, Data& j, size_t thr_num) {

		if (!j.is_ptr()) {
			save(fileName, j, false);
			return;
		}

		if (thr_num <= 0) {
			thr_num = std::thread::hardware_concurrency();
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		if (thr_num == 1) {
			save(fileName, j, false);
			return;
		}
		
		std::vector<claujson::Json*> temp(thr_num, nullptr); //
		std::vector<claujson::Json*> temp_parent(thr_num, nullptr);
		{
			std::vector<claujson::Json*> result(temp.size() - 1, nullptr);

			std::vector<int> hint(temp.size() - 1, false);

			temp = LoadData2::Divide2(thr_num, j, result, hint);

			if (temp.size() == 1 && temp[0] == nullptr) {
				save(fileName, j, false);
				return;
			}

			for (size_t i = 0; i < temp.size() - 1; ++i) {
				temp_parent[i] = temp[i]->get_parent();
			}

			std::vector<claujson::StrStream> stream(thr_num);

			std::vector<std::thread> thr(thr_num);

			thr[0] = std::thread(save_, std::ref(stream[0]), std::cref(j), temp_parent[0], (false));


			for (size_t i = 1; i < thr.size(); ++i) {
				thr[i] = std::thread(save_, std::ref(stream[i]), std::cref(result[i - 1]->get_value_list(0)), temp_parent[i], (hint[i - 1]));
			}

			for (size_t i = 0; i < thr.size(); ++i) {
				thr[i].join();
			}


			std::ofstream outFile(fileName, std::ios::binary);

			for (size_t i = 0; i < stream.size(); ++i) {
				outFile.write(stream[i].buf(), stream[i].buf_size());
			}

			outFile.close();

			int op = 0;
			int ret = claujson::LoadData2::Merge2(temp_parent[0], result[0], &temp_parent[1], op);
			for (size_t i = 1; i < temp.size() - 1; ++i) {
				int ret = claujson::LoadData2::Merge2(temp_parent[i], result[i], &temp_parent[i + 1], op);
				op = 0;
			}


			for (size_t i = 1; i < result.size(); ++i) {
				claujson::Ptr<claujson::Json> clean2(result[i]);
			}
		}
	}

	bool is_valid(_simdjson::dom::parser_for_claujson& dom_parser) {

		const auto& buf = dom_parser.raw_buf();
		const auto& string_buf = dom_parser.raw_string_buf();
		const auto buf_len = dom_parser.raw_len();

		auto* simdjson_imple = dom_parser.raw_implementation().get();
		size_t idx = 0;
		size_t depth = 0;
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
				log << warn << ("starting brace unmatched"); return false;
			}
					break;
			case '[': if (buf[simdjson_imple->structural_indexes[simdjson_imple->n_structural_indexes - 1]] != ']') {
				log << warn << ("starting bracket unmatched"); return false;
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
		if (depth >= dom_parser.max_depth()) { log << warn << ("Exceeded max depth!"); return false; }
		//dom_parser.is_array[depth] = false;
		is_array[depth - 1] = 0;
		//SIMDJSON_TRY(visitor.visit_object_start(*this));

		{
			auto key = buf[simdjson_imple->structural_indexes[idx++]]; // advance();
			if (key != '"') { log << warn << ("Object does not start with a key"); return false; }
			//SIMDJSON_TRY(visitor.increment_count(*this));
			//SIMDJSON_TRY(visitor.visit_key(*this, key));
		}

	object_field:
		if (simdjson_unlikely(buf[simdjson_imple->structural_indexes[idx++]] != ':')) { log << warn << ("Missing colon after key in object"); return false; }
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]];
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[idx]] == '}') { ++idx; break; } goto object_begin;
			case '[': if (buf[simdjson_imple->structural_indexes[idx]] == ']') { ++idx; break; } goto array_begin;
			case ',': { log << warn << "wrong comma.";  return false; }
			case ':': { log << warn << "wrong colon.";  return false; }
			case '}': { log << warn << "wrong }.";  return false; }
			case ']': { log << warn << "wrong ].";  return false; }
			default: //SIMDJSON_TRY(visitor.visit_primitive(*this, value)); 
				break;
			}
		}

	object_continue:
		switch (buf[simdjson_imple->structural_indexes[idx++]]) {
		case ',':
			//SIMDJSON_TRY(visitor.increment_count(*this));
			{
				auto key = buf[simdjson_imple->structural_indexes[idx++]]; // advance();
				if (simdjson_unlikely(key != '"')) { log << warn << ("Key string missing at beginning of field in object"); return false; }
				//SIMDJSON_TRY(visitor.visit_key(*this, key));
			}
			goto object_field;
		case '}': goto scope_end;
		case ':': { log << warn << "wrong colon.";  return false; }
		default: log << warn << ("No comma between object fields"); return false;
		}

	scope_end:
		depth--;
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
		if (depth >= dom_parser.max_depth()) { log << warn << ("Exceeded max depth!"); return false; }
		if (is_array.size() < depth) { is_array.push_back(1); }
		is_array[depth - 1] = 1;
		//SIMDJSON_TRY(visitor.visit_array_start(*this));
	//	SIMDJSON_TRY(visitor.increment_count(*this));

	array_value:
		{
			auto value = buf[simdjson_imple->structural_indexes[idx++]];
			switch (value) {
			case '{': if (buf[simdjson_imple->structural_indexes[idx]] == '}') { ++idx; break; } goto object_begin;
			case '[': if (buf[simdjson_imple->structural_indexes[idx]] == ']') { ++idx; break; } goto array_begin;
			case ',': { log << warn << "wrong comma.";  return false; }
			case ':': { log << warn << "wrong colon.";  return false; }
			case '}': { log << warn << "wrong }.";  return false; }
			case ']': { log << warn << "wrong ].";  return false; }
			default: break;
			}
		}

	array_continue:
		switch (buf[simdjson_imple->structural_indexes[idx++]]) {
		case ',': goto array_value;
		case ']': goto scope_end;
		case ':': { log << warn << "wrong colon.";  return false; }
		default: log << warn << ("Missing comma between array values"); return false;
		}

	document_end:

		// If we didn't make it to the end, it's an error
		if (idx != simdjson_imple->n_structural_indexes) {
			log << warn << ("More than one JSON value at the root of the document, or extra characters at the end of the JSON!");
			return false;
		}

		log << info << "test end\n";
		return true;
	}

	std::pair<bool, size_t> parse(const std::string& fileName, Data& ut, size_t thr_num)
	{
		if (thr_num <= 0) {
			thr_num = std::thread::hardware_concurrency();
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		int64_t length;

		auto _ = std::chrono::steady_clock::now();

		{
			// not static??
			static _simdjson::dom::parser_for_claujson test;

			auto x = test.load(fileName);

			if (x.error() != _simdjson::error_code::SUCCESS) {
				log << warn  << "stage1 error : ";
				log << warn  << x.error() << "\n";
				
				//ERROR(_simdjson::error_message(x.error()));

				return { false, 0 };
			}

			const auto& buf = test.raw_buf();
			const auto& string_buf = test.raw_string_buf();
			const auto buf_len = test.raw_len();

			auto* simdjson_imple = test.raw_implementation().get();

			std::vector<int64_t> start(thr_num + 1, 0);
			//std::vector<int> key;

			auto a = std::chrono::steady_clock::now();
			{
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(a - _);

				log << info << dur.count() << "ms\n";
			}

			{
				size_t how_many = simdjson_imple->n_structural_indexes;
				length = how_many;

				start[0] = 0;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}
			}

			if (length == 0) {
				log << warn  << "empty json";
				return { true, 0 };
			}

			auto b = std::chrono::steady_clock::now();

			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);

			log << info  << "valid1 " << dur.count() << "ms\n";
			{
				auto a1 = std::chrono::steady_clock::now();

				if (!is_valid(test)) {
					return { false, 0 };
				}
				auto a2 = std::chrono::steady_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(a2 - a1);

				log << info << dur.count() << "ms\n";
			}
			//b= clock();

			start[thr_num] = length;
			if (false == claujson::LoadData2::parse(ut, buf.get(), buf_len, string_buf.get(), simdjson_imple, length, start, thr_num)) // 0 : use all thread..
			{
				return { false, 0 };
			}
			//int c = clock();
			//log << info  << c - b << "ms\n";
		}
		//int c = clock();
		//log << info << c - _ << "ms\n";


		return  { true, length };
	}
	std::pair<bool, size_t> parse_str(std::string_view str, Data& ut, size_t thr_num)
	{

		
		log << info << str << "\n";

		if (thr_num <= 0) {
			thr_num = std::thread::hardware_concurrency();
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		int64_t length;

		int _ = clock();

		{
			// not static?
			static _simdjson::dom::parser_for_claujson test;

			auto x = test.parse(str.data(), str.length());

			if (x.error() != _simdjson::error_code::SUCCESS) {
				log << warn  << "stage1 error : ";
				log << warn  << x.error() << "\n";

				return { false, 0 };
			}
			const auto& buf = test.raw_buf();
			const auto& string_buf = test.raw_string_buf();
			const auto buf_len = test.raw_len();
			auto* simdjson_imple_ = test.raw_implementation().get();

			std::vector<int64_t> start(thr_num + 1, 0);
			//std::vector<int> key;

			int a = clock();

			log << info << a - _ << "ms\n";


			{
				size_t how_many = simdjson_imple_->n_structural_indexes;
				length = how_many;

				start[0] = 0;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}
			}

			if (length == 0) {
				log << warn  << "empty json";
				return { true, 0 };
			}

			int b = clock();

			log << info << b - a << "ms\n";
			b = clock();

			if (!is_valid(test)) {
				return { false, 0 };
			}

			log << info << clock() - b << "ms\n";

			b = clock();

			start[thr_num] = length;

			if (false == claujson::LoadData2::parse(ut, buf.get(), buf_len, string_buf.get(), simdjson_imple_, length, start, thr_num)) // 0 : use all thread..
			{
				return { false, 0 };
			}
			int c = clock();
			log << info << c - b << "ms\n";
		}
		int c = clock();
		log << info << c - _ << "ms\n";


		return  { true, length };
	}
	

	std::string save_to_str(const Data& global) {
		return LoadData::save_to_str(global);
	}


	void save(const std::string& fileName, const Data& global) {
		LoadData::save(fileName, global, false);
	}

	void save_parallel(const std::string& fileName, Data& j, size_t thr_num) {
		LoadData::save_parallel(fileName, j, thr_num);
	}

	static std::string escape_for_json_pointer(std::string str) {
		// 1. ~ -> ~0
		// 2. / -> ~1
		// 1->2 ok, 2->1 no ok.
		{
			size_t idx = 0;
			idx = str.find('~');
			while (idx != std::string::npos) {
				str = str.replace(str.begin() + idx, str.begin() + idx + 1, "~0");
				idx = str.find("~", idx + 2);
			}
		}

		{
			size_t idx = 0;
			idx = str.find('/');
			while (idx != std::string::npos) {
				str = str.replace(str.begin() + idx, str.begin() + idx + 1, "~1");
				idx = str.find('/', idx + 2);
			}
		}

		return str;
	}


	// cf) /- -> / in array.
	static Data _diff(const Data& x, const Data& y, std::string route) {
		Data result(new Array());
		Json* j = result.as_json_ptr();

		if (x == y) {
			return result;
		}

		if (x.type() != y.type()) {
			Object* obj = new Object();

			obj->add_object_element(Data("op"sv), Data("replace"sv));
			obj->add_object_element(Data("path"sv), Data(route));
			obj->add_object_element(Data("value"sv), Data(y.clone()));

			j->add_object(Ptr<Json>(obj));
			return result;
		}

		switch (x.type()) {
		case DataType::ARRAY_OR_OBJECT:
		{
			const Json* jx = x.as_json_ptr();
			const Json* jy = y.as_json_ptr();

			if (jx->is_array()) {
				size_t i = 0;
				size_t sz_x = jx->get_data_size();
				size_t sz_y = jy->get_data_size();

				for (; i < sz_x && i < sz_y; ++i) {
					std::string new_route = route;
					new_route += '/';
					new_route += std::to_string(i);

					Data inner_diff = _diff(jx->get_value_list(i), jy->get_value_list(i), std::move(new_route));

					{
						Json* w = inner_diff.as_json_ptr();
						size_t sz_w = w->get_data_size();

						for (size_t t = 0; t < sz_w; ++t) {
							Data temp = std::move(w->get_value_list(t));
							j->add_object(Ptr<Json>(temp.as_json_ptr()));
						}

						Ptr<Json> clean(inner_diff.as_json_ptr());
					}
				}

				if (i < sz_x) {
					for (size_t _i = sz_x; _i > i; --i) {
						Object* obj = new Object();

						obj->add_object_element(Data("op"sv), Data("remove"sv));

						obj->add_object_element(Data("path"sv), Data(route));
						obj->add_object_element(Data("last_idx"sv), Data(_i - 1));

						j->add_object(Ptr<Json>(obj));
					}
				}
				else {
					for (; i < sz_y; ++i) {
						Object* obj = new Object();

						obj->add_object_element(Data("op"sv), Data("add"sv));

						obj->add_object_element(Data("path"sv), Data(route));

						obj->add_object_element(Data("value"sv), Data(jy->get_value_list(i).clone()));

						j->add_object(Ptr<Json>(obj));
					}
				}
			}
			else if (jx->is_object()) {
				size_t sz_x = jx->get_data_size();
				size_t sz_y = jy->get_data_size();

				for (size_t i = sz_x; i > 0; --i) {
					std::string key = jx->get_key_list(i - 1).str_val();
					std::string new_route = route;
					new_route += '/';
					new_route += escape_for_json_pointer(key);

					if (size_t idx = jy->find(key); idx != Json::npos) {
						Data inner_diff = _diff((jx->get_value_list(i - 1)), jy->get_value_list(idx), new_route);

						{
							Json* w = inner_diff.as_json_ptr();
							size_t sz_w = w->get_data_size();

							for (size_t t = 0; t < sz_w; ++t) {
								Data temp = std::move(w->get_value_list(t));
								j->add_object(Ptr<Json>(temp.as_json_ptr()));
							}

							Ptr<Json> clean(inner_diff.as_json_ptr());
						}
					}
					else {
						Object* obj = new Object();

						obj->add_object_element(Data("op"sv), Data("remove"sv));
						obj->add_object_element(Data("path"sv), Data(route));
						obj->add_object_element(Data("last_key"sv), Data(key));

						j->add_object(Ptr<Json>(obj));
					}
				}

				for (size_t i = 0; i < sz_y; ++i) {
					std::string key = jy->get_key_list(i).str_val();

					if (size_t idx = jx->find(key); idx == Json::npos) {
						Object* obj = new Object();

						obj->add_object_element(Data("op"sv), Data("add"sv));
						obj->add_object_element(Data("path"sv), Data(route));
						obj->add_object_element(Data("key"sv), Data(jy->get_key_list(i).clone()));
						obj->add_object_element(Data("value"sv), Data(jy->get_value_list(i).clone()));

						j->add_object(Ptr<Json>(obj));
					}
				}
			}
		}
		break;

		case DataType::BOOL:
		case DataType::NULL_:
		case DataType::FLOAT:
		case DataType::INT:
		case DataType::UINT:
		case DataType::STRING:
		{
			Object* obj = new Object();

			obj->add_object_element(Data("op"sv), Data("replace"sv));
			obj->add_object_element(Data("path"sv), Data(route));
			obj->add_object_element(Data("value"sv), Data(y.clone()));

			j->add_object(Ptr<Json>(obj));
			break;
		}
		}

		return result;
	}


	Data diff(const Data& x, const Data& y) {
		return _diff(x, y, "");
	}

	Data patch(const Data& x, const Data& diff) {
		Data unvalid_data(nullptr, false);

		const Json* j_diff = diff.as_json_ptr();
		
		if (!j_diff->is_array()) {
			return unvalid_data;
		}

		Data result = x.clone();

		size_t sz_diff = j_diff->get_data_size();

		for (size_t i = 0; i < sz_diff; ++i) {
			const Object* obj = (const Object*)j_diff->get_value_list(i).as_json_ptr();
			size_t op_idx = obj->find("op"sv);
			size_t path_idx = obj->find("path"sv);
			size_t value_idx = obj->find("value"sv);
			size_t key_idx = obj->find("key"sv);

			if (op_idx == Json::npos) {
				Ptr<Json> clean(result.as_json_ptr());
				return unvalid_data;
			}

			if (path_idx == Json::npos) {
				Ptr<Json> clean(result.as_json_ptr());
				return unvalid_data;
			}

			if (obj->get_value_list(op_idx).str_val() == "replace"sv) {
				if (value_idx == Json::npos) {
					Ptr<Json> clean(result.as_json_ptr());
					return unvalid_data;
				}

				Data& value = result.json_pointer(obj->get_value_list(path_idx).str_val());
				{
					if (value.is_ptr()) {
						Ptr<Json> clean(value.as_json_ptr());
						value.clear();
					}
				}
				value = obj->get_value_list(value_idx).clone();
			}
			else if (obj->get_value_list(op_idx).str_val() == "remove"sv) {
				Json* parent = result.json_pointer(obj->get_value_list(path_idx).str_val()).as_json_ptr();
				
				// case : result.json_pointer returns root?
				if (!parent) {
					if (result.is_ptr()) {
						Ptr<Json> clean(result.as_json_ptr());
					}
					result.clear();
				}
				else if (parent->is_array()) {
					size_t last_idx_idx = obj->find("last_idx"sv);
					if (last_idx_idx == Json::npos) {
						Ptr<Json> clean(result.as_json_ptr());
						return unvalid_data;
					}

					size_t last_idx = obj->get_value_list(last_idx_idx).uint_val();

					delete parent->get_value_list(last_idx).as_json_ptr(); // chk!
					parent->erase(last_idx);
				}
				else {
					size_t last_key_idx = obj->find("last_key"sv);
					if (last_key_idx == Json::npos) {
						Ptr<Json> clean(result.as_json_ptr());
						return unvalid_data;
					}

					std::string last_key = obj->get_value_list(last_key_idx).str_val();
					size_t _idx = parent->find(last_key);
					delete parent->get_value_list(_idx).as_json_ptr();
					parent->erase(_idx);
				}
			}
			else if (obj->get_value_list(op_idx).str_val() == "add"sv) {
				if (value_idx == Json::npos) {
					Ptr<Json> clean(result.as_json_ptr());
					return unvalid_data;
				}

				Data& _ = result.json_pointer(obj->get_value_list(path_idx).str_val());

				Json* parent = _.as_json_ptr();

				// case : result.json_pointer returns root?
				if (!parent) {
					result = obj->get_value_list(value_idx).clone();
				}
				else if (parent->is_array()) {
					parent->add_array_element(obj->get_value_list(value_idx).clone());
				}
				else if (parent->is_object()) {
					if (key_idx == Json::npos) {
						Ptr<Json> clean(result.as_json_ptr());
						return unvalid_data;
					}
					parent->add_object_element(obj->get_value_list(key_idx).clone(), obj->get_value_list(value_idx).clone());
				}
			}
		}

		return result;
	}

	void clean(Data& x) {
		Ptr<Json> _(x.as_json_ptr());
		x.set_null(); //
	}


	void init() {
		if (!simdjson_imple) {
			log.no_print();

			Data ut; 
			std::string_view str = "{}"sv;

			auto x = test_.parse(str.data(), str.length());
			simdjson_imple = test_.raw_implementation().get();

			clean(ut);
		}
	}
}

