#include "uwp_storage.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace xps3::storage
{
namespace
{
std::string append_path(std::string_view root, std::string_view leaf)
{
    auto result = normalize_root(root);
    if (!result.empty() && result.back() != '/')
    {
        result.push_back('/');
    }
    result.append(leaf);
    return result;
}

std::string escape_json(std::string_view value)
{
    std::string result;
    result.reserve(value.size() + 8);
    for (const unsigned char ch : value)
    {
        switch (ch)
        {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (ch < 0x20)
            {
                std::ostringstream escaped;
                escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned int>(ch);
                result += escaped.str();
            }
            else
            {
                result.push_back(static_cast<char>(ch));
            }
        }
    }
    return result;
}

const char* device_name(device_kind value)
{
    switch (value)
    {
    case device_kind::internal: return "internal";
    case device_kind::external: return "external";
    case device_kind::custom: return "custom";
    }
    return "custom";
}

bool parse_device(std::string_view value, device_kind& result)
{
    if (value == "internal") result = device_kind::internal;
    else if (value == "external") result = device_kind::external;
    else if (value == "custom") result = device_kind::custom;
    else return false;
    return true;
}

class json_reader
{
public:
    explicit json_reader(std::string_view text) : m_text(text) {}

    bool begin_object() { return token('{'); }
    bool end_object() { return token('}'); }
    bool begin_array() { return token('['); }
    bool end_array() { return token(']'); }
    bool comma() { return token(','); }
    bool colon() { return token(':'); }

    bool key(std::string_view expected)
    {
        std::string actual;
        return string(actual) && actual == expected && colon();
    }

    bool string(std::string& result)
    {
        whitespace();
        if (m_position >= m_text.size() || m_text[m_position++] != '"')
        {
            return false;
        }

        result.clear();
        while (m_position < m_text.size())
        {
            const char ch = m_text[m_position++];
            if (ch == '"')
            {
                return true;
            }
            if (ch != '\\')
            {
                if (static_cast<unsigned char>(ch) < 0x20)
                {
                    return false;
                }
                result.push_back(ch);
                continue;
            }
            if (m_position >= m_text.size())
            {
                return false;
            }
            switch (m_text[m_position++])
            {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            default: return false;
            }
        }
        return false;
    }

    bool unsigned_number(std::uint32_t& result)
    {
        whitespace();
        if (m_position >= m_text.size() || !std::isdigit(static_cast<unsigned char>(m_text[m_position])))
        {
            return false;
        }
        std::uint64_t value = 0;
        while (m_position < m_text.size() &&
               std::isdigit(static_cast<unsigned char>(m_text[m_position])))
        {
            value = value * 10 + static_cast<unsigned int>(m_text[m_position++] - '0');
            if (value > UINT32_MAX)
            {
                return false;
            }
        }
        result = static_cast<std::uint32_t>(value);
        return true;
    }

    bool boolean(bool& result)
    {
        whitespace();
        if (m_text.substr(m_position, 4) == "true")
        {
            m_position += 4;
            result = true;
            return true;
        }
        if (m_text.substr(m_position, 5) == "false")
        {
            m_position += 5;
            result = false;
            return true;
        }
        return false;
    }

    bool next_is(char expected)
    {
        whitespace();
        return m_position < m_text.size() && m_text[m_position] == expected;
    }

    bool finished()
    {
        whitespace();
        return m_position == m_text.size();
    }

private:
    bool token(char expected)
    {
        whitespace();
        if (m_position >= m_text.size() || m_text[m_position] != expected)
        {
            return false;
        }
        ++m_position;
        return true;
    }

    void whitespace()
    {
        while (m_position < m_text.size() &&
               std::isspace(static_cast<unsigned char>(m_text[m_position])))
        {
            ++m_position;
        }
    }

    std::string_view m_text;
    std::size_t m_position = 0;
};

bool parse_profile(json_reader& reader, profile& value)
{
    std::string device;
    return reader.begin_object() &&
        reader.key("id") && reader.string(value.id) && reader.comma() &&
        reader.key("name") && reader.string(value.name) && reader.comma() &&
        reader.key("root") && reader.string(value.root) && reader.comma() &&
        reader.key("volume_id") && reader.string(value.volume_id) && reader.comma() &&
        reader.key("device") && reader.string(device) && parse_device(device, value.device) &&
        reader.end_object();
}

bool parse_library(json_reader& reader, library_root& value)
{
    return reader.begin_object() &&
        reader.key("path") && reader.string(value.path) && reader.comma() &&
        reader.key("profile_id") && reader.string(value.profile_id) && reader.comma() &&
        reader.key("enabled") && reader.boolean(value.enabled) && reader.end_object();
}

bool parse_config(std::string_view text, config& result)
{
    json_reader reader(text);
    config parsed;
    if (!reader.begin_object() ||
        !reader.key("schema") || !reader.unsigned_number(parsed.schema) || !reader.comma() ||
        !reader.key("active_profile_id") || !reader.string(parsed.active_profile_id) || !reader.comma() ||
        !reader.key("install_profile_id") || !reader.string(parsed.install_profile_id) || !reader.comma() ||
        !reader.key("cache_root") || !reader.string(parsed.cache_root) || !reader.comma() ||
        !reader.key("log_root") || !reader.string(parsed.log_root) || !reader.comma() ||
        !reader.key("cover_root") || !reader.string(parsed.cover_root) || !reader.comma() ||
        !reader.key("profiles") || !reader.begin_array())
    {
        return false;
    }

    while (!reader.next_is(']'))
    {
        profile item;
        if (!parse_profile(reader, item)) return false;
        parsed.profiles.push_back(std::move(item));
        if (!reader.next_is(']') && !reader.comma()) return false;
    }
    if (!reader.end_array() || !reader.comma() ||
        !reader.key("libraries") || !reader.begin_array())
    {
        return false;
    }
    while (!reader.next_is(']'))
    {
        library_root item;
        if (!parse_library(reader, item)) return false;
        parsed.libraries.push_back(std::move(item));
        if (!reader.next_is(']') && !reader.comma()) return false;
    }
    if (!reader.end_array() || !reader.end_object() || !reader.finished())
    {
        return false;
    }

    if (parsed.schema != 1 || parsed.profiles.empty() || parsed.active_profile_id.empty() ||
        parsed.install_profile_id.empty() || parsed.cache_root.empty() || parsed.log_root.empty() ||
        parsed.cover_root.empty())
    {
        return false;
    }

    const auto has_profile = [&parsed](std::string_view id)
    {
        return std::any_of(parsed.profiles.begin(), parsed.profiles.end(),
            [id](const profile& item) { return item.id == id && !item.root.empty(); });
    };
    if (!has_profile(parsed.active_profile_id) || !has_profile(parsed.install_profile_id))
    {
        return false;
    }
    for (const auto& library : parsed.libraries)
    {
        if (library.path.empty() || !has_profile(library.profile_id))
        {
            return false;
        }
    }

    for (auto& item : parsed.profiles) item.root = normalize_root(item.root);
    for (auto& item : parsed.libraries) item.path = normalize_root(item.path);
    parsed.cache_root = normalize_root(parsed.cache_root);
    parsed.log_root = normalize_root(parsed.log_root);
    parsed.cover_root = normalize_root(parsed.cover_root);
    result = std::move(parsed);
    return true;
}

std::string serialize(const config& value)
{
    std::ostringstream out;
    out << "{\n"
        << "  \"schema\": " << value.schema << ",\n"
        << "  \"active_profile_id\": \"" << escape_json(value.active_profile_id) << "\",\n"
        << "  \"install_profile_id\": \"" << escape_json(value.install_profile_id) << "\",\n"
        << "  \"cache_root\": \"" << escape_json(normalize_root(value.cache_root)) << "\",\n"
        << "  \"log_root\": \"" << escape_json(normalize_root(value.log_root)) << "\",\n"
        << "  \"cover_root\": \"" << escape_json(normalize_root(value.cover_root)) << "\",\n"
        << "  \"profiles\": [";
    for (std::size_t i = 0; i < value.profiles.size(); ++i)
    {
        const auto& item = value.profiles[i];
        if (i) out << ',';
        out << "\n    {\"id\": \"" << escape_json(item.id)
            << "\", \"name\": \"" << escape_json(item.name)
            << "\", \"root\": \"" << escape_json(normalize_root(item.root))
            << "\", \"volume_id\": \"" << escape_json(item.volume_id)
            << "\", \"device\": \"" << device_name(item.device) << "\"}";
    }
    out << "\n  ],\n  \"libraries\": [";
    for (std::size_t i = 0; i < value.libraries.size(); ++i)
    {
        const auto& item = value.libraries[i];
        if (i) out << ',';
        out << "\n    {\"path\": \"" << escape_json(normalize_root(item.path))
            << "\", \"profile_id\": \"" << escape_json(item.profile_id)
            << "\", \"enabled\": " << (item.enabled ? "true" : "false") << '}';
    }
    out << "\n  ]\n}\n";
    return out.str();
}
}

config make_default(std::string_view local_state_root)
{
    config result;
    const auto root = append_path(local_state_root, "XPS3");
    result.active_profile_id = "internal";
    result.install_profile_id = "internal";
    result.profiles.push_back({
        .id = "internal",
        .name = "Armazenamento interno",
        .root = root,
        .device = device_kind::internal,
    });
    result.libraries.push_back({append_path(root, "GAMES"), "internal", true});
    result.cache_root = append_path(root, "CACHE");
    result.log_root = append_path(root, "LOGS");
    result.cover_root = append_path(root, "COVERS");
    return result;
}

bool load(std::string_view file, config& value, std::string& warning)
{
    warning.clear();
    std::ifstream input(std::filesystem::u8path(file), std::ios::binary);
    if (!input)
    {
        warning = "Nao foi possivel abrir a configuracao de armazenamento.";
        return false;
    }

    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    config parsed;
    if (!parse_config(text, parsed))
    {
        warning = "Configuracao de armazenamento invalida; os valores anteriores foram preservados.";
        return false;
    }

    value = std::move(parsed);
    return true;
}

bool save(std::string_view file, const config& value, std::string& error)
{
    error.clear();
    const auto target = std::filesystem::u8path(file);
    const auto temporary = std::filesystem::u8path(std::string(file) + ".tmp");
    const auto backup = std::filesystem::u8path(std::string(file) + ".bak");
    std::error_code ec;
    if (!target.parent_path().empty())
    {
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec)
        {
            error = "Nao foi possivel criar a pasta da configuracao: " + ec.message();
            return false;
        }
    }

    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            error = "Nao foi possivel criar o arquivo temporario da configuracao.";
            return false;
        }
        output << serialize(value);
        output.flush();
        if (!output)
        {
            error = "Falha ao gravar o arquivo temporario da configuracao.";
            output.close();
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }

    std::filesystem::remove(backup, ec);
    ec.clear();
    const bool had_target = std::filesystem::exists(target, ec) && !ec;
    if (had_target)
    {
        std::filesystem::rename(target, backup, ec);
        if (ec)
        {
            error = "Nao foi possivel preparar a substituicao da configuracao: " + ec.message();
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }

    ec.clear();
    std::filesystem::rename(temporary, target, ec);
    if (ec)
    {
        error = "Nao foi possivel concluir a configuracao: " + ec.message();
        if (had_target)
        {
            std::error_code restore_error;
            std::filesystem::rename(backup, target, restore_error);
        }
        std::filesystem::remove(temporary, ec);
        return false;
    }
    if (had_target)
    {
        std::filesystem::remove(backup, ec);
    }
    return true;
}

validation_result validate_profile(const profile& value, std::uint64_t required_bytes)
{
    validation_result result;
    const auto root = std::filesystem::u8path(normalize_root(value.root));
    std::error_code ec;
    if (root.empty() || !std::filesystem::exists(root, ec) || ec ||
        !std::filesystem::is_directory(root, ec) || ec)
    {
        result.state = validation_state::missing;
        result.message = "Pasta indisponivel.";
        return result;
    }

    const auto space = std::filesystem::space(root, ec);
    if (!ec)
    {
        result.free_bytes = space.available;
        result.total_bytes = space.capacity;
        if (required_bytes > result.free_bytes)
        {
            result.state = validation_state::insufficient_space;
            result.message = "Espaco livre insuficiente.";
            return result;
        }
    }

    const auto probe = root / ".xps3-write-probe.tmp";
    {
        std::ofstream output(probe, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            result.state = validation_state::read_only;
            result.message = "Pasta sem permissao de gravacao.";
            return result;
        }
        output.put('\0');
    }
    std::filesystem::remove(probe, ec);
    result.state = validation_state::ready;
    result.message = "Pasta pronta.";
    return result;
}

std::string normalize_root(std::string_view value)
{
    std::string result;
    result.reserve(value.size());
    bool previous_separator = false;
    for (const char ch : value)
    {
        const bool separator = ch == '/' || ch == '\\';
        if (separator)
        {
            if (!previous_separator)
            {
                result.push_back('/');
            }
        }
        else
        {
            result.push_back(ch);
        }
        previous_separator = separator;
    }
    while (result.size() > 1 && result.back() == '/' &&
           !(result.size() == 3 && result[1] == ':'))
    {
        result.pop_back();
    }
    return result;
}

std::string stable_content_key(std::string_view title_id, std::string_view content_path)
{
    std::string title;
    for (const unsigned char ch : title_id)
    {
        if (std::isalnum(ch))
        {
            title.push_back(static_cast<char>(std::toupper(ch)));
        }
    }
    if (!title.empty())
    {
        return title;
    }

    auto normalized = normalize_root(content_path);
    std::uint64_t hash = 1469598103934665603ull;
    for (const unsigned char ch : normalized)
    {
        const auto folded = static_cast<unsigned char>(std::toupper(ch));
        hash ^= folded;
        hash *= 1099511628211ull;
    }
    std::ostringstream out;
    out << "PATH" << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}
}
