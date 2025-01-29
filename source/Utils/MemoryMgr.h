#ifndef __MEMORYMGR
#define __MEMORYMGR

// Switches:
// _MEMORY_NO_CRT - don't include anything "complex" like ScopedUnprotect or memset
// _MEMORY_DECLS_ONLY - don't include anything but macroes

#define WRAPPER __declspec(naked)
#define DEPRECATED __declspec(deprecated)
#define EAXJMP(a) { _asm mov eax, a _asm jmp eax }
#define VARJMP(a) { _asm jmp a }
#define WRAPARG(a) ((int)a)

#define NOVMT __declspec(novtable)
#define SETVMT(a) *((uintptr_t*)this) = (uintptr_t)a

#ifndef _MEMORY_DECLS_ONLY

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <cassert>

#ifndef _MEMORY_NO_CRT
#include <initializer_list>
#include <iterator>
#endif

#include <vector>

inline std::uint8_t* PatternScan(void* module, const char* signature)
{
	static auto pattern_to_byte = [](const char* pattern) {
		auto bytes = std::vector<int>{};
		auto start = const_cast<char*>(pattern);
		auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current) {
			if (*current == '?') {
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			}
			else {
				bytes.push_back(strtoul(current, &current, 16));
			}
		}
		return bytes;
		};

	auto dosHeader = (PIMAGE_DOS_HEADER)module;
	auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

	auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = pattern_to_byte(signature);
	auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

	auto s = patternBytes.size();
	auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i) {
		bool found = true;
		for (auto j = 0ul; j < s; ++j) {
			if (scanBytes[i + j] != d[j] && d[j] != -1) {
				found = false;
				break;
			}
		}
		if (found) {
			return &scanBytes[i];
		}
	}
	return nullptr;
}

enum
{
	PATCH_CALL,
	PATCH_JUMP
};

template<typename AT>
inline AT DynBaseAddress(AT address)
{
#ifdef _WIN64
	return (ptrdiff_t)GetModuleHandle(nullptr) - 0x140000000 + address;
#else
	return (ptrdiff_t)GetModuleHandle(nullptr) - 0x400000 + address;
#endif
}

namespace Memory
{
	template<typename T, typename AT>
	inline void		Patch(AT address, T value)
	{*(T*)address = value; }

	template<typename AT>
	inline void		Nop(AT address, size_t count)
#ifndef _MEMORY_NO_CRT
	{ memset((void*)address, 0x90, count); }
#else
	{ do {
		*(uint8_t*)address++ = 0x90;
	} while ( --count != 0 ); }
#endif

	template<typename Var, typename AT>
	inline void		WriteOffsetValue(AT address, Var var)
	{
		intptr_t dstAddr = (intptr_t)address;
		intptr_t srcAddr;
		memcpy( &srcAddr, std::addressof(var), sizeof(srcAddr) );
		*(int32_t*)dstAddr = static_cast<int32_t>(srcAddr - dstAddr - 4);
	}

	template<typename Var, typename AT>
	inline void		ReadOffsetValue(AT address, Var& var)
	{
		intptr_t srcAddr = (intptr_t)address;
		intptr_t dstAddr = srcAddr + 4 + *(int32_t*)srcAddr;
		var = {};
		memcpy( std::addressof(var), &dstAddr, sizeof(dstAddr) );
	}

	inline void* ReadOffsetValue2(void* instruction_start, int instruction_length = 7)
	{
		void* instruction_end = (void*)((unsigned long long)instruction_start + instruction_length);
		unsigned int* offset = (unsigned int*)((unsigned long long)instruction_start + (instruction_length - 4));

		void* addr = (void*)(((unsigned long long)instruction_start + instruction_length) + *offset);

		return addr;
	}

	template<typename AT, typename Func>
	inline void		InjectHook(AT address, Func hook)
	{
		WriteOffsetValue( (intptr_t)address + 1, hook );
	}

	template<typename AT, typename Func>
	inline void		InjectHook(AT address, Func hook, unsigned int nType)
	{
		*(uint8_t*)address = nType == PATCH_JUMP ? 0xE9 : 0xE8;
		InjectHook(address, hook);
	}

	template<typename Func, typename AT>
	inline void		ReadCall(AT address, Func& func)
	{
		ReadOffsetValue( (intptr_t)address+1, func );
	}

	template<typename AT>
	inline void*	ReadCallFrom(AT address, ptrdiff_t offset = 0)
	{
		uintptr_t addr;
		ReadCall( address, addr );
		return reinterpret_cast<void*>( addr + offset );
	}

	namespace DynBase
	{
		template<typename T, typename AT>
		inline void		Patch(AT address, T value)
		{
			Memory::Patch(DynBaseAddress(address), value);
		}

#ifndef _MEMORY_NO_CRT
		template<typename AT>
		inline void		Patch(AT address, std::initializer_list<uint8_t> list )
		{
			Memory::Patch(DynBaseAddress(address), std::move(list));
		}
#endif

		template<typename AT>
		inline void		Nop(AT address, size_t count)
		{
			Memory::Nop(DynBaseAddress(address), count);
		}

		template<typename Var, typename AT>
		inline void		WriteOffsetValue(AT address, Var var)
		{
			Memory::WriteOffsetValue(DynBaseAddress(address), var);
		}

		template<typename Var, typename AT>
		inline void		ReadOffsetValue(AT address, Var& var)
		{
			Memory::ReadOffsetValue(DynBaseAddress(address), var);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook)
		{
			Memory::InjectHook(DynBaseAddress(address), hook);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook, unsigned int nType)
		{
			Memory::InjectHook(DynBaseAddress(address), hook, nType);
		}

		template<typename Func, typename AT>
		inline void		ReadCall(AT address, Func& func)
		{
			Memory::ReadCall(DynBaseAddress(address), func);
		}

		template<typename AT>
		inline void*	ReadCallFrom(AT address, ptrdiff_t offset = 0)
		{
			return Memory::ReadCallFrom(DynBaseAddress(address), offset);
		}
	};

	namespace VP
	{
		template<typename T, typename AT>
		inline void		Patch(AT address, T value)
		{
			DWORD		dwProtect;
			VirtualProtect((void*)address, sizeof(T), PAGE_EXECUTE_READWRITE, &dwProtect);
			Memory::Patch( address, value );
			VirtualProtect((void*)address, sizeof(T), dwProtect, &dwProtect);
		}

#ifndef _MEMORY_NO_CRT
		template<typename AT>
		inline void		Patch(AT address, std::initializer_list<uint8_t> list )
		{
			DWORD		dwProtect;
			VirtualProtect((void*)address, list.size(), PAGE_EXECUTE_READWRITE, &dwProtect);
			Memory::Patch(address, std::move(list));
			VirtualProtect((void*)address, list.size(), dwProtect, &dwProtect);
		}
#endif

		template<typename AT>
		inline void		Nop(AT address, size_t count)
		{
			DWORD		dwProtect;
			VirtualProtect((void*)address, count, PAGE_EXECUTE_READWRITE, &dwProtect);
			Memory::Nop( address, count );
			VirtualProtect((void*)address, count, dwProtect, &dwProtect);
		}

		template<typename Var, typename AT>
		inline void		WriteOffsetValue(AT address, Var var)
		{
			DWORD		dwProtect;

			VirtualProtect((void*)address, 4, PAGE_EXECUTE_READWRITE, &dwProtect);
			Memory::WriteOffsetValue(address, var);
			VirtualProtect((void*)address, 4, dwProtect, &dwProtect);
		}

		template<typename Var, typename AT>
		inline void		ReadOffsetValue(AT address, Var& var)
		{
			Memory::ReadOffsetValue(address, var);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook)
		{
			DWORD		dwProtect;

			VirtualProtect((void*)((DWORD_PTR)address + 1), 4, PAGE_EXECUTE_READWRITE, &dwProtect);
			Memory::InjectHook( address, hook );
			VirtualProtect((void*)((DWORD_PTR)address + 1), 4, dwProtect, &dwProtect);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook, unsigned int nType)
		{
			DWORD		dwProtect;

			VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect);
			Memory::InjectHook( address, hook, nType );
			VirtualProtect((void*)address, 5, dwProtect, &dwProtect);
		}

		template<typename Func, typename AT>
		inline void		ReadCall(AT address, Func& func)
		{
			Memory::ReadCall(address, func);
		}

		template<typename AT>
		inline void*	ReadCallFrom(AT address, ptrdiff_t offset = 0)
		{
			return Memory::ReadCallFrom(address, offset);
		}

		namespace DynBase
		{
			template<typename T, typename AT>
			inline void		Patch(AT address, T value)
			{
				VP::Patch(DynBaseAddress(address), value);
			}

#ifndef _MEMORY_NO_CRT
			template<typename AT>
			inline void		Patch(AT address, std::initializer_list<uint8_t> list )
			{
				VP::Patch(DynBaseAddress(address), std::move(list));
			}
#endif

			template<typename AT>
			inline void		Nop(AT address, size_t count)
			{
				VP::Nop(DynBaseAddress(address), count);
			}

			template<typename Var, typename AT>
			inline void		WriteOffsetValue(AT address, Var var)
			{
				VP::WriteOffsetValue(DynBaseAddress(address), var);
			}

			template<typename Var, typename AT>
			inline void		ReadOffsetValue(AT address, Var& var)
			{
				VP::ReadOffsetValue(DynBaseAddress(address), var);
			}

			template<typename AT, typename HT>
			inline void		InjectHook(AT address, HT hook)
			{
				VP::InjectHook(DynBaseAddress(address), hook);
			}

			template<typename AT, typename HT>
			inline void		InjectHook(AT address, HT hook, unsigned int nType)
			{
				VP::InjectHook(DynBaseAddress(address), hook, nType);
			}

			template<typename Func, typename AT>
			inline void		ReadCall(AT address, Func& func)
			{
				Memory::ReadCall(DynBaseAddress(address), func);
			}

			template<typename AT>
			inline void*	ReadCallFrom(AT address, ptrdiff_t offset = 0)
			{
				Memory::ReadCallFrom(DynBaseAddress(address), offset);
			}
		};
	};
};

#ifndef _MEMORY_NO_CRT

#include <forward_list>
#include <tuple>
#include <memory>

namespace ScopedUnprotect
{
	class Unprotect
	{
	public:
		~Unprotect()
		{
			for ( auto& it : m_queriedProtects )
			{
				DWORD dwOldProtect;
				VirtualProtect( std::get<0>(it), std::get<1>(it), std::get<2>(it), &dwOldProtect );
			}
		}

	protected:
		Unprotect() = default;

		void UnprotectRange( DWORD_PTR BaseAddress, SIZE_T Size )
		{
			SIZE_T QueriedSize = 0;
			while ( QueriedSize < Size )
			{
				MEMORY_BASIC_INFORMATION MemoryInf;
				DWORD dwOldProtect;

				VirtualQuery( (LPCVOID)(BaseAddress + QueriedSize), &MemoryInf, sizeof(MemoryInf) );
				if ( MemoryInf.State == MEM_COMMIT && (MemoryInf.Type & MEM_IMAGE) != 0 &&
					(MemoryInf.Protect & (PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY|PAGE_READWRITE|PAGE_WRITECOPY)) == 0 )
				{
					const bool wasExecutable = (MemoryInf.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ)) != 0;
					VirtualProtect( MemoryInf.BaseAddress, MemoryInf.RegionSize, wasExecutable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, &dwOldProtect );
					m_queriedProtects.emplace_front( MemoryInf.BaseAddress, MemoryInf.RegionSize, MemoryInf.Protect );
				}
				QueriedSize += MemoryInf.RegionSize;
			}
		}

	private:
		std::forward_list< std::tuple< LPVOID, SIZE_T, DWORD > >	m_queriedProtects;
	};

	class Section : public Unprotect
	{
	public:
		Section( HINSTANCE hInstance, const char* name )
		{
			PIMAGE_NT_HEADERS		ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hInstance + ((PIMAGE_DOS_HEADER)hInstance)->e_lfanew);
			PIMAGE_SECTION_HEADER	pSection = IMAGE_FIRST_SECTION(ntHeader);

			for ( SIZE_T i = 0, j = ntHeader->FileHeader.NumberOfSections; i < j; ++i, ++pSection )
			{
				if ( strncmp( (const char*)pSection->Name, name, IMAGE_SIZEOF_SHORT_NAME ) == 0 )
				{
					const DWORD_PTR VirtualAddress = (DWORD_PTR)hInstance + pSection->VirtualAddress;
					const SIZE_T VirtualSize = pSection->Misc.VirtualSize;
					UnprotectRange( VirtualAddress, VirtualSize );

					m_locatedSection = true;
					break;
				}
			}
		};

		bool	SectionLocated() const { return m_locatedSection; }

	private:
		bool	m_locatedSection = false;
	};

	class FullModule : public Unprotect
	{
	public:
		FullModule( HINSTANCE hInstance )
		{
			PIMAGE_NT_HEADERS		ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hInstance + ((PIMAGE_DOS_HEADER)hInstance)->e_lfanew);
			UnprotectRange( (DWORD_PTR)hInstance, ntHeader->OptionalHeader.SizeOfImage );
		}
	};

	inline std::unique_ptr<Unprotect> UnprotectSectionOrFullModule( HINSTANCE hInstance, const char* name )
	{
		std::unique_ptr<Section> section = std::make_unique<Section>( hInstance, name );
		if ( !section->SectionLocated() )
		{
			return std::make_unique<FullModule>( hInstance );
		}
		return section;
	}
};

#endif

#endif

#endif