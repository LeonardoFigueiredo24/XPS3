#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace xps3::preparation
{
enum class phase
{
    verify_content,
    analyse_executables,
    compile_ppu,
    prepare_spu,
    load_shader_cache,
    prepare_pipelines,
    start_emulation,
    wait_first_frame,
    complete,
    failed,
    cancelled,
};

enum class phase_state
{
    pending,
    active,
    complete,
    failed,
    skipped,
    cancelled,
};

enum class mode
{
    quick,
    balanced,
    smoothness,
};

struct phase_progress
{
    phase id = phase::verify_content;
    phase_state state = phase_state::pending;
    std::uint64_t done = 0;
    std::uint64_t total = 0;
    bool indeterminate = true;
    std::string label;
    std::string detail;
};

struct request
{
    std::string title;
    std::string title_id;
    std::string content_path;
    mode preparation_mode = mode::balanced;
};

struct snapshot
{
    std::uint64_t serial = 0;
    bool active = false;
    request game;
    std::vector<phase_progress> phases;
    phase current = phase::verify_content;
    std::uint64_t elapsed_ms = 0;
    bool can_cancel = false;
    std::string error_code;
    std::string error_message;
};

class controller
{
public:
    std::uint64_t begin(request value);
    void cancel(std::uint64_t serial);
    void activate(std::uint64_t serial, phase value, std::string detail = {});
    void update(std::uint64_t serial, phase value, std::uint64_t done,
        std::uint64_t total, std::string detail = {});
    void complete(std::uint64_t serial, phase value, std::string detail = {});
    void skip(std::uint64_t serial, phase value, std::string reason);
    void fail(std::uint64_t serial, std::string code, std::string message);
    void first_frame_presented(std::uint64_t serial);
    snapshot get_snapshot() const;

private:
    phase_progress* find_phase(phase value);
    const phase_progress* find_phase(phase value) const;
    bool accepts(std::uint64_t serial) const;
    void finish(phase terminal);

    mutable std::mutex m_mutex;
    snapshot m_snapshot;
    std::chrono::steady_clock::time_point m_started{};
    std::uint64_t m_next_serial = 1;
};
}
