"""Compile production-C scenarios for Cortex-M3 and run their real Thumb code.

ELF loading/symbol reading uses the standard library. Test RAM is 128 KB because
the fake external EEPROM needs 32 KB in RAM; the firmware linker still uses 20 KB.
"""
import argparse
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'build' / 'arm-tests'
sys.path.insert(0, str(ROOT / 'build' / 'test-deps'))
try:
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_INTR
    from unicorn.arm_const import UC_ARM_REG_SP
except ImportError:
    raise SystemExit('Install test dependency: python -m pip install --target build/test-deps -r tests/requirements.txt')


def load_elf(path, cpu):
    """Load ELF32 little-endian PT_LOAD segments and find harness symbols."""
    data = path.read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise RuntimeError('Expected little-endian ELF32')
    entry, phoff, shoff = struct.unpack_from('<III', data, 24)
    phsize, phcount, shsize, shcount = struct.unpack_from('<HHHH', data, 42)
    for i in range(phcount):
        kind, offset, vaddr, _, size = struct.unpack_from('<IIIII', data, phoff + i * phsize)
        if kind == 1 and size:
            cpu.mem_write(vaddr, data[offset:offset + size])
    sections = [struct.unpack_from('<10I', data, shoff + i * shsize) for i in range(shcount)]
    symbols = {}
    for section in sections:
        if section[1] != 2:
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value = struct.unpack_from('<II', data, offset)
            end = names.find(b'\0', name)
            symbols[names[name:end].decode('ascii', errors='replace')] = value
    return entry, symbols


def run_unit(source, compiler, linker, includes):
    cases = re.findall(r'^int (test_\w+)\(', source.read_text(encoding='utf-8'), re.M)
    harness = OUTPUT / (source.stem + '_main.c')
    harness.write_text('#include "' + source.as_posix() + '"\n'
        + f'volatile unsigned int arm_test_results[{len(cases)}];\n'
        + 'volatile unsigned int arm_test_done;\nvoid arm_test_main(void) {\n'
        + ''.join(f'arm_test_results[{i}] = (unsigned int){name}();\n' for i, name in enumerate(cases))
        + 'arm_test_done = 1U; __asm volatile("bkpt #0"); for (;;) {} }\n', encoding='utf-8')
    obj = harness.with_suffix('.o')
    axf = harness.with_suffix('.axf')
    subprocess.run([compiler, '--target=arm-arm-none-eabi', '-mcpu=cortex-m3', '-mthumb',
        '-std=c99', '-O1', '-fshort-enums', '-fshort-wchar', '-ffunction-sections',
        '-Wall', '-Wextra', '-Wsign-conversion', '-Werror', '-DSTM32F10X_MD', '-I' + str(ROOT / 'src'),
        *['-I' + path for path in includes],
        '-c', str(harness), '-o', str(obj)], check=True)
    subprocess.run([linker, '--cpu', 'Cortex-M3', '--entry', 'arm_test_main', '--scatter',
        str(OUTPUT / 'test.sct'), str(obj), '-o', str(axf)], check=True)
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    cpu.mem_map(0x08000000, 0x20000)
    cpu.mem_map(0x20000000, 0x20000)
    entry, symbols = load_elf(axf, cpu)
    cpu.reg_write(UC_ARM_REG_SP, 0x20020000)
    cpu.hook_add(UC_HOOK_INTR, lambda machine, interrupt, context: machine.emu_stop())
    cpu.emu_start(entry | 1, 0, timeout=15000000, count=100000000)
    if struct.unpack('<I', cpu.mem_read(symbols['arm_test_done'], 4))[0] != 1:
        raise RuntimeError(f'{source.name}: emulator stopped before all tests completed')
    results = struct.unpack('<' + 'I' * len(cases), cpu.mem_read(symbols['arm_test_results'], 4 * len(cases)))
    for name, result in zip(cases, results):
        if result:
            raise RuntimeError(f'{source.name}: {name} failed at C line {result}')
        print('PASS ARM', name, flush=True)
    if 'display_snapshots' in symbols:
        count = struct.unpack('<I', cpu.mem_read(symbols['display_count'], 4))[0]
        (OUTPUT / 'display.bin').write_bytes(cpu.mem_read(symbols['display_snapshots'], count * 1024))
    return len(cases)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--linker', required=True)
    parser.add_argument('--include', action='append', default=[])
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / 'test.sct').write_text('LR_TEST 0x08000000 0x20000 {\n'
        ' ER_TEST 0x08000000 0x20000 { .ANY (+RO) }\n'
        ' RW_TEST 0x20000000 0x20000 { .ANY (+RW +ZI) }\n}\n', encoding='ascii')
    total = 0
    for source in sorted((ROOT / 'tests').glob('*.c')):
        if source.stem.endswith(('_checks', '_arm')):
            total += run_unit(source, args.compiler, args.linker, args.include)
    print(f'ARM runtime: {total} checks passed.')


if __name__ == '__main__':
    main()
