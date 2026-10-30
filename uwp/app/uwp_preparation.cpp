#include "uwp_preparation.h"

#include <algorithm>
#include <array>
#include <utility>

namespace xps3::preparation
{
namespace
{
constexpr std::array<std::pair<phase, const char*>, 8> s_phases{{
    {phase::verify_content, "Verificando conteudo"},
    {phase::analyse_executables, "Analisando executaveis"},
    {phase::compile_ppu, "Compilando PPU"},
    {phase::prepare_spu, "Preparando SPU"},
    {phase::load_shader_cache, "Carregando cache de shaders"},
    {phase::prepare_pipelines, "Preparando pipelines"},
    {phase::start_emulation, "Iniciando emulacao"},
    {phase::wait_first_frame, "Aguardando primeiro quadro"},
}};
}

std::uint64_t controller::begin(request value)
{
    std::lock_guard lock(m_mutex);
    m_snapshot = {};
    m_snapshot.serial = m_next_serial++;
    m_snapshot.active = true;
    m_snapshot.game = std::move(value);
    m_snapshot.current = phase::verify_content;
    m_snapshot.can_cancel = true;
    m_snapshot.phases.reserve(s_phases.size());

    for (const auto& [id, label] : s_phases)
    {
        m_snapshot.phases.push_back({.id = id, .label = label});
    }

    m_snapshot.phases.front().state = phase_state::active;
    m_started = std::chrono::steady_clock::now();
    return m_snapshot.serial;
}

void controller::cancel(std::uint64_t serial)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial))
    {
        return;
    }

    if (auto* current = find_phase(m_snapshot.current))
    {
        current->state = phase_state::cancelled;
    }
    finish(phase::cancelled);
}

void controller::activate(std::uint64_t serial, phase value, std::string detail)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial))
    {
        return;
    }

    auto* item = find_phase(value);
    if (!item || item->state != phase_state::pending)
    {
        return;
    }

    item->state = phase_state::active;
    item->detail = std::move(detail);
    m_snapshot.current = value;
}

void controller::update(std::uint64_t serial, phase value, std::uint64_t done,
    std::uint64_t total, std::string detail)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial))
    {
        return;
    }

    auto* item = find_phase(value);
    if (!item || (item->state != phase_state::active && item->state != phase_state::pending))
    {
        return;
    }

    item->state = phase_state::active;
    item->total = total;
    item->done = total == 0 ? done : std::min(done, total);
    item->indeterminate = total == 0;
    item->detail = std::move(detail);
    m_snapshot.current = value;
}

void controller::complete(std::uint64_t serial, phase value, std::string detail)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial))
    {
        return;
    }

    auto* item = find_phase(value);
    if (!item || (item->state != phase_state::active && item->state != phase_state::pending))
    {
        return;
    }

    item->state = phase_state::complete;
    item->detail = std::move(detail);
    if (item->total > 0)
    {
        item->done = item->total;
        item->indeterminate = false;
    }
}

void controller::skip(std::uint64_t serial, phase value, std::string reason)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial) || reason.empty())
    {
        return;
    }

    auto* item = find_phase(value);
    if (!item || item->state != phase_state::pending)
    {
        return;
    }

    item->state = phase_state::skipped;
    item->detail = std::move(reason);
}

void controller::fail(std::uint64_t serial, std::string code, std::string message)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial))
    {
        return;
    }

    if (auto* current = find_phase(m_snapshot.current))
    {
        current->state = phase_state::failed;
        current->detail = message;
    }
    m_snapshot.error_code = std::move(code);
    m_snapshot.error_message = std::move(message);
    finish(phase::failed);
}

void controller::first_frame_presented(std::uint64_t serial)
{
    std::lock_guard lock(m_mutex);
    if (!accepts(serial))
    {
        return;
    }

    const auto* started = find_phase(phase::start_emulation);
    auto* waiting = find_phase(phase::wait_first_frame);
    if (!started || started->state != phase_state::complete || !waiting ||
        waiting->state != phase_state::active)
    {
        return;
    }

    waiting->state = phase_state::complete;
    waiting->detail = "Primeiro quadro apresentado";
    finish(phase::complete);
}

snapshot controller::get_snapshot() const
{
    std::lock_guard lock(m_mutex);
    auto result = m_snapshot;
    if (result.serial != 0)
    {
        result.elapsed_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_started)
                .count());
    }
    return result;
}

phase_progress* controller::find_phase(phase value)
{
    const auto found = std::find_if(m_snapshot.phases.begin(), m_snapshot.phases.end(),
        [value](const phase_progress& item) { return item.id == value; });
    return found == m_snapshot.phases.end() ? nullptr : &*found;
}

const phase_progress* controller::find_phase(phase value) const
{
    const auto found = std::find_if(m_snapshot.phases.begin(), m_snapshot.phases.end(),
        [value](const phase_progress& item) { return item.id == value; });
    return found == m_snapshot.phases.end() ? nullptr : &*found;
}

bool controller::accepts(std::uint64_t serial) const
{
    return m_snapshot.active && serial == m_snapshot.serial;
}

void controller::finish(phase terminal)
{
    m_snapshot.active = false;
    m_snapshot.can_cancel = false;
    m_snapshot.current = terminal;
}
}
