"""Validate this ticket's complete original capture, then prepare production replay."""
from pathlib import Path
import re
import sys

INPUTS = {0, 0x80000000, 0x3f000000, 0x3f7fffff, 0x3f800000,
          0x40000000, 0xbf800000, 0x7fc12345, 0xffc12345, 0x7f800000,
          0xff800000, 0xffc54321, 0x3fc00000}
EXPECTED_SKIPS = {176 + g*104 + n for g in (14, 15, 16) for n in range(104)} | {2597}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def add(table, key, value):
    require(key not in table, f'duplicate fixture/record {key}')
    table[key] = value


def raw_words(fields):
    words = fields['raw'].split(',')
    require(len(words) == 8 and all(re.fullmatch(r'[0-9a-f]{8}', x) for x in words), 'invalid raw words')
    require(0 <= int(fields['count']) <= 8, 'invalid scalar count')
    return int(fields['count']), tuple(int(x, 16) for x in words)


def prepare(source):
    caps, traces, skips, raws, golden_raw = {}, {}, {}, {}, {}
    sets, empty, records, evaluations, empty_eval = {}, {}, {}, {}, {}
    totals, rows = {}, []
    phase, current, cleared = 'begin', None, False
    for original_line in source.splitlines():
        line = re.sub(r'^\[[^]]+\] ', '', original_line)
        tag = line.split(' ', 1)[0]
        pairs = re.findall(r'(\w+)=([^ ]+)', line)
        f = dict(pairs)
        require(len(f) == len(pairs), 'duplicate fields')
        require(phase != 'done', 'trailing capture lines')
        if tag == 'CAP-BEGIN':
            require(phase == 'begin', 'duplicate or misplaced begin')
            require(f['method'] == '1000992cc' and f['fixture_mxcsr'] == '00001f80', 'wrong method/environment')
            phase = 'capacity'
        elif tag in ('CAP', 'CAP-TRACE', 'CAP-SKIP'):
            require(phase == 'capacity', 'misplaced capacity line')
            ident = int(f['id'])
            if tag == 'CAP':
                require(ident not in skips, 'compared/excluded overlap')
                add(caps, ident, f)
            elif tag == 'CAP-SKIP':
                require(ident not in caps and line.endswith('null-backed evaluation excluded'), 'invalid exclusion')
                add(skips, ident, f)
            else:
                require(ident in caps, 'trace has no preceding comparison')
                add(traces, ident, f)
        elif tag == 'CAP-RAW':
            ident = int(f['id']); raw_words(f)
            if phase == 'capacity':
                require(ident in caps or ident in skips, 'orphan raw capacity')
                add(raws, ident, f)
            else:
                require(phase == 'golden', 'misplaced golden raw')
                add(golden_raw, ident, f)
        elif tag == 'CAP-TOTAL':
            require(phase == 'capacity', 'misplaced capacity total')
            totals['capacity'] = f; phase = 'golden'
        elif tag == 'CURVE-SET':
            require(phase == 'golden' and (current is None or cleared), 'unfinished prior curve')
            ident = int(f['id'])
            require(ident == len(sets) and ident in golden_raw, 'wrong curve ID/order')
            add(sets, ident, f); current, cleared = ident, False
        elif tag in ('CURVE-RECORD', 'CURVE-EVAL', 'CURVE-EMPTY', 'CURVE-EMPTY-EVAL'):
            require(phase == 'golden' and int(f['id']) == current, 'wrong curve association')
            if tag == 'CURVE-RECORD':
                require(not cleared, 'record after clearing')
                words = f['bits'].split(',')
                require(len(words) == 5 and all(re.fullmatch(r'[0-9a-f]{8}', x) for x in words), 'invalid segment words')
                add(records, (current, int(f['index'])), f)
            elif tag == 'CURVE-EVAL':
                require(not cleared, 'evaluation after clearing')
                add(evaluations, (current, int(f['input'], 16)), f)
            elif tag == 'CURVE-EMPTY':
                require(not cleared, 'duplicate clear')
                add(empty, current, f); cleared = True
            else:
                require(cleared and f['input'] == '7fc12345', 'wrong empty evaluation')
                add(empty_eval, current, f)
        elif tag == 'CURVE-TOTAL':
            require(phase == 'golden' and cleared, 'unfinished golden capture')
            totals['curve'] = f; phase = 'done'
        else:
            raise ValueError(f'unrecognized capture line {tag}')

    require(phase == 'done', 'missing totals/end')
    require(set(skips) == EXPECTED_SKIPS, 'missing or incorrect exclusions')
    require(set(caps) == set(range(2598)) - EXPECTED_SKIPS, 'missing or incorrect compared IDs')
    require(set(traces) == set(caps), 'missing/extra traces')
    require(set(raws) == set(range(176, 2598)), 'missing/extra raw capacity payloads')
    require(set(sets) == set(empty) == set(golden_raw) == set(range(23)), 'incomplete curve IDs')
    require(sum(int(f['count']) for f in sets.values()) == 35, 'incorrect derived record total')
    require(set(records) == {(i, r) for i, f in sets.items() for r in range(int(f['count']))}, 'incomplete per-fixture records')
    backed = {i for i, f in sets.items() if f['backed'] == '1'}
    require(backed == set(range(23)) - {14, 15, 16}, 'incorrect backing-state fixtures')
    require(set(evaluations) == {(i, x) for i in backed for x in INPUTS}, 'incomplete per-fixture evaluations')
    require(set(empty_eval) == backed, 'incomplete retained-storage evaluations')
    for ident, raw in raws.items():
        g = (ident - 176)//104 if ident < 2568 else (7 if ident < 2590 else 0)
        count, words = raw_words(raw)
        expected_count, expected_words = raw_words(golden_raw[g])
        require(words == expected_words and count == (0 if ident == 2597 else expected_count), 'capacity/golden fixture mapping differs')
    infzero = 0
    for ident, f in sorted(caps.items()):
        require(f['mismatch'] == '0' and f['orig'] == f['kernel'], 'reported capacity mismatch')
        present = f['present']; require(re.fullmatch('[01]{3}', present), 'invalid presence bits')
        trace = 'HC'
        success = present[0] == '1' and (int(f['capok'], 16) & 255) != 0
        if present[0] == '1': trace += 'cU'
        if success:
            trace += 'PHR'
            if present[2] == '1': trace += 'VDNI' + ('iF' if present[1] == '1' else '')
        require(traces[ident]['original'] == traces[ident]['kernel'] == trace, 'missing/inconsistent callback trace')
        require(int(f['evaluated']) == int(success and present[2] == '1'), 'inconsistent evaluation flag')
        infzero += int(f['evaluated'] == '1' and int(f['response'], 16) & 0x7fffffff == 0x7f800000 and int(f['cap'], 16) == 0)
        require((present[2] == '1') == (ident in raws), 'raw payload association mismatch')
        raw = raws.get(ident, dict(count='0', raw=','.join(['0']*8)))
        ret, value = f['orig'].split('/')
        rows.append('C ' + ' '.join([str(ident), f['cap'], f['scale'], f['input'], f['power'], f['capok'], f['ageok'], present, ret, value, raw['count'], *raw['raw'].split(',')]))
    ct = totals['capacity']
    require(ct['method'] == '1000992cc' and int(ct['compared']) == len(caps) and int(ct['mismatches']) == 0 and int(ct['excluded']) == len(skips) and int(ct['inf_response_zero_capacity']) == infzero == 28, 'inconsistent capacity totals')
    gt = totals['curve']
    require(gt['setter'] == '1000ac37a' and gt['evaluator'] == '1000ac4f6' and int(gt['golden_evaluations']) == len(evaluations)+len(empty_eval) == 280 and gt['mismatches'] == 'UNSCORED' and gt['native_replay_required'] == '1' and gt['fresh_null_and_all_skipped_null_faults'] == 'EXCLUDED', 'inconsistent curve totals')
    for ident, f in sorted(sets.items()):
        raw = golden_raw[ident]; e = empty[ident]
        require(f['trace'] == e['trace'] == 'VDN' and e['ret'] == '00' and e['count'] == '0' and e['backed'] == f['backed'], 'inconsistent setter/clear trace/state')
        rows.append('S ' + ' '.join([str(ident), f['ret'], f['count'], f['backed'], raw['count'], *raw['raw'].split(',')]))
        for index in range(int(f['count'])):
            r = records[ident, index]
            rows.append('R ' + ' '.join([str(ident), str(index), *r['bits'].split(',')]))
        for (i, x), v in sorted(evaluations.items()):
            if i == ident: rows.append(f"E {ident} {x:08x} {v['output']}")
        rows.append('Z ' + ' '.join([str(ident), e['ret'], e['count'], e['backed']]))
        if ident in empty_eval: rows.append(f"E {ident} 7fc12345 {empty_eval[ident]['output']}")
    return '\n'.join(rows) + '\n'


if __name__ == '__main__':
    try:
        result = prepare(Path(sys.argv[1]).read_text())
        Path(sys.argv[2]).write_text(result)
        print('Validated 2598 candidate IDs, 2285 traces, 313 exclusions, 35 records and 280 curve evaluations')
    except (ValueError, KeyError, IndexError) as exc:
        sys.exit(f'Invalid capture: {exc}')
