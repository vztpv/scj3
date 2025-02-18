#pragma once

// 64bit.. DO NOT build 32bit! //

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <set>
#include <fstream>
#include <cstring>
#include <cstdint> // uint64_t? int64_t?

#include <memory_resource> // with C++17~

#ifdef USE_PMR
template <class T>
using std_vector = std::pmr::vector<T>;
#else
template <class T>
using std_vector = std::vector<T>;
#endif


#include "ThreadPool.h"

#include "_simdjson.h" // modified simdjson // using simdjson 3.9.1


template <class From, class To>
inline To Static_Cast(From x) {
	To temp = static_cast<To>(x);
	bool valid = static_cast<From>(temp) == x;
	if (!valid) {
		throw std::runtime_error("static cast error");
	}
	return temp;
}



#if __cpp_lib_string_view
#include <string_view>
using namespace std::literals::string_view_literals;
namespace claujson {
	using StringView = std::string_view;
}

#else

namespace claujson {
	class StringView {
	public:
		explicit StringView() : m_str(nullptr), m_len(0) { }

		StringView(const std::string& str) : m_str(str.data()), m_len(str.size()) { }
		explicit StringView(const char* str) : m_str(str) { m_len = strlen(str); }
		explicit StringView(const char* str, size_t len) : m_str(str), m_len(len) { }
		StringView(const StringView& other) {
			m_str = other.m_str;
			m_len = other.m_len;
		}

	public:
		const char* data() const {
			return m_str;
		}

		uint64_t size() const {
			return m_len;
		}

		uint64_t length() const {
			return m_len;
		}

		bool empty() const {
			return 0 == m_len;
		}

		StringView substr(uint64_t pos, uint64_t n) const {
			return StringView(m_str + pos, n);
		}

		const char& operator[](uint64_t idx) const {
			return m_str[idx];
		}
		
		// returns index;
		uint64_t find(const char ch, uint64_t start = 0) {
			for (uint64_t i = start; i < size(); ++i) {
				if (ch == (*this)[i]) {
					return i;
				}
			}
			return npos;
		}

		StringView& operator=(const StringView& other) {
			StringView temp(other);
			this->m_str = temp.m_str;
			this->m_len = temp.m_len;
			return *this;
		}
	private:
		const char* m_str;
		size_t m_len;
	public:
		static uint64_t npos;

		friend std::ostream& operator<<(std::ostream& stream, const claujson::StringView& sv) {
			stream << sv.data();
			return stream;
		}

		bool operator==(const StringView view) {
			return this->compare(view) == 0;
		}

		bool operator!=(const StringView view) {
			return this->compare(view) != 0;
		}

		int compare(const StringView view) {
			uint64_t idx1 = 0, idx2 = 0;
			for (; idx1 < this->length() && idx2 < view.length(); ++idx1, ++idx2) {
				uint8_t diff = this->data()[idx1] - view.data()[idx2];
				if (diff < 0) {
					return -1;
				}
				else if (diff > 0) {
					return 1;
				}
			}
			if (idx1 < this->length()) {
				return 1;
			}
			else if (idx2 < view.length()) {
				return -1;
			}
			return 0;
		}

		bool operator<(const StringView view) {
			return this->compare(view) < 0;
		}
	};
}


claujson::StringView operator""sv(const char* str, size_t sz);
bool operator==(const std::string& str, claujson::StringView sv);


#endif





namespace claujson {

	// has static buf?
	template <class T, int SIZE = 1024>
	class Vector {
	public:
		class iterator {
		private: 
			T* ptr;
		public:
			iterator(T* now) {
				ptr = now;
			}
		public:
			void operator++() {
				++ptr;
			}
			T* operator->() {
				return ptr;
			}
			const T* operator->() const {
				return ptr;
			}
			T& operator*() {
				return *ptr;
			}
			const T& operator*() const {
				return *ptr;
			}
			bool operator!=(const iterator& other) const {
				return this->ptr != other.ptr;
			}
		};
		class const_iterator {
		private:
			T* ptr;
		public:
			const_iterator(T* now) {
				ptr = now;
			}
		public:
			const T* operator->() const {
				return ptr;
			}
			const T& operator*() const {
				return *ptr;
			}
			bool operator!=(const iterator& other) const {
				return this->ptr != other.ptr;
			}
		};
	private:
		T buf[SIZE + 1];
		T* ptr = nullptr;
		uint32_t capacity = SIZE;
		uint32_t sz = 0;
		int type = 0;
	public:
		Vector() {
			//
		}
		~Vector() {
			if (type == 1 && ptr) {
				delete ptr;
				ptr = nullptr;
				sz = 0;
				type = 0;
			}
		}
		Vector(const Vector&) = delete;
		Vector& operator=(const Vector&) = delete;
		Vector(Vector&& other) {
			std::swap(ptr, other.ptr);
			std::swap(capacity, other.capacity);
			std::swap(sz, other.sz);
			std::swap(type, other.type);
			memcpy(buf, other.buf, SIZE * sizeof(T));
		}
		Vector& operator=(Vector&& other) {
			Vector temp(std::move(other));

			std::swap(this->ptr, temp.ptr);
			std::swap(this->capacity, temp.capacity);
			std::swap(this->sz, temp.sz);
			std::swap(this->type, temp.type);
			memcpy(this->buf, temp.buf, SIZE * sizeof(T));

			return *this;
		}
	public:
		iterator begin() {
			return iterator(type == 1 ? ptr : buf);
		}
		const_iterator begin() const {
			return iterator(type == 1 ? ptr : buf);
		}
		iterator end() {
			return iterator(type == 1 ? ptr + sz : buf + SIZE);
		}
		const_iterator end() const {
			return iterator(type == 1 ? ptr + sz : buf + SIZE);
		}
	public:
		void clear() {
			if (type == 1 && ptr) {
				delete ptr;
				ptr = nullptr;
			}
			capacity = SIZE;
			sz = 0;
			type = 0;
		}

		T& back() {
			if (type == 0) {
				return buf[sz - 1];
			}
			return ptr[sz - 1];
		}
		const T& back() const {
			if (type == 0) {
				return buf[sz - 1];
			}
			return ptr[sz - 1];
		}
		void pop_back() {
			if (empty() == false) {
				--sz;
			}
		}
		bool empty() const { return 0 == sz; }
		uint64_t size() const { return sz; }
		void push_back(T val) {
			if (type == 0) {
				buf[sz] = std::move(val);
				++sz;
				if (sz == SIZE) {
					if (ptr) {
						delete ptr; ptr = nullptr;
					}
					ptr = new (std::nothrow) T[SIZE * 2];
					if (!ptr) {
						throw ("new failed");
					}
					capacity = SIZE * 2;
					memcpy(ptr, buf, SIZE * sizeof(T));
					type = 1;
				}
			}
			else {
				if (sz < capacity) {
					ptr[sz] = std::move(val);
					++sz;
				}
				else {
					T* temp = new (std::nothrow) T[2 * capacity];
					if (!ptr) {
						throw ("new failed");
					}
					memcpy(temp, ptr, sz * sizeof(T));
					capacity = capacity * 2;
					delete ptr;
					ptr = temp;
					ptr[sz] = std::move(val);
					++sz;
				}
			}
		}
	public:
		T& operator[](const uint64_t idx) {
			if (type == 0) {
				return buf[idx];
			}
			return ptr[idx];
		}
		const T& operator[](const uint64_t idx) const {
			if (type == 0) {
				return buf[idx];
			}
			return ptr[idx];
		}

	};

	class Log;

	template <class T>
	static void _print(Log& log, const T& val, const int op);

	class Log {
	public:
		class Info {
		public:
			friend std::ostream& operator<<(std::ostream& stream, const Info&) {
				stream << "[INFO] ";
				return stream;
			}
		};
		class Warning { 
		public:
			friend std::ostream& operator<<(std::ostream& stream, const Warning&) {
				stream << "[WARN] ";
				return stream;
			}
		};

		enum class Option { CONSOLE, FILE, CONSOLE_AND_FILE, NO_PRINT };
		class Option2 {
		public:
			static const int INFO = 1;
			static const int WARN = 2;
			static const int CLEAR = 0;
		};
	private:
		Option opt; // console, file, ...
		int opt2; // info, warn, ...
		int state; // 1 : info, 2 : warn. // default is info!
		std::string fileName;
	public:

		Log() : state(0), opt(Option::NO_PRINT), opt2(Option2::CLEAR), fileName("log.txt") {
			//
		}

	public:
		template <class T>
		friend void _print(Log& log, const T& val, const int op);

	public:

		Option option() const {
			return opt;
		}

		int option2() const {
			return opt2;
		}

		void console() {
			opt = Option::CONSOLE;
		}

		void file() {
			opt = Option::FILE;
		}
		
		void console_and_file() {
			opt = Option::CONSOLE_AND_FILE;
		}

		void no_print() {
			opt = Option::NO_PRINT;
			opt2 = Option2::CLEAR;
		}

		void file_name(const std::string& str) {
			fileName = str;
		}

		void info(bool only = false) {
			if (only) {
				opt2 = Option2::INFO;
			}
			else {
				opt2 = opt2 | Option2::INFO;
			}
		}
		void warn(bool only = false) {
			if (only) {
				opt2 = Option2::WARN;
			}
			else {
				opt2 = opt2 | Option2::WARN;
			}
		}
	};

	template <class T>
	static void _print(Log& log, const T& val, const int op) { // op : change_state, with op.

		if (op == 0 || op == 1) {
			log.state = op;
		}

		if (log.opt == Log::Option::CONSOLE || log.opt == Log::Option::CONSOLE_AND_FILE) {

			int count = 0;

			if ((log.opt2 & Log::Option2::INFO) && log.state == 0) {
				count = 1;
			}
			if ((log.opt2 & Log::Option2::WARN) && log.state == 1) {
				count = 1;
			}

			if (count) {
				std::cout << val;
			}
		}

		if (log.opt == Log::Option::FILE || log.opt == Log::Option::CONSOLE_AND_FILE) {
			std::ofstream outFile;
			outFile.open(log.fileName, std::ios::app);
			if (outFile) {
				int count = 0;

				if ((log.opt2 & Log::Option2::INFO) && log.state == 0) {
					count = 1;
				}
				if ((log.opt2 & Log::Option2::WARN) && log.state == 1) {
					count = 1;
				}

				if (count) {
					outFile << val;
				}
				outFile.close();
			}
		}
	}

	template <class T>
	inline Log& operator<<(Log& log, const T& val) {
		_print(log, val, -1);
		return log;
	}

	template<>
	inline Log& operator<<(Log& log, const Log::Info& x) {
		_print(log, x, 0);
		return log;
	}
	template<>
	inline Log& operator<<(Log& log, const Log::Warning& x) {
		_print(log, x, 1);
		return log;
	}

	static Log::Info info;
	static Log::Warning warn;
	extern Log log; // no static..

	// inline Error error;


	template <class T>
	using PtrWeak = T*;

	template <class T>
	using Ptr = std::unique_ptr<T>;
	// Ptr - use std::move

	class _Value;
	class Structured;
	class Array;
	class Object;

	enum class _ValueType : int32_t {
		NONE = 1,
		ARRAY, // ARRAY_OBJECT -> ARRAY, OBJECT
		OBJECT, 
		INT, UINT,
		FLOAT,
		BOOL,
		NULL_,
		STRING, SHORT_STRING,
		NOT_VALID,
		ERROR // private class?
	};

	// sz`s type is uint32_t, not uint64_t.
	class String {
		friend class _Value;
	private: // do not change of order. do not add variable.
		#define CLAUJSON_STRING_BUF_SIZE 11
		union {
			struct {
				char* str;
				uint32_t sz;
				_ValueType type; // STRING or SHORT_STRING or NOT_VALID ...
			};
			struct {
				char buf[CLAUJSON_STRING_BUF_SIZE];
				uint8_t buf_sz;
				_ValueType type_;
			};
		};
	public:
		static const uint64_t npos = -1;
	public:
		String& operator=(const String& other) {
			if (!is_valid() || !other.is_valid() || this == &other) { return *this; }

			if (this->is_str()) {
				clear();
			}

			if (other.type == _ValueType::STRING) {
				this->str = new (std::nothrow) char[other.sz + 1];
				if (this->str == nullptr) {
					this->type = _ValueType::ERROR;
					log << warn << "new error";
					return *this;
				}
				this->sz = other.sz;
				memcpy(this->str, other.str, other.sz);
				this->str[this->sz] = '\0';
				this->type = other.type;
			}
			else if (other.type == _ValueType::SHORT_STRING) {
				memcpy(buf, other.buf, CLAUJSON_STRING_BUF_SIZE);
				this->buf_sz = other.buf_sz;
				this->type_ = other.type_;
			}

			return *this;
		}
	protected:
		String(const String& other) {
			if (other.type == _ValueType::STRING) {
				this->str = new (std::nothrow) char[other.sz + 1];
				if (this->str == nullptr) {
					log << warn << "new error";
					this->type = _ValueType::ERROR; return;
				}
				this->sz = other.sz;
				memcpy(this->str, other.str, other.sz);
				this->str[this->sz] = '\0';
				this->type = other.type;
			}
			else if (other.type == _ValueType::SHORT_STRING) {
				memcpy(buf, other.buf, CLAUJSON_STRING_BUF_SIZE);
				this->buf_sz = other.buf_sz;
				this->type_ = other.type_;
			}
		}
		String(String&& other) noexcept {
			this->type = _ValueType::NONE;
			std::swap(this->str, other.str);
			std::swap(this->sz, other.sz);
			std::swap(this->type, other.type);
		}

	public:

		explicit String() : type(_ValueType::NONE) {
			str = nullptr;
			sz = 0;
		}
		
		~String() {
			if (type == _ValueType::STRING && str) {
				delete[] str; 
			}

			str = nullptr;
			sz = 0;
			type = _ValueType::NONE;
		}


		String clone() const {
			if (is_valid() == false) { String temp; temp.type = _ValueType::NOT_VALID; return temp; }
			String obj;

			if (this->type == _ValueType::STRING) {
				obj.sz = this->sz;
				obj.str = new (std::nothrow) char[this->sz + 1];
				if (obj.str == nullptr) {
					log << warn << "new error";
					obj.type = _ValueType::ERROR;
					String result;
					result.type = _ValueType::ERROR;
					return result;
				}
				memcpy(obj.str, this->str, this->sz);
				obj.str[obj.sz] = '\0';
			}
			else if (this->type == _ValueType::SHORT_STRING) {
				obj.buf_sz = this->buf_sz;
				memcpy(obj.buf, this->buf, CLAUJSON_STRING_BUF_SIZE);
			}
			
			obj.type = this->type;

			return obj;
		}

		String& operator=(String&& other) noexcept {
			if (this->is_valid() == false || other.is_valid() == false || this == &other) { return *this; }
			std::swap(this->str, other.str);
			std::swap(this->sz, other.sz);
			std::swap(this->type, other.type);
			return *this;
		}

	private:
		explicit String(const char* str) {
			if (!str) { this->type = _ValueType::ERROR; return; }
			
			this->sz = Static_Cast<uint64_t, uint32_t>(strlen(str));
			if (this->sz < CLAUJSON_STRING_BUF_SIZE) {
				this->buf_sz = (uint8_t)this->sz;
				memcpy(this->buf, str, static_cast<uint64_t>(this->buf_sz));
				this->buf[(uint64_t)this->buf_sz] = '\0';
				this->type = _ValueType::SHORT_STRING;
			}
			else {
				this->str = new (std::nothrow) char[this->sz + 1];
				if (this->str == nullptr) {
					log << warn << "new error";
					this->type = _ValueType::ERROR; return;
				}
				memcpy(this->str, str, this->sz);
				this->str[this->sz] = '\0';
				this->type = _ValueType::STRING;
			}
		}

		explicit String(const char* str, uint32_t sz) {
			if (!str) { this->type = _ValueType::ERROR; return; }

			this->sz = sz;
			if (this->sz < CLAUJSON_STRING_BUF_SIZE) {
				this->buf_sz = (uint8_t)this->sz;
				memcpy(this->buf, str, static_cast<uint64_t>(this->buf_sz));
				this->buf[(uint64_t)this->buf_sz] = '\0';
				this->type = _ValueType::SHORT_STRING;
			}
			else {
				this->str = new (std::nothrow) char[this->sz + 1];
				if (this->str == nullptr) {
					this->type = _ValueType::ERROR;
					log << warn << "new error";
					return;
				}
				memcpy(this->str, str, this->sz);
				this->str[this->sz] = '\0';
				this->type = _ValueType::STRING;
			}
		}

	public:
		bool is_valid() const {
			return type != _ValueType::NOT_VALID && type != _ValueType::ERROR;
		}

		bool is_str() const {
			return type == _ValueType::STRING || type == _ValueType::SHORT_STRING;
		}

		char* data() {
			if (type == _ValueType::STRING) {
				return str;
			}
			else if (type == _ValueType::SHORT_STRING) {
				return buf;
			}
			else {
				return nullptr;
			}
		}
		
		const char* data() const {
			if (type == _ValueType::STRING) {
				return str;
			}
			else if (type == _ValueType::SHORT_STRING) {
				return buf;
			}
			else {
				return nullptr;
			}
		}
		uint64_t size() const {
			if (type == _ValueType::STRING) {
				return sz;
			}
			else if (type == _ValueType::SHORT_STRING) {
				return static_cast<uint64_t>(buf_sz); 
			}
			else {
				return 0;
			}
		}

		// remove data.
		void clear() {
			if (type == _ValueType::STRING && str) {
				delete[] str;
			}
			sz = 0;
			str = nullptr;
			type = _ValueType::NONE;
		}

		bool operator<(const String& other) const {
			if (!this->is_valid() || !other.is_valid()) { return false; }
			return StringView(data(), size()) < StringView(other.data(), other.size());
		}
		bool operator<(const StringView other) const {
			if (!this->is_valid()) { return false; }
			return StringView(data(), size()) < other;
		}
		bool operator==(const StringView other) const {
			if (!this->is_valid()) { return false; }
			return StringView(data(), size()) == other;
		}

		bool operator==(const String& other) const {
			if (!this->is_valid() || !other.is_valid()) { return false; }
			return StringView(data(), size()) == StringView(other.data(), other.size());
		}

		std::string get_std_string(bool& fail) const {
			if (!is_str()) { fail = true; return std::string(); }
			fail = false;
			return std::string(data(), size());
		}
		StringView get_string_view(bool& fail) const {
			if (!is_str()) { fail = true; return StringView(); }
			fail = false;
			return StringView(data(), size());
		}

		uint64_t find(char ch, uint64_t start) const {
			const char* x = data();
			uint64_t sz = size();

			for (uint64_t i = start; i < sz; ++i) {
				if (x[i] == ch) {
					return i;
				}
			}
			return npos;
		}

		String substr(uint64_t start, uint64_t len) {
			return String(data() + start, len);
		}
	private:
		// suppose str is valid utf-8 string!
		explicit String(const std::string& str) {
			if (str.size() <= CLAUJSON_STRING_BUF_SIZE) {
				memcpy(buf, str.data(), str.size());
				this->sz = Static_Cast<uint64_t, uint32_t>(str.size()); // chk..
				this->type = _ValueType::SHORT_STRING;
			}
			else {
				char* temp = new (std::nothrow) char[str.size()];
				if (!temp) {
					// log << warn ...
					this->type = _ValueType::NONE;
					return;
				}
				memcpy(temp, str.data(), str.size());
				this->str = temp;
				this->sz = Static_Cast<uint64_t, uint32_t>(str.size());
				this->type = _ValueType::STRING;
			}
		}
	};


	class _Value {

	public:
		// todo - check type of Data....
		// using INT_t = int64_t; 
		// using UINT_t = uint64_t;
		// using FlOAT_t = double;
		// using STR_t = std::string;
		// using BOOL_t = bool;
		
	public:
		friend std::ostream& operator<<(std::ostream& stream, const _Value& data);

		friend bool ConvertString(_Value& data, const char* text, uint64_t len);

		friend class Object;
		friend class Array;
	private:

		// do not change!
		union {
			struct {
				union {
					int64_t _int_val;
					uint64_t _uint_val;
					double _float_val;
					Structured* _array_or_object_ptr;
					bool _bool_val;
				};
				uint32_t temp;
				_ValueType _type;
			};
			String _str_val;
		};

		/// before version..
		//union {
		//	int64_t _int_val = 0;
		//	uint64_t _uint_val;
		//	double _float_val;
		//	std::string* _str_val;
		//	Structured* _array_or_object_ptr;
		//	bool _bool_val;
		//};
		//_ValueType _type = _ValueType::NONE; 
		//bool _valid = true;

	public:

		_Value clone() const;

		explicit operator bool() const;

		explicit _Value(Structured* x);
		explicit _Value(int x);

		explicit _Value(unsigned int x);

		explicit _Value(int64_t x);
		explicit _Value(uint64_t x);
		explicit _Value(double x);

		explicit _Value(StringView x); 

#if __cpp_lib_char8_t
		// C++20~
		explicit _Value(std::u8string_view x);
		explicit _Value(const char8_t* x);
#endif
		
		explicit _Value(const char* x);

		explicit _Value(_Value*) = delete;

		explicit _Value(bool x);
		explicit _Value(std::nullptr_t x);

		explicit _Value(std::nullptr_t, bool valid);

		explicit _Value(String&& x) {
			this->_str_val = std::move(x);
		}
	public:
		_ValueType type() const;

		bool is_valid() const;

		bool is_null() const;

		bool is_primitive() const; // int, uint, float, bool(true, false), string, null

		bool is_structured() const; // array or object (or used in inner, partialjson )

		bool is_array() const;

		bool is_object() const;

		bool is_int() const;

		bool is_uint() const;

		bool is_float() const;

		bool is_number() const {
			return is_valid() && (is_int() || is_uint() || is_float());
		}

		bool is_bool() const;

		bool is_str() const;

		int64_t get_integer() const {
			return int_val();
		}

		int64_t& get_integer() {
			return int_val();
		}
		
		int64_t int_val() const;

		uint64_t get_unsigned_integer() const {
			return uint_val();
		}

		uint64_t& get_unsigned_integer() {
			return uint_val();
		}

		uint64_t uint_val() const;

		double get_floating() const {
			return float_val();
		}
		
		double& get_floating() {
			return float_val();
		}

		template <typename T>
		T get_number() const {
			if (is_float()) {
				return static_cast<T>(_float_val);
			}
			return static_cast<T>(_uint_val);
		}

		double float_val() const;

		int64_t& int_val();

		uint64_t& uint_val();

		double& float_val();

		bool get_boolean() const {
			return bool_val();
		}

		bool& get_boolean() {
			return bool_val();
		}

		bool bool_val() const;

		bool& bool_val();

		Structured* get_structured() const {
			return _array_or_object_ptr;
		}

		_Value& json_pointerB(const std_vector<_Value>& routeDataVec);
		const _Value& json_pointerB(const std_vector<_Value>& routeVec) const;

		Array* as_array();
		Object* as_object();
		Structured* as_structured_ptr();

		const Array* as_array()const;
		const Object* as_object()const;
		const Structured* as_structured_ptr()const;

		uint64_t find(const _Value& key) const; // find without key`s converting?

		// _Value (type is String or Short_String) -> no need utf8, unicode check.
		_Value& operator[](const _Value& key); // if not exist key, then nothing.
		const _Value& operator[](const _Value& key) const; // if not exist key, then nothing.


		_Value& operator[](uint64_t idx);
		const _Value& operator[](uint64_t idx) const;
	public:
		void clear(bool remove_str); 

		String& get_string() {
			return str_val();
		}

		String& str_val();

		const String& get_string() const {
			return str_val();
		}

		const String& str_val() const;

		void set_ptr(Structured* x);
		void set_int(long long x);

		void set_uint(unsigned long long x);

		void set_float(double x);

		bool set_str(const char* str, uint64_t len);

		bool set_str(String str);
	private:
		void set_str_in_parse(const char* str, uint64_t len);
	public:
		void set_bool(bool x);

		void set_null();

		// chk!! with clauscript++?
		std::string convert_primitive_to_std_string() {
			if (is_int()) {
				return std::to_string(get_integer());
			}
			else if (is_uint()) {
				return std::to_string(get_unsigned_integer());
			}
			else if (is_float()) {
				return std::to_string(get_floating());
			}
			else if (is_bool()) {
				return std::to_string(get_boolean());
			}
			else if (is_null()) {
				return "null";
			}
			else if (is_str()) {
				bool fail = false;
				return get_string().get_std_string(fail);
			}
			else {
				return "";
			}
		}

	private:
		void set_type(_ValueType type);

	public:
		~_Value();

		_Value(const _Value& other) = delete;

		_Value(_Value&& other) noexcept;

		_Value();

		bool operator==(const _Value& other) const;

		bool operator!=(const _Value& other) const;

		bool operator<(const _Value& other) const;

		_Value& operator=(const _Value& other) = delete;


		_Value& operator=(_Value&& other) noexcept;
	};

	class Value {
	private:
		_Value x;
	public:
		Value() noexcept { }

		Value(_Value&& x) noexcept : x(std::move(x)) {
			//
		}
		Value(Value&& x) noexcept : x(std::move(x.x)) {
			//
		}

		~Value() noexcept;
	public:
		Value& operator=(const Value&) = delete;
		Value(const Value& other) = delete;
	public:
		_Value& Get() noexcept { return x; }
		const _Value& Get() const noexcept { return x; }
	};
	
	class parser;

	class Document {
	public:
		friend class parser;
	private:
		_Value x;
#ifdef USE_PMR
		std::vector<std::byte>* res_buf = nullptr;
		std::vector<std::pmr::monotonic_buffer_resource*>* res = nullptr;
#endif
	public:
		Document() noexcept { }

		Document(_Value&& x) noexcept : x(std::move(x))  {
			//
		}

		~Document() noexcept;
	public:
		Document& operator=(const Document&) = delete;
		Document(const _Value&) = delete;
	public:
		_Value& Get() noexcept { return x; }
		const _Value& Get() const noexcept { return x; }
	};
}

namespace claujson {
	class LoadData;
	class LoadData2;

	class Array;
	class Object;
	class PartialJson; // rename?

	class Structured {
		friend class LoadData2;
		friend class LoadData;
		friend class Array;
		friend class Object;
		friend class PartialJson;
	protected:
		PtrWeak<Structured> parent = nullptr;
	protected:
		static _Value data_null; // valid is false..
	public:
		static uint64_t npos; // 
	public:
		Structured(const Structured& other) = delete;
		Structured& operator=(const Structured& other) = delete;

		Structured* clone() const;

		explicit Structured();

		virtual ~Structured();

		uint64_t find(const _Value& key) const; // find without key`s converting ( \uxxxx )

		_Value& operator[](const _Value& key); // if not exist key, then _Value <- is not valid.
		const _Value& operator[](const _Value& key) const; // if not exist key, then _Value <- is not valid.

		_Value& operator[](uint64_t idx);

		const _Value& operator[](uint64_t idx) const;

		PtrWeak<Structured> get_parent() const;

	public:
		bool change_key(const _Value& key, Value new_key);
		bool change_key(uint64_t idx, Value new_key);

		virtual _Value& get_value();

		virtual void reserve_data_list(uint64_t len) = 0; // if object, reserve key_list and value_list, if array, reserve value_list.

		virtual bool is_object() const = 0;
		virtual bool is_array() const = 0;
		virtual bool is_partial_json() const;

		bool is_user_type() const;

		// for valid with object or array or root.
		uint64_t size() const {
			return get_data_size();
		}
		bool empty() const {
			return 0 == get_data_size();
		}

		virtual uint64_t get_data_size() const = 0; // data_size == key_list_size (if object), and data_size == value_list_size.
		virtual _Value& get_value_list(uint64_t idx) = 0;
	private:
		virtual _Value& get_key_list(uint64_t idx) = 0;
	public:
		virtual const _Value& get_value_list(uint64_t idx) const = 0;
		virtual const _Value& get_key_list(uint64_t idx) const = 0;

		virtual const _Value& get_const_key_list(uint64_t idx) = 0;
		virtual const _Value& get_const_key_list(uint64_t idx) const = 0;

		virtual void clear(uint64_t idx) = 0;
		virtual void clear() = 0;

		virtual bool is_virtual() const = 0;

		// todo return type void -> bool.
		virtual bool add_object_element(Value key, Value val) = 0;
		virtual bool add_array_element(Value val) = 0;
		virtual bool add_array(Ptr<Structured> arr) = 0;  // change to _Value ? or remove?
		virtual bool add_object(Ptr<Structured> obj) = 0; // change to _Value ? or remove?

		virtual bool add_array(Value key, Ptr<Structured> arr) = 0;  // change to _Value ? or remove?
		virtual bool add_object(Value key, Ptr<Structured> obj) = 0; // change to _Value ? or remove?

		virtual bool assign_value_element(uint64_t idx, Value val) = 0;
		virtual bool assign_key_element(uint64_t idx, Value key) = 0;

		virtual void erase(const _Value& key, bool real = false) = 0;
		virtual void erase(uint64_t idx, bool real = false) = 0;

	private:
		void set_parent(PtrWeak<Structured> j);


	private:
		// used while parsing.
		virtual void MergeWith(PtrWeak<Structured> j, int start_offset) = 0; // start_offset is 0 or 1 (if virtual array or virtual object exists)

		// need rename param....!
		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx) = 0;

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t val_token_idx) = 0;

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			 _ValueType type, uint64_t key_token_idx 
#ifdef USE_PMR
			, std::pmr::monotonic_buffer_resource* res
#endif
			
		) = 0;

		//
		virtual void add_user_type(_ValueType type
#ifdef USE_PMR
			, std::pmr::monotonic_buffer_resource* res
#endif
			) = 0; // int type -> enum?

		virtual bool add_user_type(Value key, Ptr<Structured> j) = 0;

		virtual bool add_user_type(Ptr<Structured> j) = 0;
	};

	template <class Key, class Data>
	class Pair {
	public:
		Key first;
		Data second;
	public:
		Pair() { }
		Pair(Key&& first, Data&& second) : first(std::move(first)), second(std::move(second)) { }
		Pair(const Key& first, Data&& second) : first((first)), second(std::move(second)) {}
		Pair(Key&& first, const Data& second) : first(std::move(first)), second((second)) {}
	};

	class Object : public Structured {
	protected:
		std_vector<Pair<claujson::_Value, claujson::_Value>> obj_data;
	public:
		using _ValueIterator = std_vector<Pair<claujson::_Value, claujson::_Value>>::iterator;
		using _ConstValueIterator = std_vector<Pair<claujson::_Value, claujson::_Value>>::const_iterator;
	protected:
		//explicit Object(bool valid);
	public:
		friend class _Value;

		Structured* clone() const;

		bool chk_key_dup(uint64_t* idx) const;  // chk duplication of key. only Object, Virtual Object..

		[[nodiscard]]
		static _Value Make();

		explicit Object();
#ifdef USE_PMR
		explicit Object(std::pmr::monotonic_buffer_resource* res) : obj_data(res) { }
#endif
		virtual ~Object();

		virtual bool is_object() const;
		virtual bool is_array() const;

		virtual uint64_t get_data_size() const;

		virtual _Value& get_value_list(uint64_t idx);
	private:
		virtual _Value& get_key_list(uint64_t idx);
	public:

		virtual const _Value& get_value_list(uint64_t idx) const;

		virtual const _Value& get_key_list(uint64_t idx) const;

		virtual const _Value& get_const_key_list(uint64_t idx);

		virtual const _Value& get_const_key_list(uint64_t idx) const;

		virtual void clear(uint64_t idx);

		virtual bool is_virtual() const;

		virtual void clear();


		_ValueIterator begin();
		_ValueIterator end();


		_ConstValueIterator begin() const;
		_ConstValueIterator end() const;
		
		virtual void reserve_data_list(uint64_t len);

		virtual bool add_object_element(Value key, Value val);
		virtual bool add_array_element(Value val);
		virtual bool add_array(Ptr<Structured> arr);
		virtual bool add_object(Ptr<Structured> obj);

		virtual bool add_array(Value key, Ptr<Structured> arr);  // change to _Value ? or remove?
		virtual bool add_object(Value key, Ptr<Structured> obj); // change to _Value ? or remove?


		virtual bool assign_value_element(uint64_t idx, Value val);
		virtual bool assign_key_element(uint64_t idx, Value key);

		virtual void erase(const _Value& key, bool real = false);
		virtual void erase(uint64_t idx, bool real = false);

	private:
		virtual void MergeWith(PtrWeak<Structured> j, int start_offset);

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			_ValueType type, uint64_t key_token_idx
#ifdef USE_PMR
			, std::pmr::monotonic_buffer_resource* res
#endif

		);

		//
		virtual void add_user_type(_ValueType type
#ifdef USE_PMR
			, std::pmr::monotonic_buffer_resource* res
#endif
		); // int type -> enum?

		virtual bool add_user_type(Value key, Ptr<Structured> j);

		virtual bool add_user_type(Ptr<Structured> j);
	};

	class Array : public Structured {
	protected:
		std_vector<_Value> arr_vec;
	public:
		using _ValueIterator = std_vector<_Value>::iterator;
		using _ConstValueIterator = std_vector<_Value>::const_iterator;
	protected:
		//explicit Array(bool valid);
	public:
		friend class _Value;
		friend class PartialJson;

		Structured* clone() const;

		[[nodiscard]]
		static _Value Make();

		explicit Array();

#ifdef USE_PMR
		explicit Array(std::pmr::memory_resource* res);
#endif

		virtual ~Array();

		virtual bool is_object() const;
		virtual bool is_array() const;

		virtual uint64_t get_data_size() const;

		virtual _Value& get_value_list(uint64_t idx);
	private:
		virtual _Value& get_key_list(uint64_t idx);
	public:
		virtual const _Value& get_value_list(uint64_t idx) const;

		virtual const _Value& get_key_list(uint64_t idx) const;

		virtual const _Value& get_const_key_list(uint64_t idx);

		virtual const _Value& get_const_key_list(uint64_t idx) const;

		virtual void clear(uint64_t idx);

		virtual bool is_virtual() const;

		virtual void clear();

		virtual void reserve_data_list(uint64_t len);


		_ValueIterator begin();

		_ValueIterator end();


		_ConstValueIterator begin() const;

		_ConstValueIterator end() const;


		virtual bool add_object_element(Value key, Value val);

		virtual bool add_array_element(Value val);

		virtual bool add_array(Ptr<Structured> arr);

		virtual bool add_object(Ptr<Structured> obj);

		virtual bool add_array(Value key, Ptr<Structured> arr);  // change to _Value ? or remove?
		
		virtual bool add_object(Value key, Ptr<Structured> obj); // change to _Value ? or remove?

		virtual bool assign_value_element(uint64_t idx, Value val);
		virtual bool assign_key_element(uint64_t idx, Value key);

		virtual void erase(const _Value& key, bool real = false); 
		virtual void erase(uint64_t idx, bool real = false);

	private:
		// here only used in parsing.

		virtual void MergeWith(PtrWeak<Structured> j, int start_offset); // start_offset is 0 or 1 (if virtual array or virtual object exists)

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx, 
			char* buf,  uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			_ValueType type, uint64_t key_token_idx
#ifdef USE_PMR
			, std::pmr::monotonic_buffer_resource* res
#endif

		);

		//
		virtual void add_user_type(_ValueType type
#ifdef USE_PMR
			, std::pmr::monotonic_buffer_resource* res
#endif
		); // int type -> enum?

		virtual bool add_user_type(Value key, Ptr<Structured> j);

		virtual bool add_user_type(Ptr<Structured> j);

	};

}

namespace claujson {
	/*
	class Array2;
	class Object2;

	using Type2 = _ValueType;
	using String2 = String;

	class Primitive2 { // cf) _Value 
	public:
		union {
			struct {
				union {
					uint64_t m_uint;
					int64_t  m_int;
					double   m_float;
					bool     m_bool;
				};
				uint32_t temp;
				Type2 m_type;
			};
			String2 m_str;
		};

	public:
		Primitive2();
		~Primitive2();
	};

	class Wizard;

	class Document2 {
	private:
		friend class Wizard;
	private:
		std_vector<Primitive2> prim_vec;  // 01
		std_vector<Array2> arr_vec;       // 10
		std_vector<Object2> obj_vec;      // 11
		uint64_t root = 0;
	public:
		Primitive2& as_primitive();
		Array2& as_array();
		Object2& as_object();

		bool is_primitive() const;
		bool is_array() const;
		bool is_object() const;
	};

	class Wizard {
	private:
		Document2* res;
	public:
		Wizard(Document2* res);
	public:
		bool is_primitive(uint64_t id);
		bool is_array(uint64_t id);
		bool is_object(uint64_t id);
		
		Primitive2& as_primitive(uint64_t id);
		Array2& as_array(uint64_t id);
		Object2& as_object(uint64_t id);
	};

	class Array2 {
	private:
		std_vector<uint64_t> id_vec;
		Document2* resources = nullptr; 
	public:
		Array2(Document2* res);
	public:
		void add_element(Primitive2 value);
		void add_element(Array2 arr);
		void add_element(Object2 obj);
		
		uint64_t operator[](uint64_t idx) const;
	};

	class Object2 {
	private:
		std_vector<uint64_t> id_vec;
		Document2* resources = nullptr;
	public:
		Object2(Document2* res);
	public:
		void add_element(Primitive2 key, Primitive2 value);
		void add_element(Primitive2 key, Array2 arr);
		void add_element(Primitive2 key, Object2 obj);

		uint64_t operator[](uint64_t idx) const;
	};
	*/

	class parser {
	private:
		_simdjson::dom::parser_for_claujson test_;
		std::unique_ptr<ThreadPool> pool;
	public:
		parser(int thr_num = 0);
	public:
		// parse json file.
		std::pair<bool, uint64_t> parse(const std::string& fileName, Document& d, uint64_t thr_num);

		//std::pair<bool, uint64_t> parse2(const std::string& fileName, Document2*& j, uint64_t thr_num);
		
		// parse json str.
		std::pair<bool, uint64_t> parse_str(StringView str, Document& d, uint64_t thr_num);

#if __cpp_lib_char8_t
		// C++20~
		std::pair<bool, uint64_t> parse_str(std::u8string_view str, Document& d, uint64_t thr_num);
#endif
	};

	class writer {
	private:
		std::unique_ptr<ThreadPool> pool;
	public:
		writer(int thr_num = 0);
	public:
		std::string write_to_str(const _Value& global, bool prettty = false);
		std::string write_to_str2(const _Value& global, bool prettty = false);

		void write(const std::string& fileName, const _Value& global, bool pretty = false);

		void write_parallel(const std::string& fileName, _Value& j, uint64_t thr_num, bool pretty = false);
		void write_parallel2(const std::string& fileName, const _Value& j, uint64_t thr_num, bool pretty = false);
	};


	[[nodiscard]]
	_Value diff(const _Value& x, const _Value& y);

	_Value& patch(_Value& x, const _Value& diff);

	void clean(_Value& x); //

	std::pair<bool, std::string> convert_to_string_in_json(StringView x);
	
	bool convert_number(StringView x, claujson::_Value& data);

	bool convert_string(StringView x, claujson::_Value& data);

	bool is_valid_string_in_json(StringView x);

#if __cpp_lib_char8_t
	std::pair<bool, std::string> convert_to_string_in_json(std::u8string_view x);

	bool is_valid_string_in_json(std::u8string_view x);
#endif
}
