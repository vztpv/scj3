
#define _CRT_SECURE_NO_WARNINGS

#include "claujson.h"
#include "simdjson.h" // modified simdjson // using simdjson 2.0.1

#if SIMDJSON_IMPLEMENTATION_ICELAKE
#define SIMDJSON_IMPLEMENTATION icelake
#elif SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION haswell //
#elif SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION westmere
#elif SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION arm64
#elif SIMDJSON_IMPLEMENTATION_PPC64
#define SIMDJSON_IMPLEMENTATION ppc64
#elif SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION fallback
#else
#error "All possible implementations (including fallback) have been disabled! simdjson will not run."
#endif


// for fast save
#include "fmt/format.h"
#include "fmt/compile.h"

#include <string_view>
using namespace std::string_view_literals;

namespace simdjson {

	// fallback..
	simdjson_really_inline bool is_continuation(uint8_t c) {
		return (c & 0b11000000) == 0b10000000;
	}

	simdjson_really_inline void validate_utf8_character(uint8_t* buf, size_t& idx, size_t len, error_code& error) {
		stage1_mode partial = stage1_mode::regular;

		// Continuation
		if (simdjson_unlikely((buf[idx] & 0b01000000) == 0)) {
			// extra continuation
			error = UTF8_ERROR;
			idx++;
			return;
		}

		// 2-byte
		if ((buf[idx] & 0b00100000) == 0) {
			// missing continuation
			if (simdjson_unlikely(idx + 1 > len || !is_continuation(buf[idx + 1]))) {
				if (idx + 1 > len && is_streaming(partial)) { idx = len; return; }
				error = UTF8_ERROR;
				idx++;
				return;
			}
			// overlong: 1100000_ 10______
			if (buf[idx] <= 0b11000001) { error = UTF8_ERROR; }
			idx += 2;
			return;
		}

		// 3-byte
		if ((buf[idx] & 0b00010000) == 0) {
			// missing continuation
			if (simdjson_unlikely(idx + 2 > len || !is_continuation(buf[idx + 1]) || !is_continuation(buf[idx + 2]))) {
				if (idx + 2 > len && is_streaming(partial)) { idx = len; return; }
				error = UTF8_ERROR;
				idx++;
				return;
			}
			// overlong: 11100000 100_____ ________
			if (buf[idx] == 0b11100000 && buf[idx + 1] <= 0b10011111) { error = UTF8_ERROR; }
			// surrogates: U+D800-U+DFFF 11101101 101_____
			if (buf[idx] == 0b11101101 && buf[idx + 1] >= 0b10100000) { error = UTF8_ERROR; }
			idx += 3;
			return;
		}

		// 4-byte
		// missing continuation
		if (simdjson_unlikely(idx + 3 > len || !is_continuation(buf[idx + 1]) || !is_continuation(buf[idx + 2]) || !is_continuation(buf[idx + 3]))) {
			if (idx + 2 > len && is_streaming(partial)) { idx = len; return; }
			error = UTF8_ERROR;
			idx++;
			return;
		}
		// overlong: 11110000 1000____ ________ ________
		if (buf[idx] == 0b11110000 && buf[idx + 1] <= 0b10001111) { error = UTF8_ERROR; }
		// too large: > U+10FFFF:
		// 11110100 (1001|101_)____
		// 1111(1___|011_|0101) 10______
		// also includes 5, 6, 7 and 8 byte characters:
		// 11111___
		if (buf[idx] == 0b11110100 && buf[idx + 1] >= 0b10010000) { error = UTF8_ERROR; }
		if (buf[idx] >= 0b11110101) { error = UTF8_ERROR; }
		idx += 4;
	}


	simdjson_really_inline bool validate_string(uint8_t* buf, size_t len, error_code& error) {
		size_t idx = 0; //

		while (idx < len) {
			if (buf[idx] == '\\') {
				if (idx + 1 >= len) {
					return false;
				}
				idx += 2;
			}
			else if (simdjson_unlikely(buf[idx] & 0b10000000)) {
				validate_utf8_character(buf, idx, len, error);
			}
			else {
				if (buf[idx] < (uint8_t)0x20) { error = UNESCAPED_CHARS; }
				idx++;
			}
		}
		if (idx >= len) { return true; }
		return false;

	}
}

namespace simdjson {

	// fallback
	struct writer {
		/** The next place to write to tape */
		uint64_t* next_tape_loc;

		/** Write a signed 64-bit value to tape. */
		simdjson_really_inline void append_s64(int64_t value) noexcept;

		/** Write an unsigned 64-bit value to tape. */
		simdjson_really_inline void append_u64(uint64_t value) noexcept;

		/** Write a double value to tape. */
		simdjson_really_inline void append_double(double value) noexcept;

		/**
		 * Append a tape entry (an 8-bit type,and 56 bits worth of value).
		 */
		simdjson_really_inline void append(uint64_t val, internal::tape_type t) noexcept;

		/**
		 * Skip the current tape entry without writing.
		 *
		 * Used to skip the start of the container, since we'll come back later to fill it in when the
		 * container ends.
		 */
		simdjson_really_inline void skip() noexcept;

		/**
		 * Skip the number of tape entries necessary to write a large u64 or i64.
		 */
		simdjson_really_inline void skip_large_integer() noexcept;

		/**
		 * Skip the number of tape entries necessary to write a double.
		 */
		simdjson_really_inline void skip_double() noexcept;

		/**
		 * Write a value to a known location on tape.
		 *
		 * Used to go back and write out the start of a container after the container ends.
		 */
		simdjson_really_inline static void write(uint64_t& tape_loc, uint64_t val, internal::tape_type t) noexcept;

	private:
		/**
		 * Append both the tape entry, and a supplementary value following it. Used for types that need
		 * all 64 bits, such as double and uint64_t.
		 */
		template<typename T>
		simdjson_really_inline void append2(uint64_t val, T val2, internal::tape_type t) noexcept;
	}; // struct number_writer

	simdjson_really_inline void writer::append_s64(int64_t value) noexcept {
		append2(0, value, internal::tape_type::INT64);
	}

	simdjson_really_inline void writer::append_u64(uint64_t value) noexcept {
		append(0, internal::tape_type::UINT64);
		*next_tape_loc = value;
		next_tape_loc++;
	}

	/** Write a double value to tape. */
	simdjson_really_inline void writer::append_double(double value) noexcept {
		append2(0, value, internal::tape_type::DOUBLE);
	}

	simdjson_really_inline void writer::skip() noexcept {
		next_tape_loc++;
	}

	simdjson_really_inline void writer::skip_large_integer() noexcept {
		next_tape_loc += 2;
	}

	simdjson_really_inline void writer::skip_double() noexcept {
		next_tape_loc += 2;
	}

	simdjson_really_inline void writer::append(uint64_t val, internal::tape_type t) noexcept {
		*next_tape_loc = val | ((uint64_t(char(t))) << 56);
		next_tape_loc++;
	}

	template<typename T>
	simdjson_really_inline void writer::append2(uint64_t val, T val2, internal::tape_type t) noexcept {
		append(val, t);
		static_assert(sizeof(val2) == sizeof(*next_tape_loc), "Type is not 64 bits!");
		memcpy(next_tape_loc, &val2, sizeof(val2));
		next_tape_loc++;
	}

	simdjson_really_inline void writer::write(uint64_t& tape_loc, uint64_t val, internal::tape_type t) noexcept {
		tape_loc = val | ((uint64_t(char(t))) << 56);
	}
}

namespace claujson {

	// class Root, only used in class LoadData.
	class Root : public Json {
	protected:
		std::vector<Data> arr_vec; // 
		// in parsing...
		std::vector<Data> obj_key_vec;
		std::vector<Data> obj_val_vec;

		Data virtualJson;
	public:
		virtual ~Root();

	private:
		friend class LoadData;
		friend class LoadData2;

		Root();

	public:
		virtual bool is_root() const;

		virtual bool is_object() const;

		virtual bool is_array() const;

		virtual bool is_element() const;

		virtual size_t get_data_size() const;

		virtual Data& get_value_list(size_t idx);


		virtual Data& get_key_list(size_t idx);


		virtual const Data& get_value_list(size_t idx) const;


		virtual const Data& get_key_list(size_t idx) const;

		virtual void clear(size_t idx);

		virtual bool is_virtual() const;

		virtual void clear();

		virtual void reserve_data_list(size_t len);



		virtual void add_object_element(Data key, Data val);
		virtual void add_array_element(Data val);

		virtual void add_array(Ptr<Json> arr);
		virtual void add_object(Ptr<Json> obj);

		virtual void insert_array_element(size_t idx, Data val);

		virtual void erase(std::string_view key);

		virtual void erase(size_t idx);


	private:
		virtual void Link(Ptr<Json> j);

		virtual void add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2);

		virtual void add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id);


		virtual void add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
			uint8_t* string_buf, int type, uint64_t id);

		virtual void add_user_type(int type);

		virtual void add_user_type(Ptr<Json> j);


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
			stream << "not valid\n";
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
	Data::operator bool() const {
		return valid;
	}

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

		try {
			this->valid = set_str(x.data(), x.size());
		}
		catch (...) {
			this->valid = false;
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

	Data::Data(nullptr_t, bool valid) : valid(valid) {
		//
	}

	DataType Data::type() const {
		return _type;
	}

	bool Data::is_valid() const {
		return valid;
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

	void* Data::ptr_val() const {
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


	bool to_uint_for_json_pointer(std::string_view x, size_t* val) {
		const char* buf = x.data();
		size_t idx = 0;
		size_t idx2 = x.size();

		switch (x[0]) {
			case '0':
			case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			{
				std::unique_ptr<uint8_t[]> copy;

				uint64_t temp[2];
				simdjson::writer writer{ temp };
				const uint8_t* value = reinterpret_cast<const uint8_t*>(buf + idx);

				{ // chk code...
					copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[idx2 - idx + simdjson::SIMDJSON_PADDING]); // x.size() + padding
					if (copy.get() == nullptr) { return false; } // cf) new Json?
					std::memcpy(copy.get(), &buf[idx], idx2 - idx);
					std::memset(copy.get() + idx2 - idx, ' ', simdjson::SIMDJSON_PADDING);
					value = copy.get();
				}

				if (auto x = simdjson::SIMDJSON_IMPLEMENTATION::numberparsing::parse_number<simdjson::writer>(value, writer)
					; x != simdjson::error_code::SUCCESS) {
					std::cout << "parse number error. " << x << "\n";
					return false;
				}

				long long int_val = 0;
				unsigned long long uint_val = 0;
				double float_val = 0;

				switch (static_cast<simdjson::internal::tape_type>(temp[0] >> 56)) {
				case simdjson::internal::tape_type::INT64:
					memcpy(&int_val, &temp[1], sizeof(uint64_t));
					*val = int_val;

					return true;
					break;
				case simdjson::internal::tape_type::UINT64:
					memcpy(&uint_val, &temp[1], sizeof(uint64_t));
					*val = uint_val;
					
					return true;
					break;
				case simdjson::internal::tape_type::DOUBLE:
					// error.
					return false;
					break;
				}
			}
			break;
		}

		return false;
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
			routeDataVec.push_back(std::move(temp));
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

				bool chk = to_uint_for_json_pointer(x.str_val(), &idx);

				if (!chk) {
					return unvalid_data;
				}

				data = &j->get_value_list(idx);
			}
			else if (j->is_object()) { // object -> with key
				std::string_view str = x.str_val();
				std::string result;

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
		if (_type == DataType::STRING) {
			delete _str_val;
		}
		_int_val = x;
		_type = DataType::INT;
	}

	void Data::set_uint(unsigned long long x) {
		if (_type == DataType::STRING) {
			delete _str_val;
		}
		_uint_val = x;
		_type = DataType::UINT;
	}

	void Data::set_float(double x) {
		if (_type == DataType::STRING) {
			delete _str_val;
		}
		_float_val = x;

		_type = DataType::FLOAT;
	}

	bool Data::set_str(const char* str, size_t len) {

		const size_t block_size = 1024;


		uint8_t buf_src[block_size + simdjson::SIMDJSON_PADDING];
		uint8_t buf_dest[block_size + simdjson::SIMDJSON_PADDING];


		if (len >= block_size) {
			uint8_t* buf_src = (uint8_t*)calloc(len + simdjson::SIMDJSON_PADDING, sizeof(uint8_t));
			uint8_t* buf_dest = (uint8_t*)calloc(len + simdjson::SIMDJSON_PADDING, sizeof(uint8_t));
			if (!buf_src || !buf_dest) {
				if (buf_src) { free(buf_src); }
				if (buf_dest) { free(buf_dest); }

				return false;
			}
			memset(buf_src, '"', len + simdjson::SIMDJSON_PADDING);
			memset(buf_dest, '"', len + simdjson::SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			// chk... fallback..
			{

				simdjson::error_code e = simdjson::error_code::SUCCESS;

				bool valid = simdjson::validate_string(buf_src, len, e);

				if (!valid || e != simdjson::error_code::SUCCESS) {
					free(buf_src);
					free(buf_dest);

					std::cout << simdjson::error_message(e) << "\n";
					std::cout << "Error in Convert for string, validate...";
					return false;
				}
			}

			if (auto* x = simdjson::SIMDJSON_IMPLEMENTATION::stringparsing::parse_string(buf_src, buf_dest); x == nullptr) {
				free(buf_src);
				free(buf_dest);

				std::cout << "Error in Convert for string";
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
			memset(buf_src, '"', block_size + simdjson::SIMDJSON_PADDING);
			memset(buf_dest, '"', block_size + simdjson::SIMDJSON_PADDING);

			memcpy(buf_src, str, len);
			buf_src[len] = '"';

			{
				simdjson::error_code e = simdjson::error_code::SUCCESS;

				bool valid = simdjson::validate_string(buf_src, len, e);

				if (!valid || e != simdjson::error_code::SUCCESS) {
					std::cout << simdjson::error_message(e) << "\n";
					std::cout << "Error in Convert for string, validate...";
					return false;
				}
			}

			if (auto* x = simdjson::SIMDJSON_IMPLEMENTATION::stringparsing::parse_string(buf_src, buf_dest); x == nullptr) {
				std::cout << "Error in Convert for string";
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
		if (_type == DataType::STRING) {
			delete _str_val;
		}

		_bool_val = x;

		{
			set_type(DataType::BOOL);
		}
	}

	void Data::set_null() {
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
			//std::cout << "chk";
			delete _str_val;
			_str_val = nullptr;
		}
	}

	Data::Data(const Data& other)
		: _type(other._type) //, is_key(other.is_key) 
	{
		if (_type == DataType::STRING) {
			_str_val = new std::string(other._str_val->c_str(), other._str_val->size());

		}
		else {
			_int_val = other._int_val;
		}
		valid = other.valid;
	}

	Data::Data(Data&& other) noexcept
		: _type(other._type) //, is_key(other.is_key) 
	{

		if (_type == DataType::STRING) {
			_str_val = other._str_val;
			other._str_val = nullptr;
			other._type = DataType::NONE;
		}
		else {
			std::swap(_int_val, other._int_val);
		}

		std::swap(valid, other.valid);
	}

	Data::Data() : _int_val(0), _type(DataType::NONE) { }

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

	Data& Data::operator=(const Data& other) {
		if (this == &other) {
			return *this;
		}

		if (this->_type != DataType::STRING && other._type == DataType::STRING) {
			this->_str_val = new std::string();
		}
		else if (this->_type == DataType::STRING && other._type != DataType::STRING) {
			delete this->_str_val;
		}


		if (this->_type == DataType::STRING) {
			set_str(other._str_val->c_str(), other._str_val->size());
		}
		else {
		}

		this->_type = other._type; // fixed bug..
		this->valid = other.valid;

		return *this;
	}


	Data& Data::operator=(Data&& other) noexcept {
		if (this == &other) {
			return *this;
		}

		std::swap(this->_type, other._type);
		std::swap(this->_int_val, other._int_val);

		std::swap(valid, other.valid);

		return *this;
	}


	claujson::Data& Convert(claujson::Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
		char* buf, uint8_t* string_buf, uint64_t id, bool& err) {

		try {
			data.clear();

			uint32_t string_length;

			switch (buf[idx]) {
			case '"':
			{
				if (auto* x = simdjson::SIMDJSON_IMPLEMENTATION::stringparsing::parse_string((uint8_t*)&buf[idx] + 1,
					&string_buf[idx]); x == nullptr) {
					throw "Error in Convert for string";
				}
				else {
					*x = '\0';
					string_length = uint32_t(x - &string_buf[idx]);
				}

				// chk token_arr_start + i + 1 >= imple->n_structural_indexes...
				data.set_str_in_parse(reinterpret_cast<char*>(&string_buf[idx]), string_length);
			}
			break;
			case 't':
			{
				if (!simdjson::SIMDJSON_IMPLEMENTATION::atomparsing::is_valid_true_atom(reinterpret_cast<uint8_t*>(&buf[idx]), idx2 - idx)) {
					throw "Error in Convert for true";
				}

				data.set_bool(true);
			}
			break;
			case 'f':
				if (!simdjson::SIMDJSON_IMPLEMENTATION::atomparsing::is_valid_false_atom(reinterpret_cast<uint8_t*>(&buf[idx]), idx2 - idx)) {
					throw "Error in Convert for false";
				}

				data.set_bool(false);
				break;
			case 'n':
				if (!simdjson::SIMDJSON_IMPLEMENTATION::atomparsing::is_valid_null_atom(reinterpret_cast<uint8_t*>(&buf[idx]), idx2 - idx)) {
					throw "Error in Convert for null";
				}

				data.set_null();
				break;
			case '-':
			case '0':
			case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			{
				std::unique_ptr<uint8_t[]> copy;

				uint64_t temp[2];
				simdjson::writer writer{ temp };
				uint8_t* value = reinterpret_cast<uint8_t*>(buf + idx);

				// chk code...
				if (id == 0) {
					copy = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[idx2 - idx + simdjson::SIMDJSON_PADDING]);
					if (copy.get() == nullptr) { throw "Error in Convert for new"; } // cf) new Json?
					std::memcpy(copy.get(), &buf[idx], idx2 - idx);
					std::memset(copy.get() + idx2 - idx, ' ', simdjson::SIMDJSON_PADDING);
					value = copy.get();
				}

				if (auto x = simdjson::SIMDJSON_IMPLEMENTATION::numberparsing::parse_number<simdjson::writer>(value, writer)
					; x != simdjson::error_code::SUCCESS) {
					std::cout << "parse number error. " << x << "\n";
					throw "Error in Convert to parse number";
				}

				long long int_val = 0;
				unsigned long long uint_val = 0;
				double float_val = 0;

				switch (static_cast<simdjson::internal::tape_type>(temp[0] >> 56)) {
				case simdjson::internal::tape_type::INT64:
					memcpy(&int_val, &temp[1], sizeof(uint64_t));

					data.set_int(int_val);
					break;
				case simdjson::internal::tape_type::UINT64:
					memcpy(&uint_val, &temp[1], sizeof(uint64_t));

					data.set_uint(uint_val);
					break;
				case simdjson::internal::tape_type::DOUBLE:
					memcpy(&float_val, &temp[1], sizeof(uint64_t));

					data.set_float(float_val);
					break;
				}

				break;
			}
			default:
				std::cout << "convert error : " << (int)buf[idx] << " " << buf[idx] << "\n";
				throw "Error in Convert : not expected";
			}
			return data;
		}
		catch (const char* str) {
			std::cout << str << "\n";
			err = true;
			return data;
		}
	}

	bool Json::is_valid() const {
		return valid;
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

		size_t Json::find(std::string_view key) { // chk (uint64_t)-1 == (maximum)....-> eof?
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
				auto val = get_value_list(idx);
				erase(idx);
				add_object_element(key, std::move(val));
				return true;
			}
			return false;
		}

		Data& Json::get_value() {
			return data_null;
		}

		bool Json::is_root() const { return false; }

		bool Json::is_user_type() const {
			return is_object() || is_array() || is_root();
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
						*idx = i - 1;
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
		bool Object::is_element() const {
			return false;
		}
		size_t Object::get_data_size() const {
			return obj_val_vec.size();
		}

		Data& Object::get_value_list(size_t idx) {
			return obj_val_vec[idx];
		}

		Data& Object::get_key_list(size_t idx) { // if key change then also obj_vec[idex].second.key? change??
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

		void Object::add_object_element(Data key, Data val) {
			if (!is_valid()) {
				return;
			}

			if (val.is_ptr()) {
				auto* x = (Json*)val.ptr_val();
				x->set_key(key); // no need?
			}
			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(std::move(val));
		}

		void Object::add_array_element(Data val) { std::cout << "err"; }
		void Object::add_array(Ptr<Json> arr) {
			if (!is_valid()) {
				return;
			}

			if (arr->has_key()) {
				obj_key_vec.push_back(arr->get_key());
				obj_val_vec.push_back(Data(arr.release()));
			}
			else {
				std::cout << "err";
			}
		}
		void Object::add_object(Ptr<Json> obj) {
			if (!is_valid()) {
				return;
			}

			if (obj->has_key()) {
				obj_key_vec.push_back(obj->get_key());
				obj_val_vec.push_back(Data(obj.release()));
			}
			else {
				std::cout << "err";
			}
		}

		void Object::insert_array_element(size_t idx, Data val) { std::cout << "err"; }

		void Object::erase(std::string_view key) {
			size_t idx = this->find(key);
			erase(idx);
		}

		void Object::erase(size_t idx) {
			if (!is_valid()) {
				return;
			}

			obj_key_vec.erase(obj_key_vec.begin() + idx);
			obj_val_vec.erase(obj_val_vec.begin() + idx);
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
				std::cout << "Link errr1";
				return;
			}

			j->set_parent(this);

			obj_key_vec.push_back(j->get_key());
			obj_val_vec.push_back(Data(j.release()));
		}

		void Object::add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2) {
			if (!is_valid()) {
				return;
			}
			{
				Data temp;// key
				Data temp2;

				bool e = false;

				claujson::Convert(temp, idx11, idx12, len1, true, buf, string_buf, id, e);

				if (e) {
					throw "Error in add_item_type";
				}
				claujson::Convert(temp2, idx21, idx22, len2, false, buf, string_buf, id2, e);
				if (e) {
					throw "Error in add_item_type";
				}

				if (temp.type() != DataType::STRING) {
					throw "Error in add_item_type, key is not string";
				}

				obj_key_vec.push_back(std::move(temp));
				obj_val_vec.push_back(std::move(temp2));
			}
		}

		void Object::add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id) {
			// error

			std::cout << "errr..";
		}

		void Object::add_user_type(int type) {
			// error

			std::cout << "errr..";

			return;
		}

		void Object::add_user_type(Ptr<Json> j) {
			if (!is_valid()) {
				return;
			}

			if (j->is_virtual()) {
				j->set_parent(this);

				obj_key_vec.push_back(Data());
				obj_val_vec.push_back(Data(j.release()));
			}
			else if (j->has_key()) {
				j->set_parent(this);

				obj_key_vec.push_back(j->get_key());
				obj_val_vec.push_back(Data(j.release()));
			}
			else {
				std::cout << "chk..";
				return;
			}
		}

		 Array::Array(bool valid) : Json(valid) { }
	
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
		bool Array::is_element() const {
			return false;
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

		void Array::add_object_element(Data key, Data val) {
			std::cout << "err";
		}

		void Array::add_array_element(Data val) {
			if (!is_valid()) {
				return;
			}

			arr_vec.push_back(std::move(val));

		}
		
		void Array::add_array(Ptr<Json> arr) {
			if (!is_valid()) {
				return;
			}

			if (!arr->has_key()) {
				arr_vec.push_back(Data(arr.release()));
			}
			else {
				std::cout << "err";
			}
		}
		void Array::add_object(Ptr<Json> obj) {
			if (!is_valid()) {
				return;
			}

			if (!obj->has_key()) {
				arr_vec.push_back(Data(obj.release()));
			}
			else {
				std::cout << "err";
			}
		}

		void Array::insert_array_element(size_t idx, Data val) {
			if (!is_valid()) {
				return;
			}

			arr_vec.insert(arr_vec.begin() + idx, std::move(val));
		}

		void Array::erase(std::string_view key) {
			size_t idx = this->find(key);
			erase(idx);
		}

		void Array::erase(size_t idx) {
			if (!is_valid()) {
				return;
			}


			arr_vec.erase(arr_vec.begin() + idx);
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

				std::cout << "Link errr2";
				return;
			}

			j->set_parent(this);

			arr_vec.push_back(Data(j.release()));
		}

		void Array::add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2) {

			// error
			std::cout << "errr..";
		}

		void Array::add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id) {
			if (!is_valid()) {
				return;
			}


			{
				Data temp2;
				bool e = false;
				claujson::Convert(temp2, idx21, idx22, len2, true, buf, string_buf, id, e);
				if (e) {

					throw "Error in add_item_type";
				}
				arr_vec.push_back(std::move(temp2));
			}
		}

		void Array::add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
			uint8_t* string_buf, int type, uint64_t id) {
			std::cout << "errrr";
		}

		void Array::add_user_type(Ptr<Json> j) {
			if (!is_valid()) {
				return;
			}


			if (j->is_virtual()) {
				j->set_parent(this);
				arr_vec.push_back(Data(j.release()));
			}
			else if (j->has_key() == false) {
				j->set_parent(this);
				arr_vec.push_back(Data(j.release()));
			}
			else {
				// error..
				std::cout << "errr..";
				return;
			}
		}

	// class Root, only used in class LoadData.
	
		Root::~Root() {
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

		Root::Root() {

		}

		bool Root::is_root() const { return true; }

		bool Root::is_object() const {
			return false;
		}
		bool Root::is_array() const {
			return false;
		}
		bool Root::is_element() const {
			return false;
		}
		size_t Root::get_data_size() const {
			int count = 0;

			if (virtualJson.is_ptr()) {
				count = 1;
			}

			return arr_vec.size() + obj_val_vec.size() + count;
		}

		Data& Root::get_value_list(size_t idx) {

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


		Data& Root::get_key_list(size_t idx) {
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


		const Data& Root::get_value_list(size_t idx) const {
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


		const Data& Root::get_key_list(size_t idx) const {
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


		void Root::clear(size_t idx) { // use carefully..
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

		bool Root::is_virtual() const {
			return false;
		}
		
		void Root::clear() {
			arr_vec.clear();
			obj_key_vec.clear();
			obj_val_vec.clear();
			virtualJson.clear();
		}

		void Root::reserve_data_list(size_t len) {
			if (!arr_vec.empty()) {
				arr_vec.reserve(len);
			}
			if (!obj_val_vec.empty()) {
				obj_val_vec.reserve(len);
				obj_key_vec.reserve(len);
			}
		}



		void Root::add_object_element(Data key, Data val) {
			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(std::move(val));
		}
		void Root::add_array_element(Data val) {
			arr_vec.push_back(std::move(val));
		}


		void Root::add_array(Ptr<Json> arr) {
			std::cout << "not used..";
		}
		void Root::add_object(Ptr<Json> obj) {
			std::cout << "not used..";
		}
		void Root::insert_array_element(size_t idx, Data val) { std::cout << "not used.."; }

		void Root::erase(std::string_view key) {
			std::cout << "not used..";
		}

		void Root::erase(size_t idx) {
			std::cout << "not used..";
		}

		void Root::Link(Ptr<Json> j) { // use carefully...

			j->set_parent(this);

			if (!j->has_key()) {
				arr_vec.push_back(Data(j.release()));
			}
			else {
				obj_key_vec.push_back(j->get_key());
				obj_val_vec.push_back(Data(j.release()));
			}
		}


		void Root::add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2) {

				{
					Data temp;
					Data temp2;

					bool e = false;

					claujson::Convert(temp, idx11, idx12, len1, true, buf, string_buf, id, e);

					if (e) {

						throw "Error in add_item_type";
					}

					claujson::Convert(temp2, idx21, idx22, len2, false, buf, string_buf, id2, e);

					if (e) {

						throw "Error in add_item_type";
					}

					if (temp.type() != DataType::STRING) {
						throw "Error in add_item_type, key is not string";
					}

					obj_key_vec.push_back(std::move(temp));
					obj_val_vec.push_back(std::move(temp2));
				}
		}

		void Root::add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id) {

				{
					Data temp2;
					bool e = false;

					claujson::Convert(temp2, idx21, idx22, len2, true, buf, string_buf, id, e);

					if (e) {

						throw "Error in add_item_type";
					}

					arr_vec.push_back(std::move(temp2));
				}
		}

		void Root::add_user_type(Ptr<Json> j) {

			j->set_parent(this);

			if (j->is_virtual()) {
				virtualJson = Data(j.release());
			}
			else if (j->has_key() == false) {
				arr_vec.push_back(Data(j.release()));
			}
			else {
				if (j->has_key()) {
					obj_key_vec.push_back(j->get_key());
					obj_val_vec.push_back(Data(j.release()));
				}
				else {
					std::cout << "ERRR";
				}
			}

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
	

	inline void Object::add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
		uint8_t* string_buf, int type, uint64_t id) {
		if (!is_valid()) {
			return;
		}

		{
			Data temp;
			bool e = false;

			claujson::Convert(temp, idx, idx2, len, true, buf, string_buf, id, e);
			if (e) {
				throw "Error in add_user_type";
			}

			if (temp.type() != DataType::STRING) {
				throw "Error in add_item_type, key is not string";
			}

			Json* json = nullptr;

			if (type == 0) {
				json = new Object();
			}
			else if (type == 1) {
				json = new Array();
			}

			obj_key_vec.push_back(temp);
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
			else {
				std::cout << "ERRRRRRRR";
			}
			json->set_parent(this);
		}
	}

	inline void Root::add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
		uint8_t* string_buf, int type, uint64_t id) {
			{
				Data temp;
				bool e = false;

				claujson::Convert(temp, idx, idx2, len, true, buf, string_buf, id, e);

				if (e) {
					throw "Error in add_user_type";
				}

				if (temp.type() != DataType::STRING) {
					throw "Error in add_item_type, key is not string";
				}


				Json* json = nullptr;

				if (type == 0) {
					json = new Object();
				}
				else if (type == 1) {
					json = new Array();
				}



				if (type == 0 || type == 1) {
					obj_key_vec.push_back(temp);
					obj_val_vec.push_back(Data(json));
				}
				else {
					std::cout << "ERRRRRRRR";
				}


				json->set_key(std::move(temp));
				json->set_parent(this);
			}
	}
	inline void Root::add_user_type(int type) {
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
			else {
				std::cout << "ERRRRRRRR";
			}

			json->set_parent(this);
		}
	}

	Array& Data::as_array() {
		if (is_valid() && is_ptr() && as_json_ptr()->is_array() && as_json_ptr()->is_valid()) {
			return *static_cast<Array*>(_ptr_val);
		}
		static Array empty_arr{ false };
		return empty_arr;
	}

	Object& Data::as_object() {
		if (is_valid() && is_ptr() && as_json_ptr()->is_object() && as_json_ptr()->is_valid()) {
			return *static_cast<Object*>(_ptr_val);
		}
		static Object empty_obj{ false };
		return empty_obj;
	}


	const Array& Data::as_array() const {
		if (is_valid() && is_ptr() && as_json_ptr()->is_array() && as_json_ptr()->is_valid()) {
			return *static_cast<Array*>(_ptr_val);
		}
		static const Array empty_arr{ false };
		return empty_arr;
	}

	const Object& Data::as_object() const {
		if (is_valid() && is_ptr() && as_json_ptr()->is_object() && as_json_ptr()->is_valid()) {
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

	inline unsigned char __arr[256] = {
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

	inline unsigned char __arr2[2][256] = { { 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0
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

	inline simdjson::internal::tape_type get_type(unsigned char x) {
		return (simdjson::internal::tape_type)__arr[x]; // more fast version..
		/*
		switch (x) {
		case '-':
		case '0':
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return simdjson::internal::tape_type::DOUBLE; // number?
			break;
		case '"':
		case 't':
		case 'f':
		case 'n':
		case '{':
		case '[':
		case '}':
		case ']':
			return	(simdjson::internal::tape_type)(x);
			break;
		case ':':
		case ',':

			return	(simdjson::internal::tape_type)(x);
			break;
		}
		return simdjson::internal::tape_type::NONE;*/
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

			Json* out = new Root(); //


			while (parent && parent->is_root() == false) {
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
							out->add_array_element(parent->get_value_list(i));
						}
						else { // out->is_object
							out->add_object_element(parent->get_key_list(i), parent->get_value_list(i));
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
					else { // root
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

				//std::cout << "chk\n";
				if (ut_next && _ut == *ut_next) {
					*ut_next = _next;
					chk_ut_next = true;

					std::cout << "chked in merge...\n";
				}

				if (_next->is_array() && _ut->is_object()) {
					throw "Error in Merge, next is array but child? is object";
				}
				if (_next->is_object() && _ut->is_array()) {
					throw "Error in Merge, next is object but child? is array";
				}



				size_t _size = _ut->get_data_size();

				for (size_t i = 0; i < _size; ++i) {
					if (_ut->get_value_list(i).is_ptr()) { // root, array, object
						if (((Json*)(_ut->get_value_list(i).ptr_val()))->is_virtual()) {
							//
						}
						else {
							_next->Link(Ptr<Json>(((Json*)(_ut->get_value_list(i).ptr_val()))));
							_ut->clear(i);
						}
					}
					else { // item type.
						if (_next->is_array() || _next->is_root()) {
							_next->add_array_element(std::move(_ut->get_value_list(i)));
						}
						else {
							_next->add_object_element(std::move(_ut->get_key_list(i)), std::move(_ut->get_value_list(i)));
						}
						_ut->clear(i);
					}
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
				//std::cout << "chk\n";

				class Json* _ut = ut;
				class Json* _next = next;

				if (_next->is_array() && _ut->is_object()) {
					throw "Error in Merge, next is array but child? is object";
				}
				if (_next->is_object() && _ut->is_array()) {
					throw "Error in Merge, next is object but child? is array";
				}


				if (ut_next && _ut == (*ut_next)) {
					*ut_next = _next;
					std::cout << "chked in merge...\n";
				}

				size_t _size = _ut->get_data_size();

				for (size_t i = 0; i < _size; ++i) {
					if (_ut->get_value_list(i).is_ptr()) { // root, array, object
						if (((Json*)(_ut->get_value_list(i).ptr_val()))->is_virtual()) {
							//
						}
						else {
							_next->Link(Ptr<Json>(((Json*)(_ut->get_value_list(i).ptr_val()))));
							_ut->clear(i);
						}
					}
					else { // item type.
						if (_next->is_array()) {
							_next->add_array_element(std::move(_ut->get_value_list(i)));
						}
						else {
							_next->add_object_element(std::move(_ut->get_key_list(i)), std::move(_ut->get_value_list(i)));
						}
						_ut->clear(i);
					}
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
			int64_t idx;  // buf_idx?
			int64_t idx2; // next_buf_idx?
			int64_t len;
			//Json
			uint64_t id; // token_idx?
			//
			bool is_key = false;
		};

		static bool __LoadData(char* buf, size_t buf_len,
			uint8_t* string_buf,
			simdjson::internal::dom_parser_implementation* imple,
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

				TokenTemp key; bool is_before_comma = false;
				bool is_now_comma = false;

				if (token_arr_start > 0) {
					const simdjson::internal::tape_type before_type =
						get_type(buf[imple->structural_indexes[token_arr_start - 1]]);

					is_before_comma = before_type == simdjson::internal::tape_type::COMMA;
				}


				for (uint64_t i = 0; i < token_arr_len; ++i) {

					const simdjson::internal::tape_type type = get_type(buf[imple->structural_indexes[token_arr_start + i]]);


					if (is_before_comma && type == simdjson::internal::tape_type::COMMA) {
						std::cout << "before is comma\n";
						throw "Error in __Load... and case : , ,";
						//
					}


					if (token_arr_start + i > 0) {
						const simdjson::internal::tape_type before_type =
							get_type(buf[imple->structural_indexes[token_arr_start + i - 1]]);

						if (before_type == simdjson::internal::tape_type::START_ARRAY || before_type == simdjson::internal::tape_type::START_OBJECT) {
							is_now_comma = false; //std::cout << "2-i " << i << "\n";
						}
					}

					if (is_before_comma) {
						is_now_comma = false;
					}

					if (!is_now_comma && type == simdjson::internal::tape_type::COMMA) {
						std::cout << "now is not comma\n";
						throw "Error in __Load.., now is comma but, no expect.";							//
					}
					if (is_now_comma && type != simdjson::internal::tape_type::COMMA) {
						std::cout << "is now comma... but not..\n";
						throw "Error in __Load..., comma is expected but, is not";
					}


					is_before_comma = type == simdjson::internal::tape_type::COMMA;

					if (type == simdjson::internal::tape_type::COMMA) {
						if (token_arr_start + i + 1 < imple->n_structural_indexes) {
							const simdjson::internal::tape_type _type = // next_type
								get_type(buf[imple->structural_indexes[token_arr_start + i + 1]]);

							if (_type == simdjson::internal::tape_type::END_ARRAY || _type == simdjson::internal::tape_type::END_OBJECT) {
								throw "Error in __Load..,  case : , } or , ]";
								//
							}
							else if (_type == simdjson::internal::tape_type::COLON) {
								throw "Error in __Load... case :    , : ";
							}

							continue;
						}
						else {
							throw "Error in __Load..., last valid char? is , ";
						}
					}

					if (type == simdjson::internal::tape_type::COLON) {
						throw "Error in __Load..., checked colon..";
						//
					}


					is_now_comma = __arr2[(int)is_now_comma][(unsigned char)type]; // comma_chk_table
					/*switch (type) {
					case simdjson::internal::tape_type::END_ARRAY:
					case simdjson::internal::tape_type::END_OBJECT:
					case simdjson::internal::tape_type::STRING:
					case simdjson::internal::tape_type::INT:
					case simdjson::internal::tape_type::UINT:
					case simdjson::internal::tape_type::DOUBLE:
					case simdjson::internal::tape_type::TRUE_VALUE:
					case simdjson::internal::tape_type::FALSE_VALUE:
					case simdjson::internal::tape_type::NULL_VALUE:
					case simdjson::internal::tape_type::NONE: //
						is_now_comma = true;
						break;
					} */

					if (token_arr_start + i + 1 < imple->n_structural_indexes) {
						const simdjson::internal::tape_type _type = // next_type
							get_type(buf[imple->structural_indexes[token_arr_start + i + 1]]);

						if (_type == simdjson::internal::tape_type::END_ARRAY || _type == simdjson::internal::tape_type::END_OBJECT) {
							is_now_comma = false;
						}
					}
					else {
						is_now_comma = false;
					}

					// Left 1
					//else
					if (type == simdjson::internal::tape_type::START_OBJECT ||
						type == simdjson::internal::tape_type::START_ARRAY) { // object start, array start

						if (!Vec.empty()) {

							if (Vec[0].is_key) {
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size() / 2);

								if (Vec.size() % 2 == 1) {
									std::cout << "Vec.size()%2==1\n";
									throw "Error in __Load..., key : value  error";
								}

								for (size_t x = 0; x < Vec.size(); x += 2) {
									if (!Vec[x].is_key) {
										std::cout << "vec[x].is not key\n";
										throw "Error in __Load..., key : value  error";
									}
									if (Vec[x + 1].is_key) {
										std::cout << "vec[x].is key\n";
										throw "Error in __Load..., key : value  error";
									}
									nestedUT[braceNum]->add_item_type((Vec[x].idx), Vec[x].idx2, Vec[x].len,
										(Vec[x + 1].idx), Vec[x + 1].idx2, Vec[x + 1].len,
										buf, string_buf, Vec[x].id, Vec[x + 1].id);
								}
							}
							else {
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size());

								for (size_t x = 0; x < Vec.size(); x += 1) {
									if (Vec[x].is_key) {
										std::cout << "Vec[x].iskey\n";

										throw "Error in __Load..., key : value  error";
									}
									nestedUT[braceNum]->add_item_type((Vec[x].idx), Vec[x].idx2, Vec[x].len, buf, string_buf, Vec[x].id);
								}
							}

							Vec.clear();
						}


						if (key.is_key) {
							nestedUT[braceNum]->add_user_type(key.idx, key.idx2, key.len, buf, string_buf,
								type == simdjson::internal::tape_type::START_OBJECT ? 0 : 1, key.id); // object vs array
							key.is_key = false;
						}
						else {
							nestedUT[braceNum]->add_user_type(type == simdjson::internal::tape_type::START_OBJECT ? 0 : 1);
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
					else if (type == simdjson::internal::tape_type::END_OBJECT ||
						type == simdjson::internal::tape_type::END_ARRAY) {

						if (type == simdjson::internal::tape_type::END_ARRAY && nestedUT[braceNum]->is_object()) {
							std::cout << "{]";
							throw "Error in __Load.., case : {]?";
						}

						if (type == simdjson::internal::tape_type::END_OBJECT && nestedUT[braceNum]->is_array()) {
							std::cout << "[}";


							throw "Error in __Load.., case : [}?";
						}

						state = 0;

						if (!Vec.empty()) {
							if (type == simdjson::internal::tape_type::END_OBJECT) {
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size() / 2);


								if (Vec.size() % 2 == 1) {
									std::cout << "Vec.size() is odd\n";
									throw "Error in __Load..., key : value  error";
								}


								for (size_t x = 0; x < Vec.size(); x += 2) {
									if (!Vec[x].is_key) {
										std::cout << "is not key\n";
										throw "Error in __Load..., key : value  error";
									}
									if (Vec[x + 1].is_key) {
										std::cout << "is key\n";
										throw "Error in __Load..., key : value  error";
									}

									nestedUT[braceNum]->add_item_type(Vec[x].idx, Vec[x].idx2, Vec[x].len,
										Vec[x + 1].idx, Vec[x + 1].idx2, Vec[x + 1].len, buf, string_buf, Vec[x].id, Vec[x + 1].id);
								}
							}
							else { // END_ARRAY
								nestedUT[braceNum]->reserve_data_list(nestedUT[braceNum]->get_data_size() + Vec.size());

								for (auto& x : Vec) {
									if (x.is_key) {
										throw "Error in __Load.., expect no key but has key...";
									}

									nestedUT[braceNum]->add_item_type((x.idx), x.idx2, x.len, buf, string_buf, x.id);
								}
							}

							Vec.clear();
						}


						if (braceNum == 0) {

							Ptr<Json> ut;

							if (type == simdjson::internal::tape_type::END_OBJECT) {
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

							data.idx = imple->structural_indexes[token_arr_start + i];
							data.id = token_arr_start + i;

							if (token_arr_start + i + 1 < imple->n_structural_indexes) {
								data.idx2 = imple->structural_indexes[token_arr_start + i + 1];
							}
							else {
								data.idx2 = buf_len;
							}

							bool is_key = false;
							if (token_arr_start + i + 1 < imple->n_structural_indexes && buf[imple->structural_indexes[token_arr_start + i + 1]] == ':') {
								is_key = true;
							}

							if (is_key) {
								data.is_key = true;

								if (token_arr_start + i + 2 < imple->n_structural_indexes) {
									const simdjson::internal::tape_type _type = (simdjson::internal::tape_type)buf[imple->structural_indexes[token_arr_start + i + 2]];

									if (_type == simdjson::internal::tape_type::START_ARRAY || _type == simdjson::internal::tape_type::START_OBJECT) {
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

								is_now_comma = false;
								is_before_comma = false;
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
							if (!Vec[x].is_key) {
								throw "Error in __Load..., key : value  error";
							}

							if (Vec.size() % 2 == 1) {
								throw "Error in __Load..., key : value  error";
							}


							if (Vec[x + 1].is_key) {
								throw "Error in __Load..., key : value  error";
							}

							nestedUT[braceNum]->add_item_type(Vec[x].idx, Vec[x].idx2, Vec[x].len, Vec[x + 1].idx, Vec[x + 1].idx2, Vec[x + 1].len,
								buf, string_buf, Vec[x].id, Vec[x + 1].id);
						}
					}
					else {
						for (size_t x = 0; x < Vec.size(); x += 1) {
							if (Vec[x].is_key) {
								throw "Error in __Load..., array element has key..";
							}

							nestedUT[braceNum]->add_item_type(Vec[x].idx, Vec[x].idx2, Vec[x].len, buf, string_buf, Vec[x].id);
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

				std::cout << _err << "\n";
				return false;
			}
			catch (...) {
				*err = -11;

				return false;
			}
		}

		static int64_t FindDivisionPlace(char* buf, simdjson::internal::dom_parser_implementation* imple, int64_t start, int64_t last)
		{
			for (int64_t a = start; a <= last; ++a) {
				auto& x = imple->structural_indexes[a]; //  token_arr[a];
				const simdjson::internal::tape_type type = (simdjson::internal::tape_type)buf[x];
				bool key = false;
				bool next_is_valid = false;

				switch ((int)type) {
				case ',':
					return a + 1;
				default:
					// error?
					break;
				}
			}
			return -1;
		}

		static bool _LoadData(Data& global, char* buf, size_t buf_len,
			uint8_t* string_buf,
			simdjson::internal::dom_parser_implementation* imple, int64_t& length,
			std::vector<int64_t>& start, const int parse_num) // first, strVec.empty() must be true!!
		{
			Ptr<Json> _global = Ptr<Json>(new Root());
			std::vector<Ptr<Json>> __global;

			try {
				int a__ = clock();
				{
					// chk clear?

					const int pivot_num = parse_num - 1;
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
					//std::cout << "pivots.. " << b_ - a_ << "ms\n";
					std::vector<class Json*> next(pivots.size() - 1, nullptr);
					{

						__global = std::vector<Ptr<Json>>(pivots.size() - 1);
						for (size_t i = 0; i < __global.size(); ++i) {
							__global[i] = Ptr<Json>(new Root());
						}

						std::vector<std::thread> thr(pivots.size() - 1);


						std::vector<int> err(pivots.size() - 1, 0);

						{
							int64_t idx = pivots[1] - pivots[0];
							int64_t _token_arr_len = idx;


							thr[0] = std::thread(__LoadData, (buf), buf_len, (string_buf), (imple), start[0], _token_arr_len, std::ref(__global[0]), 0, 0,
								&next[0], &err[0], 0);
						}

						auto a = std::chrono::steady_clock::now();

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
						std::cout << "parse1 " << dur.count() << "ms\n";

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
								std::cout << "Syntax Error\n"; return false;
								break;
							case -2:
								std::cout << "error final state is not last_state!\n"; return false;
								break;
							case -3:
								std::cout << "error x > buffer + buffer_len:\n"; return false;
								break;
							default:
								std::cout << "unknown parser error\n"; return false;
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

							for (int i = 0; i < pivots.size() - 1; ++i) {
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

							if (__global[start]->get_data_size() > 0 && __global[start]->get_value_list(0).is_ptr()
								&& ((Json*)__global[start]->get_value_list(0).ptr_val())->is_virtual()) {
								std::cout << "not valid file1\n";
								throw 1;
							}
							if (next[last] && next[last]->get_parent() != nullptr) {
								std::cout << "not valid file2\n";
								throw 2;
							}

							int err = Merge(_global.get(), __global[start].get(), &next[start]);
							if (-1 == err || (pivots.size() == 0 && 1 == err)) {
								std::cout << "not valid file3\n";
								throw 3;
							}

							for (uint64_t i = start + 1; i <= last; ++i) {

								if (chk[i]) {
									continue;
								}

								// linearly merge and error check...
								uint64_t before = i - 1;
								for (uint64_t k = i - 1; k >= 0; --k) {
									if (chk[k] == 0) {
										before = k;
										break;
									}
								}

								int err = Merge(next[before], __global[i].get(), &next[i]);

								if (-1 == err) {
									std::cout << "chk " << i << " " << __global.size() << "\n";
									std::cout << "not valid file4\n";
									throw 4;
								}
								else if (i == last && 1 == err) {
									std::cout << "not valid file5\n";
									throw 5;
								}
							}
						}
						//catch (...) {
							//throw "in Merge, error";
						//	return false;
						//}
						//

						if (_global->is_user_type() && _global->get_data_size() > 1) { // bug fix..
							std::cout << "not valid file6\n";
							throw 6;
						}

						auto c = std::chrono::steady_clock::now();
						auto dur2 = std::chrono::duration_cast<std::chrono::milliseconds>(c - b);
						std::cout << "parse2 " << dur2.count() << "ms\n";
					}
					}
					int a = clock();


					if (_global->get_value_list(0).is_ptr()) {
						_global->get_value_list(0).as_json_ptr()->set_parent(nullptr);
					}

					global = std::move(_global->get_value_list(0));


					int b = clock();
					std::cout << "chk " << b - a << "ms\n";

					//	std::cout << clock() - a__ << "ms\n";
				}
				//	std::cout << clock() - a__ << "ms\n";
				return true;
			}
			catch (int err) {

				std::cout << "merge error " << err << "\n";
				return false;
			}
			catch (const char* err) {

				std::cout << err << "\n";
				return false;
			}
			catch (...) {

				std::cout << "interal error\n";
				return false;
			}

		}
		static bool parse(Data& global, char* buf, size_t buf_len,
			uint8_t* string_buf,
			simdjson::internal::dom_parser_implementation* imple,
			int64_t length, std::vector<int64_t>& start, int thr_num) {

			return LoadData2::_LoadData(global, buf, buf_len, string_buf, imple, length, start, thr_num);
		}
	};

	class StrStream {
	private:
		fmt::memory_buffer out;
	public:

		const char* buf() const {
			return out.data();
		}
		size_t buf_size() const {
			return out.size();
		}

		StrStream& operator<<(const char* x) {
			fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(double x) {
			fmt::format_to(std::back_inserter(out), "{}", x); // FMT_COMPILE("{:.10f}"), x);
			return *this;
		}

		StrStream& operator<<(int64_t x) {
			fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(uint64_t x) {
			fmt::format_to(std::back_inserter(out), "{}", x);
			return *this;
		}

		StrStream& operator<<(char ch) {
			fmt::format_to(std::back_inserter(out), "{}", ch);
			return *this;
		}
	};

	//                            todo - change Json* ut to Data& data ?
	void LoadData::_save(StrStream& stream, Data data, std::vector<Json*>& chk_list, const int depth) {
		Json* ut = nullptr;

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
						//std::cout << "Error : no key\n";
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
									sprintf(buf + 2, "%04X", code);
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
										sprintf(buf + 2, "%04X", code);
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
							sprintf(buf + 2, "%04X", code);
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
	
	void LoadData::_save(StrStream& stream, Data data, const int depth) {
		Json* ut = nullptr;

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
						//std::cout << "Error : no key\n";
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
									sprintf(buf + 2, "%04X", code);
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
										sprintf(buf + 2, "%04X", code);
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
							sprintf(buf + 2, "%04X", code);
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
	void LoadData::save(const std::string& fileName, Data& global, bool hint) {
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

		std::ofstream outFile;
		outFile.open(fileName, std::ios::binary); // binary!
		outFile.write(stream.buf(), stream.buf_size());
		outFile.close();
	}

	void LoadData::save(std::ostream& stream, Data& data) {
		StrStream str_stream;
		_save(str_stream, data, 0);
		stream << std::string_view(str_stream.buf(), str_stream.buf_size());
	}

	void LoadData::save_(StrStream& stream, Data global, Json* temp, bool hint) {

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
	}


	void LoadData::save_parallel(const std::string& fileName, Data j, size_t thr_num) {

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

			thr[0] = std::thread(save_, std::ref(stream[0]), j, temp_parent[0], (false));


			for (size_t i = 1; i < thr.size(); ++i) {
				thr[i] = std::thread(save_, std::ref(stream[i]), claujson::Data(result[i - 1]->get_value_list(0)), temp_parent[i], (hint[i - 1]));
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

	std::pair<bool, size_t> Parse(const std::string& fileName, int thr_num, Data& ut)
	{
		if (thr_num <= 0) {
			thr_num = std::thread::hardware_concurrency();
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		int64_t length;

		int _ = clock();

		{
			static simdjson::dom::parser test;

			auto x = test.load(fileName);

			if (x.error() != simdjson::error_code::SUCCESS) {
				std::cout << "stage1 error : ";
				std::cout << x.error() << "\n";

				return { false, 0 };
			}

			const auto& buf = test.raw_buf();
			const auto& string_buf = test.raw_string_buf();
			const auto& imple = test.raw_implementation();
			const auto buf_len = test.raw_len();

			std::vector<int64_t> start(thr_num + 1, 0);
			//std::vector<int> key;

			int a = clock();

			std::cout << a - _ << "ms\n";


			{
				size_t how_many = imple->n_structural_indexes;
				length = how_many;

				start[0] = 0;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}
			}

			if (length == 0) {
				std::cout << "empty json";
				return { true, 0 };
			}

			int b = clock();

			std::cout << b - a << "ms\n";

			start[thr_num] = length;
			if (false == claujson::LoadData2::parse(ut, buf.get(), buf_len, string_buf.get(), imple.get(), length, start, thr_num)) // 0 : use all thread..
			{
				return { false, 0 };
			}
			int c = clock();
			std::cout << c - b << "ms\n";
		}
		int c = clock();
		std::cout << c - _ << "ms\n";


		return  { true, length };
	}
	std::pair<bool, size_t> ParseStr(std::string_view str, int thr_num, Data& ut)
	{
		if (thr_num <= 0) {
			thr_num = std::thread::hardware_concurrency();
		}
		if (thr_num <= 0) {
			thr_num = 1;
		}

		int64_t length;

		int _ = clock();

		{
			static simdjson::dom::parser test;

			auto x = test.parse(str.data(), str.length());

			if (x.error() != simdjson::error_code::SUCCESS) {
				std::cout << "stage1 error : ";
				std::cout << x.error() << "\n";

				return { false, 0 };
			}

			const auto& buf = test.raw_buf();
			const auto& string_buf = test.raw_string_buf();
			const auto& imple = test.raw_implementation();
			const auto buf_len = test.raw_len();

			std::vector<int64_t> start(thr_num + 1, 0);
			//std::vector<int> key;

			int a = clock();

			std::cout << a - _ << "ms\n";


			{
				size_t how_many = imple->n_structural_indexes;
				length = how_many;

				start[0] = 0;
				for (int i = 1; i < thr_num; ++i) {
					start[i] = how_many / thr_num * i;
				}
			}

			if (length == 0) {
				std::cout << "empty json";
				return { true, 0 };
			}

			int b = clock();

			std::cout << b - a << "ms\n";

			start[thr_num] = length;
			if (false == claujson::LoadData2::parse(ut, buf.get(), buf_len, string_buf.get(), imple.get(), length, start, thr_num)) // 0 : use all thread..
			{
				return { false, 0 };
			}
			int c = clock();
			std::cout << c - b << "ms\n";
		}
		int c = clock();
		std::cout << c - _ << "ms\n";


		return  { true, length };
	}

}

