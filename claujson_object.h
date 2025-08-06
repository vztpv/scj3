#pragma once
#include "claujson_internal.h"

namespace claujson {
	class Object {
	protected:
		std_vector<Pair<claujson::_Value, claujson::_Value>> obj_data;
		Pointer parent;

	public:
		static _Value data_null; // valid is false..
		static const uint64_t npos;
	public:
		using _ValueIterator = std_vector<Pair<claujson::_Value, claujson::_Value>>::iterator;
		using _ConstValueIterator = std_vector<Pair<claujson::_Value, claujson::_Value>>::const_iterator;
	protected:
		//explicit Object(bool valid);
	public:
		friend class _Value;
		friend class StructuredPtr;

		Object* clone() const;

		void set_parent(StructuredPtr);

		uint64_t find(const _Value& key) const; // find without key`s converting ( \uxxxx )

		_Value& operator[](const _Value& key); // if not exist key, then _Value <- is not valid.
		const _Value& operator[](const _Value& key) const; // if not exist key, then _Value <- is not valid.

		_Value& operator[](uint64_t idx);

		const _Value& operator[](uint64_t idx) const;

		StructuredPtr get_parent() const;
		void null_parent();
	public:
		bool change_key(const _Value& key, Value new_key);
		bool change_key(uint64_t idx, Value new_key);


		// for valid with object or array or root.
		uint64_t size() const {
			return get_data_size();
		}
		bool empty() const {
			return 0 == get_data_size();
		}


		bool chk_key_dup(uint64_t* idx) const;  // chk duplication of key. only Object, Virtual Object..

		[[nodiscard]]
		static _Value Make();

		[[nodiscard]]
		static _Value MakeVirtual();

			explicit Object();

		 ~Object();

		 bool is_object() const;
		 bool is_array() const;

		 uint64_t get_data_size() const;

		 _Value& get_value_list(uint64_t idx);
	private:
		 _Value& get_key_list(uint64_t idx);
	public:

		 const _Value& get_value_list(uint64_t idx) const;

		 const _Value& get_key_list(uint64_t idx) const;

		 const _Value& get_const_key_list(uint64_t idx);

		 const _Value& get_const_key_list(uint64_t idx) const;

		 void clear(uint64_t idx);

		 bool is_virtual() const;

		 void clear();


		_ValueIterator begin();
		_ValueIterator end();


		_ConstValueIterator begin() const;
		_ConstValueIterator end() const;

		 void reserve_data_list(uint64_t len);

		 bool add_element(Value key, Value val);

		 bool assign_value_element(uint64_t idx, Value val);
		// bool assign_key_element(uint64_t idx, Value key);

		 void erase(const _Value& key, bool real = false);
		 void erase(uint64_t idx, bool real = false);


	private:
		 void MergeWith(Array* j, int start_offset);
		 void MergeWith(Object* j, int start_offset);
		 void MergeWith(PartialJson* j, int start_offset);

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

	};

}
