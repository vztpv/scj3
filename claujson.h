#pragma once

// 64bit.. DO NOT build 32bit! //

#include "claujson_internal.h"
#include "claujson_string.h"

#include "thread_pool.h"

#include "_simdjson.h" // modified simdjson // using simdjson 3.9.1

namespace claujson {

	//only used in Array, and Object, (parent Pointer + is_virtual?)
	class Pointer {
	private: 
		void* ptr = nullptr; 
	public:
		Pointer() {}
		// left_op : 1bit
		// right_op : 2bit
		Pointer(void* ptr, uint8_t left_op, uint8_t right_op) {
			uint64_t value = (uint64_t)ptr;
			if (left_op) {
				value = value | 0x8000000000000000;
			}
			if (right_op) {
				value = value | right_op;
			}
			this->ptr = (void*)value;
		}
	public:
		int left_type() const {
			int64_t value = (int64_t)ptr;
			return value < 0 ? 1 : 0;
		}
		int right_type() const {
			uint64_t value = (uint64_t)ptr;
			return value & 3;
		}
		void* use() {
			uint64_t value = (uint64_t)ptr;
			value = value & 0x7FFFFFFFFFFFFC;
			return (void*)value;
		}
		const void* use() const {
			uint64_t value = (uint64_t)ptr;
			value = value & 0x7FFFFFFFFFFFFC;
			return (void*)value;
		}
	};


	class _Value;
	class Array;
	class Object;
	class PartialJson;
	class StructuredPtr;

	class _Value {
	public:
		static _Value empty_value;
		static const uint64_t npos;
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
					Array* _array_ptr;
					Object* _obj_ptr;
					PartialJson* _pj_ptr;
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

		explicit _Value(Array* x);
		explicit _Value(Object* x);
		explicit _Value(PartialJson* x);
		explicit _Value(StructuredPtr x);

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

		bool is_partial_json() const;

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

		_Value& json_pointerB(const std_vector<_Value>& routeDataVec);
		const _Value& json_pointerB(const std_vector<_Value>& routeVec) const;

		Array* as_array();
		Object* as_object();
		PartialJson* as_partial_json();
		StructuredPtr as_structured_ptr();

		const Array* as_array()const;
		const Object* as_object()const;
		const PartialJson* as_partial_json()const;

		const StructuredPtr as_structured_ptr()const;

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

		void set_none();

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

	public:
		StructuredPtr as_structured();

		bool is_virtual() const;
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

	class _ValueView {
	private:
		claujson::_Value* p = nullptr;
	public:
		_ValueView() {}
		_ValueView(const claujson::_Value& x) {
			p = const_cast<claujson::_Value*>(&x);
		}

		claujson::_Value* operator->() {
			return p;
		}
		const claujson::_Value* operator->() const {
			return p;
		}

		claujson::_Value& operator*() {
			return *p;
		}
		const claujson::_Value& operator*() const {
			return *p;
		}

		bool operator<(_ValueView other) const {
			return (*p) < (*other.p);
		}
	};


	class parser;

	class Document {
	public:
		friend class parser;
	private:
		_Value x;

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
	class StructuredPtr {
	public:
		friend class LoadData2;
		friend class PartialJson;
		friend class _Value;
		friend class Array;
		friend class Object;

		static const uint64_t npos;
		static _Value empty_value;
		
	private:
		union {
			Array* arr = nullptr;
			Object* obj;
			PartialJson* pj;
		};
		uint32_t type = 0;
	public:
		StructuredPtr(_Value& x);

		StructuredPtr(const _Value& x);

		StructuredPtr(const StructuredPtr& other) {
			arr = other.arr;
			type = other.type;
		}

		StructuredPtr() {
			arr = nullptr;
			type = 0;
		}

		StructuredPtr(Array* arr, Object* obj, PartialJson* pj)
		{
			if (arr) {
				this->arr = arr;
				type = 1;
			}
			else if (obj) {
				this->obj = obj;
				type = 2;
			}
			else if (pj) {
				this->pj = pj;
				type = 3;
			}
		}

		 StructuredPtr(std::nullptr_t) : arr(nullptr), type(0) {
			 //
		 }
		 StructuredPtr(Array* arr) : arr(arr), type(1)
		{

		}
		 StructuredPtr(Object* obj) : obj(obj), type(2)
		{

		}

		 StructuredPtr(PartialJson* pj) : pj(pj), type(3)
		{
			//
		}
		 StructuredPtr(const Array* arr) : arr(const_cast<Array*>(arr)), type(1)
		{
			//
		}

		 StructuredPtr(const Object* obj) : obj(const_cast<Object*>(obj)), type(2)
		{
			//
		}

		 StructuredPtr(const PartialJson* pj) : pj(const_cast<PartialJson*>(pj)), type(3)
		{
			//
		}


		uint64_t get_data_size() const;
		uint64_t size() const;
		
		bool empty() const;

		_Value& get_value_list(uint64_t idx);
		_Value& get_key_list(uint64_t idx);

		const _Value& get_value_list(uint64_t idx)const;
		const _Value& get_key_list(uint64_t idx)const;

		bool insert(uint64_t idx, Value val); // from Array

		const _Value& get_const_key_list(uint64_t idx);
		const _Value& get_const_key_list(uint64_t idx) const;

		bool change_key(const _Value& key, Value&& next_key);
		bool change_key(uint64_t idx, Value&& next_key);

		explicit operator bool() const {
			return arr;
		}

		bool operator==(const StructuredPtr& other) const {
			return arr == other.arr && type == other.type;
		}


		void null_parent();

		bool is_array() const {
			return type == 1;
		}
		bool is_object() const {
			return type == 2;
		}
		bool is_partial_json() const {
			return type == 3;
		}
		bool is_nullptr() const {
			return type == 0;
		}

		bool is_user_type() const {
			return is_array() || is_object();
		}

		bool chk_key_dup(uint64_t* idx) const;
		uint64_t find_by_key(const _Value& key) const; // find without key`s converting ( \uxxxx )

		_Value& operator[](const _Value& key); // if not exist key, then _Value <- is not valid.
		const _Value& operator[](const _Value& key) const; // if not exist key, then _Value <- is not valid.

		bool add_array_element(Value v);
		bool add_object_element(Value key, Value v);

		uint64_t find_by_value(const _Value& value, uint64_t start = 0) const; // find without key`s converting ( \uxxxx )

		_Value& operator[](uint64_t idx);

		const _Value& operator[](uint64_t idx) const;

		// pj`s parent is nullptr.
		StructuredPtr get_parent();

		void erase(uint64_t idx, bool real = false);
		void erase(const _Value& key, bool real = false);

		bool operator==(std::nullptr_t) {
			return !arr;
		}
		bool operator==(StructuredPtr p) {
			return arr == p.arr && type == p.type;
		}
		bool operator!=(std::nullptr_t) {
			return arr;
		}
		void operator=(std::nullptr_t) {
			arr = nullptr;
			type = 0;
		}

		void operator=(const StructuredPtr& other) {
			arr = other.arr;
			type = other.type;
		}

		void Delete();
		void clear();
		void clear(uint64_t idx); // clear child[idx] ?
		
		bool assign_value(uint64_t idx, Value val);


		void MergeWith(StructuredPtr j, int start_offset);

		void reserve_data_list(uint64_t sz);

	private:
		// need rename param....!
		void add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint64_t key_token_idx, uint64_t val_token_idx);

		void add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
			char* buf, uint64_t val_token_idx);

		void add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
			_ValueType type, uint64_t key_token_idx
		);

		//
		void add_user_type(_ValueType type
		); // int type -> enum?
	public:

		bool is_virtual() const;

		// private: + friend?
	private:
		void set_parent(StructuredPtr p);
	};

	class LoadData;
	class LoadData2;

	class Array;
	class Object;
	class PartialJson; // rename?
}

#include "claujson_array.h"
#include "claujson_object.h"
#include "claujson_partialjson.h"

namespace claujson {

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

#define claujson_inline _simdjson_inline


#define ERROR(msg) \
	do { \
		throw msg; \
		/* error.make(__LINE__, StringView(msg)); */ \
	} while (false) 

namespace claujson {
	claujson::_Value& Convert(claujson::_Value& data, uint64_t buf_idx, uint64_t next_buf_idx, bool key,
			char* buf, uint64_t token_idx, bool& err);
}
