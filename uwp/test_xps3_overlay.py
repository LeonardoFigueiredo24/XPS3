#!/usr/bin/env python3
"""Static invariants for the XPS3 layer over the RPCS3 UWP fork."""

from pathlib import Path
import json
import re
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "uwp" / "app"
PACKAGE = ROOT / "uwp" / "package"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    checks: list[str] = []

    manifest = ET.parse(PACKAGE / "AppxManifest.xml").getroot()
    ns = {"f": "http://schemas.microsoft.com/appx/manifest/foundation/windows10"}
    identity = manifest.find("f:Identity", ns)
    require(identity is not None, "manifest Identity missing")
    require(identity.attrib["Name"] == "XPS3.Experimental.Emulator", "foreign package identity")
    require(identity.attrib["Publisher"] == "CN=XPS3Development", "foreign publisher")
    require(identity.attrib["Version"] == "0.3.4.0", "unexpected package version after content-library update")
    require(identity.attrib["ProcessorArchitecture"] == "x64", "Xbox target must be x64")
    apps = manifest.findall("f:Applications/f:Application", ns)
    require([a.attrib["Executable"] for a in apps] ==
            ["xps3_uwp.exe", "xps3_uwp_importcheck.exe"], "unexpected executables")
    require(apps[0].attrib["EntryPoint"] == "XPS3.App", "unexpected activation class")
    checks.append("manifest_identity")

    cmake = (APP / "CMakeLists.txt").read_text(encoding="utf-8")
    require("add_executable(xps3_uwp WIN32" in cmake, "XPS3 target missing")
    require("add_executable(xps3_uwp_importcheck WIN32" in cmake, "import checker target missing")
    require("xps3_offline_services.cpp" in cmake, "offline services missing")
    require("${CMAKE_SOURCE_DIR}/rpcs3/Input/blob_detector.cpp" in cmake,
            "0.1.35.0 input source missing")
    require("3rdparty::opencv" not in cmake, "retired OpenCV dependency returned")
    for inherited in ("uwp_telemetry.cpp", "uwp_telemetry_scrub.cpp", "uwp_update.cpp"):
        require(not re.search(rf"^\s*{re.escape(inherited)}\s*$", cmake, re.MULTILINE),
                f"inherited network service still compiled: {inherited}")
    checks.append("cmake_targets")

    source_names = re.findall(r"^\s*([A-Za-z0-9_./-]+\.(?:cpp|c))\s*$", cmake, re.MULTILINE)
    active_text = "\n".join((APP / name).read_text(encoding="utf-8", errors="replace")
                            for name in source_names if (APP / name).is_file())
    for endpoint in ("rpcs3.emelinhabibovic.de", "api.github.com/repos/emelin2004"):
        require(endpoint not in active_text, f"active source contains inherited endpoint: {endpoint}")
    checks.append("no_inherited_endpoints")

    main_cpp = (APP / "uwp_main.cpp").read_text(encoding="utf-8")
    xaml_cpp = (APP / "uwp_xaml.cpp").read_text(encoding="utf-8")
    require('"XPS3/storage.json"' in main_cpp, "persistent storage configuration missing")
    require("load_storage_configuration()" in main_cpp, "storage configuration is not loaded at startup")
    require("g_storage_config.install_profile_id" in main_cpp,
            "configured install destination is not connected to dev_hdd0")
    require('WindowsCreateString(L"XPS3.App", 8, name)' in xaml_cpp, "activation class mismatch")
    require(not (PACKAGE / "Assets" / "AuthorLogo.png").exists(), "foreign author logo packaged")
    checks.append("branding_and_storage")

    xaml_dir = APP / "xaml"
    for xaml_file in xaml_dir.glob("*.xaml"):
        ET.parse(xaml_file)
    shell_xaml = (xaml_dir / "shell.xaml").read_text(encoding="utf-8")
    game_item_xaml = (xaml_dir / "game_item.xaml").read_text(encoding="utf-8")
    for element_name in (
        "GamePanel", "MenuLayer", "MenuPivot", "GamesList", "GamesBusy", "GamesEmpty",
        "DetailsPanel", "DetailIcon", "DetailPlaceholder", "DetailTitle", "DetailId",
        "DetailKind", "DetailVersion", "DetailSource", "InstallBusy", "InstallEmpty",
        "InstallList", "SettingsStack", "StatusText", "VersionText", "UpdateText",
        "FirmwareText", "TelemetryIdText", "UsbHint",
    ):
        require(f'x:Name="{element_name}"' in shell_xaml, f"XAML binding removed: {element_name}")
    require("AuthorLogo.png" not in shell_xaml and "emelinhabibovic.de" not in shell_xaml,
            "foreign visual branding returned")
    require("CELL HORIZON" in shell_xaml, "XPS3 visual identity missing")
    require("#FF080B16" in shell_xaml and "#FF42D7FF" in shell_xaml,
            "Cell Horizon palette missing")
    require('<ItemsStackPanel Orientation="Horizontal"' in shell_xaml,
            "television library is not horizontal")
    require("$TITLE$" in game_item_xaml and "x:Name=\"Icon\"" in game_item_xaml,
            "game tile no longer binds real library data")
    require('TextWrapping="WrapWholeWords"' in game_item_xaml and 'MaxLines="2"' in game_item_xaml,
            "game tile title can still be cut to one line")
    detail_title = re.search(r'<TextBlock x:Name="DetailTitle"[^>]*/>', shell_xaml)
    require(detail_title is not None and 'TextWrapping="WrapWholeWords"' in detail_title.group(0)
            and 'MaxLines=' not in detail_title.group(0),
            "selected game title is still capped and can be cut")
    require('Header="Adicionar e instalar"' in shell_xaml,
            "content tab does not describe both playable and installable content")
    require("PS3_GAME" in shell_xaml and ".iso" in shell_xaml,
            "content tab does not explain directly playable PS3 formats")
    require("Nenhum conteúdo compatível encontrado" in xaml_cpp,
            "empty content state is not localized or no longer describes all content")
    checks.append("cell_horizon_ui")

    frontend_cpp = (APP / "uwp_frontend.cpp").read_text(encoding="utf-8")
    nv406e_cpp = (ROOT / "rpcs3" / "Emu" / "RSX" / "NV47" / "HW" / "nv406e.cpp").read_text(encoding="utf-8")
    iso_cpp = (ROOT / "rpcs3" / "Loader" / "ISO.cpp").read_text(encoding="utf-8")
    require("void suspend_game()" in frontend_cpp and "void resume_game()" in frontend_cpp,
            "UWP suspend/resume integration missing")
    require("g_uwp_rsx_idle_sleep_us" in nv406e_cpp, "UWP RSX semaphore yielding missing")
    require("path_component.empty()" in iso_cpp, "0.1.35.0 ISO path fix missing")
    require('"120"' in (ROOT / "rpcs3" / "Emu" / "system_config_types.cpp").read_text(encoding="utf-8"),
            "120 frame-limit option missing from core")
    require("120 requires a compatible title or patch" in frontend_cpp,
            "120 FPS compatibility warning missing")
    for marker in (
        "disc_image", "game_folder", "program", "playable content",
        "known_free", "USB drive is full", "EBOOT.BIN", 'A  Select      Y  Refresh',
    ):
        require(marker in frontend_cpp, f"content and package safety integration missing: {marker}")
    require("cover.png" in active_text and "max_encoded_bytes" in active_text,
            "bounded manual-cover integration missing")
    checks.append("upstream_0_1_35_integration")

    release_script = (ROOT / "uwp" / "xps3-docker-release.sh").read_text(encoding="utf-8")
    package_script = (ROOT / "uwp" / "xps3-docker-package.sh").read_text(encoding="utf-8")
    for phase in ("xps3-fetch-sources.sh", "xps3-fetch-packages.sh",
                  "xps3-docker-build-ffmpeg.sh", "xps3-configure-build.sh",
                  "xps3-docker-build-mesa.sh", "xps3-docker-build-msix.sh",
                  "xps3-docker-package.sh"):
        require(phase in release_script, f"release phase missing: {phase}")
    require("CN=XPS3Development" in package_script, "own development certificate subject missing")
    require("Microsoft.UI.Xaml.2.8.appx" in package_script, "XAML framework dependency not staged")
    require("checksums.sha256" in package_script, "release checksum generation missing")
    require('chmod -R a+rX "$dist"' in package_script,
            "Docker release payload is not made readable by the host runner")
    checks.append("release_pipeline")

    print(json.dumps({"result": "PASS", "checks": checks}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(json.dumps({"result": "FAIL", "error": str(exc)}, indent=2), file=sys.stderr)
        raise SystemExit(1)
