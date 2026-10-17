#!/usr/bin/env python3
"""Reads autotest results (LocalState/autotest_results.txt from the Xbox app, or the output of
rpcs3_bench_runner on PC) and prints a pass/fail table and the BENCH timings.

  report.py results.txt [--tests <ps3autotests checkout>] [--base other_results.txt]

With --tests, each program's TTY / output.txt is compared with the .expected file next to the
ELF in the ps3autotests checkout (matched by file name). With --base, BENCH lines are compared
with another run (time ratio, and whether the checksums agree).
"""
import argparse
import os
import re
import sys


def parse(path):
    runs = {}
    cur = None
    for line in open(path, encoding='utf-8', errors='replace'):
        if line.startswith('RUN '):
            cur = {'path': line[4:].strip(), 'out': [], 'end': None}
            continue
        m = re.match(r'END (.+) (\S+) ([\d.]+)s$', line.rstrip('\n'))
        if m and cur is not None:
            cur['end'] = m.group(2)
            cur['secs'] = float(m.group(3))
            runs[os.path.basename(cur['path'])] = cur
            cur = None
            continue
        if cur is not None:
            cur['out'].append(line)
    return runs


def bench_lines(run):
    out = {}
    for line in run['out']:
        m = re.match(r'BENCH (\S+) (\d+) us ([0-9a-f]+)', line)
        if m:
            out[m.group(1)] = (int(m.group(2)), m.group(3))
    return out


def expected_for(tests_dir, name):
    stem = name.split('.')[0]
    for root, _, files in os.walk(tests_dir):
        if stem + '.expected' in files and any(f.startswith(stem) and f.endswith('.elf') for f in files):
            return open(os.path.join(root, stem + '.expected'), encoding='utf-8', errors='replace').read()
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('results')
    ap.add_argument('--tests')
    ap.add_argument('--base')
    a = ap.parse_args()

    runs = parse(a.results)
    base = parse(a.base) if a.base else {}
    fails = 0
    for name, run in sorted(runs.items()):
        status = run['end']
        text = ''.join(run['out'])
        if '--- output.txt ---\n' in text:
            text = text.split('--- output.txt ---\n', 1)[1]
        verdict = ''
        if a.tests and not name.startswith('bench'):
            exp = expected_for(a.tests, name)
            if exp is None:
                verdict = 'no .expected'
            elif exp.replace('\r\n', '\n').strip() == text.replace('\r\n', '\n').strip():
                verdict = 'PASS'
            else:
                verdict = 'DIFF'
                fails += 1
        print(f'{name:45} {status:8} {run["secs"]:8.2f}s  {verdict}')

        b = bench_lines(run)
        bb = bench_lines(base[name]) if name in base else {}
        for test, (us, csum) in b.items():
            line = f'    {test:16} {us / 1000:10.1f} ms  {csum}'
            if test in bb:
                bus, bcsum = bb[test]
                line += f'   base {bus / 1000:10.1f} ms  x{bus / us if us else 0:5.2f} faster'
                if bcsum != csum:
                    line += '  CHECKSUM DIFFERS'
            print(line)
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
