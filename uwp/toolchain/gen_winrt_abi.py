#!/usr/bin/env python3
"""Generates uwp/app/winrt_xaml_abi.h: C++ ABI declarations (vtables, IIDs, enums, structs) for the
Windows Runtime types the XAML shell uses.

The zig/mingw toolchain has no Windows SDK WinRT headers, so the interfaces are read from the SDK
metadata instead of being typed in by hand. The output is checked in; run this only to change the list.

  pip install dnfile
  # winmd files: ref/netstandard2.0/*.winmd of the Microsoft.Windows.SDK.Contracts 10.0.19041.1 NuGet package
  python3 uwp/toolchain/gen_winrt_abi.py <winmd dir> uwp/app/winrt_xaml_abi.h
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import winmd  # noqa: E402

X = "Windows.UI.Xaml."
XC = "Windows.UI.Xaml.Controls."
XCP = "Windows.UI.Xaml.Controls.Primitives."

INTERFACES = [
    # Application model
    X + "IApplicationStatics", X + "IApplicationFactory", X + "IApplicationOverrides", X + "IApplication", X + "IApplication2",
    X + "IApplicationInitializationCallbackParams", X + "ApplicationInitializationCallback",
    "Windows.ApplicationModel.Activation.ILaunchActivatedEventArgs",
    X + "IWindow", X + "IWindowStatics",
    "Windows.UI.ViewManagement.IApplicationViewStatics2", "Windows.UI.ViewManagement.IApplicationView2",
    # Elements, events
    X + "IUIElement", X + "IFrameworkElement", X + "IRoutedEventArgs", X + "RoutedEventHandler",
    X + "Input.IKeyRoutedEventArgs", X + "Input.KeyEventHandler", X + "Input.IFocusManagerStatics",
    X + "IDispatcherTimer", X + "Markup.IXamlReaderStatics", X + "Markup.IXamlMetadataProvider", X + "IResourceDictionary",
    # Controls
    XC + "IPanel", XC + "IContentControl", XC + "IControl", XC + "ITextBlock", XC + "IProgressBar", XC + "IImage",
    XC + "IItemsControl", XC + "ISelectionChangedEventArgs", XC + "SelectionChangedEventHandler",
    XC + "IItemClickEventArgs", XC + "ItemClickEventHandler", XC + "IPivot", XC + "IToggleSwitch", XC + "IScrollViewer",
    XCP + "IRangeBase", XCP + "ISelector", XC + "IListViewBase", XCP + "IButtonBase", XCP + "IToggleButton",
    XC + "ITextBox", XC + "IPasswordBox",
    "Windows.Foundation.IPropertyValueStatics",
    # Pictures
    X + "Media.IBrush", X + "Media.IImageSource", X + "Media.Imaging.IWriteableBitmap", X + "Media.Imaging.IWriteableBitmapFactory",
    "Windows.Storage.Streams.IBuffer",
    # Back button
    "Windows.UI.Core.IBackRequestedEventArgs", "Windows.UI.Core.ISystemNavigationManager", "Windows.UI.Core.ISystemNavigationManagerStatics",
]

# Value types / enums are emitted on demand when a method signature mentions them.
SCALARS = {
    0x02: "boolean", 0x03: "wchar_t", 0x04: "int8_t", 0x05: "uint8_t", 0x06: "int16_t", 0x07: "uint16_t",
    0x08: "int32_t", 0x09: "uint32_t", 0x0A: "int64_t", 0x0B: "uint64_t", 0x0C: "float", 0x0D: "double",
    0x0E: "HSTRING",
}

SPECIAL = {
    "System.Guid": "GUID",
    "Windows.Foundation.EventRegistrationToken": "int64_t",
}


class Gen:
    def __init__(self, db):
        self.db = db
        self.emitted_types = {}  # full name -> declaration text, in dependency order
        self.order = []

    # ---- type decoding ----
    def type_name_cpp(self, full, kind_hint):
        if full in SPECIAL:
            return SPECIAL[full], False
        td = self.db.find(full)
        if td is None:
            return "void*", True
        if td.kind in ("struct", "enum"):
            self.need_type(td)
            return td.name, False
        if td.kind == "delegate":
            return (td.name + "*") if full in self.interface_set else "void*", True
        if td.kind == "interface":
            return (td.name + "*") if full in self.interface_set else "void*", True
        # runtime class: ABI uses the default interface
        di = getattr(td, "default_interface", None)
        if di and di in self.interface_set:
            return self.db.find(di).name + "*", True
        return "IInspectable*", True

    def decode(self, sig, pos):
        """Returns (cpp type, is_pointer, new pos). BYREF/arrays are handled by the caller."""
        t = sig[pos]
        pos += 1
        if t in SCALARS:
            return SCALARS[t], t == 0x0E, pos
        if t == 0x1C:
            return "IInspectable*", True, pos
        if t in (0x11, 0x12):
            tok, pos = winmd.read_compressed(sig, pos)
            full = self.db.resolve_token(tok)
            name, ptr = self.type_name_cpp(full, t)
            return name, ptr, pos
        if t == 0x15:  # GENERICINST
            pos += 1  # CLASS or VALUETYPE
            tok, pos = winmd.read_compressed(sig, pos)
            n, pos = winmd.read_compressed(sig, pos)
            for _ in range(n):
                _, _, pos = self.decode(sig, pos)
            return "void*", True, pos
        if t == 0x13:  # VAR
            _, pos = winmd.read_compressed(sig, pos)
            return "void*", True, pos
        raise ValueError(f"unsupported signature element 0x{t:02x}")

    def method(self, m):
        sig = m.sig
        pos = 1  # calling convention
        n, pos = winmd.read_compressed(sig, pos)
        try:
            if sig[pos] == 0x01:
                ret, pos = None, pos + 1
            else:
                if sig[pos] in (0x1D, 0x10):
                    raise ValueError("array/byref return")
                ret, _, pos = self.decode(sig, pos)
            params = []
            for i in range(n):
                byref = sig[pos] == 0x10
                if byref:
                    pos += 1
                if sig[pos] == 0x1D:
                    raise ValueError("array parameter")
                ty, _, pos = self.decode(sig, pos)
                pname = m.params.get(i + 1, f"arg{i}")
                params.append(f"{ty}{'*' if byref else ''} {pname}")
            if ret:
                params.append(f"{ret}* result")
        except (ValueError, IndexError) as e:
            return f"WINRT_ABI_SLOT({m.name}); // unsupported: {e}"
        return f"virtual HRESULT STDMETHODCALLTYPE {m.name}({', '.join(params)}) = 0;"

    # ---- value types ----
    def need_type(self, td):
        if td.full in self.emitted_types:
            return
        self.emitted_types[td.full] = None  # break cycles
        if td.kind == "enum":
            body = ",\n".join(f"\t{n} = {v}" for n, v in td.enum_members)
            text = f"enum class {td.name} : {td.enum_underlying}\n{{\n{body}\n}};\n"
        else:
            fields = []
            for fname, fsig, _ in td.fields:
                ty, _, _ = self.decode(fsig, 1)
                fields.append(f"\t{ty} {fname};")
            text = f"struct {td.name}\n{{\n" + "\n".join(fields) + "\n};\n"
        self.emitted_types[td.full] = text
        self.order.append(td.full)

    # ---- interfaces ----
    def interface(self, td):
        lines = [f"struct {td.name} : IInspectable", "{"]
        seen = {}
        for m in td.methods:
            if m.name == ".ctor":
                continue
            # Overloads get a numeric suffix so no C++ overload set depends on the vtable layout rules of the ABI.
            n = seen.get(m.name, 0) + 1
            seen[m.name] = n
            if n > 1:
                m.name = f"{m.name}_{n}"
            lines.append("\t" + self.method(m))
        lines.append("};")
        return "\n".join(lines)

    def delegate(self, td):
        inv = next(m for m in td.methods if m.name == "Invoke")
        decl = self.method(inv)
        return f"struct {td.name} : IUnknown\n{{\n\t{decl}\n}};"

    def run(self):
        self.interface_set = set(INTERFACES)
        out = []
        missing = []
        for full in INTERFACES:
            td = self.db.find(full)
            if td is None:
                missing.append(full)
                continue
            if td.kind == "delegate":
                out.append((td, self.delegate(td)))
            else:
                out.append((td, self.interface(td)))
        if missing:
            raise SystemExit("not found: " + ", ".join(missing))
        return out


def guid_cpp(g):
    p = g.split("-")
    d = p[3] + p[4]
    return "{0x%s, 0x%s, 0x%s, {%s}}" % (p[0], p[1], p[2], ", ".join("0x" + d[i:i + 2] for i in range(0, 16, 2)))


def main():
    db = winmd.load(sys.argv[1])
    gen = Gen(db)
    decls = gen.run()

    # Parameterized delegates the shell implements: IIDs from their signature strings.
    inspectable = "cinterface(IInspectable)"
    back_args = db.find("Windows.UI.Core.BackRequestedEventArgs")
    back_args_iid = db.find(back_args.default_interface).guid
    param_iids = {
        "IEventHandlerObject": winmd.pinterface_iid(f"pinterface({{9de1c535-6ae1-11e0-84e1-18a905bcc53f}};{inspectable})"),
        "IEventHandlerBackRequested": winmd.pinterface_iid(
            f"pinterface({{9de1c535-6ae1-11e0-84e1-18a905bcc53f}};rc(Windows.UI.Core.BackRequestedEventArgs;{{{back_args_iid}}}))"),
    }
    # IVector<T> instances: ItemsControl.Items (IObservableVector<object> is its default interface) and
    # Panel.Children are reached by QueryInterface with these.
    uielement_iid = db.find("Windows.UI.Xaml.IUIElement").guid
    param_iids["IVectorObject"] = winmd.pinterface_iid(f"pinterface({{913337e9-11a1-4345-a3a2-4e7f956e222d}};{inspectable})")
    param_iids["IVectorUIElement"] = winmd.pinterface_iid(
        f"pinterface({{913337e9-11a1-4345-a3a2-4e7f956e222d}};rc(Windows.UI.Xaml.UIElement;{{{uielement_iid}}}))")
    # Known-good values: the back handler in uwp_input.cpp runs on a console, and IEventHandler<object> is documented.
    assert param_iids["IEventHandlerBackRequested"] == "ca821060-002b-526d-8122-982630d7cdbe", param_iids
    assert param_iids["IEventHandlerObject"] == "c50898f6-c536-5f47-8583-8b2c2438a13b", param_iids
    assert param_iids["IVectorObject"] == "b32bdca4-5e52-5b27-bc5d-d66a1a268c2a", param_iids

    out = []
    out.append("""// Generated by uwp/toolchain/gen_winrt_abi.py from the Windows SDK contract metadata. Do not edit.
//
// ABI declarations (vtable order, IIDs) of the Windows Runtime types the XAML shell calls. The
// zig/mingw toolchain has no WinRT headers. Parameters of types that are not listed are void*.
#pragma once

#include <windows.h>
#include <inspectable.h>
#include <roapi.h>
#include <winstring.h>
#include <cstdint>

namespace winrt_abi
{
#define WINRT_ABI_SLOT(n) virtual HRESULT STDMETHODCALLTYPE slot_##n() = 0

template <class T>
struct iid_traits;

template <class T>
inline constexpr const GUID& iid_v = iid_traits<T>::value;
""")
    # Forward declarations
    out.append("// ---- Forward declarations ----")
    for td, _ in decls:
        out.append(f"struct {td.name};")
    out.append("")
    # Value types and enums first (they are by-value parameters).
    if gen.order:
        out.append("// ---- Value types and enumerations ----")
        for full in gen.order:
            out.append(gen.emitted_types[full])
    out.append("// ---- Interfaces ----")
    for td, text in decls:
        out.append(text + "\n")
    out.append("// ---- Interface IDs ----")
    for td, _ in decls:
        out.append(f"template <> struct iid_traits<{td.name}> {{ static constexpr GUID value = {guid_cpp(td.guid)}; }};")
    out.append("")
    out.append("// ---- Parameterized delegate IIDs (computed from their type signatures) ----")
    out.append("// Windows.Foundation.EventHandler<IInspectable>: Invoke(IInspectable* sender, IInspectable* args)")
    out.append("struct IEventHandlerObject : IUnknown\n{\n\tvirtual HRESULT STDMETHODCALLTYPE Invoke(IInspectable* sender, IInspectable* args) = 0;\n};")
    out.append(f"template <> struct iid_traits<IEventHandlerObject> {{ static constexpr GUID value = {guid_cpp(param_iids['IEventHandlerObject'])}; }};")
    out.append("// Windows.Foundation.EventHandler<BackRequestedEventArgs>")
    out.append("struct IEventHandlerBackRequested : IUnknown\n{\n\tvirtual HRESULT STDMETHODCALLTYPE Invoke(IInspectable* sender, IBackRequestedEventArgs* args) = 0;\n};")
    out.append(f"template <> struct iid_traits<IEventHandlerBackRequested> {{ static constexpr GUID value = {guid_cpp(param_iids['IEventHandlerBackRequested'])}; }};")
    out.append("// IVector<IInspectable> and IVector<UIElement> (Items, Children)")
    out.append(f"inline constexpr GUID IID_IVectorObject = {guid_cpp(param_iids['IVectorObject'])};")
    out.append(f"inline constexpr GUID IID_IVectorUIElement = {guid_cpp(param_iids['IVectorUIElement'])};")
    out.append("")
    out.append("#undef WINRT_ABI_SLOT\n}")
    with open(sys.argv[2], "w", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    print("wrote", sys.argv[2], len(decls), "types")


if __name__ == "__main__":
    main()
