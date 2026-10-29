#!/usr/bin/env python3
"""Bounded integrator stage/stack capture after checkpoint -> menu warp.
No movement enhancement required. No save writes or gameplay acceptance.
"""
import json
import os
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'lib'))
from xeno_headless import Port

# Bounded read-only state capture while a destination scene holds the script
# lock: the main thread's frame, the PcPort_FieldPosDiag gate variables and the
# path it takes through its early-outs for one call, and every script actor's
# instruction pointer with the opcode bytes under it.  The game is paused only
# while gdb is attached; nothing is written.  Requires gdb already present.
SCENE_GDB = r'''
set pagination off
set confirm off
set print pretty off
echo === main thread ===\n
thread 1
bt 30
echo === posdiag gates ===\n
p (int)D_800ADB04
p (int)D_800ADB2C
p (int)D_800ADB68
p (int)D_800ADB64
p (int)D_800B21D0
p (int)D_800ADBEC
p (int)D_800ADBC4
p (int)D_800ADB90
p (int)D_800B0048
p (int)D_800ADBFC
p (int)g_FieldNumActors
p (int)g_PlayerActorIndex
p g_FieldActors
p g_pFieldTriggerZones
p D_8005A4E0
p *(unsigned int*)((char*)D_8005A4E0 + 0x12c)
p g_FieldScriptVMCurScriptData
p (int)(g_GameSceneMapNum & 0x3fff)
p PcPort_FieldPosDiag::every
p PcPort_FieldPosDiag::frame
echo === script actors ===\n
set $i = 0
while $i < (int)D_800ADBFC && $i < 64
  set $a = (ActorData*)(unsigned long)g_FieldActors[$i].pActorData
  if $a != 0
    printf "ACTOR %d status=0x%04x ip=%d f0=0x%08x f4=0x%08x pos=(%d,%d,%d)\n", $i, (unsigned)g_FieldActors[$i].status & 0xffff, (int)$a->scriptInstructionPointer, *(unsigned int*)$a, *(unsigned int*)((char*)$a + 4), (int)($a->position.vx >> 16), (int)($a->position.vy >> 16), (int)($a->position.vz >> 16)
    x/16xb (char*)g_FieldScriptVMCurScriptData + (int)$a->scriptInstructionPointer
  else
    printf "ACTOR %d pActorData=NULL\n", $i
  end
  set $i = $i + 1
end
echo === one PcPort_FieldPosDiag call, line by line ===\n
tbreak PcPort_FieldPosDiag
continue
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
next
frame
p (int)D_800ADB04
detach
'''


# t9: attached from field 14 (before the warp) through the destination scene.
# Read-only.  Arms a hardware watchpoint on the destination map header's
# triggers-size word (+0x12C, which t8 found clobbered to 0x211FA0A7 by main-
# loop entry) at the loader's own read of it, so the writer is named with a
# backtrace; logs any CIRCLE release that reaches the text-box update
# (func_800345E0 is the window-advance call both passes make on a press) and
# the raw field pad masks with their gates; dumps the four text boxes once,
# ~400 frames into the wait on opcode 0x9C.  Ends by SIGINT -> detach.
WATCH_GDB = r'''
set pagination off
set confirm off
set print pretty off
set $presses = 0
set $polls = 0
break src/field/main/misc3.c:908 if ((g_GameSceneMapNum & 0x3fff) == %(map)d)
commands 1
  silent
  printf "T9 header@load mapBuf=%%p +0x12C=%%u\n", D_8005A4E0, *(unsigned int*)((char*)D_8005A4E0 + 0x12c)
  x/14xw (char*)D_8005A4E0 + 0x120
  watch -l *(unsigned int*)((char*)D_8005A4E0 + 0x12c)
  commands
    printf "T9 HEADER WRITE +0x12C now=%%u present=%%u\n", *(unsigned int*)((char*)D_8005A4E0 + 0x12c), (unsigned)g_PcPortPresentedFrames
    bt 16
    continue
  end
  continue
end
break func_800345E0
commands 2
  silent
  set $presses = $presses + 1
  printf "T9 PRESS accepted by text box %%d (window advance) released=0x%%x present=%%u\n", (int)(((char*)$rdi - 0x18 - (char*)&g_FieldTextBoxes[0]) / 0x498), (unsigned)D_800C2694, (unsigned)g_PcPortPresentedFrames
  if $presses > 8
    delete 2
  end
  continue
end
break src/field/main/misc2.c:1532 if (D_800C2694 | D_800AFE9C | D_800C3900) != 0
commands 3
  silent
  set $polls = $polls + 1
  printf "T9 POLL raw held=0x%%x released=0x%%x pressedOnce=0x%%x gates ADB00=0x%%x btnMask=0x%%x ADC18=%%d ADBDC=%%d B21D1=%%d F378=%%d map=%%d present=%%u\n", (unsigned)D_800AFE9C, (unsigned)D_800C2694, (unsigned)D_800C3900, (unsigned)D_800ADB00, (unsigned)g_FieldControl.controllerBtnMask, (int)D_800ADC18, (int)D_800ADBDC, (int)D_800B21D1, (int)D_8004F378, (int)(g_GameSceneMapNum & 0x3fff), (unsigned)g_PcPortPresentedFrames
  if $polls > 10
    delete 3
  end
  continue
end
break func_8009BB0C
ignore 4 400
commands 4
  silent
  printf "T9 TEXTBOX DUMP map=%%d present=%%u ADC18=%%d ADB00=0x%%x btnMask=0x%%x ADB68=%%d B21D0=%%d held=0x%%x released=0x%%x\n", (int)(g_GameSceneMapNum & 0x3fff), (unsigned)g_PcPortPresentedFrames, (int)D_800ADC18, (unsigned)D_800ADB00, (unsigned)g_FieldControl.controllerBtnMask, (int)D_800ADB68, (int)D_800B21D0, (unsigned)D_800AFE9C, (unsigned)D_800C2694
  set $k = 0
  while $k < 4
    set $tb = (char*)&g_FieldTextBoxes[$k]
    printf "BOX %%d status=%%d visibility=%%d order=%%u openTimer=%%d flags=0x%%x unk412=%%d owner=%%d talking=%%d cursor37C=%%d arrow3C4=%%d win+0x10=0x%%x win+0x82=%%d typingDone=%%d\n", $k, (int)*(short*)($tb+0x414), (int)*(short*)($tb+0x40E), (unsigned)*(unsigned short*)($tb+0x410), (int)*(short*)($tb+0x408), (unsigned)*(unsigned short*)($tb+0x40C), (int)*(short*)($tb+0x412), (int)*(short*)($tb+0x416), (int)*(short*)($tb+0x418), (int)*(short*)($tb+0x37C), (int)*(short*)($tb+0x3C4), (unsigned)*(unsigned short*)($tb+0x18+0x10), (int)*(short*)($tb+0x18+0x82), (int)func_80033CD0($tb+0x18)
    set $k = $k + 1
  end
  printf "cur actor ip=%%d unk81=%%d dialogFlags=0x%%x flags=0x%%x\n", (int)g_FieldScriptVMCurActor->scriptInstructionPointer, (int)g_FieldScriptVMCurActor->unk81, (unsigned)g_FieldScriptVMCurActor->dialogFlags, *(unsigned int*)g_FieldScriptVMCurActor
  printf "header now +0x12C=%%u\n", *(unsigned int*)((char*)D_8005A4E0 + 0x12c)
  x/14xw (char*)D_8005A4E0 + 0x120
  x/12xh g_pFieldTriggerZones
  delete 4
  continue
end
continue
detach
'''


class WatchSession:
    """gdb attached for the whole warp + scene; stopped with SIGINT -> detach."""

    def __init__(self, port, out, destination):
        self.proc = None
        if not shutil.which('gdb'):
            self.state = {'unavailable': 'gdb absent; no dependency installed'}
            return
        script = os.path.join(out, 'watch.gdb')
        with open(script, 'w') as f:
            f.write(WATCH_GDB % {'map': destination})
        self.log = open(os.path.join(out, 'watch-state.txt'), 'w')
        self.proc = subprocess.Popen(['gdb', '-q', '-batch', '-x', script, '-p', str(port.proc.pid)],
                                     stdout=self.log, stderr=subprocess.STDOUT)
        # Wait for the breakpoints to be armed before the warp is requested.
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and self.proc.poll() is None:
            with open(os.path.join(out, 'watch-state.txt'), errors='replace') as f:
                if 'Breakpoint 4 at' in f.read():
                    break
            time.sleep(0.2)
        self.state = {'attached': self.proc.poll() is None}

    def stop(self):
        if self.proc is None:
            return self.state
        if self.proc.poll() is None:
            self.proc.send_signal(2)
            try:
                self.proc.wait(25)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        self.log.close()
        self.state['gdb_exit'] = self.proc.returncode
        self.state['file'] = 'watch-state.txt'
        return self.state


def scene_snapshot(port, out):
    if not shutil.which('gdb'):
        return {'unavailable': 'gdb absent; no dependency installed'}
    script = os.path.join(out, 'scene.gdb')
    with open(script, 'w') as f:
        f.write(SCENE_GDB)
    with open(os.path.join(out, 'scene-state.txt'), 'w') as f:
        run = subprocess.run(['timeout', '-k', '2', '60', 'gdb', '-q', '-batch', '-x', script,
                              '-p', str(port.proc.pid)], stdout=f, stderr=subprocess.STDOUT)
    return {'gdb_exit': run.returncode, 'file': 'scene-state.txt'}


def main():
    root = os.getcwd()
    out = os.path.abspath('out/transition-probe')
    port = Port(os.path.abspath('pc_port/build_native/xeno-port'), root, out, {
        'XENO_QUICKSAVE_PATH': os.path.abspath('runner-inputs/ckpt14.xgqs'),
        'XENO_DEV_MENU': '1', 'XENO_FIELD_POS_DIAG': '1',
        'XENO_FIELD_TRANSITION_DIAG': '1', 'XENO_SPEED': '4',
        'XENO_HEARTBEAT_SECS': '2'})
    result = {'checkpoint_loaded': False, 'destination_loaded': False}
    try:
        result['checkpoint_loaded'] = port.load_checkpoint() is not None
        if result['checkpoint_loaded']:
            start = port.n()
            warp = int(os.environ.get('XENO_TRANSITION_WARP_INDEX', '4'))
            destination = int(os.environ.get('XENO_TRANSITION_MAP', '12'))
            result['warp_index'] = warp
            result['destination_map'] = destination
            watch = None
            if os.environ.get('XENO_TRANSITION_WATCH') == '1':
                watch = WatchSession(port, out, destination)
                result['watch'] = dict(watch.state)
            port.con('dev warp %d' % warp)
            result['destination_loaded'] = port.wait_for('FieldLoad begin field=%d ' % destination, 20, start) is not None
            if result['destination_loaded']:
                # Observe the destination scene without a direction held. Page
                # dialogue, then check its free-control gate through dev heal.
                # Phase A is the t6 input (CIRCLE, the title/painting-scene
                # confirm button); phase B pages with CROSS instead, to test
                # whether this scene's text button differs.  Once the lock has
                # held ~15 s, take one bounded state snapshot (see SCENE_GDB).
                refusals = 0
                snapshot = None
                result['scene_phases'] = []
                for name, mask, seconds in (('circle', 0x20, 45), ('cross', 0x40, 30)):
                    phase_start = time.monotonic()
                    deadline = phase_start + seconds
                    while time.monotonic() < deadline and port.proc.poll() is None:
                        before = port.n()
                        port.pad(mask, 4)
                        port.con('dev heal')
                        if port.wait_for('[CHEATS] heal:', 1, before) is not None:
                            result['scene_control_returned'] = True
                            result['scene_control_phase'] = name
                            break
                        i = port.find("[CHEATS] refused 'heal'", before)
                        if i is not None:
                            refusals += 1
                            with port.lock:
                                result['scene_last_refusal'] = port.lines[i]
                        if (snapshot is None and refusals >= 10 and
                                time.monotonic() - phase_start > 15):
                            # Only one tracer can be attached: t9's watch
                            # session supersedes the t8 snapshot.
                            snapshot = (scene_snapshot(port, out) if watch is None
                                        else {'skipped': 'watch session attached'})
                            result['scene_snapshot'] = snapshot
                    result['scene_phases'].append({
                        'input': name, 'seconds': seconds,
                        'control_returned': bool(result.get('scene_control_returned'))})
                    if result.get('scene_control_returned'):
                        break
                result['scene_heal_refusals'] = refusals
                if watch is not None:
                    result['watch'] = watch.stop()
                    watch = None
                with port.lock:
                    scene = [l for l in port.lines[start:] if ('POSDIAG map=%d ' % destination) in l]
                positions = set(re.findall(r'pos=\(([^)]*)\)', '\n'.join(scene)))
                result['scene_distinct_positions'] = len(positions)
                result['scene_position_samples'] = len(scene)
                if result.get('scene_control_returned'):
                    before = port.n()
                    port.con('load')
                    result['scene_reload_checkpoint'] = port.wait_for('player position restored', 25, before) is not None
            if watch is not None:
                result['watch'] = watch.stop()
                watch = None
            if not result['destination_loaded'] and port.proc.poll() is None:
                if shutil.which('gdb'):
                    with open(os.path.join(out, 'stack.txt'), 'w') as f:
                        stack = subprocess.run(['timeout', '-k', '2', '15', 'gdb', '-q', '-batch',
                                                '-ex', 'set pagination off', '-ex', 'thread apply all bt',
                                                '-ex', 'thread 1', '-ex', 'frame 0',
                                                '-ex', 'p s_ArchivePortStreamRead',
                                                '-ex', 'p g_ArchiveCurStreamFile',
                                                '-ex', 'p g_ArchiveCurFileSize',
                                                '-ex', 'p g_ArchiveCdDriveState',
                                                '-ex', 'p D_8004FDFC',
                                                '-ex', 'p D_8004FE24',
                                                '-ex', 'p D_8004FE28',
                                                '-ex', 'p *(int*)g_ArchiveCurStreamFile',
                                                '-ex', 'x/36hx g_ArchiveCurStreamFile',
                                                '-ex', 'p g_Heap',
                                                '-p', str(port.proc.pid)], stdout=f, stderr=subprocess.STDOUT)
                    result['stack_exit'] = stack.returncode
                else:
                    result['stack_unavailable'] = 'gdb absent; no dependency installed'
    finally:
        port.stop()
    with port.lock:
        result['stages'] = [l for l in port.lines if '[field-transition]' in l]
        result['heartbeat'] = [l for l in port.lines if '[heartbeat]' in l]
    json.dump(result, open(os.path.join(out, 'result.json'), 'w'), indent=2)
    print(json.dumps(result, indent=2))
    if os.environ.get('XENO_TRANSITION_EXPECT_ARRIVAL') == '1':
        return 0 if (result['checkpoint_loaded'] and result['destination_loaded'] and
                     result.get('scene_reload_checkpoint')) else 1
    return 0 if result['checkpoint_loaded'] and result['stages'] else 1


if __name__ == '__main__':
    sys.exit(main())
