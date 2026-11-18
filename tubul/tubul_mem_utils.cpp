//
// Created by Carlos Acosta on 27-02-23.
//
#include <array>
#include <fstream>
#include <sstream>
#include <charconv>
#include <vector>
#include <iomanip>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "tubul_file_utils.h"
#include "tubul_mem_utils.h"

#if defined(TUBUL_MACOS)
#include <mach/mach.h>
#elif defined(TUBUL_LINUX)
#include <unistd.h>
#include <sys/resource.h>
#elif defined(TUBUL_WINDOWS)
#include <windows.h>
#include <psapi.h>
#endif



namespace TU{

std::string bytesToStr(size_t value_in_bytes)
{
	std::vector<std::string> units ={"b", "kb", "mb", "gb"};
	std::stringstream buffer;
	buffer << std::setprecision(2) << std::fixed;
	//This starts in bytes;
	double value = static_cast<double>(value_in_bytes);
	for (auto const& unit: units)
	{
		if (value < 1024)
		{
			buffer << value << unit;
			return buffer.str();
		}
		value /= 1024;
	}
	buffer << value << "gb";
	return buffer.str();

}

#if defined(TUBUL_LINUX)
	/* /proc/[pid]/statm
 The columns are:
	size       (1) total program size (same as VmSize in /proc/[pid]/status)
	resident   (2) resident set size (same as VmRSS in /proc/[pid]/status)
	share      (3) shared pages (i.e., backed by a file)
	text       (4) text (code)
	lib        (5) library (unused in Linux 2.6)
	data       (6) data + stack
	dt         (7) dirty pages (unused in Linux 2.6)
	 */

size_t getLinuxRSS()
{
	//The statsm proc file contains the memory as page counts, so
	//we need the page size to accurately measure this as bytes
	auto stats = readToString("/proc/self/statm");
	auto page_size = sysconf(_SC_PAGE_SIZE);

	auto first_spc = stats.find_first_of(' ');
	auto second_spc = stats.find_first_of(' ', first_spc+1);
	size_t rss = 0;
	auto [ptr,ec ] = std::from_chars(stats.data()+first_spc+1, stats.data()+second_spc, rss );

	if (ec != std::errc())
		throw std::logic_error("Couldn't parse rss");

	return rss*page_size;

}



size_t getLinuxPeakRSS()
{
	auto status_procfile = readToString("/proc/self/status");
	//Get the line containing "VmHWM:"
	std::istringstream file_contents( std::move(status_procfile) );
	std::string line;
	while ( std::getline(file_contents, line) )
	{
		if ( not line.starts_with("VmHWM:"))
			continue ;
		auto first_nonspace = line.find_first_not_of(" ",7 );
		size_t hwm = 0;
		auto [ptr,ec ] = std::from_chars(line.data()+first_nonspace, line.data()+line.size()-2, hwm );

		if (ec != std::errc())
			throw std::logic_error("Couldn't parse rss");
		//Because the hwm is measured in kb
		return hwm * 1024;
	}
	return 0 ;
}

#endif

#if defined(TUBUL_MACOS)
size_t getAppleRSS()
{
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
					(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;
	return (size_t)info.resident_size;
}

size_t getApplePeakRSS()
{
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
					(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;
	return (size_t)info.resident_size_max;
}
#endif

#if defined(TUBUL_WINDOWS)
size_t getWindowsPeakRSS( )
{
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.PeakWorkingSetSize;
}

size_t getWindowsRSS( )
{
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return (size_t)info.WorkingSetSize;
}

#endif


size_t memPeakRSS()
{
#if defined(TUBUL_MACOS)
	return getApplePeakRSS();
#elif defined(TUBUL_LINUX)
	return getLinuxPeakRSS();
#elif defined(TUBUL_WINDOWS)
	return getWindowsPeakRSS();
#endif
}

size_t memCurrentRSS()
{
#if defined(TUBUL_MACOS)
	return getAppleRSS();
#elif defined(TUBUL_LINUX)
	return getLinuxRSS();
#elif defined(TUBUL_WINDOWS)
	return getWindowsRSS();
#endif

}

struct MemoryStats {
	std::atomic_size_t lifetime;
	std::atomic_size_t alive;
};

MemoryStats& getTubulStats() {
	static MemoryStats stats;
	return stats;
}

size_t memAlive() {
	const auto& stat = getTubulStats();
	return stat.alive.load();
}

size_t memLifetime() {
	const auto& stat = getTubulStats();
	return stat.lifetime.load();
}

/**
 * This is the private implementation of the memory monitoring thread.
 * It's actually quite simple using std::thread ability to run a given
 * object's method on a thread. We create an outputfile, and Impl
 * has an atomic flag (just to be sure there's no funny optimizations
 * if we were to use a naked bool).
 * The memReportWorker function will keep writing a new line
 * containing the rss and peak rss at that point and then sleeping 0.5s until
 * the exit flag is marked.
 * The exit flag is marked when Impl goes out of scope. Then the destructor
 * waits for the thread to join and life continues. The file should close
 * automatically too.
 */
struct MemoryMonitor::Impl{

	explicit Impl(const std::string& f ):
		report_(f)
	{
		th_ = std::thread(&Impl::memReportWorker, this);
	}

	void memReportWorker() {
		//Create the file and start logging every so many millisecs
		while ( !threadExitFlag_.test() )
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for( 500ms );
			report_ << memCurrentRSS() << "," << memPeakRSS() << '\n';
		}
	}

	~Impl() {
		threadExitFlag_.test_and_set();
		if(th_.joinable()) {
			th_.join(); // join() in destructor
		}
	}

private:
	std::atomic_flag threadExitFlag_ = ATOMIC_FLAG_INIT;
	std::thread th_;
	std::ofstream report_;
};

/**
 * The construct or of memory monitor pretty much just creates the private implementation.
 */
MemoryMonitor::MemoryMonitor(const std::string& reportFileName):
	impl_(std::make_unique<Impl>(reportFileName))
	{}

MemoryMonitor::~MemoryMonitor() = default;



}


#ifdef TUBUL_OPERATOR_NEW_OVERRIDE
/** Here are Tubul's custom implementations of new and delete to try to
 * keep track of the allocated memory. Even though this looked relatively simple
 * it turns out there's a lot of small details to keep in mind
 * 1) First of all, in clang the sized deallocation are not the default! Add
 * -fsized-deallocations when using this.
 * 2) Imho, the deallocation is kind of broken in clang and gcc. When you allocate an
 * array of N elements you know the size (and underlying knowledge remains too!),
 * but there's a caveat if the elements are trivial pods... in that case the compiler
 * is free to call the non-sized deallocator!!! The same happens for single objects:
 * libstdc++ itself frees through the unsized operator delete (an ostringstream growing
 * its buffer, for example). So every allocation, array or not, keeps its size in a
 * header of kHeader bytes (__STDCPP_DEFAULT_NEW_ALIGNMENT__, so that the memory after it
 * keeps that alignment) just before the memory it returns, and every delete form
 * reads it there. The size a sized delete is given is ignored: the header is the one
 * source of truth, so all the delete forms account exactly the same bytes.
 * The header is not counted in alive/lifetime, only the requested bytes are.
 * 3) std::vector saves the day for arrays of non-trivial things as it doesn't use new[],
 * because it has to work which elements are "alive" and which are not.
 * 4) A process must have one copy of these operators. Memory from the new (or new[]) of
 * one copy (a Windows DLL, or a Linux .so built with hidden symbols, each with its own
 * Tubul) freed by the delete of another frees a pointer kHeader bytes off, and corrupts
 * the heap. Linking Tubul into the program once, or into one shared library that the
 * others use, keeps a single copy.
 * 5) The over-aligned forms (the ones with std::align_val_t, for types aligned more than
 * __STDCPP_DEFAULT_NEW_ALIGNMENT__) aren't replaced, so the memory they allocate isn't
 * counted. They are paired with each other by the library, so nothing breaks.
 *
 * If you need to keep track of allocations going in the program, you can uncomment this
 * define and it will uncomment the log-to-console lines. Also can do it through compile defs.
 */
//#define TUBUL_ALLOCATIONS_PRINT
#ifdef TUBUL_ALLOCATIONS_PRINT
static constexpr bool TUBUL_LOG_ALLOCATIONS = true;
#else
static constexpr bool TUBUL_LOG_ALLOCATIONS = false;
#endif


// TODO: when malloc fails, operator new and new[] throw std::bad_alloc at once.
// The standard ones first call the handler installed with std::set_new_handler,
// if any, and try again until it succeeds, throws or aborts (the handler may free
// memory, for example). Nothing in Tubul installs one, so this only matters to a
// program that does; calling it would change what such a program sees, so it is
// left for when one needs it.
// The bytes before each allocation that hold its size (see 2) above).
static constexpr std::size_t kHeader = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
static_assert(kHeader >= sizeof(std::size_t));

// Allocates sz bytes preceded by the header, and counts them. `form` is only for the log.
static void* trackedAlloc(std::size_t sz, [[maybe_unused]] const char* form)
{
	// avoid std::malloc(0) which may return nullptr on success. A zero-length request
	// counts as 1 byte, for single objects and arrays alike.
	if (sz == 0)
		++sz;
	if (sz > SIZE_MAX - kHeader)
		throw std::bad_alloc{};
	void *block = std::malloc(kHeader + sz);
	if (block == nullptr)
		throw std::bad_alloc{}; // required by [new.delete.single]/3
	//Keep the size in the header, before the memory we return.
	std::memcpy(block, &sz, sizeof(sz));
	//get the stats and add the allocation, now that it succeeded.
	auto& stats = TU::getTubulStats();
	auto total = stats.lifetime.fetch_add(sz);
	auto cur = stats.alive.fetch_add(sz);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("%s(size_t), size = %zu alive = %zu total alloc'd= %zu\n", form, sz, cur+sz, total+sz);
	return static_cast<char*>(block) + kHeader;
}

// Frees a block from trackedAlloc, and takes its bytes out of alive. `form` is only for the log.
static void trackedFree(void* ptr, [[maybe_unused]] const char* form) noexcept
{
	if (ptr == nullptr)
		return;
	//The size is in the header that trackedAlloc wrote.
	char* const block = static_cast<char*>(ptr) - kHeader;
	std::size_t size = 0;
	std::memcpy(&size, block, sizeof(size));
	auto& stats = TU::getTubulStats();
	auto cur = stats.alive.fetch_sub(size);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("%s(void*), size = %zu alive = %zu  ptr = %p\n", form, size, cur-size, ptr);
	std::free(block);
}

// no inline, required by [replacement.functions]/3
void* operator new(std::size_t sz)
{
	return trackedAlloc(sz, "new");
}

//The nothrow form too: the library's default calls the one above, but a
//sanitizer or another allocator (jemalloc, mimalloc) may give its own, which
//has no header for our delete to read.
void* operator new(std::size_t sz, const std::nothrow_t&) noexcept
{
	try {
		return operator new(sz);
	} catch (const std::bad_alloc&) {
		return nullptr;
	}
}

// no inline, required by [replacement.functions]/3
void* operator new[](std::size_t sz)
{
	return trackedAlloc(sz, "new[]");
}

//The nothrow form too, for the same reason.
void* operator new[](std::size_t sz, const std::nothrow_t&) noexcept
{
	try {
		return operator new[](sz);
	} catch (const std::bad_alloc&) {
		return nullptr;
	}
}

//The unsized delete is not rare: libstdc++ calls it (an ostringstream growing its buffer,
//for example). It reads the size from the header, like every other delete form.
void operator delete(void* ptr) noexcept
{
	trackedFree(ptr, "delete");
}

//The size given is the one operator new got, which the header holds too.
void operator delete(void* ptr, std::size_t) noexcept
{
	operator delete(ptr);
}

//Paired with the nothrow operator new above.
void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
	operator delete(ptr);
}

void operator delete[](void* ptr) noexcept
{
	trackedFree(ptr, "delete[]");
}

//The size given is the one operator new[] got, which the header holds too.
void operator delete[](void* ptr, std::size_t) noexcept
{
	operator delete[](ptr);
}

//Paired with the nothrow operator new[] above.
void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
	operator delete[](ptr);
}

#endif
