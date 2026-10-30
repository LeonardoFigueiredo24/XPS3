#include "uwp_preparation.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
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

std::string lower_extension(const std::filesystem::path& path)
{
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension;
}

bool readable_file(const std::filesystem::path& path, std::uint64_t& size)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec)
    {
        return false;
    }
    size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    return static_cast<bool>(input);
}
}

verification_result verify_content(std::string_view path_text)
{
    verification_result result;
    if (path_text.empty())
    {
        result.error_code = "CONTENT_MISSING";
        result.message = "Nenhum conteudo foi selecionado.";
        return result;
    }

    const std::filesystem::path path{std::string(path_text)};
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec)
    {
        result.error_code = "CONTENT_MISSING";
        result.message = "O arquivo ou pasta selecionado nao esta disponivel.";
        return result;
    }

    if (std::filesystem::is_directory(path, ec) && !ec)
    {
        const auto extracted_eboot = path / "PS3_GAME" / "USRDIR" / "EBOOT.BIN";
        const auto direct_eboot = path / "USRDIR" / "EBOOT.BIN";
        std::uint64_t executable_size = 0;
        if (readable_file(extracted_eboot, executable_size))
        {
            result.ok = true;
            result.kind = content_kind::extracted_game;
            result.readable_bytes = executable_size;
            result.executable_candidates.push_back(extracted_eboot.string());
            result.message = "Pasta PS3_GAME verificada.";
            return result;
        }
        if (readable_file(direct_eboot, executable_size))
        {
            result.ok = true;
            result.kind = content_kind::installed_title;
            result.readable_bytes = executable_size;
            result.executable_candidates.push_back(direct_eboot.string());
            result.message = "Titulo instalado verificado.";
            return result;
        }
        result.error_code = "EBOOT_MISSING";
        result.message = "A pasta nao contem PS3_GAME/USRDIR/EBOOT.BIN.";
        return result;
    }

    std::uint64_t file_size = 0;
    if (!readable_file(path, file_size))
    {
        result.error_code = "CONTENT_UNREADABLE";
        result.message = "O conteudo existe, mas nao pode ser lido.";
        return result;
    }
    result.readable_bytes = file_size;

    const auto extension = lower_extension(path);
    if (extension == ".pkg")
    {
        result.kind = content_kind::package;
        result.error_code = "INSTALL_REQUIRED";
        result.message = "Pacotes PKG precisam ser instalados antes da inicializacao.";
        return result;
    }
    if (extension == ".iso")
    {
        result.ok = true;
        result.kind = content_kind::iso;
        result.message = "Imagem ISO legivel; a estrutura sera validada pelo leitor ISO do nucleo.";
        return result;
    }

    if (extension == ".elf" || extension == ".self" || extension == ".bin")
    {
        std::array<unsigned char, 4> magic{};
        std::ifstream input(path, std::ios::binary);
        input.read(reinterpret_cast<char*>(magic.data()), static_cast<std::streamsize>(magic.size()));
        if (input.gcount() == static_cast<std::streamsize>(magic.size()) &&
            magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F')
        {
            result.ok = true;
            result.kind = content_kind::elf;
            result.executable_candidates.push_back(path.string());
            result.message = "Executavel ELF verificado.";
            return result;
        }
        result.error_code = "EXECUTABLE_HEADER_UNSUPPORTED";
        result.message = "O arquivo nao possui um cabecalho ELF reconhecido.";
        return result;
    }

    result.error_code = "CONTENT_FORMAT_UNSUPPORTED";
    result.message = "O formato selecionado nao e uma entrada de jogo suportada.";
    return result;
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
