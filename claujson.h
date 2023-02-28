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
#include <iomanip>

#include <string_view>


namespace claujson {

	class Log {
	public:
		class Info {
		public:
			friend std::ostream& operator<<(std::ostream& stream, const Info&) {
				stream << "[INFO]";
				return stream;
			}
		};
		class Warning { 
		public:
			friend std::ostream& operator<<(std::ostream& stream, const Warning&) {
				stream << "[WARN]";
				return stream;
			}
		};

		enum class Option { CONSOLE, FILE, CONSOLE_AND_FILE, NO_PRINT };
		class Option2 {
		public:
			static const int INFO = 1;
			static const int WARN = 2;
		};
	private:
		Option opt;
		int opt2;
		int state;
		std::string fileName;
	public:

		Log() : state(0), opt(Option::CONSOLE), opt2(Option2::INFO | Option2::WARN), fileName("log.txt") {
			//
		}

		template <class T>
		Log& operator<<(const T& val) {
			if (opt == Option::CONSOLE || opt == Option::CONSOLE_AND_FILE) {

				int count = 0;
				count += opt2 & Option2::INFO;
				count += opt2 & Option2::WARN;

				if (count) {
					std::cout << val;
				}
			}

			if (opt == Option::FILE || opt == Option::CONSOLE_AND_FILE) {
				std::ofstream outFile;
				outFile.open(fileName, std::ios::app);
				if (outFile) {
					int count = 0;
					count += opt2 & Option2::INFO;
					count += opt2 & Option2::WARN;

					if (count) {
						outFile << val;
					}
					outFile.close();
				}
			}

			return *this;
		}

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

	/*
	class Error {
	private:
		int state = 0;
		size_t line;
		char _msg[1024 + 1];
		size_t msg_size;
	public:
		Error() : line(0), msg_size(0), _msg("") { }

		std::string_view msg() const {
			return std::string_view(_msg, msg_size);
		}

		void make(size_t line, std::string_view msg) {
			make(line, msg.data(), msg.size());
		}

		void make(size_t line, const char* msg, size_t msg_size) {
			this->state = 1;
			this->line = line;

			if (msg_size > 1024) {
				msg_size = 1024;
			}

			strncpy_s(this->_msg, msg, msg_size);

			this->_msg[msg_size] = '\0';
			this->msg_size = msg_size;
		}

		bool has_error() const {
			return state == 1;
		}

		void clean() {
			state = 0;
		}
	};
	*/

	static Log::Info info;
	static Log::Warning warn;
	inline Log log; // no static..

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

	enum ValueType : int8_t {
		NONE = 1,
		ARRAY, // ARRAY_OBJECT -> ARRAY, OBJECT
		OBJECT, 
		INT, UINT,
		FLOAT,
		BOOL,
		NULL_,
		STRING,
		ERROR // private class?
	};

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

		friend claujson::Value& Convert(Value& data, uint64_t idx, uint64_t idx2, bool key,
			char* buf,  uint64_t id, bool& err);
	private:
		union { // 64bit.. DO NOT build 32bit! //
			int64_t _int_val = 0;
			uint64_t _uint_val;
			double _float_val;
			std::string* _str_val;
			Structured* _ptr_val; // ARRAY_OR_OBJECT -> todo : Array, or Object?

			// cf) Array* _arr_val; , Object* _obj_val; 

			bool _bool_val;
		};

		ValueType _type = ValueType::NONE; 
		bool _valid = true;

	public:

		Value clone() const;

		explicit operator bool() const;

		explicit Value(Structured* x);
		explicit Value(int x);

		explicit Value(unsigned int x);

		explicit Value(int64_t x);
		explicit Value(uint64_t x);
		explicit Value(double x);

		explicit Value(std::string_view x); // C++17

#if __cplusplus >= 202002L
		// C++20~
		explicit Value(std::u8string_view x);

#endif
		
		explicit Value(Value*) = delete;

		explicit Value(bool x);
		explicit Value(nullptr_t x);

		explicit Value(nullptr_t, bool valid);

	public:
		ValueType type() const;

		bool is_valid() const;

		bool is_null() const;

		bool is_primitive() const; // int, uint, float, bool(true, false), string, null

		bool is_structured() const; // array or object (or used in inner )

		bool is_int() const;

		bool is_uint() const;

		bool is_float() const;

		bool is_bool() const;

		bool is_str() const;

		bool is_ptr() const; // check is_structured()

		int64_t int_val() const;

		uint64_t uint_val() const;

		double float_val() const;

		int64_t& int_val();

		uint64_t& uint_val();

		double& float_val();

		bool bool_val() const;

		Structured* ptr_val() const;

		Value& json_pointer(std::string_view route);

		const Value& json_pointer(std::string_view route) const;


		static bool json_pointerA(std::string_view route, std::vector<Value>& vec);

		Value& json_pointerB(const std::vector<Value>& routeDataVec);

		Array& as_array();
		Object& as_object();
		Structured* as_structured_ptr();

		const Array& as_array()const;
		const Object& as_object()const;
		const Structured* as_structured_ptr()const;
	public:
		void clear();

		std::string& str_val();

		const std::string& str_val() const;

		void set_ptr(Structured* x);
		void set_int(long long x);

		void set_uint(unsigned long long x);

		void set_float(double x);

		bool set_str(const char* str, size_t len);
	private:
		void set_str_in_parse(const char* str, size_t len);
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
		Value key;
		PtrWeak<Structured> parent = nullptr;
		bool valid = true; //
	protected:
		static inline Value data_null{ nullptr, false }; // valid is false..
	public:
		inline static size_t npos = -1; // 

		bool is_valid() const;
	protected:
		explicit Structured(bool valid);
	public:
		Structured(const Structured& other) = delete;
		Structured& operator=(const Structured& other) = delete;

		Structured* clone() const;

		explicit Structured();

		virtual ~Structured();

		const Value& at(std::string_view key) const;

		Value& at(std::string_view key);

		size_t find(std::string_view key) const;


		Value& operator[](size_t idx);

		const Value& operator[](size_t idx) const;

		bool has_key() const;

		PtrWeak<Structured> get_parent() const;

		const Value& get_key() const;
	protected:
		bool set_key(Value key);
	public:
		bool change_key(const Value& key, const Value& new_key);

		virtual Value& get_value();

		virtual void reserve_data_list(size_t len) = 0; // if object, reserve key_list and value_list, if array, reserve value_list.

		virtual bool is_object() const = 0;
		virtual bool is_array() const = 0;
		virtual bool is_partial_json() const;

		bool is_user_type() const;

		// for valid with object or array or root.
		virtual size_t get_data_size() const = 0; // data_size == key_list_size (if object), and data_size == value_list_size.
		virtual Value& get_value_list(size_t idx) = 0;
		virtual Value& get_key_list(size_t idx) = 0;

		virtual const Value& get_value_list(size_t idx) const = 0;
		virtual const Value& get_key_list(size_t idx) const = 0;

		virtual void clear(size_t idx) = 0;
		virtual void clear() = 0;

		virtual bool is_virtual() const = 0;

		// todo return type void -> bool.
		virtual bool add_object_element(Value key, Value val) = 0;
		virtual bool add_array_element(Value val) = 0;
		virtual bool add_array(Ptr<Structured> arr) = 0; // 
		virtual bool add_object(Ptr<Structured> obj) = 0;

		virtual bool insert_array_element(size_t idx, Value val) = 0;

		virtual void erase(std::string_view key, bool real = false) = 0;
		virtual void erase(size_t idx, bool real = false) = 0;

	private:
		void set_parent(PtrWeak<Structured> j);


	private:
		virtual void MergeWith(PtrWeak<Structured> j, int start_offset) = 0; // start_offset is 0 or 1.

		virtual void Link(Ptr<Structured> j) = 0;

		// need rename param....!
		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx) = 0;

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t val_token_idx) = 0;

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			 int ut_type, uint64_t key_token_idx) = 0;

		//

		virtual void add_user_type(int type) = 0; // int type -> enum?

		virtual bool add_user_type(Ptr<Structured> j) = 0;
	};

	class Object : public Structured {
	protected:
		std::vector<Value> obj_key_vec;
		std::vector<Value> obj_val_vec;
	protected:
		explicit Object(bool valid);
	public:
		friend class Value;

		Structured* clone() const;

		bool chk_key_dup(size_t* idx) const;  // chk dupplication of key. only Object, Virtual Object..

		[[nodiscard]]
		static Value Make();

		explicit Object();

		virtual ~Object();

		virtual bool is_object() const;
		virtual bool is_array() const;

		virtual size_t get_data_size() const;

		virtual Value& get_value_list(size_t idx);

		virtual Value& get_key_list(size_t idx);


		virtual const Value& get_value_list(size_t idx) const;

		virtual const Value& get_key_list(size_t idx) const;


		virtual void clear(size_t idx);

		virtual bool is_virtual() const;

		virtual void clear();


		virtual void reserve_data_list(size_t len);


		virtual bool add_object_element(Value key, Value val);
		virtual bool add_array_element(Value val);
		virtual bool add_array(Ptr<Structured> arr);
		virtual bool add_object(Ptr<Structured> obj);

		virtual bool insert_array_element(size_t idx, Value val);

		virtual void erase(std::string_view key, bool real = false);

		virtual void erase(size_t idx, bool real = false);



	private:
		virtual void MergeWith(PtrWeak<Structured> j, int start_offset);

		virtual void Link(Ptr<Structured> j);

		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			 int ut_type, uint64_t key_token_idx);

		virtual void add_user_type(int type);

		virtual bool add_user_type(Ptr<Structured> j);
	};

	class Array : public Structured {
	protected:
		std::vector<Value> arr_vec;
	protected:
		explicit Array(bool valid);
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

		virtual size_t get_data_size() const;

		virtual Value& get_value_list(size_t idx);

		virtual Value& get_key_list(size_t idx);

		virtual const Value& get_value_list(size_t idx) const;

		virtual const Value& get_key_list(size_t idx) const;

		virtual void clear(size_t idx);

		virtual bool is_virtual() const;

		virtual void clear();

		virtual void reserve_data_list(size_t len);


		std::vector<Value>::iterator begin();

		std::vector<Value>::iterator end();


		virtual bool add_object_element(Value key, Value val);

		virtual bool add_array_element(Value val);

		virtual bool add_array(Ptr<Structured> arr);

		virtual bool add_object(Ptr<Structured> obj);

		virtual bool insert_array_element(size_t idx, Value val);

		virtual void erase(std::string_view key, bool real = false);

		virtual void erase(size_t idx, bool real = false);

	private:


		virtual void MergeWith(PtrWeak<Structured> j, int start_offset);


		virtual void Link(Ptr<Structured> j);


		virtual void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf,  uint64_t key_token_idx, uint64_t val_token_idx);

		virtual void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx, 
			char* buf,  uint64_t val_token_idx);

		virtual void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			 int ut_type, uint64_t key_token_idx);

		virtual void add_user_type(int type);

		virtual bool add_user_type(Ptr<Structured> j);

	};

}

namespace claujson {

	// parse json file.
	std::pair<bool, size_t> parse(const std::string& fileName, Value& ut, size_t thr_num, bool use_all_function = false);

	// parse json str.
	std::pair<bool, size_t> parse_str(std::string_view str, Value& ut, size_t thr_num, bool use_all_function = false);

#if __cplusplus >= 202002L
	// C++20~
	std::pair<bool, size_t> parse_str(std::u8string_view str, Value& ut, size_t thr_num, bool use_all_function = false);
#endif

	std::string save_to_str(const Value& global);
	
	void save(const std::string& fileName, const Value& global);

	void save_parallel(const std::string& fileName, Value& j, size_t thr_num);

	[[nodiscard]]
	Value diff(const Value& x, const Value& y);

	[[nodiscard]]
	Value patch(const Value& x, const Value& diff);


	void init(int thr_num); // call first, before use claujson..

	void clean(Value& x);
}

