// yes, i really did write delphi form file parser LMAO
// many many many hours of staring at my hex editor

#pragma once

#include <string>
#include <vector>

#include "buf.h"

namespace dfm {
	struct long_double_t {
		// not implementing this right now
		char bytes[ 10 ];
	};

	enum class obj_type_t : uint8_t {
		normal = 0,
		inherited = 0xF1,
		weird_number_bullshit = 0xF2,
		inl = 0xF4, // inline
	};

	enum class type_t : uint8_t {
		obj = 0,
		arr = 0x01,
		int8 = 0x02,
		int16 = 0x03,
		int32 = 0x04,
		long_double = 0x05,
		string = 0x06,
		constant = 0x07,
		bool_false = 0x08,
		bool_true = 0x09,
		string_array = 0x0B,
		string_long = 0x0C,
		nil = 0x0D,
		string_long_2 = 0x14,
		int16_array = 0x12,
		int16_2 = 0x1F,
		int16_3 = 0x0E
	};
	
	class val {
	public:
		type_t m_type;
		std::vector<val> m_arr;
		uint64_t m_num_val;

		// ?????????????????????????????
		// why tf is delphi storing small numbers (like font sizes) with 80 bit floats????
		// msvc doesnt even support long doubles LOL
		long_double_t m_extended_val;

		std::string m_str_val;
		std::vector<std::string> m_str_arr;
		std::vector<uint16_t> m_int16_arr;

		bool is_array() {
			return m_type == type_t::arr;
		}

		bool is_numerical() {
			if ( m_type >= type_t::int8 && m_type <= type_t::int32 ) return true;
			if ( m_type >= type_t::bool_false && m_type <= type_t::bool_true ) return true;
			return false;
		}

		void add_element( val v ) {
			m_arr.push_back( v );
		}

		void to_binary( buffer_t& buf );
	};

	class object {
	public:
		void setup( std::string name, std::string parent_name = "" ) {
			m_name = name;
			m_parent_name = parent_name;
			m_is_object = true;
		}

		void setup( std::string name, type_t type, std::string value ) {
			setup( name );
			m_val.m_type = type;
			m_val.m_str_val = value;
		}

		void setup( std::string name, type_t type, uint64_t value ) {
			setup( name );
			m_val.m_type = type;
			m_val.m_num_val = value;
		}

		void setup( std::string name, val v ) {
			m_name = name;
			m_val = v;
		}

		val& get_val() {
			return m_val;
		}

		std::string& get_name() {
			return m_name;
		}

		void to_binary( buffer_t& buf );
		buffer_t get_full_binary();
	public:
		object* get_parent() { return m_parent; }
		
		void set_parent( object* p ) {
			m_parent = p;
		}

		object& add_child( object o ) {
			m_children.push_back( o );
			return m_children[ m_children.size() - 1 ];
		}

		std::vector<object>& get_children() {
			return m_children;
		}

		bool is_object() {
			return m_is_object;
		}

		void set_root( bool is_root ) {
			m_is_root = is_root;
		}

		void set_obj_type( obj_type_t obj_type ) {
			m_obj_type = obj_type;
		}

		void set_number( uint8_t num ) {
			m_number = num;
		}
	private:
		bool m_is_object = false;
		bool m_is_root = false;
		std::string m_name, m_parent_name;
		object* m_parent;
		val m_val;
		std::vector<object> m_children;
		obj_type_t m_obj_type;
		uint8_t m_number;
	};

	dfm::object parse( vec_byte_t file_buffer );
}