//
// Created by Carlos Acosta on 13-02-24.
//

#pragma once
#include <memory>


namespace TU
{

	/** This structure provides an easy way to maintain a memory monitoring system.
	 * It will create a thread that will wake up every 0.5s and write to a file the
	 * current rss and peakrss at that point. The filename is provided.
	 * The memory monitor will auto-clean after itself, meaning that as soon as the
	 * object goes out of scope, it will close the file and wait for the thread
	 * to properly exit. Do note this may cause a delay given the way the thread
	 * communicates that it has finished.
	 */
    struct MemoryMonitor {
        struct Impl;

        explicit MemoryMonitor(const std::string& reportFileName);
        ~MemoryMonitor();

        std::unique_ptr<Impl> impl_;
    };

	std::string memCurrentRSS();
	std::string memPeakRSS();
}
