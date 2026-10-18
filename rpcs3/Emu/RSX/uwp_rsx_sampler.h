#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

namespace rsx
{
#ifdef RPCS3_UWP
	// Xbox hitch sampler (RSXThread.cpp): a helper thread reads every 1 ms what the RSX thread is
	// doing, as phase << 16 | the FIFO method being run, and logs where slow frames spent their time.
	enum uwp_rsx_phase : u32
	{
		uwp_phase_fifo = 1, uwp_phase_fifo_idle, uwp_phase_method, uwp_phase_flip, uwp_phase_local_task,
		uwp_phase_zcull, uwp_phase_fifo_wait, uwp_phase_ext_pause, uwp_phase_semaphore, uwp_phase_frame_limit,
		uwp_phase_submit, uwp_phase_query, uwp_phase_fence, uwp_phase_invalidate, uwp_phase_fifo_fetch, uwp_phase_fifo_spin,
		uwp_phase_count
	};

	extern atomic_t<u32> g_uwp_rsx_where;

	// FIFO work counters, written by the RSX thread only (logged per slow frame and per 10 s).
	struct uwp_fifo_counters
	{
		u64 commands = 0;  // commands run (methods and register writes)
		u64 refills = 0;   // command cache refills from guest memory
		u64 refill_bytes = 0;
		u64 retries = 0;   // cache lines re-read because the guest was writing them
		u64 spins = 0;     // 200-cycle waits for such lines
		u64 jumps = 0, calls = 0, empty = 0;
	};

	extern uwp_fifo_counters g_uwp_fifo;
	extern thread_local bool g_uwp_on_rsx_thread;

	// Marks a phase on the RSX thread (no-op elsewhere), keeping the method it runs inside.
	struct uwp_rsx_scope
	{
		u32 prev = 0;
		bool active;

		uwp_rsx_scope(uwp_rsx_phase phase) : active(g_uwp_on_rsx_thread)
		{
			if (active)
			{
				prev = g_uwp_rsx_where.observe();
				g_uwp_rsx_where.release((u32{phase} << 16) | (prev & 0xffff));
			}
		}

		~uwp_rsx_scope()
		{
			if (active)
				g_uwp_rsx_where.release(prev);
		}
	};
#endif
}
