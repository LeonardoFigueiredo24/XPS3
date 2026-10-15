# MinGW headers are lower-case, but RPCS3 and its libraries include e.g. <Windows.h>.
# On a case-sensitive host, create symlinks for every mixed-case include name found
# in the sources that matches a MinGW header case-insensitively.
import os, re, sys

mingw_inc, src_root, out_dir = sys.argv[1:4]
headers = {}
for root, _, files in os.walk(mingw_inc):
    for f in files:
        rel = os.path.relpath(os.path.join(root, f), mingw_inc)
        headers.setdefault(rel.lower(), rel)

pattern = re.compile(rb'#\s*include\s*[<"]([^>"]+)[>"]')
wanted = set()
for root, dirs, files in os.walk(src_root):
    dirs[:] = [d for d in dirs if not d.startswith('.') and d not in ('build', 'llvm')]
    for f in files:
        if f.endswith(('.c', '.cc', '.cpp', '.h', '.hpp', '.inl', '.rc')):
            try:
                data = open(os.path.join(root, f), 'rb').read()
            except OSError:
                continue
            for m in pattern.finditer(data):
                wanted.add(m.group(1).decode('utf-8', 'ignore').replace('\\', '/'))

count = 0
for name in wanted:
    real = headers.get(name.lower())
    if real and real != name:
        dst = os.path.join(out_dir, name)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        if not os.path.lexists(dst):
            os.symlink(os.path.join(mingw_inc, real), dst)
            count += 1
print(f"case links: {count}")
