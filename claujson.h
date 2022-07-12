#pragma once


#include "fmt/format.h"
#include "fmt/compile.h"

#include <iostream>
#include "simdjson.h" // modified simdjson // using simdjson 2.0.0

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include <iomanip>

#define INLINE inline

#include <string_view>
using namespace std::string_view_literals;

namespace claujson {

	class Data {
	private:
		union {
			int64_t _int_val = 0;
			uint64_t _uint_val;
			double _float_val;
			std::string* _str_val;
			void* _ptr_val; // Array or Object , ...
			bool _bool_val;
		};

		bool valid = true;
		simdjson::internal::tape_type _type = simdjson::internal::tape_type::NONE;

	public:

		explicit operator bool() const {
			return valid;
		}

		explicit Data(void* x) {
			set_ptr(x);
		}
		explicit Data(int x) {
			set_int(x);
		}

		explicit Data(unsigned int x) {
			set_uint(x);
		}

		explicit Data(int64_t x) {
			set_int(x);
		}
		explicit Data(uint64_t x) {
			set_uint(x);
		}
		explicit Data(double x) {
			set_float(x);
		}
		explicit Data(std::string_view x) {
			set_str(x.data(), x.size());
		}
		explicit Data(const char* x) {
			set_str(x, strlen(x));
		}
		explicit Data(bool x) {
			set_bool(x);
		}
		explicit Data(nullptr_t x) {
			set_type(simdjson::internal::tape_type::NULL_VALUE);
		}

		explicit Data(nullptr_t, bool valid) : valid(valid) {
			//
		}

	public:
		simdjson::internal::tape_type type() const {
			return _type;
		}

		bool is_valid() const {
			return valid;
		}

		bool is_int() const {
			return type() == simdjson::internal::tape_type::INT64;
		}

		bool is_uint() const {
			return type() == simdjson::internal::tape_type::UINT64;
		}

		bool is_float() const {
			return type() == simdjson::internal::tape_type::DOUBLE;
		}

		bool is_bool() const {
			return type() == simdjson::internal::tape_type::TRUE_VALUE ||
				type() == simdjson::internal::tape_type::FALSE_VALUE;
		}

		bool is_str() const {
			return type() == simdjson::internal::tape_type::STRING;
		}

		bool is_ptr() const {
			return type() == simdjson::internal::tape_type::ROOT;
		}

		int64_t int_val() const {
			return _int_val;
		}

		uint64_t uint_val() const {
			return _uint_val;
		}

		double float_val() const {
			return _float_val;
		}

		int64_t& int_val() {
			return _int_val;
		}

		uint64_t& uint_val() {
			return _uint_val;
		}

		double& float_val() {
			return _float_val;
		}

		bool bool_val() const {
			return _bool_val;
		}

		void* ptr_val() const {
			return _ptr_val;
		}

		template <class T>
		T& as() {
			return *static_cast<T*>(_ptr_val);
		}

		template <class T>
		const T& as() const {
			return *static_cast<T*>(_ptr_val);
		}

	private:
	public:
		void clear() {


			if (_type == simdjson::internal::tape_type::STRING) {
				delete _str_val; _str_val = nullptr;
			}
			else {
				_int_val = 0;
			}
			valid = true;
			_type = simdjson::internal::tape_type::NONE;
		}

		std::string& get_str_val() {
			// type check...
			return *_str_val;
		}

		const std::string& get_str_val() const {
			// type check...
			return *_str_val;
		}

		void set_ptr(void* x) {
			if (_type == simdjson::internal::tape_type::STRING) {
				delete _str_val;
			}

			_ptr_val = x;
			_type = simdjson::internal::tape_type::ROOT; // chk change simdjson::internal::tape_type:: ~~ -> DataType:: ~~
		}
		void set_int(long long x) {
			if (_type == simdjson::internal::tape_type::STRING) {
				delete _str_val;
			}
			_int_val = x;
			_type = simdjson::internal::tape_type::INT64;
		}

		void set_uint(unsigned long long x) {
			if (_type == simdjson::internal::tape_type::STRING) {
				delete _str_val;
			}
			_uint_val = x;
			_type = simdjson::internal::tape_type::UINT64;
		}

		void set_float(double x) {
			if (_type == simdjson::internal::tape_type::STRING) {
				delete _str_val;
			}
			_float_val = x;

			_type = simdjson::internal::tape_type::DOUBLE;
		}

		void set_str(const char* str, size_t len) {
			if (_type != simdjson::internal::tape_type::STRING) {
				_str_val = new std::string(str, len);
			}
			else {
				_str_val->assign(str, len);
			}
			_type = simdjson::internal::tape_type::STRING;
		}

		void set_bool(bool x) {
			_bool_val = x;

			if (x) {
				set_type(simdjson::internal::tape_type::TRUE_VALUE);
			}
			else {
				set_type(simdjson::internal::tape_type::FALSE_VALUE);
			}
		}

		void set_type(simdjson::internal::tape_type type) {
			this->_type = type;
		}

	public:
		virtual ~Data() {
			if (_type == simdjson::internal::tape_type::STRING && _str_val) {
				//std::cout << "chk";
				delete _str_val;
				_str_val = nullptr;
			}
		}

		Data(const Data& other)
			: _type(other._type) //, is_key(other.is_key) 
		{
			if (_type == simdjson::internal::tape_type::STRING) {
				_str_val = new std::string(other._str_val->c_str(), other._str_val->size());

			}
			else {
				_int_val = other._int_val;
			}
			valid = other.valid;
		}

		Data(Data&& other) noexcept
			: _type(other._type) //, is_key(other.is_key) 
		{

			if (_type == simdjson::internal::tape_type::STRING) {
				_str_val = other._str_val;
				other._str_val = nullptr;
				other._type = simdjson::internal::tape_type::NONE;
			}
			else {
				std::swap(_int_val, other._int_val);
			}

			std::swap(valid, other.valid);
		}

		Data() : _int_val(0), _type(simdjson::internal::tape_type::NONE) { }

		bool operator==(const Data& other) const {
			if (this->_type == other._type) {
				switch (this->_type) {
				case simdjson::internal::tape_type::STRING:
					return this->_str_val == other._str_val;
					break;
				case simdjson::internal::tape_type::INT64:
					return this->_int_val == other._int_val;
					break;
				case simdjson::internal::tape_type::UINT64:
					return this->_uint_val == other._uint_val;
					break;
				case simdjson::internal::tape_type::DOUBLE:
					return this->_float_val == other._float_val;
					break;
				}
				return true;
			}
			return false;
		}

		bool operator<(const Data& other) const {
			if (this->_type == other._type) {
				switch (this->_type) {
				case simdjson::internal::tape_type::STRING:
					return this->_str_val < other._str_val;
					break;
				case simdjson::internal::tape_type::INT64:
					return this->_int_val < other._int_val;
					break;
				case simdjson::internal::tape_type::UINT64:
					return this->_uint_val < other._uint_val;
					break;
				case simdjson::internal::tape_type::DOUBLE:
					return this->_float_val < other._float_val;
					break;
				}

			}
			return false;
		}

		Data& operator=(const Data& other) {
			if (this == &other) {
				return *this;
			}

			if (this->_type != simdjson::internal::tape_type::STRING && other._type == simdjson::internal::tape_type::STRING) {
				this->_str_val = new std::string();
			}
			else if (this->_type == simdjson::internal::tape_type::STRING && other._type != simdjson::internal::tape_type::STRING) {
				delete this->_str_val;
			}

			this->_type = other._type;

			if (this->_type == simdjson::internal::tape_type::STRING) {
				set_str(other._str_val->c_str(), other._str_val->size());
			}
			else {
				this->_int_val = other._int_val;
			}

			this->valid = other.valid;

			return *this;
		}


		Data& operator=(Data&& other) noexcept {
			if (this == &other) {
				return *this;
			}

			std::swap(this->_type, other._type);
			std::swap(this->_int_val, other._int_val);

			std::swap(valid, other.valid);

			return *this;
		}

		friend std::ostream& operator<<(std::ostream& stream, const Data& data) {

			switch (data._type) {
			case simdjson::internal::tape_type::INT64:
				stream << data._int_val;
				break;
			case simdjson::internal::tape_type::UINT64:
				stream << data._uint_val;
				break;
			case simdjson::internal::tape_type::DOUBLE:
				stream << data._float_val;
				break;
			case simdjson::internal::tape_type::STRING:
				stream << (*data._str_val);
				break;
			case simdjson::internal::tape_type::TRUE_VALUE:
				stream << "true";
				break;
			case simdjson::internal::tape_type::FALSE_VALUE:
				stream << "false";
				break;
			case simdjson::internal::tape_type::NULL_VALUE:
				stream << "null";
				break;
			case simdjson::internal::tape_type::START_ARRAY:
				stream << "[";
				break;
			case simdjson::internal::tape_type::START_OBJECT:
				stream << "{";
				break;
			case simdjson::internal::tape_type::END_ARRAY:
				stream << "]";
				break;
			case simdjson::internal::tape_type::END_OBJECT:
				stream << "}";
				break;
			}

			return stream;
		}
	};
}

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
	// todo 
	//- add bool is_key ...
	INLINE claujson::Data& Convert(::claujson::Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
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
				data.set_str(reinterpret_cast<char*>(&string_buf[idx]), string_length);
			}
			break;
			case 't':
			{
				if (!simdjson::SIMDJSON_IMPLEMENTATION::atomparsing::is_valid_true_atom(reinterpret_cast<uint8_t*>(&buf[idx]), idx2 - idx)) {
					throw "Error in Convert for true";
				}

				data.set_type((simdjson::internal::tape_type)buf[idx]);
			}
			break;
			case 'f':
				if (!simdjson::SIMDJSON_IMPLEMENTATION::atomparsing::is_valid_false_atom(reinterpret_cast<uint8_t*>(&buf[idx]), idx2 - idx)) {
					throw "Error in Convert for false";
				}

				data.set_type((simdjson::internal::tape_type)buf[idx]);
				break;
			case 'n':
				if (!simdjson::SIMDJSON_IMPLEMENTATION::atomparsing::is_valid_null_atom(reinterpret_cast<uint8_t*>(&buf[idx]), idx2 - idx)) {
					throw "Error in Convert for null";
				}

				data.set_type((simdjson::internal::tape_type)buf[idx]);
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
}


namespace claujson {
	class LoadData;


	template <class T>
	class PtrWeak;

	template <class T>
	class Ptr {
		friend class PtrWeak<T>;
	private:
		T* ptr = nullptr;
	public:

		Ptr() : ptr(nullptr) {

		}


		explicit Ptr(T* ptr) : ptr(ptr)
		{ }

	private:
		Ptr(const Ptr& other) = delete;
		Ptr& operator=(const Ptr& other) = delete;
	public:

		void operator=(nullptr_t) {
			clear();
		}

		Ptr(Ptr&& other)noexcept {
			std::swap(ptr, other.ptr);
		}

		void operator=(Ptr&& other) noexcept {
			std::swap(ptr, other.ptr);
		}

		void clear() {
			if (ptr) { delete ptr; }
			ptr = nullptr;
		}

	public:

		void reset(Ptr<T> p) {
			if (ptr != p.ptr) {
				if (ptr) { delete ptr; }
				ptr = p.ptr;
				p.ptr = nullptr;
			}
		}

	public:
		~Ptr() {
			if (ptr) {
				delete ptr;
				ptr = nullptr;
			}
		}

	public:
		explicit operator bool() const { return ptr; }
		T* get() { return ptr; }
		const T* get() const { return ptr; }


		T* Get() { T* temp = ptr; ptr = nullptr; return temp; }

		T* operator->() { return ptr; }
		const T* operator->()const { return ptr; }

		T& operator*() { return *ptr; }
		const T& operator*() const { return *ptr; }

		auto operator[](size_t idx) {
			return (*ptr)[idx];
		}
		const auto operator[](size_t idx) const {
			return (*ptr)[idx];
		}
	};

	template <class T>
	class PtrWeak {
	private:

		T* ptr;
	public:

		PtrWeak() : ptr(nullptr) { }

		PtrWeak(nullptr_t) : ptr(nullptr) { }

		explicit PtrWeak(const Ptr<T>& x) {
			ptr = x.ptr;
		}

		PtrWeak(const PtrWeak& other) {
			ptr = other.ptr;
		}

		PtrWeak(T* x) {
			ptr = x;
		}

		explicit operator bool() const { return ptr; }
		T* get() { return ptr; }
		const T* get() const { return ptr; }

		T* operator->() { return ptr; }
		const T* operator->()const { return ptr; }

		T& operator*() { return *ptr; }
		const T& operator*() const { return *ptr; }
	};

	class Array;
	class Object;
	class Root;

	class Json {
		friend class LoadData;
	protected:
		Data key;
		PtrWeak<Json> parent;

		// check...
		static inline Data data_null{nullptr, false}; // valid is false..
	public:

		Json() { }

		Json(const Json&) = delete;
		Json& operator=(const Json&) = delete;

		virtual ~Json() {

		}

		const Data& at(std::string_view key) const {
			if (!is_object()) {
				return data_null;
			}

			size_t len = get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (get_key_list(i).get_str_val().compare(key) == 0) {
					return get_data_list(i);
				}
			}

			return data_null;
		}

		Data& at(std::string_view key) {
			if (!is_object()) {
				return data_null;
			}

			size_t len = get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (get_key_list(i).get_str_val().compare(key) == 0) {
					return get_data_list(i);
				}
			}

			return data_null;
		}

		size_t find(std::string_view key) { // chk (uint64_t)-1 == (maximum)....-> eof?
			if (!is_object()) {
				return -1;
			}

			size_t len = get_data_size();
			for (size_t i = 0; i < len; ++i) {
				if (get_key_list(i).get_str_val().compare(key) == 0) {
					return i;
				}
			}

			return -1;
		}


		Data& operator[](size_t idx) {
			return get_data_list(idx);
		}

		const Data& operator[](size_t idx) const {
			return get_data_list(idx);
		}

		bool has_key() const {
			return key.is_str();
		}

		PtrWeak<Json> get_parent() const {
			return parent;
		}

		void set_parent(PtrWeak<Json> j) {
			parent = j;
		}
		virtual Data& get_key() {
			return key;
		}

		virtual bool set_key(Data key) {
			if (key.is_str()) {
				this->key = std::move(key);
				return true;
			}
			return false; // no change..
		}

		virtual Data& get_value() {
			return data_null;
		}

		virtual void reserve_data_list(size_t len) = 0;

		virtual bool is_object() const = 0;
		virtual bool is_array() const = 0;
		virtual bool is_root() const { return false; }
		virtual bool is_element() const = 0;
		bool is_user_type() const {
			return is_object() || is_array() || is_root();
		}

		// for valid with obejct or array or root.
		virtual size_t get_data_size() const = 0;
		virtual Data& get_data_list(size_t idx) = 0;
		virtual Data& get_key_list(size_t idx) = 0;

		virtual const Data& get_data_list(size_t idx) const = 0;
		virtual const Data& get_key_list(size_t idx) const = 0;

		virtual void clear(size_t idx) = 0;
		virtual void clear() = 0;

		virtual bool is_virtual() const = 0;

		// 
		virtual void add_object_element(Data key, Data val) = 0; 
		virtual void add_array_element(Data val) = 0;
		virtual void add_array(Ptr<Json> arr) = 0; // 
		virtual void add_object(Ptr<Json> obj) = 0;

		virtual void insert_array_element(size_t idx, Data val) = 0;
		
		virtual void erase(std::string_view key) = 0;
		virtual void erase(size_t idx) = 0;

	private:
		virtual void Link(Ptr<Json> j) = 0;

		// private, friend?

		virtual void add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2) = 0;

		virtual void add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id) = 0;

		virtual void add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
			uint8_t* string_buf, int type, uint64_t id) = 0;

		//

		virtual void add_user_type(int type) = 0; // int type -> enum?

		virtual void add_user_type(Ptr<Json> j) = 0;
	};

	class Object : public Json {
	protected:
		std::vector<Data> obj_key_vec;
		std::vector<Data> obj_val_vec;
	public:
		virtual ~Object() {
			for (auto& x : obj_val_vec) {
				if (x.is_ptr()) {
					delete ((Json*)x.ptr_val());
				}
			}
		}

		virtual bool is_object() const {
			return true;
		}
		virtual bool is_array() const {
			return false;
		}
		virtual bool is_element() const {
			return false;
		}
		virtual size_t get_data_size() const {
			return obj_val_vec.size();
		}

		virtual Data& get_data_list(size_t idx) {
			return obj_val_vec[idx];
		}
		
		virtual Data& get_key_list(size_t idx) { // if key change then also obj_vec[idex].second.key? change??
			return obj_key_vec[idx];
		}


		virtual const Data& get_data_list(size_t idx) const {
			return obj_val_vec[idx];
		}

		virtual const Data& get_key_list(size_t idx) const {
			return obj_key_vec[idx];
		}


		virtual void clear(size_t idx) {
			obj_val_vec[idx].clear();
		}

		virtual bool is_virtual() const {
			return false;
		}

		virtual void clear() {
			obj_val_vec.clear();
			obj_key_vec.clear();
		}


		virtual void reserve_data_list(size_t len) {
			obj_val_vec.reserve(len);
			obj_key_vec.reserve(len);
		}


		virtual void add_object_element(Data key, Data val) {
			if (val.is_ptr()) {
				auto* x = (Json*)val.ptr_val();
				x->set_key(key); // no need?
			}
			obj_key_vec.push_back(std::move(key));
			obj_val_vec.push_back(std::move(val));
		}
		virtual void add_array_element(Data val) { std::cout << "err"; }
		virtual void add_array(Ptr<Json> arr) { 
			if (arr->has_key()) {
				obj_key_vec.push_back(arr->get_key());
				obj_val_vec.push_back(Data(arr.Get()));
			}
			else {
				std::cout << "err";
			}
		}
		virtual void add_object(Ptr<Json> obj) { 
			if (obj->has_key()) {
				obj_key_vec.push_back(obj->get_key());
				obj_val_vec.push_back(Data(obj.Get()));
			}
			else {
				std::cout << "err";
			}
		}

		virtual void insert_array_element(size_t idx, Data val) { std::cout << "err"; }

		virtual void erase(std::string_view key) { 
			size_t idx = this->find(key);
			erase(idx);
		}

		virtual void erase(size_t idx) { 
			obj_key_vec.erase(obj_key_vec.begin() + idx);
			obj_val_vec.erase(obj_val_vec.begin() + idx);
		}



	private:
		virtual void Link(Ptr<Json> j) {
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
			obj_val_vec.push_back(Data(j.Get())); 
		}

		virtual void add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2) {

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

					if (temp.type() != simdjson::internal::tape_type::STRING) {
						throw "Error in add_item_type, key is not string";
					}

					obj_key_vec.push_back(std::move(temp));
					obj_val_vec.push_back(std::move(temp2));
				}
		}

		virtual void add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id) {
			// error

			std::cout << "errr..";
		}

		virtual void add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
			uint8_t* string_buf, int type, uint64_t id);

		virtual void add_user_type(int type) {
			// error

			std::cout << "errr..";

			return;
		}

		virtual void add_user_type(Ptr<Json> j) {
			if (j->is_virtual()) {
				j->set_parent(this);

				obj_key_vec.push_back(Data());
				obj_val_vec.push_back(Data(j.Get()));
			}
			else if (j->has_key()) {
				j->set_parent(this);

				obj_key_vec.push_back(j->get_key());
				obj_val_vec.push_back(Data(j.Get()));
			}
			else {
				std::cout << "chk..";
				return;
			}

		}
	};

	class Array : public Json {
	protected:
		std::vector<Data> arr_vec;
	public:
		virtual ~Array() {
			for (auto& x : arr_vec) {
				if (x.is_ptr()) {
					delete ((Json*)x.ptr_val());
				}
			}
		}

		virtual bool is_object() const {
			return false;
		}
		virtual bool is_array() const {
			return true;
		}
		virtual bool is_element() const {
			return false;
		}
		virtual size_t get_data_size() const {
			return arr_vec.size();
		}

		virtual Data& get_data_list(size_t idx) {
			return arr_vec[idx];
		}
		
		virtual Data& get_key_list(size_t idx) {
			return data_null;
		}

		virtual const Data& get_data_list(size_t idx) const{
			return arr_vec[idx];
		}

		virtual const Data& get_key_list(size_t idx) const {
			return data_null;
		}

		virtual void clear(size_t idx) {
			arr_vec[idx].clear();
		}

		virtual bool is_virtual() const {
			return false;
		}
		virtual void clear() {
			arr_vec.clear();
		}

		virtual void reserve_data_list(size_t len) {
			arr_vec.reserve(len);
		}


		std::vector<Data>::iterator begin() {
			return arr_vec.begin();
		}

		std::vector<Data>::iterator end() {
			return arr_vec.end();
		}

		virtual void add_object_element(Data key, Data val) {
			std::cout << "err";
		}
		virtual void add_array_element(Data val) { 
			arr_vec.push_back(std::move(val));
		
		}
		virtual void add_array(Ptr<Json> arr) {
			if (!arr->has_key()) {
				arr_vec.push_back(Data(arr.Get()));
			}
			else {
				std::cout << "err";
			}
		}
		virtual void add_object(Ptr<Json> obj) {
			if (!obj->has_key()) {
				arr_vec.push_back(Data(obj.Get()));
			}
			else {
				std::cout << "err";
			}
		}

		virtual void insert_array_element(size_t idx, Data val) {
			arr_vec.insert(arr_vec.begin() + idx, std::move(val));
		}

		virtual void erase(std::string_view key) {
			size_t idx = this->find(key);
			erase(idx);
		}

		virtual void erase(size_t idx) {
			arr_vec.erase(arr_vec.begin() + idx);
		}
	private:
		virtual void Link(Ptr<Json> j) {
			if (!j->has_key()) {
				//
			}
			else {
				// error...

				std::cout << "Link errr2";
				return;
			}

			j->set_parent(this);

			arr_vec.push_back(Data(j.Get()));
		}


		virtual void add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
			char* buf, uint8_t* string_buf, uint64_t id, uint64_t id2) {

			// error
			std::cout << "errr..";
		}

		virtual void add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
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

		virtual void add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
			uint8_t* string_buf, int type, uint64_t id) {
			std::cout << "errrr";
		}

		virtual void add_user_type(int type);

		virtual void add_user_type(Ptr<Json> j) {

			if (j->is_virtual()) {
				j->set_parent(this);
				arr_vec.push_back(Data(j.Get()));
			}
			else if (j->has_key() == false) {
				j->set_parent(this);
				arr_vec.push_back(Data(j.Get()));
			}
			else {
				// error..
				std::cout << "errr..";
				return;
			}
		}

	};

	// class Root, only used in class LoadData.
	class Root : public Json {
	protected:
		std::vector<Data> arr_vec; // 
		// in parsing...
		std::vector<Data> obj_key_vec;
		std::vector<Data> obj_val_vec;

		Data virtualJson;
	public:
		virtual ~Root() {
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

	private:
		friend class LoadData;

		Root() {
			
		}

	public:
		virtual bool is_root() const { return true; }

		virtual bool is_object() const {
			return false;
		}
		virtual bool is_array() const {
			return false;
		}
		virtual bool is_element() const {
			return false;
		}
		virtual size_t get_data_size() const {
			int count = 0;

			if (virtualJson.is_ptr()) {
				count = 1;
			}

			return arr_vec.size() + obj_val_vec.size() + count;
		}

		virtual Data& get_data_list(size_t idx) {
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


		virtual Data& get_key_list(size_t idx) {
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


		virtual const Data& get_data_list(size_t idx) const  {
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


		virtual const Data& get_key_list(size_t idx) const {
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


		virtual void clear(size_t idx) { // use carefully..
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

		virtual bool is_virtual() const {
			return false;
		}
		virtual void clear() {
			arr_vec.clear();
			obj_key_vec.clear();
			obj_val_vec.clear();
			virtualJson.clear();
		}

		virtual void reserve_data_list(size_t len) {
			if (!arr_vec.empty()) {
				arr_vec.reserve(len);
			}
			if (!obj_val_vec.empty()) {
				obj_val_vec.reserve(len);
				obj_key_vec.reserve(len);
			}
		}



		virtual void add_object_element(Data key, Data val) {
			std::cout << "not used..";
		}
		virtual void add_array_element(Data val) { std::cout << "not used.."; }
		virtual void add_array(Ptr<Json> arr) {
			std::cout << "not used..";
		}
		virtual void add_object(Ptr<Json> obj) {
			std::cout << "not used..";
		}

		virtual void insert_array_element(size_t idx, Data val) { std::cout << "not used.."; }

		virtual void erase(std::string_view key) {
			std::cout << "not used..";
		}

		virtual void erase(size_t idx) {
			std::cout << "not used..";
		}


	private:
		virtual void Link(Ptr<Json> j) { // use carefully...

			j->set_parent(this);
			
			if (!j->has_key()) {
				arr_vec.push_back(Data(j.Get()));
			}
			else {
				obj_key_vec.push_back(j->get_key());
				obj_val_vec.push_back(Data(j.Get())); 
			}
		}


		virtual void add_item_type(int64_t idx11, int64_t idx12, int64_t len1, int64_t idx21, int64_t idx22, int64_t len2,
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

					if (temp.type() != simdjson::internal::tape_type::STRING) {
						throw "Error in add_item_type, key is not string";
					}

					obj_key_vec.push_back(std::move(temp));
					obj_val_vec.push_back(std::move(temp2));
				}
		}

		virtual void add_item_type(int64_t idx21, int64_t idx22, int64_t len2,
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

		virtual void add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
			uint8_t* string_buf, int type, uint64_t id);

		virtual void add_user_type(int type);

		virtual void add_user_type(Ptr<Json> j) {

			j->set_parent(this);

			if (j->is_virtual()) {
				virtualJson = Data(j.Get());
			}
			else if (j->has_key() == false) {
				arr_vec.push_back(Data(j.Get()));
			}
			else {
				if (j->has_key()) {
					obj_key_vec.push_back(j->get_key());
					obj_val_vec.push_back(Data(j.Get()));
				}
				else {
					std::cout << "ERRR";
				}
			}

		}


	};

	class VirtualObject : public Object {
	public:

		virtual bool is_virtual() const {
			return true;
		}

		virtual ~VirtualObject() {
			//
		}
	};

	class VirtualArray : public Array {
	public:

		virtual bool is_virtual() const {
			return true;
		}

		virtual ~VirtualArray() {
			//
		}
	};

	inline void Object::add_user_type(int64_t idx, int64_t idx2, int64_t len, char* buf,
		uint8_t* string_buf, int type, uint64_t id) {
			{
				Data temp;
				bool e = false;

				claujson::Convert(temp, idx, idx2, len, true, buf, string_buf, id, e);
				if (e) {
					throw "Error in add_user_type";
				}

				if (temp.type() != simdjson::internal::tape_type::STRING) {
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

				if (temp.type() != simdjson::internal::tape_type::STRING) {
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

}


namespace claujson {

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
		return simdjson::internal::tape_type::NONE;
	}

	class LoadData
	{
	public:

	public:

		static int Merge(class Json* next, class Json* ut, class Json** ut_next)
		{
			if (next->is_user_type() && ut->is_user_type()) {
				//
			}
			else {
				std::cout << "merge err:";
				return -2; // error?
			}
			if (ut_next == nullptr || (*ut_next)->is_user_type()) {
				//
			}
			else {
				std::cout << "merge err";
				return -3;
			}


			// check!!
			while (ut->get_data_size() >= 1
				&& ut->get_data_list(0).is_ptr() && ((Json*)ut->get_data_list(0).ptr_val())->is_virtual())
			{
				ut = (Json*)ut->get_data_list(0).ptr_val();
			}

			bool chk_ut_next = false;


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


				if (ut_next && _ut == *ut_next) {
					*ut_next = _next;
					chk_ut_next = true;

					std::cout << "chked in merge...\n";
				}

				size_t _size = _ut->get_data_size();

				for (size_t i = 0; i < _size; ++i) {
					if (_ut->get_data_list(i).is_ptr()) { // root, array, object
						if (((Json*)(_ut->get_data_list(i).ptr_val()))->is_virtual()) {
							//
						}
						else {
							_next->Link(Ptr<Json>(((Json*)(_ut->get_data_list(i).ptr_val()))));
							_ut->clear(i);
						}
					}
					else { // item type.
						if (_next->is_array()) {
							_next->add_array_element(std::move(_ut->get_data_list(i)));
						}
						else {
							_next->add_object_element(std::move(_ut->get_key_list(i)), std::move(_ut->get_data_list(i)));
						}
						_ut->clear(i);
					}
				}

				_ut->clear();

				ut = ut->get_parent().get();
				next = next->get_parent().get();


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

	private:

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
					case simdjson::internal::tape_type::INT64:
					case simdjson::internal::tape_type::UINT64:
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


						class Json* pTemp = (Json*)nestedUT[braceNum]->get_data_list(nestedUT[braceNum]->get_data_size() - 1).ptr_val();

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
								if (nestedUT[braceNum]->get_data_list(i).is_ptr()) {
									ut->add_user_type(Ptr<Json>((Json*)nestedUT[braceNum]->get_data_list(i).ptr_val()));
								}
								else {
									if (ut->is_object()) {
										ut->add_object_element(std::move(nestedUT[braceNum]->get_key_list(i)),
																std::move(nestedUT[braceNum]->get_data_list(i)));
									}
									else {
										ut->add_array_element(std::move(nestedUT[braceNum]->get_data_list(i)));
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
	public:

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

							if (__global[start]->get_data_size() > 0 && __global[start]->get_data_list(0).is_ptr()
								&& ((Json*)__global[start]->get_data_list(0).ptr_val())->is_virtual()) {
								std::cout << "not valid file1\n";
								throw 1;
							}
							if (next[last] && next[last]->get_parent().get() != nullptr) {
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

					
					global = std::move(_global->get_data_list(0));
					
					
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

			return LoadData::_LoadData(global, buf, buf_len, string_buf, imple, length, start, thr_num);
		}


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
		static void _save(StrStream& stream, Json* ut, const int depth = 0) {
			if (!ut) { return; }

			if (ut->is_object()) {
				for (size_t i = 0; i < ut->get_data_size(); ++i) {
					if (ut->get_data_list(i).is_ptr()) {
						auto& x = ut->get_key_list(i);

						if (x.type() == simdjson::internal::tape_type::STRING) {
							stream << "\"";
							
							size_t len = x.get_str_val().size();
							for (uint64_t j = 0; j < len; ++j) {
								switch ((x.get_str_val())[j]) {
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

									int code = (x.get_str_val())[j];
									if (code > 0 && (code < 0x20 || code == 0x7F))
									{
										char buf[] = "\\uDDDD";
										sprintf(buf + 2, "%04X", code);
										stream << buf;
									}
									else {
										stream << (x.get_str_val())[j];
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

						if (((Json*)(ut->get_data_list(i).ptr_val()))->is_object()) {
							stream << " { \n";
						}
						else {
							stream << " [ \n";
						}

						_save(stream, ((Json*)(ut->get_data_list(i).ptr_val())), depth + 1);

						if (((Json*)(ut->get_data_list(i).ptr_val()))->is_object()) {
							stream << " } \n";
						}
						else {
							stream << " ] \n";
						}
					}
					else {
						auto& x = ut->get_key_list(i);

						if (x.type() == simdjson::internal::tape_type::STRING) {
							stream << "\"";
							
							size_t len = x.get_str_val().size();
							for (uint64_t j = 0; j < len; ++j) {
								switch ((x.get_str_val())[j]) {
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

									int code = (x.get_str_val())[j];
									if (code > 0 && (code < 0x20 || code == 0x7F))
									{
										char buf[] = "\\uDDDD";
										sprintf(buf + 2, "%04X", code);
										stream << buf;
									}
									else {
										stream << (x.get_str_val())[j];
									}
										
								}
							}

							stream << "\"";

							{
								stream << " : ";
							}
						}

						{
							auto& x = ut->get_data_list(i);

							if (x.type() == simdjson::internal::tape_type::STRING) {
								stream << "\"";

								size_t len = x.get_str_val().size();
								for (uint64_t j = 0; j < len; ++j) {
									switch ((x.get_str_val())[j]) {
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

										int code = (x.get_str_val())[j];
										if (code > 0 && (code < 0x20 || code == 0x7F))
										{
											char buf[] = "\\uDDDD";
											sprintf(buf + 2, "%04X", code);
											stream << buf;
										}
										else {
											stream << (x.get_str_val())[j];
										}

									}
								}
								stream << "\"";

							}
							else if (x.type() == simdjson::internal::tape_type::TRUE_VALUE) {
								stream << "true";
							}
							else if (x.type() == simdjson::internal::tape_type::FALSE_VALUE) {
								stream << "false";
							}
							else if (x.type() == simdjson::internal::tape_type::DOUBLE) {
								stream << (x.float_val());
							}
							else if (x.type() == simdjson::internal::tape_type::INT64) {
								stream << x.int_val();
							}
							else if (x.type() == simdjson::internal::tape_type::UINT64) {
								stream << x.uint_val();
							}
							else if (x.type() == simdjson::internal::tape_type::NULL_VALUE) {
								stream << "null";
							}
						}
					}

					if (i < ut->get_data_size() - 1) {
						stream << ", ";
					}
				}
			}
			else if (ut->is_array()) {
				for (size_t i = 0; i < ut->get_data_size(); ++i) {
					if (ut->get_data_list(i).is_ptr()) {


						if (((Json*)ut->get_data_list(i).ptr_val())->is_object()) {
							stream << " { \n";
						}
						else {
							stream << " [ \n";
						}


						_save(stream, ((Json*)ut->get_data_list(i).ptr_val()), depth + 1);

						if (((Json*)ut->get_data_list(i).ptr_val())->is_object()) {
							stream << " } \n";
						}
						else {
							stream << " ] \n";
						}
					}
					else {

						auto& x = ut->get_data_list(i);

						if (x.type() == simdjson::internal::tape_type::STRING) {
							stream << "\"";

							size_t len = x.get_str_val().size();
							for (uint64_t j = 0; j < len; ++j) {
								switch ((x.get_str_val())[j]) {
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

									int code = (x.get_str_val())[j];
									if (code > 0 && (code < 0x20 || code == 0x7F))
									{
										char buf[] = "\\uDDDD";
										sprintf(buf + 2, "%04X", code);
										stream << buf;
									}
									else {
										stream << (x.get_str_val())[j];
									}

								}
							}stream << "\"";
						}
						else if (x.type() == simdjson::internal::tape_type::TRUE_VALUE) {
							stream << "true";
						}
						else if (x.type() == simdjson::internal::tape_type::FALSE_VALUE) {
							stream << "false";
						}
						else if (x.type() == simdjson::internal::tape_type::DOUBLE) {
							stream << (x.float_val());
						}
						else if (x.type() == simdjson::internal::tape_type::INT64) {
							stream << x.int_val();
						}
						else if (x.type() == simdjson::internal::tape_type::UINT64) {
							stream << x.uint_val();
						}
						else if (x.type() == simdjson::internal::tape_type::NULL_VALUE) {
							stream << "null";
						}


						stream << " ";
					}

					if (i < ut->get_data_size() - 1) {
						stream << ", ";
					}
				}
			}
		}

		// todo... just Data has one element 
		static void save(const std::string& fileName, Data& global) {
			StrStream stream;

			if (global.is_ptr()) {
				auto* x = &global.as<Json>();
				bool is_arr = x->is_array();
				if (is_arr) {
					stream << " [ ";
				}
				else {
					stream << " { ";
				}
				
				_save(stream, x);
			
				if (is_arr) {
					stream << " ] ";
				}
				else {
					stream << " } ";
				}
			}
			else {
				// todo~~ from _save val.in array.
			}

			std::ofstream outFile;
			outFile.open(fileName, std::ios::binary); // binary!
			outFile.write(stream.buf(), stream.buf_size());
			outFile.close();
		}

		static void save(std::ostream& stream, class Json& global) {
			StrStream str_stream;
			_save(str_stream, &global);
			stream << std::string_view(str_stream.buf(), str_stream.buf_size());
		}
	};

	INLINE 	std::pair<bool, size_t> Parse(const std::string& fileName, int thr_num, Data& ut)
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


			int b = clock();

			std::cout << b - a << "ms\n";

			start[thr_num] = length;
			if (false == claujson::LoadData::parse(ut, buf.get(), buf_len, string_buf.get(), imple.get(), length, start, thr_num)) // 0 : use all thread..
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
