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

	class Data;
	class Json;
	class Array;
	class Object;

	claujson::Data& Convert(::claujson::Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
		char* buf, uint8_t* string_buf, uint64_t id, bool& err);

	enum class DataType {
		NONE, ARRAY_OR_OBJECT, INT, UINT, FLOAT, BOOL, NULL_, STRING
	};

	class Data {
		friend std::ostream& operator<<(std::ostream& stream, const Data& data);

		friend claujson::Data& Convert(Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
			char* buf, uint8_t* string_buf, uint64_t id, bool& err);
	private:
		union {
			int64_t _int_val = 0;
			uint64_t _uint_val;
			double _float_val;
			std::string* _str_val;
			Json* _ptr_val; // Array or Object , ...
			bool _bool_val;
		};

		bool valid = true;
		DataType _type = DataType::NONE;

	public:

		explicit operator bool() const;

		explicit Data(Json* x);
		explicit Data(int x);

		explicit Data(unsigned int x);

		explicit Data(int64_t x);
		explicit Data(uint64_t x);
		explicit Data(double x);
		explicit Data(std::string_view x);
		//explicit Data(const char* x) {
		//	set_str(x, strlen(x));
		//}
		// C++20~
		//explicit Data(const char8_t* x) {
		//	set_str((const char*)x, strlen((const char*)x));
		//}
		explicit Data(bool x);
		explicit Data(nullptr_t x);

		explicit Data(nullptr_t, bool valid);

	public:
		DataType type() const;

		bool is_valid() const;

		bool is_int() const;

		bool is_uint() const;

		bool is_float() const;

		bool is_bool() const;

		bool is_str() const;

		bool is_ptr() const;

		int64_t int_val() const;

		uint64_t uint_val() const;

		double float_val() const;

		int64_t& int_val();

		uint64_t& uint_val();

		double& float_val();

		bool bool_val() const;

		void* ptr_val() const;

		// todo - rename, and add  as_ref, as_ptr ?

		Json& as_json();
		Array& as_array();
		Object& as_object();
		Json* as_json_ptr();

		const Json& as_json()const;
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

		bool operator<(const Data& other) const;

		Data& operator=(const Data& other);


		Data& operator=(Data&& other) noexcept;
	};
}

namespace claujson {
	// todo 
	//- add bool is_key ...
	INLINE claujson::Data& Convert(::claujson::Data& data, uint64_t idx, uint64_t idx2, uint64_t len, bool key,
		char* buf, uint8_t* string_buf, uint64_t id, bool& err);
}


namespace claujson {
	class LoadData;
	class LoadData2;

	class Array;
	class Object;
	class Root;

	class Json {
		friend class LoadData2;
		friend class LoadData;
		friend class Data;
		friend class Array;
		friend class Object;
		friend class Root;
	protected:
		Data key;
		PtrWeak<Json> parent = nullptr;
		bool valid = true; //
	protected:
		// check...
		static inline Data data_null{ nullptr, false }; // valid is false..
	public:

		bool is_valid() const;
	protected:
		explicit Json(bool valid);
	public:
		explicit Json();

		Json(const Json&) = delete;
		Json& operator=(const Json&) = delete;

		virtual ~Json();
		


		const Data& at(std::string_view key) const;

		Data& at(std::string_view key);

		size_t find(std::string_view key);


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
		virtual bool is_root() const;
		virtual bool is_element() const = 0;
		bool is_user_type() const;

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
		static Data Make();

		explicit Object();

		virtual ~Object();

		virtual bool is_object() const;
		virtual bool is_array() const;
		virtual bool is_element() const;
		virtual size_t get_data_size() const;

		virtual Data& get_data_list(size_t idx);

		virtual Data& get_key_list(size_t idx);


		virtual const Data& get_data_list(size_t idx) const;
		
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

	class Array : public Json {
		friend class Data;
	protected:
		std::vector<Data> arr_vec;
	protected:
		explicit Array(bool valid);
	public:

		static Data Make();

		explicit Array();

		virtual ~Array();

		virtual bool is_object() const;
		virtual bool is_array() const;
		virtual bool is_element() const;
		virtual size_t get_data_size() const;
		
		virtual Data& get_data_list(size_t idx);

		virtual Data& get_key_list(size_t idx);

		virtual const Data& get_data_list(size_t idx) const;

		virtual const Data& get_key_list(size_t idx) const;

		virtual void clear(size_t idx);
		
		virtual bool is_virtual() const;

		virtual void clear();

		virtual void reserve_data_list(size_t len);


		std::vector<Data>::iterator begin();

		std::vector<Data>::iterator end();
		

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

		virtual Data& get_data_list(size_t idx);


		virtual Data& get_key_list(size_t idx);


		virtual const Data& get_data_list(size_t idx) const;


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

}


namespace claujson {

	class StrStream;

	class LoadData //
	{
	public:
		//                            todo - change Json* ut to Data& data ?
		static void _save(StrStream& stream, Data data, std::vector<Json*>& chk_list, const int depth);
		static void _save(StrStream& stream, Data data, const int depth);

		// todo... just Data has one element 
		static void save(const std::string& fileName, Data& global, bool hint);

		static void save(std::ostream& stream, Data& data);
		static void save_(StrStream& stream, Data global, Json* temp, bool hint);


		static void save_parallel(const std::string& fileName, Data j, size_t thr_num);

	};



	std::pair<bool, size_t> Parse(const std::string& fileName, int thr_num, Data& ut);

}