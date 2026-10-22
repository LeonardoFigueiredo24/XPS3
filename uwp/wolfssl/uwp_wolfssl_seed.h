/* Forced into every wolfSSL source on UWP (see 3rdparty/wolfssl/CMakeLists.txt): wolfCrypt's
 * random seed comes from uwp_wolfssl_seed() instead of the desktop CryptoAPI
 * (CryptAcquireContext/CryptGenRandom), which UWP apps aren't meant to use. */
#pragma once

#define CUSTOM_RAND_GENERATE_SEED uwp_wolfssl_seed

#ifdef __cplusplus
extern "C"
#endif
int uwp_wolfssl_seed(unsigned char* output, unsigned int size);
