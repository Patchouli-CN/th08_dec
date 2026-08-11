#!/usr/bin/env python
"""Replace raw `*(type*)((u8*)this/player + 0xNN)` offset accesses with named struct fields
for offsets that now have real field names in Player.hpp."""
import re
import sys

# offset -> (field, type)
MAP = {
    0x4: ("unk4", "u8"), 0x5: ("unk5", "u8"), 0x6: ("unk6", "u8"),
    0xfe4: ("shotInterval", "i32"), 0xfec: ("powerLevel", "i32"),
    0xff4: ("shotTimer", "ZunTimer"),
    0xe2a68: ("shotIndex", "i32"), 0xe2a6c: ("shotCooldown", "i32"),
    0xe2a70: ("unkE2a70", "i32"), 0xe2a7c: ("shotState", "i32"),
    0xe2a90: ("unkE2a90", "i32"), 0xe2a98: ("unkE2a98", "i32"),
    0xe2aa4: ("unkE2aa4", "Float3"), 0xe2ab0: ("unkE2ab0", "Float3"),
    0xe2ac0: ("unkE2ac0", "i32"), 0xe2ac4: ("shotTimer2", "ZunTimer"),
    0xe2ad0: ("unkE2ad0", "ZunTimer"), 0xe2ae8: ("unkE2ae8", "ZunTimer"),
    0xe2b0c: ("unkE2b0c", "f32"), 0xe2b1c: ("unkE2b1c", "i32"),
    0xe2b24: ("unkE2b24", "i32"), 0xe2b28: ("unkE2b28", "i32"),
    0xe2b2c: ("unkE2b2c", "i32"),
}

# cast helper types that get replaced by a field reference directly
TYPE_FIX = {"i8", "u8", "i16", "u16", "i32", "u32", "f32", "f64"}

def repl_line(ln):
    out = ln
    for var in ("this", "player"):
        for off, (field, ftype) in MAP.items():
            offs = f"0x{off:x}"
            # 1. dereferenced access: *(i32 *)((u8 *)this + 0xfe4) -> this->field
            pat = re.compile(
                r"\*\s*\(\s*(u8|i8|i16|i32|u16|u32|f32|f64|Float3)\s*\*\s*\)\s*\(\s*\(u8 \*\)%s\s*\+\s*%s\s*\)" % (var, offs)
            )
            out = pat.sub(lambda m: "%s->%s" % (var, field), out)
            # 2. pointer cast: (ZunTimer *)((u8 *)this + 0xff4) -> &this->field
            pat2 = re.compile(
                r"\(\s*(ZunTimer|Float3)\s*\*\s*\)\s*\(\s*\(u8 \*\)%s\s*\+\s*%s\s*\)" % (var, offs)
            )
            out = pat2.sub(lambda m: "&%s->%s" % (var, field), out)
    # 3. cleanup: *&x->f -> x->f ; (&x->f)->m -> x->f.m
    out = re.sub(r"\*&(this|player)->", r"\1->", out)
    out = re.sub(r"\(&(this|player)->([A-Za-z0-9_]+)\)->", r"\1->\2.", out)
    return out

def main():
    if len(sys.argv) < 2:
        print("usage: readability_replace.py <file.cpp> [--apply]")
        return
    fp = sys.argv[1]
    apply = "--apply" in sys.argv
    lines = open(fp, encoding="utf-8").read().split("\n")
    changed = 0
    new_lines = []
    for i, ln in enumerate(lines, 1):
        new = repl_line(ln)
        if new != ln:
            changed += 1
            if not apply:
                print(f"  L{i}: {ln.strip()}\n      -> {new.strip()}")
        new_lines.append(new)
    print(f"changed lines: {changed}")
    if apply:
        open(fp, "w", encoding="utf-8").write("\n".join(new_lines))
        print("applied")

if __name__ == "__main__":
    main()
