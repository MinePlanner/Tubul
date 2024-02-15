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
#include <algorithm>
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

#include <cstdio>
#include <cstdlib>



namespace TU{

std::string bytesToStr(size_t value_in_bytes)
{
	std::vector<std::string> units ={"b", "kb", "mb", "gb"};
	std::stringstream buffer;
	buffer << std::setprecision(2) << std::fixed;
	//This starts in bytes;
	double value = value_in_bytes;
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
	using PtrSize = std::pair<void*,size_t>;
	std::atomic_size_t lifetime;
	std::atomic_size_t alive;
	std::vector<PtrSize> sizes;

	auto findArrayAlloc(void *ptr) {
		return std::find_if(std::begin(sizes), std::end(sizes),
		                    [ptr](auto sizeItem) {
			                    const auto &[p,s] = sizeItem;
			                    return (p == ptr);
		                    });
	}

	void removeArrayAlloc(std::vector<PtrSize>::iterator it) {
		//Move the iterator to the last position and drop it.
		std::iter_swap(it, sizes.end() - 1);
		sizes.pop_back();
	}
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


/** Here are Tubul's custom implementations of new and delete to try to
 * keep track of the allocated memory. Even though this looked relatively simple
 * it turns out there's a lot of small details to keep in mind
 * 1) First of all, in clang the sized deallocation are not the default! Add
 * -fsized-deallocations when using this.
 * 2) Imho, the array deallocation is kind of broken in clang and gcc. When you
 * allocate an array of N elements you know the size (and underlying knowledge remains too!),
 * but there's a caveat if the elements are trivial pods... in that case the compiler
 * is free to call the non-sized deallocator!!! That is quite a pain because to properly
 * get back the size of that allocation would require either risky assumptions (basically
 * that we can use the trick of looking at position -1 and retrieve the size) or a lot of
 * extra work to keep knowledge of this. I did a small solution here for completeness-sake
 * but i am not very happy with it.
 * 3) new and sized delete for single objects seems to work as expected. In particular,
 * std::vector saves the day here as it doesn't use new[], because it has to work which
 * elements are "alive" and which are not.
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


// no inline, required by [replacement.functions]/3
void* operator new(std::size_t sz)
{
	// avoid std::malloc(0) which may return nullptr on success
	if (sz == 0)
		++sz;
	//get the stats and add the allocation.
	auto& stats = TU::getTubulStats();
	auto total = stats.lifetime.fetch_add(sz);
	auto cur = stats.alive.fetch_add(sz);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("1) new(size_t), size = %zu alive = %zu total alloc'd= %zu\n", sz, cur+sz, total+sz);
	//return the allocated memory
	if (void *ptr = std::malloc(sz))
		return ptr;

	throw std::bad_alloc{}; // required by [new.delete.single]/3
}

// no inline, required by [replacement.functions]/3
void* operator new[](std::size_t sz)
{
	// avoid std::malloc(0) which may return nullptr on success
	if (sz == 0)
		++sz;
	//get the stats and add the allocation.
	auto& stats = TU::getTubulStats();
	auto total = stats.lifetime.fetch_add(sz);
	auto cur = stats.alive.fetch_add(sz);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("2) new[](size_t), size = %zu alive = %zu total alloc'd= %zu\n", sz, cur+sz, total+sz);

	//Return the ptr allocated, but just in case (because this route sucks) store the size of this ptr.
	if (void *ptr = std::malloc(sz)) {
		if ( not stats.sizes.capacity())
			stats.sizes.reserve(512);
		stats.sizes.emplace_back(ptr, sz);
		return ptr;
	}

	throw std::bad_alloc{}; // required by [new.delete.single]/3
}

void operator delete(void* ptr, std::size_t size) noexcept
{
	//Simply record the we are freen size amount of bytes.
	auto& stats = TU::getTubulStats();
	auto cur = stats.alive.fetch_sub(size);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("4) delete(void*, size_t), size = %zu alive = %zu  ptr = %p\n", size, cur -size, ptr);
	//Call the real deallocation.
	std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
	//Now, because the array handling sucks, we have to try to retrieve the size of this
	//pointer for accurate bookkeeping.
	auto& stats = TU::getTubulStats();
	auto ptrInfo = stats.findArrayAlloc(ptr);
	//If for some reason, we don't have information for this ptr, we panic a little and go on with life.
	if ( ptrInfo == stats.sizes.end() ) {
		std::printf("Deleting pointer for which no size information is recorded!");
		std::free(ptr);
		return;
	}
	//We can get the size of this ptr to properly track it.
	const auto& [p,s] = *ptrInfo;
	auto cur = stats.alive.fetch_sub(s);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("****) delete[](void*), alive = %zu \n", cur-s);
	std::free(ptr);

	//now we remove this ptr from the list of pointers we keep track of.
	stats.removeArrayAlloc(ptrInfo);
}

void operator delete[](void* ptr, std::size_t size) noexcept
{
	auto& stats = TU::getTubulStats();
	auto cur = stats.alive.fetch_sub(size);
	if constexpr (TUBUL_LOG_ALLOCATIONS)
		std::printf("6) delete[](void*, size_t), size = %zu alive = %zu\n", size, cur-size);
	std::free(ptr);

	//remove the array data if it was recorded
	auto ptrInfo = stats.findArrayAlloc(ptr);
	if ( ptrInfo != stats.sizes.end())
		stats.removeArrayAlloc(ptrInfo);
}
