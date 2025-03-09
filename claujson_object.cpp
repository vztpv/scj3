#include "claujson.h"

namespace claujson {
	extern Log log;

	_Value Object::data_null{ nullptr, false }; // valid is false..
	const uint64_t Object::npos = -1; // 

	class CompKey {
	private:
		const std_vector<Pair<_Value, _Value>>* vec;
	public:

		CompKey(const std_vector<Pair<_Value, _Value>>* vec) : vec(vec) {
			//
		}

		bool operator()(uint64_t x, uint64_t y) const {
			return (*vec)[x].first < (*vec)[y].first;
		}
	};

	Object* Object::clone() const {
		Object* result = new (std::nothrow) Object();
		if (result == nullptr) {
			return nullptr;
		}
		uint64_t sz = this->get_data_size();

		for (uint64_t i = 0; i < sz; ++i) {
			auto x = this->get_value_list(i).clone();
			result->add_element(this->get_key_list(i).clone(), std::move(x));
		}

		return result;
	}

	bool Object::chk_key_dup(uint64_t* idx) const {
		bool has_dup = false;
		std_vector<uint64_t> copy_(obj_data.size(), 0);

		for (uint64_t i = 0; i < copy_.size(); ++i) {
			copy_[i] = i;
		}

		CompKey comp(&obj_data);

		std::stable_sort(copy_.begin(), copy_.end(), comp);

		for (uint64_t i = 1; i < copy_.size(); ++i) {
			if (obj_data[copy_[i]].first == obj_data[copy_[i - 1]].first) {
				has_dup = true;
				if (idx) {
					*idx = copy_[i - 1]; //
				}
				break;
			}
		}

		return has_dup;
	}

	_Value Object::Make() {
		Object* obj = new (std::nothrow) Object();

		if (obj == nullptr) {
			_Value v;
			v._type = _ValueType::ERROR;
			return v;
		}

		return _Value(obj);
	}
	_Value Object::MakeVirtual() {
		Object* obj = new (std::nothrow) Object();

		if (obj == nullptr) {
			_Value v;
			v._type = _ValueType::ERROR;
			return v;
		}
		obj->_is_virtual = true;
		return _Value(obj);
	}

	Object::Object() {}

	Object::~Object() {
		for (auto& x : obj_data) {
			if (x.second.is_array()) {
				delete x.second.as_array();
			}
			else if (x.second.is_object()) {
				delete x.second.as_object();
			}
		}
	}

	bool Object::is_object() const {
		return true;
	}
	bool Object::is_array() const {
		return false;
	}

	uint64_t Object::get_data_size() const {
		return obj_data.size();
	}

	_Value& Object::get_value_list(uint64_t idx) {
		return obj_data[idx].second;
	}

	_Value& Object::get_key_list(uint64_t idx) { // if key change then also obj_data[idx].key? change??
		return obj_data[idx].first;
	}

	const _Value& Object::get_const_key_list(uint64_t idx) {
		return obj_data[idx].first;
	}
	const _Value& Object::get_const_key_list(uint64_t idx) const {
		return obj_data[idx].first;
	}
	const _Value& Object::get_value_list(uint64_t idx) const {
		return obj_data[idx].second;
	}

	const _Value& Object::get_key_list(uint64_t idx) const {
		return obj_data[idx].first;
	}

	void Object::clear(uint64_t idx) {
		obj_data[idx].second.clear(false);
		obj_data[idx].first.clear(false);
	}

	bool Object::is_virtual() const {
		return _is_virtual;
	}

	void Object::clear() {
		obj_data.clear();
	}


	Object::_ValueIterator Object::begin() {
		return obj_data.begin();
	}

	Object::_ValueIterator Object::end() {
		return obj_data.end();
	}

	Object::_ConstValueIterator Object::begin() const {
		return obj_data.begin();
	}

	Object::_ConstValueIterator Object::end() const {
		return obj_data.end();
	}

	void Object::reserve_data_list(uint64_t len) {
		obj_data.reserve(len);
	}


	void Object::set_parent(StructuredPtr a) {
		parent = (a);
	}


	uint64_t Object::find(const _Value& key) const {
		if (!is_object() || !key.is_str()) { // } || !is_valid()) {
			return npos;
		}

		uint64_t len = get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (get_key_list(i) == key) {
				return i;
			}
		}

		return -1;
	}

	_Value& Object::operator[](uint64_t idx) {
		if (idx >= get_data_size()) {
			return data_null;
		}
		return get_value_list(idx);
	}

	const _Value& Object::operator[](uint64_t idx) const {
		if (idx >= get_data_size()) {
			return data_null;
		}
		return get_value_list(idx);
	}

	_Value& Object::operator[](const _Value& key) { // if not exist key, then nothing.
		uint64_t idx = npos;
		if ((idx = find(key)) == npos) {
			return data_null;
		}

		return get_value_list(idx);
	}
	const _Value& Object::operator[](const _Value& key) const { // if not exist key, then nothing.
		uint64_t idx = npos;
		if ((idx = find(key)) == npos) {
			return data_null;
		}

		return get_value_list(idx);
	}

	const StructuredPtr Object::get_parent() const {
		return parent;
	}

	bool Object::change_key(const _Value& key, Value new_key) { // chk test...
		if (this->is_object() && key.is_str() && new_key.Get().is_str()) {
			auto idx = find(key);
			if (idx == npos) {
				return false;
			}

			get_key_list(idx) = std::move(new_key.Get());

			return true;
		}
		return false;
	}

	bool Object::change_key(uint64_t idx, Value new_key) {
		if (this->is_object() && new_key.Get().is_str()) {
			if (idx == npos) {
				return false;
			}

			get_key_list(idx) = std::move(new_key.Get());

			return true;
		}
		return false;
	}


	bool Object::add_element(Value key, Value val) {
		if (val.Get().is_virtual()) {
			if (val.Get().is_array()) {
				Array* x = val.Get().as_array();
				x->set_parent(this);
			}
			else if (val.Get().is_object()) {
				Object* x = val.Get().as_object();
				x->set_parent(this);
			}
			obj_data.push_back({ std::move(key.Get()), std::move(val.Get()) });
			return true;
		}

		if (!key.Get().is_str()) {
			return false;
		}

		if (val.Get().is_structured()) {
			if (val.Get().is_array()) {
				Array* x = val.Get().as_array();
				x->set_parent(this);
			}
			else if (val.Get().is_object()) {
				Object* x = val.Get().as_object();
				x->set_parent(this);
			}
		}
		obj_data.push_back({ std::move(key.Get()), std::move(val.Get()) });

		return true;
	}

	bool Object::assign_value_element(uint64_t idx, Value val) { this->obj_data[idx].second = std::move(val.Get()); return true; }
	bool Object::assign_key_element(uint64_t idx, Value key) {
		if (!key.Get() || !key.Get().is_str()) {
			return false;
		}
		this->obj_data[idx].first = std::move(key.Get());
		return true;
	}

	void Object::erase(const _Value& key, bool real) {
		uint64_t idx = this->find(key);
		erase(idx, real);
	}

	void Object::erase(uint64_t idx, bool real) {

		if (real) {
			clean(obj_data[idx].first);
			clean(obj_data[idx].second);
		}

		obj_data.erase(obj_data.begin() + idx);
	}


	void Object::MergeWith(Array* j, int start_offset) {
		ERROR("Object::MergeWith Error");
		return;
	}


	void Object::MergeWith(Object* j, int start_offset) {
		auto* x = j;

		uint64_t len = j->get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (j->get_value_list(i).is_structured()) {
				if (j->get_value_list(i).is_array()) {
					j->get_value_list(i).as_array()->set_parent(this);
				}
				else if (j->get_value_list(i).is_object()) {
					j->get_value_list(i).as_object()->set_parent(this);
				}
			}
		}

		if (x->obj_data.empty() == false) {
			obj_data.insert(obj_data.end(), std::make_move_iterator(x->obj_data.begin()) + start_offset,
				std::make_move_iterator(x->obj_data.end()));
		}
		else {
			log << info << "test1";
		}
	}
	void Object::MergeWith(PartialJson* j, int start_offset) {

		auto* x = j;

		if (x->arr_vec.empty() == false) { // not object?
			ERROR("partial json is not object");
		}

		uint64_t len = j->get_data_size();
		for (uint64_t i = 0; i < len; ++i) {
			if (j->get_value_list(i).is_structured()) {
				if (j->get_value_list(i).is_array()) {
					j->get_value_list(i).as_array()->set_parent(this);
				}
				else if (j->get_value_list(i).is_object()) {
					j->get_value_list(i).as_object()->set_parent(this);
				}
			}
		}

		if (x->virtualJson.is_structured()) {
			start_offset = 0;
		}

		if (x->obj_data.empty() == false) {
			obj_data.insert(obj_data.end(), std::make_move_iterator(x->obj_data.begin()) + start_offset,
				std::make_move_iterator(x->obj_data.end()));
		}
		else {
			log << info << "test2";
		}
	}

	void Object::add_item_type(int64_t key_buf_idx, int64_t key_next_buf_idx, int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t key_token_idx, uint64_t val_token_idx) {

			{
				_Value temp;// key
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

				if (!temp.is_str()) {
					ERROR("Error in add_item_type, key is not string");
				}

				obj_data.emplace_back(std::move(temp), std::move(temp2));
			}
	}

	void Object::add_item_type(int64_t val_buf_idx, int64_t val_next_buf_idx,
		char* buf, uint64_t val_token_idx) {
		// error

		log << warn << "errr..";
		ERROR("Error Object::add_item_type");
	}

	void Object::add_user_type(_ValueType type

	) {
		// error

		log << warn << "errr..";
		ERROR("Error Object::add_user_type");
		return;
	}

	void Object::add_user_type(int64_t key_buf_idx, int64_t key_next_buf_idx, char* buf,
		_ValueType type, uint64_t key_token_idx

	) {

		{
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
				Object* json = new (std::nothrow) Object(
				);
				if (json == nullptr) {
					log << warn << "new error";
					return;
				}

				json->set_parent(this);
				obj_data.emplace_back(std::move(temp), _Value(json));

			}
			else if (type == _ValueType::ARRAY) {
				Array* json = new (std::nothrow) Array(
				);
				if (json == nullptr) {
					log << warn << "new error";
					return;
				}

				json->set_parent(this);
				obj_data.emplace_back(std::move(temp), _Value(json));

			}
		}
	}
}
