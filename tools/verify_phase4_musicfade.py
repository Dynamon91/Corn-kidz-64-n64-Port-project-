#!/usr/bin/env python3
"""Verify reported MusicFade facts against decoded GameCtrl IL evidence.

Checks structural invariants only. Passing does not prove C/gameplay equivalence.
"""
import json
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "reports/gamectrl_il.json"
    blob = json.loads(path.read_text(encoding="utf-8"))
    methods = [m for m in blob["methods"] if m["name"] == "GameCtrl::MusicFade"]
    if len(methods) != 1:
        raise AssertionError("Missing or duplicated GameCtrl::MusicFade method")
    m = methods[0]
    il = m["instructions"]
    at = {int(i["offset"]): i for i in il}
    checks = []

    def verify(label, condition):
        if not condition:
            raise AssertionError(label)
        checks.append(label)

    def op(offset, opcode, target=None):
        instruction = at.get(offset)
        if instruction is None or instruction["opcode"] != opcode:
            return False
        if target is not None:
            operand = instruction.get("operand")
            return isinstance(operand, dict) and operand.get("target") == target
        return True

    def signed_float(offset, expected):
        i = at.get(offset)
        return (i and i["opcode"] == "ldc.r4" and
                abs(float(i["operand"]) - expected) < 1e-7)

    verify("570-byte method body", m["il_bytes"] == 570)
    verify("196 IL instructions", len(il) == 196)
    verify("currentTrack and nextTrack gate", op(1, "ldfld", "GameCtrl.nextTrack") and
           op(7, "ldfld", "GameCtrl.currentTrack"))
    verify("main channel fade-out constant", signed_float(48, .04))
    verify("main channel fade-in constant", signed_float(91, .04))
    verify("sub channel fade-in constant", signed_float(226, .04))
    verify("sub channel fade-out constant", signed_float(284, .04))
    verify("main clip replacement", op(174, "callvirt", "UnityEngine.AudioSource::set_clip"))
    verify("main channel Play", op(187, "callvirt", "UnityEngine.AudioSource::Play"))
    verify("sub channel isPlaying condition", op(251, "callvirt", "UnityEngine.AudioSource::get_isPlaying"))
    verify("sub channel Stop", op(350, "callvirt", "UnityEngine.AudioSource::Stop"))
    verify("pitch adjustment 0.02 (main)", signed_float(403, .02))
    verify("pitch adjustment 0.02 (sub)", signed_float(445, .02))
    verify("main pitch reset 0.02", signed_float(505, .02))
    verify("pitch gate from sub channel", op(363, "callvirt", "UnityEngine.AudioSource::get_pitch"))
    verify("music enabled gate", op(526, "ldfld", "Data.bMusic"))
    verify("hard mute main", op(546, "callvirt", "UnityEngine.AudioSource::set_volume"))
    verify("hard mute sub", op(564, "callvirt", "UnityEngine.AudioSource::set_volume"))
    verify("end of method", op(569, "ret"))
    print(f"{len(checks)}/{len(checks)} MusicFade IL evidence checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
