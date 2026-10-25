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
    require('root + "XPS3/"' in main_cpp, "external storage root is not XPS3")
    require('WindowsCreateString(L"XPS3.App", 8, name)' in xaml_cpp, "activation class mismatch")
    require(not (PACKAGE / "Assets" / "AuthorLogo.png").exists(), "foreign author logo packaged")
    checks.append("branding_and_storage")

    print(json.dumps({"result": "PASS", "checks": checks}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(json.dumps({"result": "FAIL", "error": str(exc)}, indent=2), file=sys.stderr)
        raise SystemExit(1)
