#pragma once

#include "claujson.h"

namespace claujson {

	class Array {
	protected:
		std_vector<_Value> arr_vec;
		//StructuredPtr parent;
		Pointer parent;

		static _Value data_null; // valid is false..
		static const uint64_t npos;
	public:
		using _ValueIterator = std_vector<_Value>::iterator;
		using _ConstValueIterator = std_vector<_Value>::const_iterator;
	protected:
		//explicit Array(bool valid);
	public:
		friend class _Value;
		friend class PartialJson;
		friend class StructuredPtr;

		Array* clone() const;

		void set_parent(StructuredPtr p);

		[[nodiscard]]
		static _Value Make();

		[[nodiscard]]
		static _Value MakeVirtual();

		explicit Array();

		~Array();

		bool is_object() const;
		bool is_array() const;

		uint64_t find(const _Value& value, uint64_t start = 0) const; // find without key`s converting ( \uxxxx )

		_Value& operator[](uint64_t idx);

		const _Value& operator[](uint64_t idx) const;

		StructuredPtr get_parent() const;

	public:

		void reserve_data_list(uint64_t len); // if object, reserve key_list and value_list, if array, reserve value_list.

		// for valid with object or array or root.
		uint64_t size() const {
			return get_data_size();
		}
		bool empty() const {
			return 0 == get_data_size();
		}

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

		bool add_element(Value val);
		bool assign_element(uint64_t idx, Value val);

		bool insert(uint64_t idx, Value val);

		void erase(const _Value& key, bool real = false);
		void erase(uint64_t idx, bool real = false);

		void null_parent();
	private:
		// here only used in parsing.

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
