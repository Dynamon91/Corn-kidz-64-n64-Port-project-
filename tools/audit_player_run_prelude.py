#!/usr/bin/env python3
"""Audit *private* PlayerCtrl IL for the public Phase 16 C11 prelude.

Usage: python3 tools/audit_player_run_prelude.py PRIVATE/playerctrl_il.json
This script does not upload game binaries, private IL, or Unity resources.
"""
import argparse
import json
from pathlib import Path


def analyze(doc):
    methods = {m["name"]: m for m in doc["methods"]}
    run = methods["PlayerCtrl::Run"]
    assert run["token"] == "0x06000294"
    assert run["il_bytes"] == 76019
    a = run["instructions"]
    assert len(a) == 22052
    offsets = {i["offset"]: i for i in a}

    def check(off, opcode, operand=None):
        i = offsets[off]
        assert i["opcode"] == opcode, (off, i)
        actual = i["operand"]
        if isinstance(actual, dict):
            actual = actual.get("target")
        if operand is not None:
            assert actual == operand, (off, actual, operand)

    for off, field in [(1, "changeState"), (7, "plState")]:
        check(off, "ldfld", "PlayerCtrl." + field)
    check(12, "beq", 350)
    for off, field in [(19, "plTimer"), (26, "plTimer2"),
                       (33, "plTimer3"), (40, "plCounter"),
                       (51, "plFloat"), (91, "bNoHurt"),
                       (103, "lastState"), (115, "changeState"),
                       (217, "bNoCol"), (224, "bNoLook"),
                       (231, "wallObj")]:
        check(off, "stfld", "PlayerCtrl." + field)
    check(291, "bne.un.s", 308)
    check(316, "bne.un.s", 333)
    check(303, "call", "PlayerCtrl::StopSFX")
    check(328, "call", "PlayerCtrl::StopSFX")
    check(361, "stfld", "PlayerCtrl.plSwitch")

    check(439, "stfld", "PlayerCtrl.rumbleTimer")
    check(464, "call", "UnityEngine.Mathf::MoveTowards")
    check(492, "call", "UnityEngine.Mathf::MoveTowards")
    check(503, "ldfld", "PlayerCtrl.bNoCtrl")
    check(508, "brtrue", 1812)
    check(1796, "stfld", "PlayerCtrl.Atimer")
    check(1805, "stfld", "PlayerCtrl.Atimer")
    input_cleared = ["Adown", "Aup", "Ahold", "Xdown", "Xhold", "Bdown",
                     "Ydown", "Yhold", "RTTimer", "LTTimer", "ctrlX", "ctrlY",
                     "Atimer"]
    a_disabled = [i["operand"]["target"].split(".", 1)[1]
                  for i in a if 1812 <= i["offset"] <= 1910
                  and i["opcode"] == "stfld"]
    assert a_disabled == input_cleared, a_disabled
    check(1930, "stfld", "PlayerCtrl.camType")
    check(1949, "stfld", "PlayerCtrl.camType")
    check(1975, "stfld", "PlayerCtrl.camType")
    check(2032, "switch", [2058, 3062, 3679, 4234])
    assert offsets[1929]["opcode"] == "ldc.i4.2"
    assert offsets[1948]["opcode"] == "ldc.i4.3"
    assert offsets[1974]["opcode"] == "ldc.i4.1"

    return {
        "original_method": "PlayerCtrl::Run",
        "original_token": run["token"],
        "il_bytes": run["il_bytes"],
        "decoded_instructions": len(a),
        "entry_guard": "changeState != plState",
        "entry_reset_fields": ["plTimer", "plTimer2", "plTimer3", "plCounter",
                               "plFloat", "bNoHurt", "lastState", "changeState",
                               "bNoCol", "bNoLook", "wallObj"],
        "no_control_zeroed_fields": input_cleared,
        "camera_modes_dispatched": [1, 2, 3, 4],
        "state18_selects_camera3_except_underwater_default": True,
        "verified_call_counts": {
            name: sum(1 for i in a if i["opcode"] == "call" and
                      isinstance(i.get("operand"), dict) and
                      i["operand"].get("target") == name)
            for name in ("PlayerCtrl::Gravity", "PlayerCtrl::MovPl")},
        "scope": "Run prelude only; full gameplay state machine not ported",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("private_il", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    summary = analyze(json.loads(args.private_il.read_text(encoding="utf-8")))
    out = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(out, encoding="utf-8")
    print(out)


if __name__ == "__main__":
    main()
