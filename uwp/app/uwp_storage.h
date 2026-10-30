#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace xps3::storage
{
enum class device_kind
{
    internal,
    external,
    custom,
};

enum class validation_state
{
    unknown,
    ready,
    missing,
    read_only,
    insufficient_space,
};

struct profile
{
    std::string id;
    std::string name;
    std::string root;
    std::string volume_id;
    device_kind device = device_kind::custom;
    validation_state state = validation_state::unknown;
    std::uint64_t free_bytes = 0;
    std::uint64_t total_bytes = 0;
};

struct library_root
{
    std::string path;
    std::string profile_id;
    bool enabled = true;
};

struct config
{
    std::uint32_t schema = 1;
    std::string active_profile_id;
    std::string install_profile_id;
    std::vector<profile> profiles;
    std::vector<library_root> libraries;
    std::string cache_root;
    std::string log_root;
    std::string cover_root;
};

struct validation_result
{
    validation_state state = validation_state::unknown;
    std::uint64_t free_bytes = 0;
    std::uint64_t total_bytes = 0;
    std::string message;
};

config make_default(std::string_view local_state_root);
bool load(std::string_view file, config& value, std::string& warning);
bool save(std::string_view file, const config& value, std::string& error);
validation_result validate_profile(const profile& value, std::uint64_t required_bytes);
std::string normalize_root(std::string_view value);
std::string stable_content_key(std::string_view title_id, std::string_view content_path);
}
