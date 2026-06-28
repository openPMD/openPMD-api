#!/usr/bin/env python3
"""List the (transitive) shared-library dependencies of a native binary.

A small, portable, libtree-style dependency lister. Pure Python, cross-platform
(ELF / PE / Mach-O / wasm), no third-party modules, so it runs in the minimal
cibuildwheel test environment and works even on cross-compiled binaries that
cannot be executed on the host.

For every dependency it reports HOW it was resolved (RPATH / RUNPATH /
LD_LIBRARY_PATH / default path / System32 / PATH / @rpath / @loader_path /
absolute / binary dir / wheel-repair bundle dir), or flags it `[system]`
(OS / language runtime, not bundled) or `[not found]`. The non-system,
not-found libraries are the ones that must be bundled into a wheel -- this
surfaces them BEFORE the opaque "DLL load failed" / "cannot open shared object
file" import error.

A wheel repaired by delvewheel (Windows) / auditwheel (Linux) puts its bundled,
name-mangled libraries in a sibling `<dist>.libs` directory and delocate
(macOS) in a `<pkg>/.dylibs` directory; the repaired package `__init__`
re-adds those to the runtime search path (os.add_dll_directory / RPATH). We
discover them PORTABLY -- by their conventional names, walking up from the
binary, with no dependency on the repair tool -- so a bundled library shows as
resolved (e.g. `[bundled .libs (delvewheel/auditwheel) -> ...]`) instead of a
misleading `[not found]`.

Usage:
    python list_library_deps.py [--strict] <binary> [<binary> ...]

    --strict   exit non-zero if any non-system dependency is not found
"""
import os
import struct
import sys


# ---------------------------------------------------------------------------
# Format detection
# ---------------------------------------------------------------------------
def detect_format(data):
    if data[:4] == b"\x7fELF":
        return "elf"
    if data[:2] == b"MZ":
        return "pe"
    if data[:4] in (b"\xfe\xed\xfa\xce", b"\xfe\xed\xfa\xcf",
                    b"\xce\xfa\xed\xfe", b"\xcf\xfa\xed\xfe"):
        return "macho"
    if data[:4] in (b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf"):
        return "macho-fat"
    if data[:4] == b"\x00asm":
        return "wasm"
    return None


# ---------------------------------------------------------------------------
# ELF (Linux): DT_NEEDED + DT_RPATH/DT_RUNPATH via the section headers
# ---------------------------------------------------------------------------
def _elf_deps(data):
    is64 = data[4] == 2
    le = "<" if data[5] == 1 else ">"
    if is64:
        e_shoff, = struct.unpack_from(le + "Q", data, 0x28)
        e_shentsize, e_shnum = struct.unpack_from(le + "HH", data, 0x3a)
    else:
        e_shoff, = struct.unpack_from(le + "I", data, 0x20)
        e_shentsize, e_shnum = struct.unpack_from(le + "HH", data, 0x2e)

    sections = []  # (sh_type, sh_offset, sh_size, sh_link, sh_entsize)
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        sh_type, = struct.unpack_from(le + "I", data, off + 4)
        if is64:
            sh_offset, sh_size = struct.unpack_from(le + "QQ", data, off + 0x18)
            sh_link, = struct.unpack_from(le + "I", data, off + 0x28)
            sh_entsize, = struct.unpack_from(le + "Q", data, off + 0x38)
        else:
            sh_offset, sh_size = struct.unpack_from(le + "II", data, off + 0x10)
            sh_link, = struct.unpack_from(le + "I", data, off + 0x18)
            sh_entsize, = struct.unpack_from(le + "I", data, off + 0x24)
        sections.append((sh_type, sh_offset, sh_size, sh_link, sh_entsize))

    SHT_DYNAMIC = 6
    needed, rpaths = [], []
    for sh_type, sh_offset, sh_size, sh_link, sh_entsize in sections:
        if sh_type != SHT_DYNAMIC:
            continue
        str_off = sections[sh_link][1]  # linked .dynstr section file offset

        def s(o):
            end = data.index(b"\0", str_off + o)
            return data[str_off + o:end].decode("utf-8", "replace")

        ent = sh_entsize or (16 if is64 else 8)
        for o in range(sh_offset, sh_offset + sh_size, ent):
            if is64:
                d_tag, d_val = struct.unpack_from(le + "qQ", data, o)
            else:
                d_tag, d_val = struct.unpack_from(le + "iI", data, o)
            if d_tag == 0:        # DT_NULL
                break
            if d_tag == 1:        # DT_NEEDED
                needed.append(s(d_val))
            elif d_tag == 15:     # DT_RPATH
                rpaths += [(p, "RPATH") for p in s(d_val).split(":") if p]
            elif d_tag == 29:     # DT_RUNPATH
                rpaths += [(p, "RUNPATH") for p in s(d_val).split(":") if p]
    return needed, rpaths


# ---------------------------------------------------------------------------
# PE (Windows): imported DLL names from the import directory
# ---------------------------------------------------------------------------
def _pe_deps(data):
    e_lfanew, = struct.unpack_from("<I", data, 0x3c)
    if data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
        return [], []
    coff = e_lfanew + 4
    num_sections, = struct.unpack_from("<H", data, coff + 2)
    opt_size, = struct.unpack_from("<H", data, coff + 16)
    opt = coff + 20
    magic, = struct.unpack_from("<H", data, opt)
    dd = opt + (112 if magic == 0x20b else 96)  # data dirs: PE32+ vs PE32
    import_rva, = struct.unpack_from("<I", data, dd + 1 * 8)  # entry 1 = import
    if not import_rva:
        return [], []

    sechdr = opt + opt_size
    secs = []  # (vaddr, vsize, raw_off, raw_size)
    for i in range(num_sections):
        o = sechdr + i * 40
        vsize, vaddr, raw_size, raw_off = struct.unpack_from("<IIII", data, o + 8)
        secs.append((vaddr, vsize, raw_off, raw_size))

    def rva2off(rva):
        for vaddr, vsize, raw_off, raw_size in secs:
            if vaddr <= rva < vaddr + max(vsize, raw_size):
                return rva - vaddr + raw_off
        return None

    def cstr(off):
        end = data.index(b"\0", off)
        return data[off:end].decode("ascii", "replace")

    names, off = [], rva2off(import_rva)
    if off is None:
        return [], []
    while True:
        name_rva, = struct.unpack_from("<I", data, off + 12)  # descriptor.Name
        if name_rva == 0:
            break
        no = rva2off(name_rva)
        if no is None:
            break
        names.append(cstr(no))
        off += 20
    return names, []


# ---------------------------------------------------------------------------
# Mach-O (macOS): LC_LOAD*_DYLIB + LC_RPATH (thin or fat, first arch is enough)
# ---------------------------------------------------------------------------
def _macho_thin(data, base):
    le = "<" if data[base:base + 4] in (b"\xce\xfa\xed\xfe", b"\xcf\xfa\xed\xfe") else ">"
    is64 = data[base:base + 4] in (b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe")
    ncmds, = struct.unpack_from(le + "I", data, base + 16)
    off = base + (32 if is64 else 28)
    LC_LOAD_DYLIB, LC_LOAD_WEAK, LC_REEXPORT, LC_RPATH = \
        0xc, 0x80000018, 0x8000001f, 0x8000001c
    dylibs, rpaths = [], []
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from(le + "II", data, off)
        if cmd in (LC_LOAD_DYLIB, LC_LOAD_WEAK, LC_REEXPORT):
            so, = struct.unpack_from(le + "I", data, off + 8)
            end = data.index(b"\0", off + so)
            dylibs.append(data[off + so:end].decode("utf-8", "replace"))
        elif cmd == LC_RPATH:
            so, = struct.unpack_from(le + "I", data, off + 8)
            end = data.index(b"\0", off + so)
            rpaths.append((data[off + so:end].decode("utf-8", "replace"), "LC_RPATH"))
        off += cmdsize
    return dylibs, rpaths


def _macho_deps(data, fat):
    if fat:
        base, = struct.unpack_from(">I", data, 8 + 8)  # first fat_arch's offset
        return _macho_thin(data, base)
    return _macho_thin(data, 0)


# ---------------------------------------------------------------------------
# direct dependencies of a single file
# ---------------------------------------------------------------------------
def direct_deps(path):
    """-> (format, [dependency names], [(rpath, kind)]); ([], []) if none."""
    with open(path, "rb") as f:
        data = f.read()
    fmt = detect_format(data)
    try:
        if fmt == "elf":
            return ("elf",) + _elf_deps(data)
        if fmt == "pe":
            return ("pe",) + tuple(_pe_deps(data))
        if fmt == "macho":
            return ("macho",) + tuple(_macho_deps(data, fat=False))
        if fmt == "macho-fat":
            return ("macho",) + tuple(_macho_deps(data, fat=True))
        if fmt == "wasm":
            return ("wasm", [], [])
    except Exception as e:  # never let the diagnostic crash a build
        return ("error: %s" % e, [], [])
    return (fmt or "unknown", [], [])


# ---------------------------------------------------------------------------
# system-library heuristic: OS / language-runtime libraries not bundled
# ---------------------------------------------------------------------------
_SYS_PREFIXES = (
    "api-ms-win-", "ext-ms-", "vcruntime", "msvcp", "msvcr", "concrt",
    "ucrtbase", "kernel32", "kernelbase", "user32", "gdi32", "advapi32",
    "shell32", "ole32", "oleaut32", "ws2_32", "wsock32", "crypt32", "bcrypt",
    "ntdll", "rpcrt4", "shlwapi", "comdlg32", "winmm", "version", "setupapi",
    "dbghelp", "secur32", "iphlpapi", "userenv", "psapi", "python",
    "libc.", "libm.", "libdl.", "librt.", "libpthread.", "libgcc_s.",
    "ld-linux", "libresolv.", "libutil.", "libnsl.", "libomp.",
    "libsystem", "libobjc.", "libc++.", "libc++abi.",
)


def is_system(name):
    # macOS system dylibs live in the dyld shared cache (not on-disk files),
    # so they cannot be located by stat(); treat the system locations as system.
    if name.startswith(("/usr/lib/", "/System/Library/")):
        return True
    base = os.path.basename(name).lower()
    return any(base.startswith(p) for p in _SYS_PREFIXES)


# ---------------------------------------------------------------------------
# resolution: build the ordered (dir, hint) search list, then locate
# ---------------------------------------------------------------------------
def _bundled_dirs(origin):
    """Wheel-repair bundle directories, discovered PORTABLY (no dependency on
    delvewheel/auditwheel/delocate), by their conventional layout:

      * delvewheel (Windows) / auditwheel (Linux): a sibling `<dist>.libs`
        directory of the import package, e.g. `site-packages/openpmd_api.libs`
        -- matched by the `.libs` suffix while walking up from the binary. (The
        dist name differs from the import package, so we match the suffix, not a
        derived name.)
      * delocate (macOS): a `.dylibs` directory inside the package tree, e.g.
        `openpmd_api/.dylibs`.

    The repaired package `__init__` re-adds these to the runtime search path
    (os.add_dll_directory / RPATH `$ORIGIN/../<dist>.libs`), so a library that
    the OS-level search list would call `[not found]` is in fact resolved here.
    Returns an ordered list of (directory, hint)."""
    out, seen = [], set()
    d = os.path.abspath(origin)
    while True:
        dylibs = os.path.join(d, ".dylibs")             # delocate, inside pkg
        if dylibs not in seen and os.path.isdir(dylibs):
            seen.add(dylibs)
            out.append((dylibs, "bundled .dylibs (delocate)"))
        parent = os.path.dirname(d)                     # delvewheel/auditwheel
        if parent and parent != d:                      # sibling *.libs dirs
            try:
                for name in sorted(os.listdir(parent)):
                    cand = os.path.join(parent, name)
                    if (name.endswith(".libs") and cand not in seen
                            and os.path.isdir(cand)):
                        seen.add(cand)
                        out.append((cand, "bundled .libs (delvewheel/auditwheel)"))
            except OSError:
                pass
        # stop once we have climbed past the install root
        if (not parent or parent == d
                or os.path.basename(d) in ("site-packages", "dist-packages")):
            break
        d = parent
    return out


def _search(fmt, origin, rpaths):
    out = []  # ordered list of (directory, hint)

    def expand(p):
        return (p.replace("${ORIGIN}", origin).replace("$ORIGIN", origin)
                 .replace("@loader_path", origin).replace("@executable_path", origin))

    for raw, kind in rpaths:                    # binary's own RPATH/RUNPATH/LC_RPATH
        out.append((expand(raw), kind))
    out += _bundled_dirs(origin)                # delvewheel/auditwheel/delocate
    if fmt == "elf":
        for d in filter(None, os.environ.get("LD_LIBRARY_PATH", "").split(":")):
            out.append((d, "LD_LIBRARY_PATH"))
    elif fmt == "macho":
        for d in filter(None, os.environ.get("DYLD_LIBRARY_PATH", "").split(":")):
            out.append((d, "DYLD_LIBRARY_PATH"))
    out.append((origin, "binary dir"))          # where a wheel bundles its libs
    if fmt == "elf":
        for d in ("/lib", "/usr/lib", "/lib64", "/usr/lib64",
                  "/usr/lib/x86_64-linux-gnu", "/usr/local/lib"):
            out.append((d, "default path"))
    elif fmt == "pe":
        out.append((os.path.join(os.environ.get("SystemRoot", r"C:\Windows"),
                                 "System32"), "System32"))
        for d in filter(None, os.environ.get("PATH", "").split(os.pathsep)):
            out.append((d, "PATH"))
    elif fmt == "macho":
        for d in ("/usr/lib", "/usr/local/lib", "/opt/homebrew/lib"):
            out.append((d, "default path"))
    return out


def _resolve(name, search, origin):
    """-> (resolved_path, hint) or (None, None)."""
    if name.startswith(("@loader_path/", "@executable_path/")):
        cand = os.path.join(origin, name.split("/", 1)[1])
        return (cand, "@loader_path") if os.path.exists(cand) else (None, None)
    if name.startswith("@rpath/"):
        base = name[len("@rpath/"):]
        for d, hint in search:
            cand = os.path.join(d, base)
            if os.path.exists(cand):
                return cand, "@rpath via " + hint
        return None, None
    if os.path.isabs(name):
        return (name, "absolute path") if os.path.exists(name) else (None, None)
    base = os.path.basename(name)
    for d, hint in search:
        cand = os.path.join(d, base)
        if os.path.exists(cand):
            return cand, hint
    return None, None


def walk(path, _seen=None, _depth=0, _out=None):
    """Recursively collect the dependency tree. Recurses only into non-system,
    locatable libraries; system libs and missing libs are leaves.
    Yields (depth, name, status, hint, resolved_path)."""
    if _out is None:
        _out = []
    if _seen is None:
        _seen = set()
    fmt, deps, rpaths = direct_deps(path)
    origin = os.path.dirname(os.path.abspath(path))
    search = _search(fmt, origin, rpaths)
    for name in deps:
        if is_system(name):
            _out.append((_depth, name, "system", "", None))
            continue
        resolved, hint = _resolve(name, search, origin)
        if resolved is None:
            _out.append((_depth, name, "missing", "", None))
            continue
        _out.append((_depth, name, "found", hint, resolved))
        key = os.path.basename(name).lower()
        if key not in _seen:
            _seen.add(key)
            walk(resolved, _seen, _depth + 1, _out)
    return _out


def report(path):
    """Print the dependency tree of one binary; return (#external, #missing)."""
    fmt, _, _ = direct_deps(path)
    print("== %s  [%s]" % (path, fmt))
    tree = walk(path)
    if not tree:
        print("   (no dynamic dependencies)")
        return 0, 0
    external = missing = 0
    for depth, name, status, hint, resolved in tree:
        if status == "system":
            label = "[system]"
        elif status == "missing":
            label = "[not found]"
            missing += 1
            external += 1
        else:
            label = "[%s -> %s]" % (hint, resolved)
            external += 1
        print("   " + "  " * depth + name + "   " + label)
    print("   summary: %d external (non-system) dependenc%s, %d not found"
          % (external, "y" if external == 1 else "ies", missing))
    return external, missing


def main(argv):
    strict = "--strict" in argv
    paths = [a for a in argv if not a.startswith("--")]
    if not paths:
        print("usage: list_library_deps.py [--strict] <binary> [<binary> ...]")
        return 0
    total_missing = 0
    for p in paths:
        _external, missing = report(p)
        total_missing += missing
        print()
    if strict and total_missing:
        print("ERROR: %d dependency/-ies not found (--strict)" % total_missing)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
