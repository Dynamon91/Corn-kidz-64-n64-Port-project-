#!/usr/bin/env python3
"""Audit a PRIVATE user-owned PlayerCtrl.Run IL extraction, without publishing IL."""
import argparse,json
from pathlib import Path
def audit(path):
 obj=json.loads(Path(path).read_text())
 m=next(x for x in obj["methods"] if x["name"]=="PlayerCtrl::Run")
 assert m["token"]=="0x06000294" and len(m["instructions"])==22052
 ins={x["offset"]:x for x in m["instructions"]}
 fields={75851:("ldfld","PlayerCtrl.plTimer"),
 75858:("stfld","PlayerCtrl.plTimer"),
 75865:("ldfld","PlayerCtrl.plTimer2"),
 75872:("stfld","PlayerCtrl.plTimer2"),
 75879:("ldfld","PlayerCtrl.plTimer3"),
 75886:("stfld","PlayerCtrl.plTimer3"),
 75893:("stfld","PlayerCtrl.bNoAir"),
 75899:("ldfld","PlayerCtrl.bOnFloor"),
 75907:("ldfld","PlayerCtrl.onFloorTimer"),
 75917:("stfld","PlayerCtrl.onFloorTimer"),
 75924:("ldfld","PlayerCtrl.onFloorTimer"),
 75931:("stfld","PlayerCtrl.onFloorTimer"),
 75939:("ldfld","PlayerCtrl.onFloorTimer"),
 75949:("stfld","PlayerCtrl.onFloorTimer"),
 75956:("ldfld","PlayerCtrl.onFloorTimer"),
 75963:("stfld","PlayerCtrl.onFloorTimer"),
 75969:("ldfld","PlayerCtrl.bOnFloor"),
 75977:("ldfld","PlayerCtrl.plState"),
 75987:("ldfld","PlayerCtrl.plState"),
 75997:("ldfld","PlayerCtrl.plState"),
 76007:("stfld","PlayerCtrl.lastWallObj"),
 76018:("ret",None)}
 for offset,(op,target) in fields.items():
  i=ins[offset];val=i.get("operand")
  if isinstance(val,dict):val=val.get("target")
  assert (i["opcode"],val)==(op,target),(offset,i)
 branches={75904:("brfalse.s",75938),75913:("bge.s",75922),
  75936:("br.s",75968),75945:("ble.s",75954),
  75974:("brtrue.s",76005),75984:("beq.s",76005),
  75994:("beq.s",76005),76003:("bne.un.s",76012)}
 for offset,(op,dest) in branches.items():
  i=ins[offset];assert i["opcode"]==op and i["operand"]==dest
 return {"method":"PlayerCtrl::Run","token":"0x06000294",
         "source_offset_range":[75849,76018],
         "verified_instruction_count":len(fields),
         "verified_branch_count":len(branches),
         "disclaimer":"Private IL source structural audit, not physical gameplay equivalence"}
if __name__=="__main__":
 p=argparse.ArgumentParser();p.add_argument("private_il");a=p.parse_args()
 print(json.dumps(audit(a.private_il),indent=2))
