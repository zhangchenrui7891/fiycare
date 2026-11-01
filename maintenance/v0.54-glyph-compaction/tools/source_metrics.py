"""Audit first-party vs third-party source and the C++ files actually in the build.

Use --compile-commands on a local CMake build, or the shipped manifest when no
compiler is installed. Physical lines include comments/blanks. Non-comment
lines are a lexical approximation, not a claim about implementation effort.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re

CPP = {".cpp", ".cc", ".cxx"}
SOURCE = CPP | {".h", ".hpp", ".hxx", ".inl"}


def counts(paths):
    physical = nonblank = approximate_code = 0
    pattern = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*')
    for path in paths:
        text = path.read_text(errors="replace")
        physical += len(text.splitlines())
        nonblank += sum(bool(line.strip()) for line in text.splitlines())

        def strip(match):
            value = match.group(0)
            return "\n" * value.count("\n") if value.startswith(("/*", "//")) else value

        stripped = pattern.sub(strip, text)
        approximate_code += sum(bool(line.strip()) for line in stripped.splitlines())
    return {"files": len(paths), "physical_lines": physical,
            "nonblank_lines": nonblank, "approx_noncomment_lines": approximate_code}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compile-commands", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    own = sorted(p for p in root.rglob("*") if p.suffix in SOURCE and p.is_file()
                 and "third_party" not in p.relative_to(root).parts
                 and "out" not in p.relative_to(root).parts)
    third_party = sorted(p for p in (root / "third_party").rglob("*") if p.suffix in SOURCE and p.is_file())
    if args.compile_commands:
        commands = json.loads(args.compile_commands.read_text())
        active = sorted({Path(entry["file"]).resolve() for entry in commands if Path(entry["file"]).suffix in CPP})
        relative = [p.relative_to(root) for p in active]
        manifest = "\n".join(str(p).replace("\\", "/") for p in relative) + "\n"
        (root / "COMPILED_CPP_MANIFEST.txt").write_text(manifest)
        (root / "COMPILED_CPP_SHA256.json").write_text(json.dumps({str(p): hashlib.sha256((root/p).read_bytes()).hexdigest()
                                                                  for p in relative}, indent=2) + "\n")
    else:
        active = [root / line for line in (root / "COMPILED_CPP_MANIFEST.txt").read_text().splitlines() if line]
    active_own = [p for p in active if "third_party" not in p.relative_to(root).parts]
    active_third_party = [p for p in active if p not in active_own]
    result = {
        "counting_note": "Headers included in repository counts; compiled counts include .cpp/.cc/.cxx only. Third-party code is not first-party work.",
        "platform_of_verified_build": "Linux GCC 13.3, CMake 3.31.6, Release",
        "first_party_all_cpp_and_headers": counts(own),
        "third_party_all_cpp_and_headers": counts(third_party),
        "compiled_cpp": counts(active),
        "compiled_first_party_cpp": counts(active_own),
        "compiled_third_party_cpp": counts(active_third_party),
        "target_at_least_200_compiled_cpp": len(active) >= 200,
        "target_at_least_50000_physical_lines": counts(active)["physical_lines"] >= 50000,
        "target_at_least_50000_approx_noncomment_lines": counts(active)["approx_noncomment_lines"] >= 50000
    }
    (root / "SOURCE_METRICS.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
