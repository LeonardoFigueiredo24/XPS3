#!/usr/bin/env python3
"""Checks the XAML pages in uwp/app/xaml/ against the Windows SDK metadata: every element must be a
Windows.UI.Xaml type and every attribute a property (or attached property) of it, so a typo shows up
here instead of as a XamlReader failure on the console.

  python3 uwp/toolchain/check_xaml.py <winmd dir> uwp/app/xaml
"""

import os
import re
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import winmd  # noqa: E402

NAMESPACES = [
    "Windows.UI.Xaml.Controls", "Windows.UI.Xaml", "Windows.UI.Xaml.Controls.Primitives", "Windows.UI.Xaml.Shapes",
    "Windows.UI.Xaml.Media", "Windows.UI.Xaml.Media.Imaging", "Windows.UI.Xaml.Documents",
]
PRESENTATION = "http://schemas.microsoft.com/winfx/2006/xaml/presentation"
XAML = "http://schemas.microsoft.com/winfx/2006/xaml"
# Properties that exist on every element through markup rather than as a put_ method.
SPECIAL = {"Name"}


def class_members(db, td):
    names = set()
    seen = set()
    while td is not None and td.full not in seen:
        seen.add(td.full)
        for itf, _ in getattr(td, "interfaces", []):
            it = db.find(itf)
            if it is not None:
                names.update(m.name for m in it.methods)
        td = db.find(td.extends) if td.extends else None
    return names


def find_type(db, name):
    for ns in NAMESPACES:
        td = db.find(f"{ns}.{name}")
        if td is not None:
            return td
    return None


def check(db, path):
    text = open(path, encoding="utf-8").read()
    # Placeholders from the item templates are replaced by plain text before the page is parsed.
    text = re.sub(r"\$[A-Z]+\$", "x", text)
    text = text.replace("<Grid.ColumnDefinitions>", "<Grid.ColumnDefinitions>")
    try:
        root = ET.fromstring(text)
    except ET.ParseError as e:
        return [f"{path}: not well-formed: {e}"]
    problems = []

    def walk(el):
        tag = el.tag
        ns, _, local = tag[1:].partition("}") if tag.startswith("{") else ("", "", tag)
        if "." in local:
            owner, prop = local.split(".", 1)  # property element, e.g. Pivot.HeaderTemplate
            td = find_type(db, owner)
            if td is None or (f"put_{prop}" not in class_members(db, td) and f"get_{prop}" not in class_members(db, td)):
                problems.append(f"{path}: property element {local} unknown")
        elif ns == PRESENTATION:
            td = find_type(db, local)
            if td is None:
                problems.append(f"{path}: element <{local}> is not a Windows.UI.Xaml type")
            else:
                members = class_members(db, td)
                for attr in el.attrib:
                    if attr.startswith("{"):
                        ans, _, aname = attr[1:].partition("}")
                        if ans == XAML and aname in ("Name",):
                            continue
                        if ans == "http://www.w3.org/XML/1998/namespace":
                            continue
                        problems.append(f"{path}: <{local}> attribute {attr} not understood")
                        continue
                    if "." in attr:  # attached: Grid.Row -> Grid.SetRow
                        owner, prop = attr.split(".", 1)
                        otd = find_type(db, owner)
                        if otd is None or f"Set{prop}" not in {m.name for m in otd.methods}:
                            problems.append(f"{path}: <{local}> attached property {attr} unknown")
                        continue
                    if attr in SPECIAL:
                        continue
                    if f"put_{attr}" not in members and f"get_{attr}" not in members:
                        problems.append(f"{path}: <{local}> has no property {attr}")
        for child in el:
            walk(child)

    walk(root)
    return problems


def main():
    db = winmd.load(sys.argv[1])
    problems = []
    for name in sorted(os.listdir(sys.argv[2])):
        if name.endswith(".xaml"):
            problems += check(db, os.path.join(sys.argv[2], name))
    for p in problems:
        print(p)
    print(f"{len(problems)} problem(s)")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
