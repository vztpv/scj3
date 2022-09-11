#pragma once


#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include <iomanip>

#define INLINE inline

#include <string_view>

namespace claujson {

	template <class T>
	using PtrWeak = T*;

	template <class T>
	using Ptr = std::unique_ptr<T>; 
	// Ptr - use std::move

	class Data;
	class Json;
	class Array;
	class Object;

	claujson::Data& Convert(::claujson::Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
		char* buf, uint8_t* string_buf, uint64_t id, bool& err);

	enum class DataType {
		NONE, 
		ARRAY_OR_OBJECT, // todo - ARRAY, OBJECT ?
		INT, UINT, 
		FLOAT, 
		BOOL, 
		NULL_, 
		STRING
	};

	class Data {

	public:
		// todo - check type of Data....
		// using INT_t = int64_t; 
		// using UINT_t = uint64_t;
		// using FlOAT_t = double;
		// using STR_t = std::string;
		// using BOOL_t = bool;

	public:
		friend std::ostream& operator<<(std::ostream& stream, const Data& data);

		friend claujson::Data& Convert(Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
			char* buf, uint8_t* string_buf, uint64_t id, bool& err);
	private:
		union {
			int64_t _int_val = 0;
			uint64_t _uint_val;
			double _float_val;
			std::string* _str_val;
			Json* _ptr_val; // ARRAY_OR_OBJECT -> todo : Array, or Object?
			
			// cf) Array* _arr_val; , Object* _obj_val; 
			
			bool _bool_val;
		};

		bool valid = true;
		DataType _type = DataType::NONE;

	public:

		Data clone() const;

		explicit operator bool() const;

		explicit Data(Json* x);
		explicit Data(int x);

		explicit Data(unsigned int x);

		explicit Data(int64_t x);
		explicit Data(uint64_t x);
		explicit Data(double x);
		
		explicit Data(std::string_view x); // C++17
		
		// C++20~
		// todo - 
		//explicit Data(std::u8string_view x) {
		//	//
		//}

		
		
		explicit Data(bool x);
		explicit Data(nullptr_t x);

		explicit Data(nullptr_t, bool valid);

	public:
		DataType type() const;

		bool is_valid() const;

		bool is_primitive() const;

		bool is_structured() const;

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

		void* ptr_val() const;

		Data& json_pointer(std::string_view route); 
		
		const Data& json_pointer(std::string_view route) const;


		static bool json_pointerA(std::string_view route, std::vector<Data>& vec);

		Data& json_pointerB(const std::vector<Data>& routeDataVec);

		// todo - rename, and add  as_ref, as_ptr ?
		Array& as_array();
		Object& as_object();
		Json* as_json_ptr();

		const Array& as_array()const;
		const Object& as_object()const;
		const Json* as_json_ptr()const;
	public:
		void clear();

		std::string& str_val();

		const std::string& str_val() const;

		void set_ptr(Json* x);
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
		void set_type(DataType type);

	public:
		virtual ~Data();

		Data(const Data& other);

		Data(Data&& other) noexcept;

		Data();

		bool operator==(const Data& other) const;

		bool operator!=(const Data& other) const;

		bool operator<(const Data& other) const;

		Data& operator=(const Data& other);


		Data& operator=(Data&& other) noexcept;
	};
}

namespace claujson {

	INLINE claujson::Data& Convert(::claujson::Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
		char* buf, uint8_t* string_buf, uint64_t id, bool& err);
}


namespace claujson {
	class LoadData;
	class LoadData2;

	class Array;
	class Object;
	class PartialJson;

	class Json {
		friend class LoadData2;
		friend class LoadData;
		friend class Data;
		friend class Array;
		friend class Object;
		friend class PartialJson;
	protected:
		Data key;
		PtrWeak<Json> parent = nullptr;
		bool valid = true; //
	protected:
		// check...  
		static inline Data data_null{ nullptr, false }; // valid is false..
	public:
		inline static size_t npos = -1; // 

		bool is_valid() const;
	protected:
		explicit Json(bool valid);
	public:
		Json(const Json& other) = delete;
		Json& operator=(const Json& other) = delete;

		Json* clone() const;

		explicit Json();

		virtual ~Json();
		
		const Data& at(std::string_view key) const;

		Data& at(std::string_view key);

		size_t find(std::string_view key) const;


		Data& operator[](size_t idx);

		const Data& operator[](size_t idx) const;

		bool has_key() const;

		PtrWeak<Json> get_parent() const;


		const Data& get_key() const;
	protected:
		bool set_key(Data key);
	public:
		bool change_key(const Data& key, const Data& new_key);
		
		virtual Data& get_value();

		virtual void reserve_data_list(size_t len) = 0;

		virtual bool is_object() const = 0;
		virtual bool is_array() const = 0;
		virtual bool is_partial_json() const;

		bool is_user_type() const;

		// for valid with object or array or root.
		virtual size_t get_data_size() const = 0;
		virtual Data& get_value_list(size_t idx) = 0;
		virtual Data& get_key_list(size_t idx) = 0;

		virtual const Data& get_value_list(size_t idx) const = 0;
		virtual const Data& get_key_list(size_t idx) const = 0;

		virtual void clear(size_t idx) = 0;
		virtual void clear() = 0;

		virtual bool is_virtual() const = 0;

		// todo return type void -> bool.
		virtual bool add_object_element(Data key, Data val) = 0;
		virtual bool add_array_element(Data val) = 0;
		virtual bool add_array(Ptr<Json> arr) = 0; // 
		virtual bool add_object(Ptr<Json> obj) = 0;

		virtual bool insert_array_element(size_t idx, Data val) = 0;

		virtual void erase(std::string_view key, bool real = false) = 0;
		virtual void erase(size_t idx, bool real = false) = 0;

	private:	
		void set_parent(PtrWeak<Json> j);


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
		friend class Data;
	protected:
		std::vector<Data> obj_key_vec;
		std::vector<Data> obj_val_vec;
	protected:
		explicit Object(bool valid);
	public:

		Json* clone() const;

		bool chk_key_dup(size_t* idx) const;  // chk dupplication of key.

		[[nodiscard]]
		static Data Make();

		explicit Object();

		virtual ~Object();

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

	class Array : public Json {
		friend class Data;
	protected:
		std::vector<Data> arr_vec;
	protected:
		explicit Array(bool valid);
	public:

		Json* clone() const;

		[[nodiscard]]
		static Data Make();

		explicit Array();

		virtual ~Array();

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


		std::vector<Data>::iterator begin();

		std::vector<Data>::iterator end();
		

		virtual bool add_object_element(Data key, Data val);
		
		virtual bool add_array_element(Data val);

		virtual bool add_array(Ptr<Json> arr);

		virtual bool add_object(Ptr<Json> obj);

		virtual bool insert_array_element(size_t idx, Data val);

		virtual void erase(std::string_view key, bool real = false);

		virtual void erase(size_t idx, bool real = false);

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

}


namespace claujson {

	void save(const std::string& fileName, const Data& global);

	void save_parallel(const std::string& fileName, Data j, size_t thr_num);
	
	// parse json file.
	std::pair<bool, size_t> Parse(const std::string& fileName, Data& ut, int thr_num);
	// parse json str.
	std::pair<bool, size_t> ParseStr(std::string_view str, Data& ut, int thr_num);

	// todo - c++20~
	//inline std::pair<bool, size_t> ParseStr(std::u8string_view str, int thr_num, Data& ut) {
	//	//
	//	return { false , 0 };
	//}
	
	[[nodiscard]]
	Data diff(const Data& x, const Data& y);

	[[nodiscard]]
	Data patch(const Data& x, const Data& diff);

	void clean(Data& x);
}

