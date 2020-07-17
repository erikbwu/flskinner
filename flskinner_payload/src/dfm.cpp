#include "dfm.hpp"

void dfm::object::to_binary( buffer_t& buf ) {
	if ( m_obj_type == obj_type_t::inherited ) {
		buf.write<uint8_t>( 0xF1 );
	} else if ( m_obj_type == obj_type_t::weird_number_bullshit ) {
		buf.write<uint8_t>( 0xF2 );
		buf.write<uint8_t>( 0x02 );
		buf.write<uint8_t>( m_number );
	} else if ( m_obj_type == obj_type_t::inl ) {
		buf.write<uint8_t>( 0xF4 );
	}

	buf.smart_write_str( m_parent_name );
	buf.smart_write_str( m_name );

	for ( auto& c : m_children ) {
		if ( c.is_object() ) {
			buf.write<uint8_t>( 0 );
			c.to_binary( buf );
		} else {	
			buf.smart_write_str( c.m_name );
			buf.write<uint8_t>( static_cast< uint8_t >( c.get_val().m_type ) );

			auto var = c.get_val();
			var.to_binary( buf );
		}
	}

	buf.write<uint8_t>( 0 );
}

void dfm::val::to_binary( buffer_t & buf ) {
	switch ( m_type ) {
	case type_t::arr: {
		for ( auto &el : m_arr ) {
			buf.write<uint8_t>( static_cast< uint8_t >( el.m_type ) );
			el.to_binary( buf );
		}
		buf.write<uint8_t>( 0 );
		break;
	}
	case type_t::int8: {
		buf.write<int8_t>( m_num_val );
		break;
	}
	case type_t::int16:
	case type_t::int16_2:
	case type_t::int16_3: {
		buf.write<int16_t>( m_num_val );
		break;
	}
	case type_t::int32: {
		buf.write<int32_t>( m_num_val );
		break;
	}
	case type_t::long_double: {
		buf.write<long_double_t>( m_extended_val );
		break;
	}
	case type_t::string: {
		buf.smart_write_str( m_str_val );
		break;
	}
	case type_t::string_long:
	case type_t::string_long_2: {
		buf.smart_write_str_32( m_str_val );
		break;
	}
	case type_t::constant: {
		buf.smart_write_str( m_str_val );
		break;
	}
	case type_t::bool_false: // here the type alone represents the value
	case type_t::bool_true: {
		break;
	}
	case type_t::string_array: {
		for ( auto& str : m_str_arr ) {
			buf.smart_write_str( str );
		}
		buf.write<uint8_t>( 0 );
		break;
	}
	case type_t::int16_array: {
		buf.write<uint32_t>( m_int16_arr.size() );

		for ( auto& el : m_int16_arr ) {
			buf.write<uint16_t>( el );
		}

		break;
	}
	}
}

dfm::object dfm::parse( vec_byte_t file_buffer ) {
	buffer_t buf( file_buffer );

	// skip TPF0 header
	buf.offset( 4 );

	dfm::object root_obj;
	root_obj.set_root( true );

	dfm::object* cur_obj = &root_obj;

	// reserve these variable names
	{
		auto object_parent_size = buf.read<uint8_t>();
		bool object_inherited = false;

		if ( object_parent_size == 0xF1 ) {
			cur_obj->set_obj_type( dfm::obj_type_t::inherited );
			object_parent_size = buf.read<uint8_t>();
		}

		// object
		auto object_parent = buf.read_str( object_parent_size );
		auto object_name_size = buf.read<uint8_t>();
		auto object_name = buf.read_str( object_name_size );

		cur_obj->setup( object_name, object_parent );
	}

	int indentions = 1;
	uint8_t unhandled_type;
	size_t unhandled_type_pos;

	while ( true ) {
		auto var_name_size = buf.read<uint8_t>();

		if ( var_name_size == 0 ) {
			bool unindent = false;

			if ( buf.cur_pos() >= buf.raw().size() ) {
				unindent = true;
			} else {
				auto next_byte = buf.read<uint8_t>( false );
				if ( next_byte == 0 ) {
					unindent = true;
				}
			}

			if ( unindent ) {
				indentions--;

				if ( indentions == 0
					 || ( cur_obj->get_parent() == nullptr ) ) {
					goto quit;
				}

				cur_obj = cur_obj->get_parent();

				continue;
			} else {
			indent_again:
				indentions++;

				auto object_parent_size = buf.read<uint8_t>();
				uint8_t object_number;

				dfm::object obj;

				if ( object_parent_size == 0xF1 ) {
					obj.set_obj_type( dfm::obj_type_t::inherited );
					object_parent_size = buf.read<uint8_t>();
				} else if ( object_parent_size == 0xF2 ) {
					buf.offset( 1 ); // skip the type (it's always a fucking int8)
					object_number = buf.read<uint8_t>();
					object_parent_size = buf.read<uint8_t>();

					obj.set_obj_type( dfm::obj_type_t::weird_number_bullshit );
					obj.set_number( object_number );
				} else if ( object_parent_size == 0xF4 ) {
					obj.set_obj_type( dfm::obj_type_t::inl );
					object_parent_size = buf.read<uint8_t>();
				}

				auto object_parent = buf.read_str( object_parent_size );
				auto object_name_size = buf.read<uint8_t>();
				auto object_name = buf.read_str( object_name_size );

				obj.setup( object_name, object_parent );
				obj.set_parent( cur_obj );
				cur_obj = &cur_obj->add_child( obj );

				auto next_byte = buf.read<uint8_t>( false );
				if ( next_byte == 0 ) {
					buf.offset( 1 );
					goto indent_again;
				}

				bool no_parent = false;

				if ( object_name_size == 0 ) {
					no_parent = true;
				}

				var_name_size = buf.read<uint8_t>();
			}
		}

		auto var_name = buf.read_str( var_name_size );

		bool in_array = false;
		dfm::val val_array;

	start_value:

		dfm::val val;

		auto value_type = buf.read<uint8_t>();

		if ( value_type == 0 ) {
			in_array = false;

			dfm::object var_obj;
			var_obj.setup( var_name, val_array );
			cur_obj->add_child( var_obj );

			goto skip;
		}

		switch ( value_type ) {
		case 0x01: { // array, uses ()
			in_array = true;

			val_array = {};
			val_array.m_type = dfm::type_t::arr;

			goto start_value;
			break;
		}
		case 0x02: { // uint8_t 
			auto value = buf.read<int8_t>();

			val.m_type = dfm::type_t::int8;
			val.m_num_val = value;

			break;
		}
		case 0x03:
		case 0x0E:
		case 0x1F: { // uint16_t 
			auto value = buf.read<int16_t>();

			val.m_type = static_cast< dfm::type_t >( value_type );
			val.m_num_val = value;

			break;
		}
		case 0x04: { // uint32_t 
			auto value = buf.read<int32_t>();

			val.m_type = dfm::type_t::int32;
			val.m_num_val = value;

			break;
		}
		case 0x05: { // long double (10 bytes)
			auto value = buf.read<dfm::long_double_t>();

			val.m_type = dfm::type_t::long_double;
			val.m_extended_val = value;

			break;
		}
		case 0x06: { // string
			auto value_len = buf.read<uint8_t>();
			auto value = buf.read_str( value_len );

			val.m_type = dfm::type_t::string;
			val.m_str_val = value;

			break;
		}
		case 0x07: { // string (constant, no quotes)
			auto value_size = buf.read<uint8_t>();
			auto value = buf.read_str( value_size );

			val.m_type = dfm::type_t::constant;
			val.m_str_val = value;

			break;
		}
		case 0x08: { // bool (false)

			val.m_type = dfm::type_t::bool_false;

			break;
		}
		case 0x09: { // bool (true)

			val.m_type = dfm::type_t::bool_true;

			break;
		}
		case 0x0B: { // array (only supports strings atm, not sure if anything else shows up)
			auto value_len = buf.read<uint8_t>();

			val.m_type = dfm::type_t::string_array;

			while ( value_len != 0 ) {
				auto value = buf.read_str( value_len );
				value_len = buf.read<uint8_t>();

				val.m_str_arr.push_back( value );
			}

			break;
		}
		case 0x0C:
		case 0x14: { // long string (size is 32 bit)
			auto value_len = buf.read<uint32_t>();
			auto value = buf.read_str( value_len );

			val.m_type = static_cast< dfm::type_t >( value_type );
			val.m_str_val = value;

			break;
		}
		case 0xD: { // nil
			val.m_type = dfm::type_t::nil;

			break;
		}
		case 0x12: { // id or something? example: Caption = #61474
			const auto size = buf.read<uint32_t>();

			val.m_type = dfm::type_t::int16_array;

			for ( auto i = 0; i < size; i++ ) {
				val.m_int16_arr.push_back( buf.read<uint16_t>() );
			}

			break;
		}
		default:
			unhandled_type = value_type;
			unhandled_type_pos = buf.cur_pos();
			goto unhandled_value_type;
		}

	next:
		if ( in_array ) {
			val_array.add_element( val );
			goto start_value;
		} else {
			dfm::object var_obj;
			var_obj.setup( var_name, val );
			cur_obj->add_child( var_obj );
		}
	skip: {}
	}

quit:
	return root_obj;

unhandled_value_type:
	goto quit;
}


buffer_t dfm::object::get_full_binary() {
	buffer_t bin;
	bin.write_str( "TPF0" );

	to_binary( bin );
	bin.write<uint8_t>( 0 );

	return bin;
}