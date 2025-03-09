#include "claujson.h"

namespace claujson {
	extern Log log;

	_Value _Value::clone() const {
		if (!is_valid()) {
			return _Value(nullptr, false);
		}

		_Value x;

		x._type = this->_type;

		if (x.is_str()) {
			x._str_val = (this->_str_val);
		}
		else {
			x._int_val = this->_int_val;
		}

		if (x.is_array()) {
			x._array_ptr = this->as_array()->clone();
		}
		else if (x.is_object()) {
			x._obj_ptr = this->as_object()->clone();
		}

		return x;
	}
	_Value::operator bool() const {
		return this->is_valid();
	}

	_Value::_Value(Array* x) {
		this->_int_val = 0;
		this->_type = _ValueType::ARRAY;
		this->_array_ptr = (x);
	}
	_Value::_Value(Object* x) {
		this->_int_val = 0;
		this->_type = _ValueType::OBJECT;
		this->_obj_ptr = (x);
	}
	_Value::_Value(PartialJson* x) {
		this->_int_val = 0;
		this->_type = _ValueType::PARTIAL_JSON;
		this->_pj_ptr = (x);
	}
	_Value::_Value(StructuredPtr x) {
		this->_int_val = 0;
		if (x.is_array()) {
			this->_type = _ValueType::ARRAY;
			this->_array_ptr = x.arr;
		}
		else if (x.is_object()) {
			this->_type = _ValueType::OBJECT;
			this->_obj_ptr = x.obj;
		}
		else if (x.is_partial_json()) {
			this->_type = _ValueType::PARTIAL_JSON;
			this->_pj_ptr = x.pj;
		}
		else {
			this->_type = _ValueType::ERROR;
		}
	}

	_Value::_Value(int x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_int(x);
	}

	_Value::_Value(unsigned int x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_uint(x);
	}

	_Value::_Value(int64_t x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_int(x);
	}
	_Value::_Value(uint64_t x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_uint(x);
	}
	_Value::_Value(double x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_float(x);
	}
	_Value::_Value(StringView x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		if (!set_str(x.data(), x.size())) {
			set_type(_ValueType::NOT_VALID);
		}
	}

#if __cpp_lib_char8_t
	// C++20~
	_Value::_Value(std::u8string_view x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		if (!set_str(reinterpret_cast<const char*>(x.data()), x.size())) {
			set_type(_ValueType::NOT_VALID);
		}
	}

	_Value::_Value(const char8_t* x) {
		std::u8string_view sv(x);
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		if (!set_str(reinterpret_cast<const char*>(sv.data()), sv.size())) {
			set_type(_ValueType::NOT_VALID);
		}
	}

#endif

	_Value::_Value(const char* x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		if (!set_str(x, strlen(x))) {
			set_type(_ValueType::NOT_VALID);
		}
	}

	_Value::_Value(bool x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_bool(x);
	}
	_Value::_Value(std::nullptr_t x) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_type(_ValueType::NULL_);
	}

	_Value::_Value(std::nullptr_t, bool valid) {
		this->_int_val = 0;
		this->_type = _ValueType::NONE;
		set_type(_ValueType::NULL_);
		if (!valid) {
			set_type(_ValueType::NOT_VALID);
		}
	}

	_ValueType _Value::type() const {
		return _type;
	}

	bool _Value::is_valid() const {
		return type() != _ValueType::NOT_VALID && type() != _ValueType::ERROR;
	}

	bool _Value::is_null() const {
		return is_valid() && type() == _ValueType::NULL_;
	}

	bool _Value::is_primitive() const {
		return is_valid() && !is_structured();
	}

	bool _Value::is_structured() const {
		return is_valid() && (type() == _ValueType::ARRAY || type() == _ValueType::OBJECT);
	}

	bool _Value::is_array() const {
		return is_valid() && type() == _ValueType::ARRAY;
	}

	bool _Value::is_object() const {
		return is_valid() && type() == _ValueType::OBJECT;
	}

	bool _Value::is_partial_json() const {
		return is_valid() && type() == _ValueType::PARTIAL_JSON;
	}

	bool _Value::is_int() const {
		return is_valid() && type() == _ValueType::INT;
	}

	bool _Value::is_uint() const {
		return is_valid() && type() == _ValueType::UINT;
	}

	bool _Value::is_float() const {
		return is_valid() && type() == _ValueType::FLOAT;
	}

	bool _Value::is_bool() const {
		return is_valid() && (type() == _ValueType::BOOL);
	}

	bool _Value::is_str() const {
		return is_valid() && (type() == _ValueType::STRING || type() == _ValueType::SHORT_STRING);
	}

	int64_t _Value::int_val() const {
		return _int_val;
	}

	uint64_t _Value::uint_val() const {
		return _uint_val;
	}

	double _Value::float_val() const {
		return _float_val;
	}

	int64_t& _Value::int_val() {
		return _int_val;
	}

	uint64_t& _Value::uint_val() {
		return _uint_val;
	}

	double& _Value::float_val() {
		return _float_val;
	}

	bool _Value::bool_val() const {
		if (!is_bool()) {
			return false;
		}
		return _bool_val;
	}


	bool& _Value::bool_val() {
		return _bool_val;
	}

	// this _Value is Array or Object.
	_Value& _Value::json_pointerB(const std_vector<_Value>& routeVec) {
		static _Value unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (routeVec.empty()) {
			return *this;
		}

		// 4. find Data with route. and return
		_Value* data = this;

		for (uint64_t i = 0; i < routeVec.size(); ++i) {
			bool fail = false;
			auto& x = routeVec[i];
			if (fail) {
				// log << warn..
				return unvalid_data;
			}

			if (data->is_primitive()) {
				return unvalid_data;
			}

			if (data->is_array()) { // array -> with idx
				Array* j = data->as_array();
				uint64_t idx = x.get_unsigned_integer();
				//bool found = false;
				//uint64_t arr_size = j->get_data_size();

				data = &j->get_value_list(idx);
			}
			else if (data->is_object()) { // object -> with key
				Object* j = data->as_object();
				data = &((*j)[x]);
			}
		}

		return *data;
	}

	const _Value& _Value::json_pointerB(const std_vector<_Value>& routeVec) const { // option-> StringView route?
		static _Value unvalid_data(nullptr, false);

		if (is_structured() == false) {
			return unvalid_data;
		}

		// the whole document.
		if (routeVec.empty()) {
			return *this;
		}

		// 4. find Data with route. and return
		const _Value* data = this;

		for (uint64_t i = 0; i < routeVec.size(); ++i) {
			bool fail = false;
			auto& x = routeVec[i];
			if (fail) {
				// log << warn..
				return unvalid_data;
			}

			if (data->is_primitive()) {
				return unvalid_data;
			}

			if (data->is_array()) { // array -> with idx
				const Array* j = data->as_array();
				uint64_t idx = x.get_unsigned_integer();
				//bool found = false;
				//uint64_t arr_size = j->get_data_size();

				data = &j->get_value_list(idx);
			}
			else if (data->is_object()) { // object -> with key
				const Object* j = data->as_object();
				data = &((*j)[x]);
			}
		}

		return *data;
	}


	void _Value::clear(bool remove_str) {

		if (remove_str && is_str()) {
			_str_val.clear();
			_int_val = 0;
			temp = 0;
			_type = _ValueType::NONE;
		}
		else if (is_str()) {
			//
		}
		else {
			_int_val = 0;
			temp = 0;
			_type = _ValueType::NONE;
		}
	}

	String& _Value::str_val() {
		// type check...
		return _str_val;
	}

	const String& _Value::str_val() const {
		// type check...
		return _str_val;
	}

	void _Value::set_int(long long x) {
		if (!is_valid()) {
			return;
		}

		if (is_str()) {
			_str_val.clear();
		}
		_int_val = x;
		_type = _ValueType::INT;
	}

	void _Value::set_uint(unsigned long long x) {
		if (!is_valid()) {
			return;
		}
		if (is_str()) {
			_str_val.clear();
		}
		_uint_val = x;
		_type = _ValueType::UINT;
	}

	void _Value::set_float(double x) {
		if (!is_valid()) {
			return;
		}
		if (is_str()) {
			_str_val.clear();
		}
		_float_val = x;

		_type = _ValueType::FLOAT;
	}

	bool _Value::set_str(const char* str, uint64_t len) {

		bool convert = true;

		if (!is_valid()) {
			return false;
		}

		if (!convert) {
			_str_val = String(str, Static_Cast<uint64_t, uint32_t>(len));
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
				bool valid = _simdjson::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					free(buf_src);
					free(buf_dest);

					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return false;
				}
			}

			auto* x = _simdjson::parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				free(buf_src);
				free(buf_dest);

				log << warn << "Error in Convert for string";
				return false;
			}
			else {
				*x = '\0';
				uint32_t string_length = uint32_t(x - buf_dest);

				if (is_str() == false) {
					_str_val = String((char*)buf_dest, string_length);
				}
				else {
					_str_val = String((char*)buf_dest, string_length);
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
				bool valid = _simdjson::validate_utf8(reinterpret_cast<char*>(buf_src), len);

				if (!valid) {
					log << warn << "not valid utf8" << "\n";
					log << warn << "Error in Convert for string, validate...";
					return false;
				}
			}
			auto* x = _simdjson::parse_string(buf_src, buf_dest, false);
			if (x == nullptr) {
				log << warn << "Error in Convert for string";
				return false;
			}
			else {
				*x = '\0';
				uint32_t string_length = uint32_t(x - buf_dest);

				if (!is_str()) {
					_str_val = String((char*)buf_dest, string_length);
				}
				else {
					_str_val = String((char*)buf_dest, string_length);
				}
			}
		}

		//_type = _ValueType::STRING;

		return true;
	}



	bool _Value::set_str(String str) {
		if (!is_valid()) {
			return false;
		}
		if (is_str()) {
			_str_val.clear();
		}

		_str_val = std::move(str);
		return true;
	}

	void _Value::set_str_in_parse(const char* str, uint64_t len) {
		_str_val = String(str, Static_Cast<uint64_t, uint32_t>(len));
	}

	void _Value::set_bool(bool x) {
		if (!is_valid()) {
			return;
		}
		if (is_str()) {
			_str_val.clear();
		}

		_bool_val = x;

		{
			set_type(_ValueType::BOOL);
		}
	}

	void _Value::set_null() {
		if (!is_valid()) {
			return;
		}
		if (is_str()) {
			_str_val.clear();
		}

		set_type(_ValueType::NULL_);
	}

	void _Value::set_type(_ValueType type) {
		this->_type = type;
	}

	_Value::~_Value() {
		if (is_str()) {
			_str_val.clear();
		}
	}

	_Value::_Value(_Value&& other) noexcept
		: _type(_ValueType::NONE)
	{
		if (!other.is_valid()) {
			return;
		}

		if (other.is_str()) {
			_str_val = std::move(other._str_val);
		}
		else {
			std::swap(_int_val, other._int_val);
			std::swap(this->_type, other._type);
		}
	}

	_Value::_Value() : _int_val(0), _type(_ValueType::NONE) {}

	bool _Value::operator==(const _Value& other) const { // chk array or object?
		if (this->_type == other._type) {
			switch (this->_type) {
			case _ValueType::STRING:
			case _ValueType::SHORT_STRING:
				return this->_str_val == other._str_val;
				break;
			case _ValueType::INT:
				return this->_int_val == other._int_val;
				break;
			case _ValueType::UINT:
				return this->_uint_val == other._uint_val;
				break;
			case _ValueType::FLOAT:
				return this->_float_val == other._float_val;
				break;
			case _ValueType::BOOL:
				return this->_bool_val == other._bool_val;
				break;
			case _ValueType::ARRAY:
			{
				const Array* j = this->as_array();
				const Array* k = other.as_array();

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
			}
			break;
			case _ValueType::OBJECT:
				{
				const Object* j = this->as_object();
				const Object* k = other.as_object();

				const uint64_t sz_j = j->get_data_size();
				const uint64_t sz_k = k->get_data_size();

				if (sz_j != sz_k) { return false; }

				const uint64_t sz = sz_j;

				if (j->is_object() && k->is_object()) {
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

	bool _Value::operator!=(const _Value& other) const {
		return !((*this) == other);
	}

	bool _Value::operator<(const _Value& other) const {
		if (this->_type == other._type) {
			switch (this->_type) {
			case _ValueType::STRING:
			case _ValueType::SHORT_STRING:
				return this->_str_val < other._str_val;
				break;
			case _ValueType::INT:
				return this->_int_val < other._int_val;
				break;
			case _ValueType::UINT:
				return this->_uint_val < other._uint_val;
				break;
			case _ValueType::FLOAT:
				return this->_float_val < other._float_val;
				break;
			case _ValueType::BOOL:
				return this->_bool_val < other._bool_val;
				break;
			}

		}
		return false;
	}


	_Value& _Value::operator=(_Value&& other) noexcept {
		if (this == &other) {
			return *this;
		}

		if (!is_valid()) {
			return *this;
		}

		std::swap(this->_type, other._type);
		std::swap(this->_int_val, other._int_val);
		std::swap(this->temp, other.temp);
		
		clean(other);

		return *this;
	}

	StructuredPtr _Value::as_structured() {
		return StructuredPtr{ as_array(), as_object(), as_partial_json() };
	}

	bool _Value::is_virtual() const {
		if (is_array()) {
			return as_array()->is_virtual();
		}
		if (is_object()) {
			return as_object()->is_virtual();
		}
		return false;
	}

	Array* _Value::as_array() {
		if (is_array()) {
			return static_cast<Array*>(_array_ptr);
		}
		return nullptr;
	}

	Object* _Value::as_object() {
		if (is_object()) {
			return static_cast<Object*>(_obj_ptr);
		}
		return nullptr;
	}

	PartialJson* _Value::as_partial_json() {
		if (is_partial_json()) {
			return static_cast<PartialJson*>(_pj_ptr);
		}
		return nullptr;
	}

	StructuredPtr _Value::as_structured_ptr() {
		if (is_array()) {
			return { _array_ptr };
		}
		else if (is_object()) {
			return { _obj_ptr };
		}
		else if (is_partial_json()) {
			return { _pj_ptr };
		}
		return { nullptr };
	}

	const Array* _Value::as_array() const {
		if (is_array()) {
			return const_cast<const Array*>(_array_ptr);
		}
		return nullptr;
	}

	const Object* _Value::as_object() const {
		if (is_object()) {
			return const_cast<const Object*>(_obj_ptr);
		}
		return nullptr;
	}
	const PartialJson* _Value::as_partial_json() const {
		if (is_partial_json()) {
			return const_cast<const PartialJson*>(_pj_ptr);
		}
		return nullptr;
	}


	const StructuredPtr _Value::as_structured_ptr()const {
		if (is_array()) {
			return { _array_ptr };
		}
		else if (is_object()) {
			return { _obj_ptr };
		}
		else if (is_partial_json()) {
			return { _pj_ptr };
		}
		return { nullptr };
	}

	_Value& _Value::operator[](uint64_t idx) {
		static _Value empty_value(nullptr, false);

		if (is_array()) {
			return as_array()->operator[](idx);
		}
		if (is_object()) {
			return as_object()->operator[](idx);
		}

		return empty_value;
	}

	const _Value& _Value::operator[](uint64_t idx) const {
		static const _Value empty_value(nullptr, false);

		if (is_array()) {
			return as_array()->operator[](idx);
		}
		if (is_object()) {
			return as_object()->operator[](idx);
		}

		return empty_value;
	}

	uint64_t _Value::find(const _Value& key) const { // find without key`s converting?
		if (is_object()) {
			return as_object()->find(key);
		}

		return npos;
	}


	_Value& _Value::operator[](const _Value& key) { // if not exist key, then nothing.
		if (is_object()) {
			return as_object()->operator[](key);
		}

		return empty_value;
	}
	const _Value& _Value::operator[](const _Value& key) const { // if not exist key, then nothing.
		if (is_object()) {
			return as_object()->operator[](key);
		}

		return empty_value;
	}

}
