#!/usr/bin/env python3
"""Analyze PRIVATE PlayerCtrl IL from an owned Corn Kidz 64 v1.029 copy.

Does not ship binaries or raw IL. Usage:
python3 tools/verify_player_movpl_il.py path/to/playerctrl_il_private.json
"""
import argparse
import collections
import json
from pathlib import Path

def summarize(path: Path) -> dict:
    obj=json.loads(path.read_text(encoding="utf-8"))
    methods={m["name"]:m for m in obj["methods"]}
    motion=methods["PlayerCtrl::MovPl"]
    instr=motion["instructions"]
    calls=collections.Counter(
        x["operand"].get("target","") for x in instr
        if x["opcode"] in ("call","callvirt")
        and isinstance(x.get("operand"),dict))
    fields=collections.Counter(
        x["operand"].get("target","") for x in instr
        if x["opcode"] in ("ldfld","ldflda","stfld")
        and isinstance(x.get("operand"),dict))
    run=methods["PlayerCtrl::Run"]["instructions"]
    sites=[i for i,x in enumerate(run)
        if x["opcode"]=="call" and isinstance(x.get("operand"),dict)
        and x["operand"].get("target")=="PlayerCtrl::MovPl"]
    patterns=collections.Counter()
    for i in sites:
        flags=[x["opcode"][-1] for x in run[max(0,i-14):i]
               if x["opcode"] in ("ldc.i4.0","ldc.i4.1")]
        patterns[",".join(flags[-2:]) if len(flags)>=2 else "nonconstant"]+=1
    assert motion["token"]=="0x060002a0"
    assert motion["il_bytes"]==565 and len(instr)==170
    assert len(sites)==33
    for name,count in {
      "UnityEngine.Mathf::MoveTowards":1,
      "UnityEngine.Mathf::Lerp":1,
      "UnityEngine.Vector3::MoveTowards":1,
      "UnityEngine.Vector3::Lerp":1,
      "UnityEngine.Quaternion::FromToRotation":2,
      "UnityEngine.Quaternion::RotateTowards":2
    }.items():
        assert calls[name]==count, (name,calls[name])
    for field in ("PlayerCtrl.zMov","PlayerCtrl.plVel","PlayerCtrl.ctrlDir",
                  "PlayerCtrl.ctrlRot","PlayerCtrl.floorNorm"):
        assert fields[field]>0,field
    return {
        "method":motion["name"],"token":motion["token"],
        "il_bytes":motion["il_bytes"],"instructions":len(instr),
        "run_calls":len(sites),
        "flag_pairs_heuristic":dict(patterns),
        "relevant_fields":{k:v for k,v in sorted(fields.items())
                           if k.startswith("PlayerCtrl.")},
        "unity_math_calls":{k:v for k,v in sorted(calls.items())
                           if k.startswith(("UnityEngine.Vector3::",
                                            "UnityEngine.Quaternion::",
                                            "UnityEngine.Mathf::"))},
        "scope":"structural evidence, NOT an assertion of float/Unity parity"
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("private_il_json",type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    report=json.dumps(summarize(args.private_il_json),indent=2,sort_keys=True)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(report,encoding="utf-8")
    print(report)

if __name__=="__main__":
    main()
