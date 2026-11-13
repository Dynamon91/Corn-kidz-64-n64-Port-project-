#!/usr/bin/env python3
"""Check reconstructed Data schema & ResetSav mapping against extracted IL.

Operates on archived reports only; it never executes or distributes game code.
"""
from __future__ import annotations
import argparse
import csv
import json
import re
from pathlib import Path

RESET_FIELDS = {
    'switches','items','upgrades','ints','cranks','lvlItems1','itemTotal',
    'currentLvl','HPupTotal','screwTotal','maxHP','totalDeaths','gameTime',
    'bNonEmpty','currentCamMode','camInvertOff','bRumble','bSkipLauncher',
    'bWidescreen','clothes'
}
ARRAYS = {
    'switches':('bool[]',1000),'items':('bool[]',1000),
    'upgrades':('bool[]',100),'ints':('int32[]',100),
    'cranks':('int32[]',100),'lvlItems1':('int32[]',100)
}

def verify(root:Path) -> dict:
    data=json.loads((root/'reports/data_fields.json').read_text())
    il=json.loads((root/'reports/gamectrl_il.json').read_text())
    header=(root/'portable/include/ck64_save.h').read_text()
    impl=(root/'portable/src/ck64_save.c').read_text()
    tests=(root/'portable/tests/test_save.c').read_text()
    assert data['target_type']=='Data'
    fields=data['fields'];names={f['name'] for f in fields}
    checks={}
    checks['source_has_34_fields']=len(fields)==34 and len(names)==34
    checks['source_six_arrays']=all(next((f['type'] for f in fields if f['name']==k),None)==v[0] for k,v in ARRAYS.items())
    match=re.search(r'typedef struct CK64SaveData\s*\{(.*?)\}\s*CK64SaveData',header,re.S)
    if not match:raise ValueError('Missing C save struct')
    c_fields={}
    for typ,name in re.findall(r'\b(bool|int32_t|uint8_t)\s+(\w+)\s*(?:\[(?:\w+)\])?\s*;',match.group(1)):
        if name in c_fields:raise ValueError('Repeated C field '+name)
        c_fields[name]=typ
    checks['c_struct_has_exact_same_34_names']=set(c_fields)==names and len(c_fields)==34
    checks['c_scalar_types_match_metadata']=all(
        c_fields.get(f['name']) == ('uint8_t' if f['type']=='bool[]' else 'int32_t' if f['type'] in ('int32','int32[]') else 'bool')
        for f in fields)
    reset=next(m for m in il['methods'] if m['name']=='GameCtrl::ResetSav')
    stfld=[str(i['operand'].get('target','')).split('.',1)[1]
           for i in reset['instructions'] if i['opcode']=='stfld' and
           isinstance(i['operand'],dict) and str(i['operand'].get('target','')).startswith('Data.')]
    checks['reset_writes_exactly_20_fields']=set(stfld)==RESET_FIELDS and len(stfld)==20
    reset_body=re.search(r'void ck64_save_reset\([^)]*\)\s*\{(.*?)\n\}',impl,re.S)
    if not reset_body:raise ValueError('Missing save_reset definition')
    body=reset_body.group(1)
    explicit_writes=set(re.findall(r'\bd->(\w+)\s*=',body))
    arrays_zeroed=set(re.findall(r'memset\(d->(\w+),0,',body))
    checks['reset_c_assigns_only_source_fields']=(explicit_writes|arrays_zeroed)==RESET_FIELDS
    checks['reset_defaults_match_source'] = 'd->currentLvl=1;' in body and 'd->maxHP=8;' in body
    checks['remaining_14_settings_preserved'] = len(names-RESET_FIELDS)==14 and not (names-RESET_FIELDS)&(explicit_writes|arrays_zeroed)
    encoded=next(m for m in il['methods'] if m['name']=='GameCtrl::SaveGame')
    decoded=next(m for m in il['methods'] if m['name']=='GameCtrl::LoadGame')
    checks['binaryformatter_source_confirmed']=all(
        any('BinaryFormatter::'+target in str(i['operand']) for i in m['instructions'])
        for m,target in ((encoded,'Serialize'),(decoded,'Deserialize')))
    checks['save_file_name_confirmed']=any('save1.cow' in str(i['operand']) for i in encoded['instructions'])
    checks['fixed_wire_length']=all(s in header for s in ('CK64_SAVE_HEADER_SIZE = 20','CK64_SAVE_PAYLOAD_SIZE = 1513'))
    checks['test_includes_independent_crc_golden']=('independent Python zlib.crc32' in tests)
    # Verify original ResetSav newarr capacities via the IL call sequence.
    newarr={}
    ins=reset['instructions']
    for i,v in enumerate(ins):
        if v['opcode']=='newarr':
            field=next((s['operand']['target'].split('.',1)[1] for s in ins[i+1:] if s['opcode']=='stfld' and isinstance(s['operand'],dict) and str(s['operand'].get('target','')).startswith('Data.')),None)
            prev=ins[i-1]
            size=(int(prev['operand']) if prev['opcode'] in ('ldc.i4','ldc.i4.s') else
                  int(prev['opcode'].split('.')[-1]) if prev['opcode'].startswith('ldc.i4.') else None)
            if field:newarr[field]=size
    checks['reset_original_array_capacities']=all(newarr.get(k)==v[1] for k,v in ARRAYS.items())
    return {'verified_checks':checks, 'all_pass':all(checks.values()),
            'field_count':len(fields),'reset_field_count':len(RESET_FIELDS),
            'other_fields_preserved':sorted(names-RESET_FIELDS),
            'original_array_capacities':newarr}

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('root',type=Path)
    a.add_argument('--output',type=Path)
    args=a.parse_args()
    result=verify(args.root)
    print(json.dumps(result,indent=2))
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n')
    if not result['all_pass']:raise SystemExit(1)
if __name__=='__main__':main()
