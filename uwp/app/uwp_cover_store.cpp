#include "uwp_cover_store.h"

#include "uwp_storage.h"

#include <array>
#include <filesystem>
#include <system_error>

namespace xps3::covers
{
namespace
{
struct candidate
{
    std::filesystem::path path;
    source_kind source;
};

bool usable_file(const candidate& value, std::string& warning)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(value.path, ec) || ec)
    {
        return false;
    }
    const auto size = std::filesystem::file_size(value.path, ec);
    if (ec)
    {
        warning = "Nao foi possivel consultar o tamanho da capa: " + value.path.string();
        return false;
    }
    if (size == 0)
    {
        warning = "A capa esta vazia: " + value.path.string();
        return false;
    }
    if (size > max_encoded_bytes)
    {
        warning = "A capa excede o limite de 16 MiB: " + value.path.string();
        return false;
    }
    return true;
}
}

lookup_result resolve(std::string_view cover_root, std::string_view title_id,
    std::string_view content_path, std::string_view embedded_icon_path)
{
    const auto key = storage::stable_content_key(title_id, content_path);
    const std::filesystem::path central{storage::normalize_root(cover_root)};
    const std::filesystem::path content{storage::normalize_root(content_path)};
    std::error_code content_ec;
    const bool content_is_directory = std::filesystem::is_directory(content, content_ec) && !content_ec;
    const auto parent = content_is_directory
        ? content
        : (content.has_parent_path() ? content.parent_path() : std::filesystem::path{});
    const auto stem = content_is_directory ? content.filename().string() : content.stem().string();

    const std::array<candidate, 6> candidates{{
        {central / (key + ".png"), source_kind::central_png},
        {central / (key + ".jpg"), source_kind::central_jpg},
        {std::filesystem::path{storage::normalize_root(embedded_icon_path)}, source_kind::embedded_icon},
        {parent / (stem + ".png"), source_kind::sibling_named},
        {parent / "cover.png", source_kind::sibling_cover},
        {parent / "ICON0.PNG", source_kind::sibling_icon0},
    }};

    lookup_result result;
    for (const auto& item : candidates)
    {
        if (item.path.empty())
        {
            continue;
        }
        if (usable_file(item, result.warning))
        {
            result.path = item.path.string();
            result.source = item.source;
            result.warning.clear();
            return result;
        }
    }
    return result;
}
}
