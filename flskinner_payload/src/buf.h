#pragma once

#include <vector>
#include <string>
#include <stdint.h>

using vec_byte_t = std::vector<uint8_t>;

class buffer_t {
public:
	buffer_t( vec_byte_t data = {} ) {
		m_data = data;
	}

	template <typename T>
	const T read( bool add_pos = true ) {
		const auto data = *reinterpret_cast< T* >( m_data.data() + pos );
		if ( add_pos ) {
			pos += sizeof( T );
		}
		return data;
	}

	std::string read_str( size_t size ) {
		const auto start = m_data.data() + pos;
		pos += size;
		return std::string( start, start + size );
	}

	template <typename T>
	size_t write( T data, bool add_pos = true ) {
		m_data.insert(
			m_data.end(),
			reinterpret_cast< char* >( &data ),
			reinterpret_cast< char* >( &data ) + sizeof( T )
		);
		if ( add_pos ) {
			pos += sizeof( T );
		}
		return sizeof( T );
	}

	size_t write_str( std::string str, bool add_pos = true ) {
		m_data.insert(
			m_data.end(),
			str.begin(),
			str.end()
		);
		if ( add_pos ) {
			pos += str.size();
		}
		return str.size();
	}

	void smart_write_str( std::string str ) {
		write<uint8_t>( str.size() );
		write_str( str );
	}

	void smart_write_str_32( std::string str ) {
		write<uint32_t>( str.size() );
		write_str( str );
	}

	void offset( ptrdiff_t delta ) {
		pos += delta;
	}

	ptrdiff_t cur_pos() {
		return pos;
	}

	vec_byte_t& raw() {
		return m_data;
	}
private:
	vec_byte_t m_data;
	ptrdiff_t pos;
};