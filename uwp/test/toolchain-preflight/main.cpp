#include <windows.h>

#include <cstdint>

std::uint32_t xps3_uwp_toolchain_preflight() noexcept
{
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    return info.dwPageSize;
}
