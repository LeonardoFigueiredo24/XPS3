#include "../uwp_storage.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main()
{
    using namespace xps3::storage;

    const auto defaults = make_default("C:\\Users\\Tester\\LocalState");
    require(defaults.schema == 1, "default schema must be version one");
    require(defaults.profiles.size() == 1, "default configuration must have one internal profile");
    require(defaults.profiles.front().device == device_kind::internal,
        "default profile must be internal storage");
    require(defaults.profiles.front().root == "C:/Users/Tester/LocalState/XPS3",
        "default root must be normalized below LocalState");
    require(defaults.cache_root == "C:/Users/Tester/LocalState/XPS3/CACHE",
        "default cache directory is wrong");
    require(defaults.log_root == "C:/Users/Tester/LocalState/XPS3/LOGS",
        "default log directory is wrong");
    require(defaults.cover_root == "C:/Users/Tester/LocalState/XPS3/COVERS",
        "default manual-cover directory is wrong");
    require(defaults.libraries.size() == 1 &&
            defaults.libraries.front().path == "C:/Users/Tester/LocalState/XPS3/GAMES",
        "default game library is wrong");

    require(normalize_root("E:\\XPS3\\\\GAMES\\") == "E:/XPS3/GAMES",
        "normalization must collapse separators and trim the final separator");
    require(stable_content_key("bles-12345", "E:/ignored/game.iso") == "BLES12345",
        "valid title IDs must be normalized and preferred");
    require(stable_content_key("", "E:\\XPS3\\GAMES\\demo.elf") ==
            stable_content_key("", "E:/XPS3/GAMES/demo.elf"),
        "path fallback key must be stable across separator styles");

    const auto temp = std::filesystem::temp_directory_path() / "xps3-storage-test";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp);
    const auto config_file = temp / "storage.json";

    auto configured = defaults;
    configured.profiles.push_back({
        .id = "usb-games",
        .name = "USB Jogos",
        .root = "E:/XPS3",
        .volume_id = "volume-test",
        .device = device_kind::external,
    });
    configured.active_profile_id = "usb-games";
    configured.install_profile_id = "usb-games";
    configured.libraries.push_back({"E:/XPS3/GAMES", "usb-games", true});

    std::string error;
    require(save(config_file.string(), configured, error), "valid configuration must save");
    require(error.empty(), "successful save must not return an error");
    require(std::filesystem::exists(config_file), "save must create storage.json");
    require(!std::filesystem::exists(config_file.string() + ".tmp"),
        "successful atomic save must not leave its temporary file");

    config restored;
    std::string warning;
    require(load(config_file.string(), restored, warning), "saved configuration must load");
    require(warning.empty(), "valid configuration must not produce a warning");
    require(restored.active_profile_id == "usb-games" && restored.profiles.size() == 2,
        "round trip must retain profiles and active selection");
    require(restored.libraries.size() == 2 && restored.libraries.back().enabled,
        "round trip must retain library roots");
    require(profile_root(restored, restored.active_profile_id) == "E:/XPS3",
        "active profile root must resolve by stable profile id");
    require(profile_root(restored, "missing").empty(),
        "unknown profile ids must not silently select another device");

    {
        std::ofstream invalid(config_file, std::ios::trunc);
        invalid << "{invalid";
    }
    const auto before_invalid_load = restored.active_profile_id;
    require(!load(config_file.string(), restored, warning),
        "malformed configuration must be rejected");
    require(!warning.empty(), "malformed configuration must explain the fallback");
    require(restored.active_profile_id == before_invalid_load,
        "failed load must not partially mutate the caller configuration");

    const auto ready = validate_profile({
        .id = "temp",
        .name = "Temp",
        .root = temp.string(),
        .device = device_kind::custom,
    }, 1);
    require(ready.state == validation_state::ready,
        "existing writable directory with free space must validate as ready");

    const auto missing = validate_profile({
        .id = "missing",
        .name = "Missing",
        .root = (temp / "not-there").string(),
        .device = device_kind::custom,
    }, 0);
    require(missing.state == validation_state::missing,
        "missing directory must not validate as ready");

    std::filesystem::remove_all(temp);
    std::cout << "PASS: storage model\n";
    return 0;
}
