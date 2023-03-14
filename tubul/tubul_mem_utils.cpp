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

std::string read_file_to_string(const std::string& filename)
{
	std::ifstream proc_file( filename );
	std::string str((std::istreambuf_iterator<char>(proc_file)),
                    std::istreambuf_iterator<char>());
	return str;
}

std::string bytes_to_string(size_t value_in_bytes)
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
	auto stats = read_file_to_string("/proc/self/statm");
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
	auto status_procfile = read_file_to_string("/proc/self/status");
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

std::string memPeakRSS()
{
#if defined(TUBUL_MACOS)
	return bytes_to_string( getApplePeakRSS() );
#elif defined(TUBUL_LINUX)
	return bytes_to_string( getLinuxPeakRSS() );
#elif defined(TUBUL_WINDOWS)
	return bytes_to_string( getWindowsPeakRSS() );
#endif
}

std::string memCurrentRSS()
{
#if defined(TUBUL_MACOS)
	return bytes_to_string( getAppleRSS() );
#elif defined(TUBUL_LINUX)
	return bytes_to_string( getLinuxRSS() );
#elif defined(TUBUL_WINDOWS)
	return bytes_to_string( getWindowsRSS() );
#endif

}

}