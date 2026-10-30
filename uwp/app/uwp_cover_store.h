#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace xps3::covers
{
inline constexpr std::uintmax_t max_encoded_bytes = 16u * 1024u * 1024u;

enum class source_kind
{
    central_png,
    central_jpg,
    embedded_icon,
    sibling_named,
    sibling_cover,
    sibling_icon0,
    placeholder,
};

struct lookup_result
{
    std::string path;
    source_kind source = source_kind::placeholder;
    std::string warning;
};

lookup_result resolve(std::string_view cover_root, std::string_view title_id,
    std::string_view content_path, std::string_view embedded_icon_path = {});
}
