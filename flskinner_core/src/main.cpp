#include <Windows.h>
#include <iostream>

extern "C" {
	__declspec( dllexport ) void __cdecl inject(const char*);
}

__declspec( dllexport ) void __cdecl inject( const char* fl_studio_path ) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof( si ) );
	si.cb = sizeof( si );
	ZeroMemory( &pi, sizeof( pi ) );

	const auto res = CreateProcessA(
		NULL,
		const_cast< char* >( fl_studio_path ),
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&si,
		&pi
	);

	TCHAR full_path[ MAX_PATH ];
	GetFullPathNameA( "flskinner_payload.dll", MAX_PATH, full_path, NULL );

	std::string path( full_path );

	const auto hndl = OpenProcess( PROCESS_ALL_ACCESS, false, pi.dwProcessId );

	if ( !hndl ) {
		MessageBoxA( NULL, "Failure on OpenProcess.", NULL, NULL );
		return;
	}

	const auto address = VirtualAllocEx( hndl, nullptr, path.size() + 1, MEM_COMMIT, PAGE_READWRITE );
	WriteProcessMemory( hndl, address, path.data(), path.size() + 1, nullptr );

	const auto loadlibrarya = GetProcAddress( LoadLibrary( "kernel32" ), "LoadLibraryA" );

	uint32_t thread_id;
	const auto thread_hndl = CreateRemoteThread(
		hndl,
		nullptr,
		0,
		reinterpret_cast< LPTHREAD_START_ROUTINE >( loadlibrarya ),
		address,
		0,
		reinterpret_cast< LPDWORD >( &thread_id ) );

	if ( !thread_hndl ) {
		MessageBoxA( NULL, "Failure on CreateRemoteThread.", NULL, NULL );
		return;
	}

	WaitForSingleObject( thread_hndl, 1000 );

	VirtualFreeEx( hndl, address, 0, MEM_RELEASE );

	ResumeThread( pi.hThread );
}