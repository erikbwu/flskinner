#include <Windows.h>
#include <MinHook.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <locale>
#include <codecvt>
#include <filesystem>
namespace fs = std::filesystem;

#include <shlobj.h>

#include "patterns.hpp"
#include "dfm.hpp"
#include "json.hpp"

std::string module_name = "FLEngine_x64.dll";

// winapi hooks

using fn_LoadResource_t = HGLOBAL( __stdcall* )( HMODULE, HRSRC );
fn_LoadResource_t orig_LoadResource;

using fn_LockResource_t = LPVOID( __stdcall* )( HGLOBAL );
fn_LockResource_t orig_LockResource;

using fn_SizeofResource_t = DWORD( __stdcall* )( HMODULE, HRSRC );
fn_SizeofResource_t orig_SizeofResource;

// FL hooks

using fn_get_color_t = uint64_t( * )( uint32_t );
fn_get_color_t orig_get_mixer_color;

using fn_unk_set_color_t = uint64_t( * )( uint64_t, uint64_t, uint64_t );
fn_unk_set_color_t orig_unk_set_color;

std::wstring skin_path;

struct resource_t {
	std::string name;
	size_t new_size;

	HRSRC _rsrc;
	HGLOBAL _mem;
	HMODULE _module;
	SIZE_T _size;
};

std::vector<resource_t> resources;

bool resources_loaded = false;
bool hack_mixer_tracks = false;

BOOL CALLBACK EnumResNameProc(
  _In_opt_ HMODULE  hModule,
  _In_     LPCTSTR  lpszType,
  _In_     LPTSTR   lpszName,
  _In_     LONG_PTR lParam
) {
	const auto hrsrc = FindResourceA( hModule, lpszName, lpszType );

	resource_t resource;
	resource._rsrc = hrsrc;

	const auto _mem = orig_LoadResource( hModule, hrsrc );
	const auto data = orig_LockResource( _mem );

	// TPF0 header
	if ( *reinterpret_cast< uint32_t* >( data ) == 0x30465054 ) {
		// only add DFM files
		resources.push_back( resource );
	}

	return true;
}

void load_resources() {
	const auto mod = GetModuleHandleA( module_name.c_str() );

	EnumResourceNamesA( mod, RT_RCDATA, EnumResNameProc, NULL );
}

struct col_replacement_t {
	uint32_t a, b;
};

std::vector<col_replacement_t> dfm_replacements = {
	//{ 0x676259, 0x272727 },
	//{ 0x6d685f, 0x272727 },
	//{ 0x4d483f, 0x1f1f1f },
	//{ 0x74735f, 0x1b1b1b },
	//{ 0x666553, 0x1f1f1f },
	//{ 0x6d685f, 0x1f1f1f },
	//{ 0x4b463e, 0x1f1f1f },
	//{ 0x3d3830, 0x1f1f1f },
	//{ 0x453c31, 0x272727 },
	//{ 0x5e5950, 0x292929 }
};

std::vector<col_replacement_t> hook_replacements = {
	//{ 0x313C45, 0x232323 }, // pattern picker background
	//{ 0x34444e, 0x1F1F1F }, // playlist background
	//{ 0x5F686D, 0x29363E }, // mixer eq background
	//{ 0x182832, 0x33343C }, // playlist grid lines
	//{ 0x2a3a44, 0x2C353A }, // playlist grid lines
	//{ 0x22323c, 0x3C3D45 }, // playlist grid lines
	//{ 0x10202a, 0x2C353A }, // playlist grid lines
	//{ 0x1e2b31, 0x171F22 },
	//{ 0x716C63, 0x292929 }
};

uint32_t mixer_color;// = 0x202020;

void do_color_replacements( dfm::object& obj ) {
	for ( auto& c : obj.get_children() ) {
		if ( c.get_children().size() > 0 ) {
			do_color_replacements( c );
		}

		if ( c.get_val().m_type != dfm::type_t::int32 ) continue;
		if ( c.get_name().find( "Color" ) == std::string::npos ) continue;

		for ( auto& replacement : dfm_replacements ) {
			if ( ( c.get_val().m_num_val & 0xFFFFFF ) == ( replacement.a & 0xFFFFFF ) ) {
				c.get_val().m_num_val = replacement.b | ( replacement.a & 0xFF000000 );

				break;
			}
		}
	}
}

vec_byte_t process_resource( void* memory, size_t size ) {
	vec_byte_t raw_buffer;
	raw_buffer.assign(
		reinterpret_cast< char* >( memory ),
		reinterpret_cast< char* >( memory ) + size
	);

	auto obj = dfm::parse( raw_buffer );

	do_color_replacements( obj );

	return obj.get_full_binary().raw();
}

uint64_t hk_get_mixer_color( uint32_t a1 ) {
	auto res = orig_get_mixer_color( a1 );

	res = mixer_color;

	return res;
}

// i think this is used for when different panels' colors are set in FL
// it also might have more args :shrug:
uint64_t hk_unk_set_color( uint64_t a, uint64_t b, uint64_t col ) {
	for ( auto& replacement : hook_replacements ) {
		if ( ( col & 0xFFFFFF ) == ( replacement.a & 0xFFFFFF ) ) {
			col = replacement.b | ( replacement.a & 0xFF000000 );

			break;
		}
	}

	return orig_unk_set_color( a, b, col );
}

// LoadResource hook
HGLOBAL __stdcall hk_LoadResource(
  HMODULE hModule,
  HRSRC   hResInfo
) {
	if ( !resources_loaded ) {
		load_resources();

		const auto get_mixer_color_addy =
			reinterpret_cast< void* >( pattern::find( module_name.c_str(), "55 48 83 EC 30 48 8B EC 89 C8" ) );

		MH_CreateHook( get_mixer_color_addy, hk_get_mixer_color, reinterpret_cast< void** >( &orig_get_mixer_color ) );
		MH_EnableHook( get_mixer_color_addy );

		const auto unk_set_color_addy =
			reinterpret_cast< void* >( pattern::find_rel( module_name.c_str(), "E8 ? ? ? ? 48 8B 45 48 F2 0F 2A 80 ? ? ? ?", 0, 1, 5 ) );

		MH_CreateHook( unk_set_color_addy, hk_unk_set_color, reinterpret_cast< void** >( &orig_unk_set_color ) );
		MH_EnableHook( unk_set_color_addy );

		resources_loaded = true;
	}

	const auto res = orig_LoadResource( hModule, hResInfo );

	for ( auto& resource : resources ) {
		if ( resource._rsrc == hResInfo ) {
			resource._mem = res;
			resource._size = orig_SizeofResource( hModule, hResInfo ); 
			break;
		}
	}

	return res;
}

// LockResource hook
LPVOID __stdcall hk_LockResource(
  HGLOBAL hResData
) {
	hack_mixer_tracks = true;

	const auto res = orig_LockResource( hResData );

	for ( auto& resource : resources ) {
		if ( resource._mem == hResData ) {
			
			auto new_data = process_resource( res, resource._size );
			resource.new_size = new_data.size();

			const auto p = VirtualAlloc( NULL, new_data.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
			
			memcpy( p, new_data.data(), new_data.size() );
			
			// FL will crash if the memory is not read-only
			DWORD old_protect;
			VirtualProtect( reinterpret_cast< LPVOID >( p ), new_data.size(), PAGE_READONLY, &old_protect );
			
			return p;

			break;
		}
	}

	return res;
}

// SizeofResource hook
DWORD __stdcall hk_SizeofResource(
  HMODULE hModule,
  HRSRC   hResInfo
) {
	const auto res = orig_SizeofResource( hModule, hResInfo );

	for ( auto& resource : resources ) {
		if ( resource._rsrc == hResInfo ) {
			return resource.new_size;
		}
	}

	return res;
}

template<typename T = std::string>
vec_byte_t read_file( T path ) {
	std::ifstream file( path, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios::beg );

	auto buf = vec_byte_t( size );
	file.read( reinterpret_cast< char* >( buf.data() ), size );

	return buf;
}

bool replace( std::wstring& str, const std::wstring& from, const std::wstring& to ) {
	size_t start_pos = str.find( from );
	if ( start_pos == std::wstring::npos )
		return false;
	str.replace( start_pos, from.length(), to );
	return true;
}

void start() {
	//AllocConsole();
	//freopen( "CONOUT$", "w", stdout );

	char exe_path[ MAX_PATH ];
	GetModuleFileNameA( GetModuleHandle( "FL64.exe" ), exe_path, MAX_PATH );

	if ( fs::exists( fs::path( exe_path ).parent_path() / "_FLEngine_x64.dll" ) ) {
		module_name = "_FLEngine_x64.dll";
	}
	
	PWSTR path_tmp;
	auto get_folder_path_ret = SHGetKnownFolderPath( FOLDERID_RoamingAppData, 0, nullptr, &path_tmp );

	auto path = std::wstring( path_tmp );
	path += LR"(\flskinner\)";

	nlohmann::json j;

	std::string current_skin_file;

	try {
		const auto config_buffer = read_file( path + L"flskinner.json" );
		j = nlohmann::json::parse( config_buffer.begin(), config_buffer.end() );

		current_skin_file = j[ "currentSkin" ].get<std::string>();

		path += LR"(skins\)";
		path += std::wstring( current_skin_file.begin(), current_skin_file.end() );

	} catch ( std::exception& e ) {
		std::stringstream err;
		err << "An exception occured when loading the config file (flskinner.json in %appdata%/flskinner)";
		err << std::endl;
		err << e.what();

		MessageBoxA( NULL, err.str().c_str(), "FLSkinner", MB_OK );
		exit( 1 );
	}

	try {
		const auto skin_buffer = read_file( path );
		j = nlohmann::json::parse( skin_buffer.begin(), skin_buffer.end() );

		const auto parse_kv = [] ( std::string key, nlohmann::json& value, bool flip_hex = false ) {
			col_replacement_t r;

			std::stringstream ss;
			ss << std::hex << key;
			ss >> r.a;

			ss.str( "" );
			ss.clear();

			ss << std::hex << value.get<std::string>();
			ss >> r.b;

			if ( flip_hex ) {
				const auto flip = [] ( uint32_t& val ) {
					uint32_t flipped = 0;
					flipped |= ( val & 0xFF ) << 16;
					flipped |= ( val & 0xFF00 );
					flipped |= ( val & 0xFF0000 ) >> 16;
					val = flipped;
				};

				flip( r.a );
				flip( r.b );
			}

			return r;
		};

		for ( auto& item : j[ "dfmReplacements" ].items() ) {
			dfm_replacements.push_back( parse_kv( item.key(), item.value(), true ) );
		}

		for ( auto& item : j[ "hookReplacements" ].items() ) {
			hook_replacements.push_back( parse_kv( item.key(), item.value() ) );
		}

		const auto& _dfm_replacements = dfm_replacements;
		const auto& _hook_replacements = hook_replacements;

		std::stringstream ss;
		ss << std::hex << j[ "mixerColor" ].get<std::string>();
		ss >> mixer_color;
	} catch ( std::exception& e ) {
		std::stringstream err;
		err << "An exception occured when loading the skin file (" << current_skin_file << ")";
		err << std::endl;
		err << e.what();

		MessageBoxA( NULL, err.str().c_str(), "FLSkinner", MB_OK );
		exit( 1 );
	}

	MH_Initialize();
	MH_CreateHook( &LoadResource, &hk_LoadResource, reinterpret_cast< void** >( &orig_LoadResource ) );
	MH_EnableHook( &LoadResource );
	MH_CreateHook( &LockResource, &hk_LockResource, reinterpret_cast< void** >( &orig_LockResource ) );
	MH_EnableHook( &LockResource );
	MH_CreateHook( &SizeofResource, &hk_SizeofResource, reinterpret_cast< void** >( &orig_SizeofResource ) );
	MH_EnableHook( &SizeofResource );
}

BOOL WINAPI DllMain(
  _In_ HINSTANCE hinstDLL,
  _In_ DWORD     fdwReason,
  _In_ LPVOID    lpvReserved
) {
	switch ( fdwReason ) {
	case DLL_PROCESS_ATTACH:
		start();
		break;
	}

	return TRUE;
}