#pragma once

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
		explicit StringView() : m_str(nullptr), m_len(0) {}

		StringView(const std::string& str) : m_str(str.data()), m_len(str.size()) {}
		explicit StringView(const char* str) : m_str(str) { m_len = strlen(str); }
		explicit StringView(const char* str, size_t len) : m_str(str), m_len(len) {}
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
		static const uint64_t npos;

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
				delete[] ptr;
				ptr = nullptr;
				sz = 0;
				type = 0;
			}
		}
		Vector(const Vector&) = delete;
		Vector& operator=(const Vector&) = delete;
		Vector(Vector&& other) noexcept {
			std::swap(ptr, other.ptr);
			std::swap(capacity, other.capacity);
			std::swap(sz, other.sz);
			std::swap(type, other.type);
			memcpy(buf, other.buf, SIZE * sizeof(T));
		}
		Vector& operator=(Vector&& other) noexcept {
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
				delete[] ptr;
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
						delete[] ptr; ptr = nullptr;
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
					if (!temp) {
						throw ("new failed");
					}
					memcpy(temp, ptr, sz * sizeof(T));
					capacity = capacity * 2;
					delete[] ptr;
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

	extern Log::Info info;
	extern Log::Warning warn;
	extern Log log; // no static..
	// inline Error error;

	template <class T>
	using PtrWeak = T*;

	template <class T>
	using Ptr = std::unique_ptr<T>;
	// Ptr - use std::move


	enum class _ValueType : int32_t {
		NONE = 0, // chk 
		ARRAY, // ARRAY_OBJECT -> ARRAY, OBJECT
		OBJECT,
		PARTIAL_JSON,
		INT, UINT,
		FLOAT,
		BOOL,
		NULL_,
		STRING, SHORT_STRING,
		NOT_VALID,
		ERROR // private class?
	};

	template <class Key, class Data>
	class Pair {
	public:
		Key first = Key();
		Data second = Data();
	public:
		Pair() {}
		Pair(Key&& first, Data&& second) : first(std::move(first)), second(std::move(second)) {}
		Pair(const Key& first, Data&& second) : first((first)), second(std::move(second)) {}
		Pair(Key&& first, const Data& second) : first(std::move(first)), second((second)) {}
	};

	template <class T>
	using std_vector = std::vector<T>;


} // end of claujson

