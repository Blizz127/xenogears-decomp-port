#!/usr/bin/env python3
"""Ensure shutdown drains final runtime receipts; retain old-close negative control."""
import io,subprocess,sys,threading,time,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from xeno_drive import Session
class Display:
 def __init__(self,release):self.release=release
 def terminate(self):pass
 def wait(self):threading.Timer(.05,self.release.set).start()
def check(close):
 s=Session.__new__(Session);s.lines=[];s.lock=threading.Lock();s.t0=time.time();s.log=io.StringIO();release=threading.Event();errors=[]
 s.proc=subprocess.Popen([sys.executable,'-u','-c',"import signal,time,sys; signal.signal(signal.SIGTERM,lambda *_:(print('FINAL COUNTER RECEIPT',flush=True),sys.exit(0)));print('READY',flush=True);time.sleep(30)"],stdout=subprocess.PIPE)
 assert s.proc.stdout.readline()==b'READY\n';s.xv=Display(release)
 def read():
  release.wait(2)
  try:s._reader()
  except Exception as e:errors.append(str(e))
 s.reader_thread=threading.Thread(target=read);s.reader_thread.start();close(s);s.reader_thread.join(2)
 return {'runtime_rc':s.proc.returncode,'lines':s.lines,'errors':errors,'reader_live':s.reader_thread.is_alive()}
def oldclose(s):
 if s.proc.poll() is None:s.proc.terminate();s.proc.wait(15)
 s.xv.terminate();s.xv.wait();s.log.close()
fixed=check(Session.close);baseline=check(oldclose)
assert fixed=={'runtime_rc':0,'lines':['FINAL COUNTER RECEIPT'],'errors':[],'reader_live':False},fixed
assert baseline['errors'] and not baseline['reader_live'],baseline
print(json.dumps({'fixed':fixed,'baseline_negative_control':baseline,'result':'PASS'}))
