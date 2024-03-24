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


#if __cpp_lib_string_view
#include <string_view>
using namespace std::literals::string_view_literals;

using StringView = std::string_view;
#else

namespace claujson {
	class StringView {
	public:
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
			return 0 == strcmp(this->data(), view.data());
		}

		bool operator<(const StringView view) {
			return strcmp(this->data(), view.data()) < 0;
		}
	};
}


claujson::StringView operator""sv(const char* str, size_t sz);
bool operator==(const std::string& str, claujson::StringView sv);


#endif

namespace claujson {

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

		Log() : state(0), opt(Option::CONSOLE), opt2(Option2::INFO | Option2::WARN), fileName("log.txt") {
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

	class Value;
	class Structured;
	class Array;
	class Object;

	enum class ValueType : int32_t {
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
	private: // do not change of order. do not add variable.
		static const int BUF_SIZE = 11;
		union {
			struct {
				char* str;
				uint32_t sz;
				ValueType type; // STRING or NOT_VALID
			};
			struct {
				char buf[BUF_SIZE];
				uint8_t buf_sz;
				ValueType type_;
			};
		};

	public:
		enum class STR_OPTION { GENERAL, NOT_VALID_STR, MOVED_STR }; 
	private:
		static const String not_valid_str;
	
	public:
		String& operator=(const String& other) {
			if (!is_valid() || !other.is_valid() || this == &other) { return *this; }

			if (is_str()) {
				clear();
			}

			if (other.type == ValueType::STRING) {
				this->str = new char[other.sz + 1];
				this->sz = other.sz;
				memcpy(this->str, other.str, other.sz);
				this->str[this->sz] = '\0';
				this->type = other.type;
			}
			else if (other.type == ValueType::SHORT_STRING) {
				memcpy(buf, other.buf, BUF_SIZE);
				this->buf_sz = other.buf_sz;
				this->type_ = other.type_;
			}

			return *this;
		}
	protected:
		String(const String&) = default;

	public:

		// not valid String. used like null?
		explicit String(STR_OPTION opt = STR_OPTION::GENERAL/* not valid str */) : type(opt == STR_OPTION::GENERAL? ValueType::STRING : ValueType::NOT_VALID) {
			str = nullptr;
			sz = 0;
		}
		
		~String() {
			if (type == ValueType::STRING && str) {
				delete[] str; // has bug, todo - cancell moved str?
			}

			str = nullptr;
			sz = 0;
			type = ValueType::NONE;
		}


		String clone() const {
			if (is_valid() == false) { not_valid_str; }
			String obj;

			if (this->type == ValueType::STRING) {
				obj.sz = this->sz;
				obj.str = new char[this->sz + 1];

				memcpy(obj.str, this->str, this->sz);
				obj.str[obj.sz] = '\0';
			}
			else if (this->type == ValueType::SHORT_STRING) {
				obj.buf_sz = this->buf_sz;
				memcpy(obj.buf, this->buf, BUF_SIZE);
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


		explicit String(const char* str) {
			if (!str) { this->type = ValueType::ERROR; return; }
			
			this->sz = strlen(str);
			if (this->sz < BUF_SIZE) {
				this->buf_sz = (uint8_t)this->sz;
				memcpy(this->buf, str, (uint64_t)this->buf_sz);
				this->buf[(uint64_t)this->buf_sz] = '\0';
				this->type = ValueType::SHORT_STRING;
			}
			else {
				this->str = new char[this->sz + 1];
				memcpy(this->str, str, this->sz);
				this->str[this->sz] = '\0';
				this->type = ValueType::STRING;
			}
		}

		explicit String(const char* str, uint32_t sz) {
			if (!str) { this->type = ValueType::ERROR; return; }

			this->sz = sz;
			if (this->sz < BUF_SIZE) {
				this->buf_sz = (uint8_t)this->sz;
				memcpy(this->buf, str, (uint64_t)this->buf_sz);
				this->buf[(uint64_t)this->buf_sz] = '\0';
				this->type = ValueType::SHORT_STRING;
			}
			else {
				this->str = new char[this->sz + 1];
				memcpy(this->str, str, this->sz);
				this->str[this->sz] = '\0';
				this->type = ValueType::STRING;
			}
		}

	public:
		bool is_valid() const {
			return type != ValueType::NOT_VALID;
		}

		bool is_str() const {
			return type == ValueType::STRING || type == ValueType::SHORT_STRING;
		}

		char* data() {
			if (type == ValueType::STRING) {
				return str;
			}
			else if (type == ValueType::SHORT_STRING) {
				return buf;
			}
			else {
				return nullptr;
			}
		}

		const char* data() const {
			if (type == ValueType::STRING) {
				return str;
			}
			else if (type == ValueType::SHORT_STRING) {
				return buf;
			}
			else {
				return nullptr;
			}
		}
		uint64_t size() const {
			if (type == ValueType::STRING) {
				return sz;
			}
			else if (type == ValueType::SHORT_STRING) {
				return (uint64_t)buf_sz; // static_cast?
			}
			else {
				return 0;
			}
		}

		// remove data.
		void clear() {
			if (type == ValueType::STRING && str) {
				delete[] str;
			}
			sz = 0;
			str = nullptr;
			type = ValueType::NONE;
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

		std::string get_std_string() const {
			if (!str) { return std::string(); }
			return std::string(data(), size());
		}
	};

	const String not_valid_str(String::STR_OPTION::NOT_VALID_STR);


	class Value {

	public:
		// todo - check type of Data....
		// using INT_t = int64_t; 
		// using UINT_t = uint64_t;
		// using FlOAT_t = double;
		// using STR_t = std::string;
		// using BOOL_t = bool;
		
	public:
		friend std::ostream& operator<<(std::ostream& stream, const Value& data);

		friend bool ConvertString(Value& data, char* text, uint64_t len);

	private:

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
				ValueType _type;
			};
			String _str_val;
		};


		//union {
		//	int64_t _int_val = 0;
		//	uint64_t _uint_val;
		//	double _float_val;
		//	std::string* _str_val;
		//	Structured* _array_or_object_ptr;
		//	bool _bool_val;
		//};

		//ValueType _type = ValueType::NONE; 
		//bool _valid = true;

	public:

		Value clone() const;

		explicit operator bool() const;

		explicit Value(Structured* x);
		explicit Value(int x);

		explicit Value(unsigned int x);

		explicit Value(int64_t x);
		explicit Value(uint64_t x);
		explicit Value(double x);

		explicit Value(StringView x); // C++17

#if __cpp_lib_char8_t
		// C++20~
		explicit Value(std::u8string_view x);
		explicit Value(const char8_t* x);
#endif
		
		explicit Value(const char* x);

		explicit Value(Value*) = delete;

		explicit Value(bool x);
		explicit Value(nullptr_t x);

		explicit Value(nullptr_t, bool valid);

	public:
		ValueType type() const;

		bool is_valid() const;

		bool is_null() const;

		bool is_primitive() const; // int, uint, float, bool(true, false), string, null

		bool is_structured() const; // array or object (or used in inner, partialjson )

		bool is_array() const;

		bool is_object() const;

		bool is_int() const;

		bool is_uint() const;

		bool is_float() const;

		bool is_bool() const;

		bool is_str() const;

		//bool is_ptr() const; // check is_structured()

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

	//	Structured* ptr_val() const;

		Value& json_pointer(StringView route);

		const Value& json_pointer(StringView route) const;

		Value& json_pointer(const Value& route);
		const Value& json_pointer(const Value& route) const;

		static bool json_pointerA(StringView route, std::vector<Value>& vec);
#if __cpp_lib_char8_t

		Value& json_pointer(std::u8string_view route);

		const Value& json_pointer(std::u8string_view route) const;


		static bool json_pointerA(std::u8string_view route, std::vector<Value>& vec);
#endif

		Value& json_pointerB(const std::vector<Value>& routeDataVec);

		Array* as_array();
		Object* as_object();
		Structured* as_structured_ptr();

		const Array* as_array()const;
		const Object* as_object()const;
		const Structured* as_structured_ptr()const;

		/// \overload
		uint64_t find(StringView key) const;
		/// \overload
		uint64_t find(const Value& key) const; // find without key`s converting?
#if __cpp_lib_char8_t

		uint64_t find(std::u8string_view key) const;

		// without convert
		Value& operator[](std::u8string_view key); // if not exist key, then nothing.
		const Value& operator[](std::u8string_view key) const; // if not exist key, then nothing.
#endif

		// at vs [] (at <- convert key to key_in_json.) ([] <- do not convert.)
		Value& operator[](StringView key); // if not exist key, then nothing.
		const Value& operator[](StringView key) const; // if not exist key, then nothing.

		Value& operator[](const Value& key); // if not exist key, then nothing.
		const Value& operator[](const Value& key) const; // if not exist key, then nothing.


		Value& operator[](uint64_t idx);

		const Value& operator[](uint64_t idx) const;
	public:
		void clear(bool real);

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
	private:
		void set_str_in_parse(char* str, uint64_t len);
	public:
		void set_bool(bool x);

		void set_null();

	private:
		void set_type(ValueType type);

	public:
		~Value();

		Value(const Value& other) = delete;

		Value(Value&& other) noexcept;

		Value();

		bool operator==(const Value& other) const;

		bool operator!=(const Value& other) const;

		bool operator<(const Value& other) const;

		Value& operator=(const Value& other) = delete;


		Value& operator=(Value&& other) noexcept;
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
		static Value data_null; // valid is false..
	public:
		static uint64_t npos; // 
	public:
		Structured(const Structured& other) = delete;
		Structured& operator=(const Structured& other) = delete;

		Structured* clone() const;

		explicit Structured();

		virtual ~Structured();

		/// \overload
		uint64_t find(StringView key) const;

		/// \overload
		uint64_t find(const Value& key) const; // find without key`s converting ( \uxxxx )


#if __cpp_lib_char8_t

		uint64_t find(std::u8string_view key) const;

		Value& operator[](std::u8string_view key); // if not exist key, then Value <- is not valid.
		const Value& operator[](std::u8string_view key) const; // if not exist key, then Value <- is not valid.
#endif

		// at vs [] (at <- convert key to key_in_json.) ([] <- do not convert.)
		Value& operator[](StringView key); // if not exist key, then Value <- is not valid.
		const Value& operator[](StringView key) const; // if not exist key, then Value <- is not valid.

		Value& operator[](const Value& key); // if not exist key, then Value <- is not valid.
		const Value& operator[](const Value& key) const; // if not exist key, then Value <- is not valid.

		Value& operator[](uint64_t idx);

		const Value& operator[](uint64_t idx) const;

		PtrWeak<Structured> get_parent() const;

	public:
		bool change_key(const Value& key, const Value& new_key);

		virtual Value& get_value();

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
		virtual Value& get_value_list(uint64_t idx) = 0;
	private:
		virtual Value& get_key_list(uint64_t idx) = 0;
	public:
		virtual const Value& get_value_list(uint64_t idx) const = 0;
		virtual const Value& get_key_list(uint64_t idx) const = 0;

		virtual const Value& get_const_key_list(uint64_t idx) = 0;
		virtual const Value& get_const_key_list(uint64_t idx) const = 0;

		virtual void clear(uint64_t idx) = 0;
		virtual void clear() = 0;

		virtual bool is_virtual() const = 0;

		// todo return type void -> bool.
		virtual bool add_object_element(Value key, Value val) = 0;
		virtual bool add_array_element(Value val) = 0;
		virtual bool add_array(Ptr<Structured> arr) = 0;  // change to Value ? or remove?
		virtual bool add_object(Ptr<Structured> obj) = 0; // change to Value ? or remove?

		virtual bool add_array(Value key, Ptr<Structured> arr) = 0;  // change to Value ? or remove?
		virtual bool add_object(Value key, Ptr<Structured> obj) = 0; // change to Value ? or remove?

		virtual bool assign_value_element(uint64_t idx, Value val) = 0;
		virtual bool assign_key_element(uint64_t idx, Value key) = 0;

		virtual void erase(const Value& key, bool real = false) = 0;
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
			 ValueType type, uint64_t key_token_idx) = 0;

		//

		virtual void add_user_type(ValueType type) = 0; // int type -> enum?


		virtual bool add_user_type(Value key, Ptr<Structured> j) = 0;

		virtual bool add_user_type(Ptr<Structured> j) = 0;
	};

	class Object : public Structured {
	protected:
		std::vector<std::pair<claujson::Value, claujson::Value>> obj_data;
	public:
		using ValueIterator = std::vector<std::pair<claujson::Value, claujson::Value>>::iterator;
	protected:
		//explicit Object(bool valid);
	public:
		friend class Value;

		Structured* clone() const;

		bool chk_key_dup(uint64_t* idx) const;  // chk dupplication of key. only Object, Virtual Object..

		[[nodiscard]]
		static Value Make();

		explicit Object();

		virtual ~Object();

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


		ValueIterator begin();
		ValueIterator end();
		
		virtual void reserve_data_list(uint64_t len);

		virtual bool add_object_element(Value key, Value val);
		virtual bool add_array_element(Value val);
		virtual bool add_array(Ptr<Structured> arr);
		virtual bool add_object(Ptr<Structured> obj);

		virtual bool add_array(Value key, Ptr<Structured> arr);  // change to Value ? or remove?
		virtual bool add_object(Value key, Ptr<Structured> obj); // change to Value ? or remove?


		virtual bool assign_value_element(uint64_t idx, Value val);
		virtual bool assign_key_element(uint64_t idx, Value key);

		virtual void erase(const Value& key, bool real = false);
		virtual void erase(uint64_t idx, bool real = false);

	private:
		virtual void MergeWith(PtrWeak<Structured> j, int start_offset);

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			 ValueType type, uint64_t key_token_idx);

		virtual void add_user_type(ValueType type);

		virtual bool add_user_type(Value key, Ptr<Structured> j);

		virtual bool add_user_type(Ptr<Structured> j);
	};

	class Array : public Structured {
	protected:
		std::vector<Value> arr_vec;
	public:
		using ValueIterator = std::vector<Value>::iterator;
	protected:
		//explicit Array(bool valid);
	public:
		friend class Value;
		friend class PartialJson;

		Structured* clone() const;

		[[nodiscard]]
		static Value Make();

		explicit Array();

		virtual ~Array();

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


		ValueIterator begin();

		ValueIterator end();


		virtual bool add_object_element(Value key, Value val);

		virtual bool add_array_element(Value val);

		virtual bool add_array(Ptr<Structured> arr);

		virtual bool add_object(Ptr<Structured> obj);

		virtual bool add_array(Value key, Ptr<Structured> arr);  // change to Value ? or remove?
		
		virtual bool add_object(Value key, Ptr<Structured> obj); // change to Value ? or remove?

		virtual bool assign_value_element(uint64_t idx, Value val);
		virtual bool assign_key_element(uint64_t idx, Value key);

		virtual void erase(const Value& key, bool real = false); 
		virtual void erase(uint64_t idx, bool real = false);

	private:
		// here only used in parsing.

		virtual void MergeWith(PtrWeak<Structured> j, int start_offset); // start_offset is 0 or 1 (if virtual array or virtual object exists)

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx, 
			char* buf,  uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			 ValueType type, uint64_t key_token_idx);

		virtual void add_user_type(ValueType type);

		virtual bool add_user_type(Value key, Ptr<Structured> j);

		virtual bool add_user_type(Ptr<Structured> j);

	};

}

namespace claujson {

	// parse json file.
	std::pair<bool, uint64_t> parse(const std::string& fileName, Value& ut, uint64_t thr_num);

	// parse json str.
	std::pair<bool, uint64_t> parse_str(StringView str, Value& ut, uint64_t thr_num);

#if __cpp_lib_char8_t
	// C++20~
	std::pair<bool, uint64_t> parse_str(std::u8string_view str, Value& ut, uint64_t thr_num);
#endif

	std::string save_to_str(const Value& global, bool prettty = true);
	
	void save(const std::string& fileName, const Value& global, bool pretty = true);

	void save_parallel(const std::string& fileName, Value& j, uint64_t thr_num, bool pretty = false);

	[[nodiscard]]
	Value diff(const Value& x, const Value& y);

	[[nodiscard]]
	Value patch(const Value& x, const Value& diff);


	void init(int thr_num); // call first, before use claujson..

	void clean(Value& x);

	std::pair<bool, std::string> convert_to_string_in_json(StringView x);

	bool is_valid_string_in_json(StringView x);

#if __cpp_lib_char8_t
	std::pair<bool, std::string> convert_to_string_in_json(std::u8string_view x);

	bool is_valid_string_in_json(std::u8string_view x);
#endif
}
