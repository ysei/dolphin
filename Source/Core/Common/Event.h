// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

// Multithreaded event class. This allows waiting in a thread for an event to
// be triggered in another thread. While waiting, the CPU will be available for
// other tasks.
// * Set(): triggers the event and wakes up the waiting thread.
// * Wait(): waits for the event to be triggered.
// * Reset(): tries to reset the event before the waiting thread sees it was
//            triggered. Usually a bad idea.

#pragma once

#ifdef _WIN32
#include <concrt.h>
#endif

#include "Common/Flag.h"
#include "Common/StdConditionVariable.h"
#include "Common/StdMutex.h"

namespace Common {

// Windows uses a specific implementation because std::condition_variable has
// terrible performance for this kind of workload with MSVC++ 2013.
#ifndef _WIN32
class Event final
{
public:
	void Set()
	{
		if (m_flag.TestAndSet())
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_condvar.notify_one();
		}
	}

	void Wait()
	{
		if (m_flag.TestAndClear())
			return;

		std::unique_lock<std::mutex> lk(m_mutex);
		m_condvar.wait(lk, [&]{ return m_flag.IsSet(); });
		m_flag.Clear();
	}

	void Reset()
	{
		// no other action required, since wait loops on
		// the predicate and any lingering signal will get
		// cleared on the first iteration
		m_flag.Clear();
	}

private:
	Flag m_flag;
	std::condition_variable m_condvar;
	std::mutex m_mutex;
};
#else
class Event final
{
public:
	void Set() { m_event.set(); }
	void Wait() { m_event.wait(); m_event.reset(); }
	void Reset() { m_event.reset(); }

private:
	concurrency::event m_event;
};
#endif

}  // namespace Common
