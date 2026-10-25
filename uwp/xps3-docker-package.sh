#!/bin/sh
# Assemble the signed XPS3 APPX and its only separately installed framework dependency.
set -eu

root=${XPS3_SOURCE_ROOT:-/src}
state=${XPS3_BUILD_ROOT:-/build}
build=${XPS3_APP_BUILD:-$state/app-build}
dist=${XPS3_DIST:-$root/dist}
version=$(sed -n 's/.*Version="\([0-9.]*\)".*/\1/p' "$root/uwp/package/AppxManifest.xml" | head -n 1)
[ -n "$version" ] || { echo "package version not found" >&2; exit 1; }

mesa=$state/mesa-build/src/microsoft/vulkan/vulkan_dzn.dll
dxc=$state/packages/dxc-1.9.2609.5/build/native/bin/x64
xaml=$state/packages/xaml-2.8.7/tools/AppX/x64/Release/Microsoft.UI.Xaml.2.8.appx
makemsix=$(cat "$state/msix-build/makemsix.path")
for file in "$build/bin/xps3_uwp.exe" "$build/bin/xps3_uwp_importcheck.exe" "$mesa" \
            "$dxc/dxil.dll" "$dxc/dxcompiler.dll" "$xaml" "$makemsix"; do
    [ -f "$file" ] || { echo "missing release input: $file" >&2; exit 1; }
done

signing=$state/signing
mkdir -p "$signing" "$dist/Dependencies/x64" "$dist/LICENSES"
if [ ! -f "$signing/XPS3.Development.pfx" ]; then
    umask 077
    password=$(openssl rand -hex 24)
    printf '%s' "$password" >"$signing/password.txt"
    openssl req -x509 -newkey rsa:3072 -sha256 -nodes -days 825 \
        -subj '/CN=XPS3Development' \
        -addext 'keyUsage=critical,digitalSignature' \
        -addext 'extendedKeyUsage=codeSigning' \
        -keyout "$signing/XPS3.Development.key" -out "$signing/XPS3.Development.pem"
    openssl pkcs12 -export -legacy -out "$signing/XPS3.Development.pfx" \
        -inkey "$signing/XPS3.Development.key" -in "$signing/XPS3.Development.pem" \
        -passout "pass:$password"
    openssl x509 -in "$signing/XPS3.Development.pem" -outform DER -out "$signing/XPS3.Development.cer"
fi
password=$(cat "$signing/password.txt")

out="$dist/XPS3_${version}_x64.appx"
export MAKEMSIX="$makemsix"
export UWP_EXTRA_FILES="$mesa:$dxc/dxil.dll:$dxc/dxcompiler.dll:$state/packages/dxc-1.9.2609.5/LICENSE-LLVM.txt:$state/packages/dxc-1.9.2609.5/LICENSE-MS.txt"
"$root/uwp/package/package.sh" "$build" "$out" "$signing/XPS3.Development.pfx" "$password"

cp "$signing/XPS3.Development.cer" "$dist/XPS3.Development.cer"
cp "$xaml" "$dist/Dependencies/x64/Microsoft.UI.Xaml.2.8.appx"
cp "$state/packages/xaml-2.8.7/license.txt" "$dist/LICENSES/Microsoft.UI.Xaml.2.8.txt"
cp "$root/LICENSE" "$dist/LICENSES/RPCS3-GPL-2.0.txt"

cat >"$dist/INSTALL.md" <<EOF
# XPS3 $version — Xbox Developer Mode

Status: experimental RPCS3-based port. This package does not claim universal game compatibility.

1. Keep the Xbox application type set to **Game** for the 5 GiB resource budget.
2. In Xbox Device Portal, install `Dependencies/x64/Microsoft.UI.Xaml.2.8.appx` as a dependency if it is not already installed.
3. Add `XPS3_${version}_x64.appx`. If the portal requests a certificate, provide `XPS3.Development.cer`.
4. Install legitimate PS3 firmware through the XPS3 Install tab. Firmware and games are not included.
5. Use Auto frame limit first. 120 requires a compatible title or patch, a 120 Hz Xbox output and measured headroom.
EOF

(cd "$dist" && sha256sum "XPS3_${version}_x64.appx" XPS3.Development.cer \
    Dependencies/x64/Microsoft.UI.Xaml.2.8.appx >checksums.sha256)
echo "XPS3_RELEASE=$out"
