#!/usr/bin/env python3
"""Read-only ECMA-335 IL disassembly for user-supplied managed code.

Designed to recover fully specified instruction operands and metadata names for
Corn Kidz 64's GameCtrl; does not execute assemblies or claim decompilation.
Python 3.9+, stdlib only.
"""
from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path
from dotnet_metadata_inventory import (u8, u16, u32, u64, read_cstring,
    parse_pe, parse_streams, rva_to_offset)

# Operand encoding: i8, i32, i64, f32, f64, u8, u16, u32, short branch,
# long branch, N-way switch, or absent. The CLI opcode byte table is ECMA-335.
OP = {}
def _add(spec, encoding=''):
    for c, name in spec:
        OP[c] = (name, encoding)
_add([(i, n) for i, n in enumerate([
    'nop','break','ldarg.0','ldarg.1','ldarg.2','ldarg.3',
    'ldloc.0','ldloc.1','ldloc.2','ldloc.3',
    'stloc.0','stloc.1','stloc.2','stloc.3'])])
_add([(0x0E,'ldarg.s'),(0x0F,'ldarga.s'),(0x10,'starg.s'),
      (0x11,'ldloc.s'),(0x12,'ldloca.s'),(0x13,'stloc.s')], 'u8')
_add([(0x14,'ldnull'),(0x15,'ldc.i4.m1')])
_add([(i,f'ldc.i4.{i-0x16}') for i in range(0x16,0x1F)])
_add([(0x1F,'ldc.i4.s')], 'i8')
_add([(0x20,'ldc.i4')], 'i32')
_add([(0x21,'ldc.i8')], 'i64')
_add([(0x22,'ldc.r4')], 'f32')
_add([(0x23,'ldc.r8')], 'f64')
_add([(0x25,'dup'),(0x26,'pop')])
_add([(0x27,'jmp'),(0x28,'call'),(0x29,'calli')], 'token')
_add([(0x2A,'ret')])
_add(list(zip(range(0x2B,0x38),[
    'br.s','brfalse.s','brtrue.s','beq.s','bge.s','bgt.s','ble.s','blt.s',
    'bne.un.s','bge.un.s','bgt.un.s','ble.un.s','blt.un.s'])), 'sbr')
_add(list(zip(range(0x38,0x45),[
    'br','brfalse','brtrue','beq','bge','bgt','ble','blt',
    'bne.un','bge.un','bgt.un','ble.un','blt.un'])), 'br')
_add([(0x45,'switch')], 'switch')
_add(list(zip(range(0x46,0x6F),[
    'ldind.i1','ldind.u1','ldind.i2','ldind.u2','ldind.i4','ldind.u4',
    'ldind.i8','ldind.i','ldind.r4','ldind.r8','ldind.ref',
    'stind.ref','stind.i1','stind.i2','stind.i4','stind.i8','stind.r4','stind.r8',
    'add','sub','mul','div','div.un','rem','rem.un','and','or','xor',
    'shl','shr','shr.un','neg','not','conv.i1','conv.i2','conv.i4',
    'conv.i8','conv.r4','conv.r8','conv.u4','conv.u8'])))
_add([(0x6F,'callvirt'),(0x70,'cpobj'),(0x71,'ldobj'),(0x72,'ldstr'),
      (0x73,'newobj'),(0x74,'castclass'),(0x75,'isinst')], 'token')
_add([(0x76,'conv.r.un')])
_add([(0x79,'unbox')], 'token')
_add([(0x7A,'throw')])
_add([(v,n) for v,n in zip(range(0x7B,0x82),
    ['ldfld','ldflda','stfld','ldsfld','ldsflda','stsfld','stobj'])], 'token')
_add([(0x8C,'box'),(0x8D,'newarr')], 'token')
_add([(0x8E,'ldlen')])
_add([(0x8F,'ldelema')], 'token')
_add([(v,n) for v,n in zip(range(0x90,0xA3),[
    'ldelem.i1','ldelem.u1','ldelem.i2','ldelem.u2','ldelem.i4','ldelem.u4',
    'ldelem.i8','ldelem.i','ldelem.r4','ldelem.r8','ldelem.ref',
    'stelem.i','stelem.i1','stelem.i2','stelem.i4','stelem.i8',
    'stelem.r4','stelem.r8','stelem.ref'])])
_add([(0xA3,'ldelem'),(0xA4,'stelem'),(0xA5,'unbox.any')], 'token')
_add([(0xC2,'refanyval'),(0xC6,'mkrefany'),(0xD0,'ldtoken')], 'token')
_add([(0xC3,'ckfinite'),(0xD1,'conv.u2'),(0xD2,'conv.u1'),
      (0xD3,'conv.i'),(0xD4,'conv.ovf.i'),(0xD5,'conv.ovf.u'),
      (0xD6,'add.ovf'),(0xD7,'add.ovf.un'),(0xD8,'mul.ovf'),
      (0xD9,'mul.ovf.un'),(0xDA,'sub.ovf'),(0xDB,'sub.ovf.un'),
      (0xDC,'endfinally'),(0xDF,'stind.i'),(0xE0,'conv.u')])
_add([(0xDD,'leave')], 'br')
_add([(0xDE,'leave.s')], 'sbr')
FE = {}
for code, name, encoding in [
    (0x00,'arglist',''),(0x01,'ceq',''),(0x02,'cgt',''),(0x03,'cgt.un',''),
    (0x04,'clt',''),(0x05,'clt.un',''),(0x06,'ldftn','token'),
    (0x07,'ldvirtftn','token'),(0x09,'ldarg','u16'),(0x0A,'ldarga','u16'),
    (0x0B,'starg','u16'),(0x0C,'ldloc','u16'),(0x0D,'ldloca','u16'),
    (0x0E,'stloc','u16'),(0x0F,'localloc',''),(0x11,'endfilter',''),
    (0x12,'unaligned.','u8'),(0x13,'volatile.',''),(0x14,'tail.',''),
    (0x15,'initobj','token'),(0x16,'constrained.','token'),
    (0x17,'cpblk',''),(0x18,'initblk',''),(0x1A,'rethrow',''),
    (0x1C,'sizeof','token'),(0x1D,'refanytype',''),(0x1E,'readonly.','')]:
    FE[code] = (name,encoding)


def cli_compressed(data, off):
    first=data[off]
    if first<0x80: return first,off+1
    if first<0xC0: return ((first & 0x3f)<<8)|data[off+1],off+2
    if first<0xE0:
        return ((first & 0x1f)<<24)|(data[off+1]<<16)|(data[off+2]<<8)|data[off+3],off+4
    raise ValueError('Invalid CLI compressed integer')

class Metadata:
    def __init__(self, data):
        self.data=data
        self.pe=parse_pe(data)
        self.streams,self.cli_version=parse_streams(data,self.pe['metadata_offset'])
        b=data
        off=self.streams['#~']['offset']
        mask=b[off+6]
        valid=u64(b,off+8); cursor=off+24
        rows={}
        for t in range(64):
            if valid&(1<<t):
                rows[t]=u32(b,cursor);cursor+=4
        self.rows=rows
        self.ws=4 if mask&1 else 2
        self.wb=4 if mask&4 else 2
        self.wg=4 if mask&2 else 2
        r=lambda t: 4 if rows.get(t,0)>=65536 else 2
        c=lambda seq,bits: 4 if max(rows.get(t,0) for t in seq)>=(1<<(16-bits)) else 2
        self.wr=r
        self.wtr=c((2,1,27),2)
        self.wmr=c((2,1,26,6,27),3)
        self.sz={
            0:2+self.ws+3*self.wg,
            1:c((0,26,35,1),2)+2*self.ws,
            2:4+2*self.ws+self.wtr+r(4)+r(6),
            3:r(4),
            4:2+self.ws+self.wb,
            5:r(6),
            6:8+self.ws+self.wb+r(8),
            7:r(8),
            8:4+self.ws,
            9:r(2)+self.wtr,
            10:self.wmr+self.ws+self.wb,
        }
        self.offset={}
        for t in sorted(rows):
            if t>10: break
            self.offset[t]=cursor
            cursor+=rows[t]*self.sz[t]
        self.names={}
        self.types={}
        self.fields={}
        self.methods={}
        self.memberrefs={}
        for i in range(rows.get(1,0)):
            p=self.offset[1]+i*self.sz[1]+(self.sz[1]-2*self.ws)
            nm=self.string(self.readindex(p,self.ws))
            ns=self.string(self.readindex(p+self.ws,self.ws))
            self.names[0x01000000+i+1]=f'{ns}.{nm}'.strip('.')
        typefields=[]; typemethods=[]
        for i in range(rows.get(2,0)):
            p=self.offset[2]+i*self.sz[2]
            name=self.string(self.readindex(p+4,self.ws))
            ns=self.string(self.readindex(p+4+self.ws,self.ws))
            t=f'{ns}.{name}'.strip('.')
            tok=0x02000000+i+1
            self.names[tok]=t
            a=p+4+2*self.ws+self.wtr
            typefields.append(self.readindex(a,r(4)))
            typemethods.append(self.readindex(a+r(4),r(6)))
        for i in range(rows.get(4,0)):
            p=self.offset[4]+i*self.sz[4]
            name=self.string(self.readindex(p+2,self.ws))
            token=0x04000000+i+1
            self.fields[token]=name
        for i in range(rows.get(6,0)):
            p=self.offset[6]+i*self.sz[6]
            rva=u32(b,p); flags=u16(b,p+6)
            name=self.string(self.readindex(p+8,self.ws))
            token=0x06000000+i+1
            self.methods[token]={'name':name,'rva':rva,'flags':flags}
        for ti in range(len(typefields)):
            tname=self.names[0x02000000+ti+1]
            fend=typefields[ti+1] if ti+1<len(typefields) else rows.get(4,0)+1
            mend=typemethods[ti+1] if ti+1<len(typefields) else rows.get(6,0)+1
            for i in range(typefields[ti],fend):
                self.names[0x04000000+i]=f'{tname}.{self.fields[0x04000000+i]}'
            for i in range(typemethods[ti],mend):
                tok=0x06000000+i
                self.names[tok]=f"{tname}::{self.methods[tok]['name']}"
        for i in range(rows.get(10,0)):
            p=self.offset[10]+i*self.sz[10]
            parent=self.readindex(p,self.wmr)
            name=self.string(self.readindex(p+self.wmr,self.ws))
            # MemberRefParent: TypeDef(0),TypeRef(1),ModuleRef(2),MethodDef(3),TypeSpec(4)
            parent_table=(2,1,26,6,27)[parent & 7]
            parent_token=(parent_table<<24)|(parent>>3)
            desc=self.names.get(parent_token,hex(parent_token))
            self.memberrefs[0x0A000000+i+1]=f'{desc}::{name}'
        self.names.update(self.memberrefs)

    def readindex(self,p,w):return u16(self.data,p) if w==2 else u32(self.data,p)
    def string(self,idx):
        return read_cstring(self.data,self.streams['#Strings']['offset']+idx) if idx else ''
    def userstring(self,tok):
        index=tok&0xFFFFFF
        p=self.streams['#US']['offset']+index
        n,p=cli_compressed(self.data,p)
        raw=self.data[p:p+n]
        # last byte is ECMA-335 special-character flag
        return raw[:-1].decode('utf-16-le','replace') if raw else ''
    def describe(self,tok):
        if (tok>>24)==0x70: return repr(self.userstring(tok))
        return self.names.get(tok,f'{tok:#010x}')
    def method_il(self,token):
        info=self.methods[token]
        if not info['rva']:return b''
        pe=self.pe
        pe_pos=u32(self.data,0x3c)
        headers=u32(self.data,pe_pos+4+20+60)
        p=rva_to_offset(info['rva'],pe['sections'],headers)
        lead=self.data[p]
        if (lead&3)==2:
            size=lead>>2; head=1
        elif (lead&3)==3:
            word=u16(self.data,p)
            head=(word>>12)*4
            size=u32(self.data,p+4)
        else:raise ValueError('Invalid IL header')
        if size>len(self.data)-(p+head):raise ValueError('IL body extends beyond binary')
        return self.data[p+head:p+head+size]


def disassemble(il, metadata):
    b=il; p=0; out=[]
    while p<len(b):
        start=p
        if b[p]==0xFE:
            if p+1>=len(b): raise ValueError(f'truncated FE at {p}')
            opname,encoding=FE[b[p+1]];p+=2
        else:
            if b[p] not in OP:raise ValueError(f'unknown IL opcode {b[p]:02x} at {p}')
            opname,encoding=OP[b[p]];p+=1
        operand=None
        if encoding in ('u8','i8','u16','i32','i64','f32','f64','token'):
            fmt={'u8':'<B','i8':'<b','u16':'<H','i32':'<i',
                 'i64':'<q','f32':'<f','f64':'<d','token':'<I'}[encoding]
            v=struct.unpack_from(fmt,b,p)[0];p+=struct.calcsize(fmt)
            operand={'token':f'{v:#010x}','target':metadata.describe(v)} if encoding=='token' else v
        elif encoding=='sbr':
            rel=struct.unpack_from('<b',b,p)[0];p+=1; operand=p+rel
        elif encoding=='br':
            rel=struct.unpack_from('<i',b,p)[0];p+=4; operand=p+rel
        elif encoding=='switch':
            n=u32(b,p);p+=4
            if n>len(b)//4:raise ValueError('Illegal switch case count')
            v=struct.unpack_from('<'+'i'*n,b,p);p+=4*n
            operand=[p+rel for rel in v]
        out.append({'offset':start,'opcode':opname,'operand':operand})
    starts={o['offset'] for o in out}
    for instruction in out:
        if instruction['opcode'].startswith(('br','beq','bge','bgt','ble','blt','bne','leave')):
            if instruction['operand'] not in starts and instruction['operand']!=len(b):
                raise ValueError(f"Invalid branch target {instruction['operand']} in method")
        if instruction['opcode']=='switch':
            if not all(t in starts or t==len(b) for t in instruction['operand']):
                raise ValueError('Invalid switch target')
    return out


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('dll',type=Path)
    parser.add_argument('--type',default='GameCtrl')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    md=Metadata(args.dll.read_bytes())
    methods=[]
    for token,name in md.names.items():
        if (token>>24)==6 and name.startswith(args.type+'::'):
            il=md.method_il(token)
            methods.append({'name':name,'token':f'{token:#010x}',
                            'il_bytes':len(il),'instructions':disassemble(il,md)})
    if not methods:raise SystemExit(f'no methods of type {args.type!r}')
    doc={'target_type':args.type,'source_dll':args.dll.name,'method_count':len(methods),'methods':methods}
    txt=json.dumps(doc,ensure_ascii=False,indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(txt+'\n',encoding='utf-8')
    else: print(txt)

if __name__=='__main__':main()
