#!/usr/bin/env python3
"""Cross-check technical claims against the disassembled methods (no game execution)."""
from __future__ import annotations
import argparse
import json
from pathlib import Path

def getmethod(doc, name):
    for m in doc['methods']:
        if m['name'].endswith('::' + name): return m['instructions']
    raise AssertionError('Missing method ' + name)

def has(m, opcode, target=None):
    return any(op['opcode'] == opcode and
               (target is None or
                (isinstance(op['operand'],dict) and op['operand']['target']==target) or
                (not isinstance(op['operand'],dict) and op['operand']==target))
               for op in m)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('gamectrl',type=Path)
    ap.add_argument('coroutine',type=Path)
    ap.add_argument('--output',type=Path)
    a=ap.parse_args()
    ctrl=json.loads(a.gamectrl.read_text(encoding='utf-8'))
    coro=json.loads(a.coroutine.read_text(encoding='utf-8'))
    checks={}
    def check(n, condition):
        checks[n]=bool(condition)
        if not condition: raise AssertionError('Evidence mismatch: ' + n)
    awake=getmethod(ctrl,'Awake')
    check('Awake configures nominal 30FPS',has(awake,'ldc.i4.s',30)
          and has(awake,'call','UnityEngine.Application::set_targetFrameRate'))
    for threshold in (70,140,200,300,360,999):
        check(f'Awake level threshold {threshold}',
              has(awake,'ldc.i4',threshold) or has(awake,'ldc.i4.s',threshold))
    check('SetCurrentLvl compares items against threshold',
          has(getmethod(ctrl,'SetCurrentLvl'),'ldelem.i4')
          and has(getmethod(ctrl,'SetCurrentLvl'),'ldfld','Data.itemTotal')
          and has(getmethod(ctrl,'SetCurrentLvl'),'stfld','Data.currentLvl'))
    fade=getmethod(ctrl,'FadeToScene')
    check('FadeToScene triggers UI transition',has(fade,'callvirt','UI::transOut'))
    check('FadeToScene starts coroutine',has(fade,'call','GameCtrl::WaitForFade')
          and has(fade,'call','UnityEngine.MonoBehaviour::StartCoroutine'))
    move=getmethod(coro,'MoveNext')
    check('WaitForFade 50-iteration loop',has(move,'ldc.i4.s',50)
          and has(move,'blt.s')
          and has(move,'call','GameCtrl::LoadScene'))
    check('SaveGame .NET BinaryFormatter',
          has(getmethod(ctrl,'SaveGame'),'callvirt',
              'System.Runtime.Serialization.Formatters.Binary.BinaryFormatter::Serialize')
          and has(getmethod(ctrl,'SaveGame'),'ldstr',"'/save1.cow'"))
    check('LoadGame .NET BinaryFormatter',
          has(getmethod(ctrl,'LoadGame'),'callvirt',
              'System.Runtime.Serialization.Formatters.Binary.BinaryFormatter::Deserialize'))
    check('Die increments totalDeaths',
          has(getmethod(ctrl,'Die'),'stfld','Data.totalDeaths'))
    check('LoadScene stores lastScene',
          has(getmethod(ctrl,'LoadScene'),'stfld','GameCtrl.lastScene'))
    report={'result':'PASS','checks_passed':len(checks),'checks':checks,
            'scope':'Static IL assertions only; not behavioral equivalence to game.'}
    if a.output:
        a.output.parent.mkdir(parents=True,exist_ok=True)
        a.output.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f"PASS: {len(checks)} source IL assertions")

if __name__=='__main__': main()
