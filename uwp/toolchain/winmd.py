#!/usr/bin/env python3
"""Minimal Windows Runtime metadata (.winmd) reader on top of dnfile.

Used by gen_winrt_abi.py to emit the C++ ABI declarations the UWP shell needs, because the
zig/mingw toolchain has no WinRT headers. Reads the Windows SDK contract winmd files from the
Microsoft.Windows.SDK.Contracts NuGet package (ref/netstandard2.0/*.winmd).

Run directly to inspect a type:  winmd.py <winmd dir> Windows.UI.Xaml.Application
"""

import hashlib
import os
import struct
import sys

import dnfile

# WinRT parameterized-interface IID namespace.
PINTERFACE_NAMESPACE = bytes.fromhex("11f47ad57b7342c0abae878b1e16adee")


def pinterface_iid(signature: str) -> str:
    """IID of a parameterized interface instance from its type signature string."""
    digest = bytearray(hashlib.sha1(PINTERFACE_NAMESPACE + signature.encode("utf-8")).digest()[:16])
    digest[6] = (digest[6] & 0x0F) | 0x50
    digest[8] = (digest[8] & 0x3F) | 0x80
    h = digest.hex()
    return f"{h[0:8]}-{h[8:12]}-{h[12:16]}-{h[16:20]}-{h[20:32]}"


def _name(x):
    return str(x.value) if hasattr(x, "value") else str(x)


class Method:
    def __init__(self, row):
        self.name = _name(row.Name)
        self.sig = bytes(row.Signature.value)
        self.params = {p.row.Sequence: _name(p.row.Name) for p in row.ParamList}


class TypeDef:
    def __init__(self, db, row):
        self.db = db
        self.row = row
        self.namespace = _name(row.TypeNamespace)
        self.name = _name(row.TypeName)
        self.full = f"{self.namespace}.{self.name}"
        self.methods = [Method(m.row) for m in row.MethodList]
        self.fields = [(_name(f.row.Name), bytes(f.row.Signature.value), f.row) for f in row.FieldList]
        self.attrs = {}

    @property
    def extends(self):
        ext = self.row.Extends
        if ext is None or ext.row is None:
            return None
        r = ext.row
        return f"{_name(r.TypeNamespace)}.{_name(r.TypeName)}"

    @property
    def is_interface(self):
        return bool(self.row.Flags.tdInterface)

    @property
    def kind(self):
        if self.is_interface:
            return "interface"
        e = self.extends
        if e == "System.Enum":
            return "enum"
        if e == "System.ValueType":
            return "struct"
        if e == "System.MulticastDelegate":
            return "delegate"
        return "class"

    @property
    def guid(self):
        blob = self.attrs.get("GuidAttribute")
        if not blob or len(blob) < 20:
            return None
        a, b, c = struct.unpack_from("<IHH", blob, 2)
        d = blob[10:18]
        return f"{a:08x}-{b:04x}-{c:04x}-{d[0]:02x}{d[1]:02x}-{d[2:8].hex()}"


class Database:
    def __init__(self, directory, files):
        self.types = {}
        self.refs = {}
        self.enum_values = {}
        for f in files:
            self._load(os.path.join(directory, f))

    def _load(self, path):
        pe = dnfile.dnPE(path)
        t = pe.net.mdtables
        by_row = {}
        for r in t.TypeDef.rows:
            td = TypeDef(self, r)
            self.types[td.full] = td
            by_row[id(r)] = td
        # Custom attributes (Guid, DefaultAttribute is an InterfaceImpl attribute).
        for c in t.CustomAttribute.rows:
            parent = c.Parent.row if c.Parent is not None else None
            if parent is None or id(parent) not in by_row:
                continue
            ctor = c.Type.row
            cls = getattr(ctor, "Class", None)
            cname = _name(cls.row.TypeName) if cls is not None and cls.row is not None else None
            if cname:
                by_row[id(parent)].attrs[cname] = bytes(c.Value.value)
        # Enum constants: the literal fields of the enum's TypeDef.
        consts = {}
        for c in t.Constant.rows:
            p = c.Parent.row if c.Parent is not None else None
            if p is not None:
                consts[id(p)] = (c.Type, bytes(c.Value.value))
        for td in by_row.values():
            if td.kind != "enum":
                continue
            vals = []
            for fname, fsig, frow in td.fields:
                if fname == "value__":
                    td.enum_underlying = "int32_t" if fsig[-1] == 0x08 else "uint32_t"
                    continue
                if id(frow) in consts:
                    raw = consts[id(frow)][1]
                    signed = getattr(td, "enum_underlying", "int32_t") == "int32_t"
                    vals.append((fname, struct.unpack("<i" if signed else "<I", raw[:4])[0]))
            td.enum_members = vals
        # TypeRef table, to resolve signature tokens.
        self.typeref_rows = getattr(self, "typeref_rows", [])
        self.typeref_rows.append([(_name(r.TypeNamespace), _name(r.TypeName)) for r in t.TypeRef.rows])
        self.typedef_rows = getattr(self, "typedef_rows", [])
        self.typedef_rows.append([td for td in (by_row[id(r)] for r in t.TypeDef.rows)])
        # Interface impls (default interface of a runtime class).
        for ii in t.InterfaceImpl.rows:
            cls = ii.Class.row
            if id(cls) not in by_row:
                continue
            iface = ii.Interface.row
            td = by_row[id(cls)]
            if not hasattr(td, "interfaces"):
                td.interfaces = []
            iname = self._resolve_row(iface)
            td.interfaces.append((iname, False))
        self._load_defaults(t, by_row)

    def _load_defaults(self, t, by_row):
        for c in t.CustomAttribute.rows:
            parent = c.Parent.row if c.Parent is not None else None
            if parent is None:
                continue
            ctor = c.Type.row
            cls = getattr(ctor, "Class", None)
            if cls is None or cls.row is None or _name(cls.row.TypeName) != "DefaultAttribute":
                continue
            cls_row = getattr(parent, "Class", None)
            owner = cls_row.row if cls_row is not None else None
            if owner is None or id(owner) not in by_row:
                continue
            td = by_row[id(owner)]
            iface = self._resolve_row(parent.Interface.row)
            td.default_interface = iface

    @staticmethod
    def _resolve_row(row):
        if row is None:
            return None
        if hasattr(row, "TypeNamespace"):
            return f"{_name(row.TypeNamespace)}.{_name(row.TypeName)}"
        return None

    def find(self, full):
        return self.types.get(full)

    def resolve_token(self, coded):
        """TypeDefOrRef coded index from a signature -> full type name (or None)."""
        tag, idx = coded & 3, coded >> 2
        if tag == 0:
            for rows in self.typedef_rows:
                if 0 < idx <= len(rows):
                    td = rows[idx - 1]
                    return td.full
        elif tag == 1:
            for rows in self.typeref_rows:
                if 0 < idx <= len(rows):
                    ns, nm = rows[idx - 1]
                    return f"{ns}.{nm}"
        return None


def read_compressed(sig, pos):
    b = sig[pos]
    if b & 0x80 == 0:
        return b, pos + 1
    if b & 0xC0 == 0x80:
        return ((b & 0x3F) << 8) | sig[pos + 1], pos + 2
    return ((b & 0x1F) << 24) | (sig[pos + 1] << 16) | (sig[pos + 2] << 8) | sig[pos + 3], pos + 4


def load(directory):
    return Database(directory, [
        "Windows.Foundation.FoundationContract.winmd",
        "Windows.Foundation.UniversalApiContract.winmd",
    ])


if __name__ == "__main__":
    db = load(sys.argv[1])
    for name in sys.argv[2:]:
        td = db.find(name)
        if not td:
            print(name, "not found")
            continue
        print(td.full, td.kind, "guid=", td.guid, "default=", getattr(td, "default_interface", None))
        for itf in getattr(td, "interfaces", []):
            print("   implements", itf[0])
        for m in td.methods:
            print("   ", m.name)
