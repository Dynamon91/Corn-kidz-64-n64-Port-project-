#!/usr/bin/env python3
"""Recover ECMA-335 Data field names/types from CLI Field signatures, no code execution."""
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path
from dotnet_metadata_inventory import parse_pe, parse_streams, u8, u16, u32, u64, read_cstring, inspect

TYPE_CODES = {0x02:'bool',0x03:'char',0x04:'int8',0x05:'uint8',0x06:'int16',0x07:'uint16',0x08:'int32',0x09:'uint32',0x0a:'int64',0x0b:'uint64',0x0c:'float32',0x0d:'float64',0x0e:'string',0x1c:'object'}

def compressed_uint(blob:bytes, off:int=0):
    n=blob[off]
    if n<0x80:return n,off+1
    if n<0xc0:return ((n&0x3f)<<8)|blob[off+1],off+2
    if n<0xe0:return ((n&0x1f)<<24)|(blob[off+1]<<16)|(blob[off+2]<<8)|blob[off+3],off+4
    raise ValueError('Invalid CLI compressed unsigned integer')

def decode_field(blob:bytes):
    if not blob or blob[0] != 0x06:raise ValueError('Not a field signature')
    def inner(i):
        typ=blob[i]
        if typ==0x1d:
            sub,j=inner(i+1)
            return sub+'[]',j
        if typ in TYPE_CODES:return TYPE_CODES[typ],i+1
        raise ValueError(f'Unknown element code 0x{typ:x} in {blob.hex()}')
    out,stop=inner(1)
    if stop!=len(blob):raise ValueError(f'Trailing field signature bytes: {blob.hex()}')
    return out

def extract(path: Path, type_name: str='Data') -> dict:
    data=path.read_bytes()
    pe=parse_pe(data)
    streams,_=parse_streams(data,pe['metadata_offset'])
    tables=streams.get('#~',streams.get('#-'))
    if tables is None:raise ValueError('Missing metadata table stream')
    off=tables['offset']; strings=streams['#Strings']['offset']; blobs=streams['#Blob']['offset']
    flags=u8(data,off+6); valid=u64(data,off+8); pos=off+24; rows={}
    for tid in range(64):
        if valid&(1<<tid):rows[tid]=u32(data,pos);pos+=4
    sw=4 if flags&1 else 2; gw=4 if flags&2 else 2; bw=4 if flags&4 else 2
    rid=lambda t:4 if rows.get(t,0)>=65536 else 2
    coded=lambda ts,bits:4 if max(rows.get(t,0) for t in ts)>=(1<<(16-bits)) else 2
    sizes={0:2+sw+3*gw,1:coded((0,26,35,1),2)+2*sw,2:4+2*sw+coded((2,1,27),2)+rid(4)+rid(6),3:rid(4),4:2+sw+bw}
    field_table=pos
    for tid in range(4):
        if tid in rows:field_table+=sizes[tid]*rows[tid]
    existing=inspect(path)['types']; td=next((i for i,t in enumerate(existing) if t['name']==type_name),None)
    if td is None:raise ValueError('Type not found '+type_name)
    first=existing[td]['field_list'];last=existing[td+1]['field_list'] if td+1<len(existing) else rows.get(4,0)+1
    def idx(p,w):return u16(data,p) if w==2 else u32(data,p)
    fields=[]
    for ridnum in range(first,last):
        row=field_table+(ridnum-1)*sizes[4]
        field_flags=u16(data,row)
        name=read_cstring(data,strings+idx(row+2,sw))
        blobpos=blobs+idx(row+2+sw,bw)
        length,start=compressed_uint(data,blobpos)
        sig=data[start:start+length]
        fields.append({'token':f'0x0400{ridnum:04x}', 'name':name, 'type':decode_field(sig), 'flags':field_flags, 'signature_hex':sig.hex()})
    return {'source_dll':path.name,'target_type':type_name,'field_count':len(fields),'fields':fields}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('assembly',type=Path)
    parser.add_argument('--type',default='Data')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    schema=extract(args.assembly,args.type)
    for f in schema['fields']:print(f['token'],f['type'],f['name'])
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(schema,indent=2)+'\n')

if __name__=='__main__':main()
