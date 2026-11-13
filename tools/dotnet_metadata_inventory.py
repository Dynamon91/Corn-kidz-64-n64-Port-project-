#!/usr/bin/env python3
"""Inspect PE/CLI metadata safely (no execution, no runtime dependencies).

This implements PE section mapping, CLI metadata stream discovery, and the
ECMA-335 Module/TypeRef/TypeDef/Field/MethodDef table records. Its scope is
intentionally read-only and aimed at managed game-logic triage; it is not an
IL decompiler. It does not reconstruct method bodies.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


def u8(b: bytes, off: int) -> int:
    return b[off]


def u16(b: bytes, off: int) -> int:
    return struct.unpack_from('<H', b, off)[0]


def u32(b: bytes, off: int) -> int:
    return struct.unpack_from('<I', b, off)[0]


def u64(b: bytes, off: int) -> int:
    return struct.unpack_from('<Q', b, off)[0]


def read_cstring(b: bytes, off: int) -> str:
    end = b.find(b'\0', off)
    if end < 0:
        raise ValueError(f'Unterminated string at {off}')
    return b[off:end].decode('utf-8', errors='replace')


def align4(n: int) -> int:
    return (n + 3) & ~3


def rva_to_offset(rva: int, sections: list[dict], headers_size: int) -> int:
    for section in sections:
        start = section['rva']
        end = start + max(section['virtual_size'], section['raw_size'])
        if start <= rva < end:
            delta = rva - start
            if delta >= section['raw_size']:
                raise ValueError('RVA points to section virtual padding')
            return section['raw_pointer'] + delta
    if 0 <= rva < headers_size:
        return rva
    raise ValueError(f'RVA 0x{rva:X} not mapped')


def parse_pe(b: bytes) -> dict:
    if b[:2] != b'MZ':
        raise ValueError('Not an MZ PE file')
    pe = u32(b, 0x3C)
    if b[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Missing PE signature')
    coff = pe + 4
    machine = u16(b, coff)
    nsec = u16(b, coff + 2)
    opt_size = u16(b, coff + 16)
    opt = coff + 20
    magic = u16(b, opt)
    if magic not in (0x10B, 0x20B):
        raise ValueError(f'Unexpected optional header magic {magic:04x}')
    headers_size = u32(b, opt + 60)
    dir_off = opt + (96 if magic == 0x10B else 112)
    n_dirs = u32(b, opt + (92 if magic == 0x10B else 108))
    if n_dirs <= 14:
        raise ValueError('No CLI data directory')
    cli_rva = u32(b, dir_off + 14 * 8)
    if not cli_rva:
        raise ValueError('No CLI header; native PE?')
    sections = []
    for i in range(nsec):
        s = opt + opt_size + i * 40
        sections.append({
            'name': b[s:s+8].rstrip(b'\0').decode('ascii', 'replace'),
            'virtual_size': u32(b, s + 8),
            'rva': u32(b, s + 12),
            'raw_size': u32(b, s + 16),
            'raw_pointer': u32(b, s + 20),
        })
    cli = rva_to_offset(cli_rva, sections, headers_size)
    metadata_rva = u32(b, cli + 8)
    metadata_size = u32(b, cli + 12)
    meta_off = rva_to_offset(metadata_rva, sections, headers_size)
    return {
        'machine': hex(machine),
        'pe_magic': hex(magic),
        'sections': sections,
        'metadata_offset': meta_off,
        'metadata_size': metadata_size,
        'cli_flags': hex(u32(b, cli + 16)),
    }


def parse_streams(b: bytes, meta_off: int) -> tuple[dict, str]:
    if u32(b, meta_off) != 0x424A5342:
        raise ValueError('Missing CLI metadata signature')
    length = u32(b, meta_off + 12)
    version = b[meta_off + 16:meta_off + 16 + length].rstrip(b'\0').decode('utf-8', 'replace')
    pos = meta_off + 16 + align4(length)
    nstreams = u16(b, pos + 2)
    pos += 4
    streams = {}
    for _ in range(nstreams):
        rel = u32(b, pos)
        size = u32(b, pos + 4)
        name = read_cstring(b, pos + 8)
        streams[name] = {'offset': meta_off + rel, 'size': size}
        pos += 8 + align4(len(name.encode('utf-8')) + 1)
    return streams, version


def parse_tables(b: bytes, stream_off: int, strings_off: int, current_sections: list[dict], current_headers_size: int) -> dict:
    pos = stream_off
    heap_sizes = u8(b, pos + 6)
    valid = u64(b, pos + 8)
    pos += 24
    rows = {}
    for t in range(64):
        if valid & (1 << t):
            rows[t] = u32(b, pos)
            pos += 4
    strings_w = 4 if (heap_sizes & 1) else 2
    guid_w = 4 if (heap_sizes & 2) else 2
    blob_w = 4 if (heap_sizes & 4) else 2

    def rid_width(t: int) -> int:
        return 4 if rows.get(t, 0) >= 65536 else 2

    def coded_width(tables: tuple[int, ...], bits: int) -> int:
        return 4 if max(rows.get(t, 0) for t in tables) >= (1 << (16 - bits)) else 2

    resolution_scope_w = coded_width((0, 26, 35, 1), 2)
    typedef_or_ref_w = coded_width((2, 1, 27), 2)
    sizes = {
        0: 2 + strings_w + 3 * guid_w,
        1: resolution_scope_w + 2 * strings_w,
        2: 4 + 2 * strings_w + typedef_or_ref_w + rid_width(4) + rid_width(6),
        3: rid_width(4),
        4: 2 + strings_w + blob_w,
        5: rid_width(6),
        6: 4 + 2 + 2 + strings_w + blob_w + rid_width(8),
    }
    for table_id in rows:
        if table_id < 6 and table_id not in sizes:
            raise ValueError(f'Unsupported preceding metadata table {table_id}')
    offsets = {}
    cursor = pos
    for table_id in sorted(rows):
        if table_id > 6:
            break
        offsets[table_id] = cursor
        cursor += rows[table_id] * sizes[table_id]

    def read_heap_string(index: int) -> str:
        if index == 0:
            return ''
        return read_cstring(b, strings_off + index)

    def idx(offset: int, width: int) -> int:
        return u16(b, offset) if width == 2 else u32(b, offset)

    typerefs = []
    for i in range(rows.get(1, 0)):
        cur = offsets[1] + i * sizes[1]
        scope = idx(cur, resolution_scope_w)
        cur += resolution_scope_w
        name_idx = idx(cur, strings_w)
        cur += strings_w
        namespace_idx = idx(cur, strings_w)
        typerefs.append({'token': f'0x010000{i+1:02X}', 'name': read_heap_string(name_idx),
                         'namespace': read_heap_string(namespace_idx), 'resolution_scope': scope})

    methods = []
    for i in range(rows.get(6, 0)):
        cur = offsets[6] + i * sizes[6]
        rva = u32(b, cur)
        flags = u16(b, cur + 6)
        name = read_heap_string(idx(cur + 8, strings_w))
        # ECMA-335 method bodies begin with a Tiny (0b10) or Fat (0b11)
        # header. The IL length is a metadata fact, not proof of semantics.
        il_size = 0
        if rva:
            raw = rva_to_offset(rva, current_sections, current_headers_size)
            header = u8(b, raw)
            if header & 0x3 == 2:  # Tiny
                il_size = header >> 2
            elif header & 0x3 == 3:  # Fat
                il_size = u32(b, raw + 4)
            else:
                raise ValueError(f'Unknown IL method header at RVA {rva:#x}')
            if raw + il_size > len(b):
                raise ValueError('IL length extends past file')
        methods.append({'token': f'0x0600{i+1:04X}', 'name': name,
                        'rva': hex(rva), 'flags': hex(flags), 'il_bytes': il_size})

    types = []
    for i in range(rows.get(2, 0)):
        cur = offsets[2] + i * sizes[2]
        flags = u32(b, cur)
        name = read_heap_string(idx(cur + 4, strings_w))
        namespace = read_heap_string(idx(cur + 4 + strings_w, strings_w))
        extends = idx(cur + 4 + 2 * strings_w, typedef_or_ref_w)
        field_list = idx(cur + 4 + 2 * strings_w + typedef_or_ref_w, rid_width(4))
        method_list = idx(cur + 4 + 2 * strings_w + typedef_or_ref_w + rid_width(4), rid_width(6))
        types.append({'token': f'0x0200{i+1:04X}', 'name': name, 'namespace': namespace,
                      'extends_coded': extends, 'flags': hex(flags), 'field_list': field_list,
                      'method_list': method_list})
    for i, td in enumerate(types):
        start = td['method_list']
        stop = types[i+1]['method_list'] if (i+1) < len(types) else len(methods) + 1
        td['methods'] = [m['name'] for m in methods[start-1:stop-1]]
        td['method_definitions'] = methods[start-1:stop-1]
        td['method_count'] = len(td['methods'])
        # TypeDefOrRef: tag 0=TypeDef, 1=TypeRef, 2=TypeSpec
        ext_tag = td['extends_coded'] & 3
        ext_rid = td['extends_coded'] >> 2
        if ext_tag == 1 and ext_rid <= len(typerefs) and ext_rid:
            tr = typerefs[ext_rid-1]
            td['extends'] = '.'.join(x for x in [tr['namespace'], tr['name']] if x)
        elif ext_tag == 0 and ext_rid <= len(types) and ext_rid:
            other = types[ext_rid-1]
            td['extends'] = '.'.join(x for x in [other['namespace'], other['name']] if x)
        else:
            td['extends'] = None
    return {'row_counts': {str(t): n for t, n in sorted(rows.items())},
            'heap_sizes_flag': heap_sizes, 'type_refs': typerefs,
            'types': types, 'method_count': len(methods),
            'il_bytes_total': sum(m['il_bytes'] for m in methods)}


def inspect(path: Path) -> dict:
    b = path.read_bytes()
    pe = parse_pe(b)
    streams, cli_version = parse_streams(b, pe['metadata_offset'])
    table_name = '#~' if '#~' in streams else '#-'
    if table_name not in streams or '#Strings' not in streams:
        raise ValueError('Required CLI metadata streams missing')
    table = parse_tables(b, streams[table_name]['offset'], streams['#Strings']['offset'],
                         pe['sections'], u32(b, u32(b, 0x3C) + 4 + 20 + 60))
    return {'file': path.name, 'file_bytes': len(b), 'pe': pe,
            'cli_version': cli_version, 'streams': streams, **table}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('assembly', type=Path)
    parser.add_argument('--json', type=Path, help='write complete JSON inventory')
    args = parser.parse_args()
    inventory = inspect(args.assembly)
    print('Assembly:', inventory['file'])
    print('PE machine:', inventory['pe']['machine'], 'CLI:', inventory['cli_version'])
    print('Managed type defs:', len(inventory['types']), 'type refs:', len(inventory['type_refs']))
    print('Methods:', inventory['method_count'], 'IL bytes:', inventory['il_bytes_total'])
    for td in inventory['types']:
        if td['name'] != '<Module>':
            print(f"{td['namespace']+'.' if td['namespace'] else ''}{td['name']} : "
                  f"{td['extends'] or '?'} | {td['method_count']} methods")
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(inventory, indent=2), encoding='utf-8')
        print('Wrote', args.json)


if __name__ == '__main__':
    main()
