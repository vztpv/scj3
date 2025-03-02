#include "claujson_partialjson.h"

namespace claujson {
	extern Log log;

	_Value PartialJson::data_null{ nullptr, false }; // valid is false..
	const uint64_t PartialJson::npos = -1; // 

	PartialJson::~PartialJson() {
		for (auto& x : obj_data) {
			if (x.second.is_array()) {
				delete (x.second.as_array());
			}
			if (x.second.is_object()) {
				delete (x.second.as_object());
			}
		}

		for (auto& x : arr_vec) {
			if (x.is_array()) {
				delete (x.as_array());
			}
			if (x.is_object()) {
				delete (x.as_object());
			}
		}

		if (virtualJson.is_structured()) {
			clean(virtualJson);
		}
	}

	PartialJson::PartialJson() : virtualJson(), arr_vec(), obj_data() {

	}

	bool PartialJson::is_partial_json() const { return true; }

	bool PartialJson::is_object() const {
		return false;
	}
	bool PartialJson::is_array() const {
		return false;
	}

	uint64_t PartialJson::get_data_size() const {
		int count = 0;

		if (virtualJson.is_structured()) {
			count = 1;
		}

		return arr_vec.size() + obj_data.size() + count;
	}

	_Value& PartialJson::get_value_list(uint64_t idx) {
		if (virtualJson.is_structured() && idx == 0) {
			return virtualJson;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return arr_vec[idx];
		}
		else {
			return obj_data[idx].second;
		}
	}


	_Value& PartialJson::get_key_list(uint64_t idx) {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_data[idx].first;
		}
	}

	const _Value& PartialJson::get_const_key_list(uint64_t idx) {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_data[idx].first;
		}
	}
	const _Value& PartialJson::get_const_key_list(uint64_t idx) const {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_data[idx].first;
		}
	}
	const _Value& PartialJson::get_value_list(uint64_t idx) const {
		if (virtualJson.is_structured() && idx == 0) {
			return virtualJson;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return arr_vec[idx];
		}
		else {
			return obj_data[idx].second;
		}
	}


	const _Value& PartialJson::get_key_list(uint64_t idx) const {
		if (virtualJson.is_structured() && idx == 0) {
			return data_null;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}

		if (!arr_vec.empty()) {
			return data_null;
		}
		else {
			return obj_data[idx].first;
		}
	}


	void PartialJson::clear(uint64_t idx) { // use carefully..
		if (virtualJson.is_structured() && idx == 0) {
			virtualJson.clear(false);
			return;
		}
		if (virtualJson.is_structured()) {
			--idx;
		}
		if (!arr_vec.empty()) {
			arr_vec[idx].clear(false);
		}
		else {
			obj_data[idx].first.clear(false);
			obj_data[idx].second.clear(false);
		}
	}

	bool PartialJson::is_virtual() const {
		return false;
	}

	void PartialJson::clear() {
		arr_vec.clear();
		obj_data.clear();
		virtualJson.clear(false);
	}

	void PartialJson::reserve_data_list(uint64_t len) {
		if (!arr_vec.empty()) {
			arr_vec.reserve(len);
		}
		if (!obj_data.empty()) {
			obj_data.reserve(len);
		}
	}
	void PartialJson::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t key_token_idx, uint64_t val_token_idx) {

			{
				_Value temp;
				_Value temp2;

				bool e = false;

				claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, key_token_idx, e);

				if (e) {
					ERROR("Error in add_item_type");
				}

				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, val_token_idx, e);

				if (e) {
					ERROR("Error in add_item_type");
				}

				if (temp.is_str() == false) {
					ERROR("Error in add_item_type, key is not string");
				}


				if (!arr_vec.empty()) {
					ERROR("partialJson is array or object.6");
				}

				obj_data.push_back({ std::move(temp), std::move(temp2) });
			}
	}

	void PartialJson::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t val_token_idx) {

			{
				_Value temp2;
				bool e = false;

				claujson::Convert(temp2, val_buf_idx, val_next_buf_idx, false, buf, val_token_idx, e);

				if (e) {

					ERROR("Error in add_item_type");
				}

				if (!obj_data.empty()) {
					ERROR("partialJson is array or object.5");
				}

				arr_vec.push_back(std::move(temp2));
			}
	}

	 void PartialJson::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		_ValueType type, uint64_t key_token_idx

	) {
		{
			if (!arr_vec.empty()) {
				ERROR("partialJson is array or object.4"); // chk?
			}

			_Value temp;
			bool e = false;

			claujson::Convert(temp, key_buf_idx, key_next_buf_idx, true, buf, key_token_idx, e);

			if (e) {
				ERROR("Error in add_user_type");
			}

			if (temp.is_str() == false) {
				ERROR("Error in add_item_type, key is not string");
			}

			if (type == _ValueType::OBJECT) {
				StructuredPtr json = new (std::nothrow) Object(
#ifdef USE_PMR
					res
#endif
				);

				if (json == nullptr) {
					log << warn << "new error";
					return;
				}

				obj_data.push_back({ std::move(temp), _Value(json) });

				json.set_parent(StructuredPtr(this));
			}
			else if (type == _ValueType::ARRAY) {
				StructuredPtr json = new (std::nothrow) Array(

				);

				if (json == nullptr) {
					log << warn << "new error";
					return;
				}

				obj_data.push_back({ std::move(temp), _Value(json) });

				json.set_parent(StructuredPtr(this));
			}
		}
	}
	 void PartialJson::add_user_type(_ValueType type

	) {
		{
			if (!obj_data.empty()) {
				ERROR("PartialJson is array or object.3");
			}

			StructuredPtr json;

			if (type == _ValueType::OBJECT) {
				json = new (std::nothrow) Object(

				);
			}
			else if (type == _ValueType::ARRAY) {
				json = new (std::nothrow) Array(

				);
			}

			if (json == nullptr) {
				log << warn << "new error";
				return;
			}

			arr_vec.push_back(_Value(json));

			json.set_parent(this);
		}
	}

	bool PartialJson::add_object_element(Value key, Value val) {
		if (val.Get().is_virtual()) {
			if (val.Get().is_array()) {
				val.Get().as_array()->set_parent(this);
			}
			else if (val.Get().is_object()) {
				val.Get().as_object()->set_parent(this);
			}
			this->virtualJson = std::move(val.Get());
			return true;
		}
		
		if (!key.Get().is_str()) {
			return false;
		}
		if (!arr_vec.empty()) {
			ERROR("partialJson is array or object.2");
			return false;
		}
		
		if (val.Get().is_array()) {
			val.Get().as_array()->set_parent(this);
		}
		else if (val.Get().is_object()) {
			val.Get().as_object()->set_parent(this);
		}

		obj_data.push_back({ std::move(key.Get()), std::move(val.Get()) });

		return true;
	}

	bool PartialJson::add_array_element(Value val) {
		
		if (val.Get().is_virtual()) {
			if (val.Get().is_array()) {
				val.Get().as_array()->set_parent(this);
			}
			else if (val.Get().is_object()) {
				val.Get().as_object()->set_parent(this);
			}

			this->virtualJson = std::move(val.Get());
			return true;
		}
		
		if (!obj_data.empty()) {
			ERROR("partialJson is array or object.1");
			return false;
		}


		if (val.Get().is_array()) {
			val.Get().as_array()->set_parent(this);
		}
		else if (val.Get().is_object()) {
			val.Get().as_object()->set_parent(this);
		}

		arr_vec.push_back(std::move(val.Get()));

		return true;
	}
	void  PartialJson::MergeWith(Array* j, int start_offset) {
		auto* x = j;

		uint64_t len = j->get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (j->get_value_list(i).is_array()) {
				j->get_value_list(i).as_array()->set_parent(this);
			}
			else if (j->get_value_list(i).is_object()) {
				j->get_value_list(i).as_object()->set_parent(this);
			}
		}
		if (x->arr_vec.empty() == false) {
			arr_vec.insert(arr_vec.end(), std::make_move_iterator(x->arr_vec.begin()) + start_offset,
				std::make_move_iterator(x->arr_vec.end()));
		}
		else {

			log << info << "test5";
		}
	}
	void  PartialJson::MergeWith(Object* j, int start_offset) {
		ERROR("PartialJson::MergeWith Error");
	}
	void  PartialJson::MergeWith(PartialJson* j, int start_offset) {
		auto* x = dynamic_cast<PartialJson*>(j);

		uint64_t len = j->get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (j->get_value_list(i).is_array()) {
				j->get_value_list(i).as_array()->set_parent(this);
			}
			else if (j->get_value_list(i).is_object()) {
				j->get_value_list(i).as_object()->set_parent(this);
			}
		}

		if (x->virtualJson.is_structured()) {
			start_offset = 0;
		}

		if (x->arr_vec.empty() == false) {
			arr_vec.insert(arr_vec.end(), std::make_move_iterator(x->arr_vec.begin()) + start_offset,
				std::make_move_iterator(x->arr_vec.end()));
		}
		else {

			log << info << "test6";
		}
	}

}
