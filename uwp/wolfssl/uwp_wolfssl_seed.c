/* wolfCrypt seed source for UWP: BCryptGenRandom, which the app already uses for the PS3's
 * sys_ss random numbers (Emu/Cell/lv2/sys_ss.cpp). Returns 0 on success, as wolfCrypt expects. */

#include <windows.h>
#include <bcrypt.h>

#include "uwp_wolfssl_seed.h"

int uwp_wolfssl_seed(unsigned char* output, unsigned int size)
{
	return BCRYPT_SUCCESS(BCryptGenRandom(NULL, output, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG)) ? 0 : -1;
}
