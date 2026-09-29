#!/usr/bin/env python3
"""Exercise the production entry prefix after a prior world visit.

Only unrelated subsystem-reset calls are stubs. No retail game state is seeded:
these are native diagnostic/ownership guards outside the guest address space.
"""
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('pc_port/src/world_map_init.c').read_text()
start = source.index('void PcPort_WorldMapInitMain(void)')
end = source.index('    fprintf(stderr, "[worldmap-init] entry', start)
prefix = source[start:end]
# The prefix's local guest-data declarations are irrelevant to guard lifetime.
prefix = re.sub(r'^    (?:int decoded_size;|u32 final_write = 0;|u8 bss_(?:before|after)\[16\];)\n', '', prefix, flags=re.M)
names = re.findall(r'^static int (s_wm\w+);', source, re.M)
required = '''s_wm8440c_completed s_wm979c8_ran s_wm84580_ran s_wm72090_ran
s_wm736dc_ran s_wm73e30_ran s_wm85f58_ran s_wm74594_ran s_wm863E0_ran
s_wm74e58_ran s_wm75030_ran s_wm739b8_ran s_wm88f64_ran
s_wm_first_wds_ran s_wm_archive_set_index_ran s_wm_gfx_work_rung_dispatches
s_wm_gfx_work_world_hits s_wm32b_entry'''.split()
assert set(required) <= set(names)
calls = re.findall(r'^    (wm_\w+_reset)\(\);', prefix, re.M)
harness = '#include <stdio.h>\n' + ''.join(f'static int {n};\n' for n in names)
harness += ''.join(f'static void {n}(void) {{}}\n' for n in calls)
harness += prefix + '}\nint main(void) {\n'
harness += 'for (int visit=1; visit<=3; ++visit) {\n'
harness += ''.join(f'{n}=visit;\n' for n in required)
harness += 'PcPort_WorldMapInitMain();\n'
harness += ''.join(f'if ({n} != 0) {{ fprintf(stderr,"WORLD ENTRY FAIL: {n} visit=%d\\n",visit); return 1; }}\n' for n in required)
harness += '} puts("WORLD ENTRY PASS: repeated entry clears native guards"); return 0; }\n'
with tempfile.TemporaryDirectory(prefix='world-entry-guards-') as tmp:
    c = Path(tmp)/'test.c'
    exe = Path(tmp)/'test'
    c.write_text(harness)
    for flags in (['-O0'], ['-O2'], ['-O2','-fsanitize=undefined','-fno-sanitize-recover=all']):
        subprocess.run(['gcc','-std=c11',*flags,str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    for name in required:
        reset = f'    {name} = 0;'
        assert reset in prefix, name
        c.write_text(harness.replace(reset, '', 1))
        subprocess.run(['gcc','-std=c11','-O2',str(c),'-o',str(exe)],check=True)
        result = subprocess.run([str(exe)],capture_output=True,text=True)
        assert result.returncode != 0 and f'WORLD ENTRY FAIL: {name}' in result.stderr, name
    print(f'WORLD ENTRY PASS: {len(required)} omitted-reset mutations rejected')
