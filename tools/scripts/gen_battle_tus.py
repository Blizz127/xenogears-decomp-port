import re, glob, os

S = '/var/tmp/xeno-harness/'
END = 0x800C204C

FAMILY_DECLS = '''extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u32 D_800D3410[];
extern u16 D_800CCD64[];
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u8 D_800C3EB7[];
extern u8 D_800D2DC0;
extern u8* D_800D3364;
extern s32 func_80079E7C(u32 value);
extern void func_80078508(void* pBuffer);'''
B8D4_DECLS = '''extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u32 D_800D3410[];
extern u16 D_8005A3A0[];
extern u8 D_800CCE34[];
extern u8 D_800CCE3E[];
extern u8 D_800D343F;
extern u8 D_800D342E;'''
R1_DECLS = '''extern u16 D_800C3468[];
extern u16 D_800C3448[];'''
R2_DECLS = '''extern int ArchiveSetIndex(int directoryIndex, int entryIndex);'''
TINY3_DECLS = '''extern u32 D_800D2D40;
extern u32 D_800D2D48;
extern u8 D_800C3BCC[];
extern u16 D_8005A3A0[];
extern u8 D_800D2E62[];
extern u16 D_800D39E0;
extern u8* D_800D3278;
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);'''
TINY3_SET = set('''func_800A5E9C func_800B00D0 func_800769E8 func_8007A9A8
func_80078D48 func_80080C6C func_800B168C func_8007E7C0'''.split())
TINY2_DECLS = '''extern u32 D_800D3344;
extern u32 D_800D39CC;
extern u8 D_800C3B74;
extern u8 D_800C3D6C;
extern u8* D_800C3610;'''
TINY2_SET = set('''func_800A577C func_800A578C func_800BF0B4 func_80079934
func_800AA788 func_800B14B8'''.split())
TINY_DECLS = '''extern u8* D_800D2D28;
extern u8* D_800D2DC8;'''
TINY_SET = set('''func_8009795C func_8009F5B0 func_800B3348 func_800B3350 func_800B89F4
func_800BAF40 func_800BDD34 func_80077980 func_8009E268'''.split())
R3_DECLS = '''extern u32 D_800C3610;
extern u32 D_80059464;
extern u32 D_800D2D68;
extern void func_800BD2E4(void);
extern u32 func_800BF720(void);'''

A9_DECLS = '''extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u32 D_800D3410[];
extern u16 D_8005A3A0[];'''

CDK_DECLS = '''extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(u32 a0, u32 a1);
extern u8 D_800C3EB0[];
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
extern u8 D_800D3420[];
extern u32 D_800C3CE8[];
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
extern u32 D_800C3548[];'''
CDK_SET = set('''func_800B5924 func_800B5C18 func_800BF7C8
func_800B56E4 func_800B5DC4 func_800BDC78 func_800B64D4
func_800B73A0 func_800BF5E8 func_800B6438 func_8007ADF4
func_800B16A4 func_800B35C0 func_800B7C28 func_800BCD8C
func_800BF730 func_800BC454 func_800BC3F8 func_800B8054
func_800BE108 func_800BED30 func_800B9B30 func_800B8048 func_800B7134
func_800B16F0 func_800B8068 func_800B3588 func_800B383C func_800BDCF8
func_800C0F70 func_800B397C func_800BEDE8 func_800BF3A4 func_800B3C2C
func_800BB7F8 func_800BB690 func_800BC404 func_800BF354 func_800BF998
func_800BCB54 func_800BEBC4 func_800B8D04 func_800B8840 func_800BF9EC func_800BF600
func_800BEB04 func_800BFBA0 func_800B8774 func_800B9258 func_800BE0DC'''.split())

T4_DECLS = '''extern u8* D_800D2D28;
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u32 D_800D3410[];
extern u16 D_8005A3A0[];
extern u8 D_800D366C;
extern u8* D_800C3EAC;
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_80085454(s32);
extern void func_80085618(s32);
extern void func_8008AA40(u8 v);
extern u32 D_800C3EA4;
extern void* func_8008ABB8(s32 size, s32 flag);
extern void* bzero(unsigned char* p, int size);
extern void func_80077074(void);'''
T4_SET = set('''func_8007A8B4 func_80085C88 func_8008BC40 func_8008CCCC
func_8008B108 func_80077610 func_8008AA74'''.split())

T5A_DECLS = '''extern u32 D_800D3368[];
extern u8 D_800C3B74;'''
T5B_DECLS = '''extern u8 D_800D343F;
extern u8 D_800D342E;'''
T5C_DECLS = '''extern void func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);
extern void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);'''
T5D_DECLS = '''extern u8 D_800D3410[];
extern u8 g_GameState[];'''
T5E_DECLS = '''extern void func_8008FC1C(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4);'''
T5A_SET = set(['func_800AA760'])
T5B_SET = set(['func_8007BA88'])
T5C_SET = set(['func_8007B3B0'])
T5D_SET = set(['func_8007D1A8'])
T5E_SET = set(['func_8008FDE4'])

T6_DECLS = {
    'func_800BE11C': '''extern s32 func_80021AD8(u8 v, s32 n);''',
    'func_800B9B54': '''extern u32 func_800BEF24(u8* a, u8* b);
extern void func_800223B0(u8* p, s16 v);
extern void func_80021FE0(u8* p, s16 v);''',
    'func_800BA8F4': '''typedef struct { long vx, vy; long vz, pad; } VECTOR;
typedef struct { short vx, vy; short vz, pad; } SVECTOR;
extern s32 func_800A5914(SVECTOR* v, u32 arg1, s32 arg2);
extern s32 func_800A579C(SVECTOR* v);
extern void func_800A5870(SVECTOR* v, s32 r, VECTOR* out);''',
    'func_800B6CEC': '''typedef struct { long vx, vy; long vz, pad; } VECTOR;
extern VECTOR* Square0(VECTOR* v0, VECTOR* v1);
extern long SquareRoot0(long a);
extern s32 ratan2(s32 y, s32 x);''',
    'func_800B6DC0': '',
    'func_800B6C44': '''extern s32 ratan2(s32 y, s32 x);''',
    'func_800B639C': '''extern void func_800B3CD4(u32 a0, u32 a1, u32 a2, s32 a3,
                           s32 a4, s32 a5);''',
    'func_800B6930': '',
    'func_800B6990': '',
    'func_8008A274': '''extern u8 D_800CCC58;
extern u8* D_800D2D28;
extern u8* D_800C3EA4;
extern u8 D_800D39D4;
extern s32 D_800D3288;
extern void func_8007171C(void);
extern void func_80089CCC(u8 v);
extern void func_8008A144(void);''',
    'func_8008A9C0': '''extern u8 D_800C3E4C;
extern void func_8008A684(u8 v);
extern void func_8008A274(u8 v);
extern void func_8008A3EC(u8 v);
extern u32 D_8005919C;
extern void func_80039DB8(u32 v);
extern u8 D_800D366C;''',
    'func_800800E8': '''extern u8* D_800D2D28;
extern u8 D_800D32A1[];
extern u32 D_800D2DB4;
extern void HeapFree(u32 p);''',
    'func_80089AF8': '''extern void func_8008860C(void);
extern void func_80089038(void);
extern void func_80089110(void);
extern void func_800891E4(void);
extern void func_80089348(void);
extern void func_8008946C(void);
extern void func_8008963C(void);
extern void func_800897CC(void);''',
    'func_8008FA60': '''extern u8* D_800D2D28;
extern u32 D_800D2E38[];
extern u32 D_800D2D90[];
extern void func_800716D8(void);
extern void HeapFree(u32 p);''',
    'func_80080B64': '''extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u8 D_800C400B[];
extern u8 D_800D366C;
extern u8* D_800D3278;
extern u8 D_800C204C;
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_800800E8(u8 index);
extern void func_8007FB70(u8 index);''',
    'func_8009A074': '''extern u8* D_800C34B0;''',
    'func_8009B098': '''extern u8* D_800C34B0;''',
    'func_8009D354': '''extern u8* D_800C34B0;
extern u8 D_800C3E50;
extern u32 func_8009DBFC(u32 a0);''',
    'func_8009CB68': '''extern u8 D_800CCD8C[];''',
    'func_8009AEFC': '''extern u8* D_800C34B0;
extern u8 D_800CCD68[];
extern void func_8009B104(u32 a0, u8* a1);''',
    'func_8007D148': '''extern u8 D_800D3420[];
extern u8 D_800D3410[];
extern u8 g_GameState[];''',
    'func_8007C580': '''extern u8 D_800D2E0C[];
extern u8 D_800CCD34[];
extern u8 D_800D3420[];
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);''',
    'func_8007D6A8': '''extern u8 D_800D32A1[];
extern u8 D_800CCDEC[];
extern u8 D_800CCD34[];
extern u8 D_800D3420[];
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);''',
    'func_8007C4A0': '''extern u8 D_800D2E06[];
extern u8 D_800D2E0C[];
extern u8 D_800CCD34[];
extern u8 D_800D3420[];
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);''',
    'func_8007C678': '''extern u8 D_800CCD34[];
extern u8 D_800D3420[];
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);''',
    'func_800B6BFC': '''typedef struct { s16 x; s16 y; s16 w; s16 h; } RECT;
extern int MoveImage(RECT* rect, int x, int y);
extern void func_800B73A0(void);''',
    'func_80097D08': '''extern u8* D_800C34B0;''',
    'func_8009C0E0': '''extern u8 D_800CCE30[];
extern u8 D_800CCE4A[];
extern u8 g_GameState[];''',
    'func_8007B0C8': '''extern u8 D_800D3420[];''',
    'func_8007BB2C': '''extern u8 D_800CCE3D[];
extern u8 D_800CCE3B[];
extern u8 D_800CCE39[];
extern u8 D_800CCE3C[];
extern u8 D_800CCE3A[];
extern u8 D_800CCE38[];
extern u8 D_800CCE3E[];''',
    'func_8007A828': '',
    'func_8007AFFC': '''extern u8 D_800D3430[];''',
    'func_8007B208': '''extern u8 D_800D3420[];''',
    'func_8007BC40': '',
    'func_80085C48': '''extern u16 D_800D39DC;
extern u16 D_800C48E8;
extern u16 D_800D2C94;
extern u16 D_800D2C96;
extern void func_80098C6C(u32 v);
extern u8* D_800D2D28;
extern void func_80085454(s32);
extern void func_80085618(s32);''',
    'func_8007B3E4': '''extern void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);''',
    'func_800879A8': '''extern u8 D_800C402F[];
extern u8 D_800C400B[];
extern u8 D_800C3FFE[];
extern u16 func_80089C08(u8 idx);''',
    'func_80071A08': '''extern u8* D_800C3EAC;
extern u8 g_GameState[];
extern u8* D_800D2D28;
extern void func_800716D8(void);
extern u8 D_800D3725[];''',
    'func_8007AAF4': '''extern u8 D_800D3430[];''',
    'func_80085350': '''extern u8 D_800D2D5C[];
extern u16 D_800D2D70[];''',
    'func_80085E78': '''extern u8* D_800C3EAC;''',
    'func_800885D0': '''extern u8 D_800C3EB4[];
extern u8 D_800D301C[];''',
    'func_800B73EC': '''extern void* HeapAlloc(u32 allocSize, u32 allocFlags);
extern void func_800B7424(u8* p);''',
    'func_800B8354': '''extern s32 ArchiveDataSync(void);
extern void func_800BE790(void);''',
    'func_8007AB30': '''extern u8 D_800D3430[];''',
    'func_8007E674': '''extern u32 D_800D2C60[];
extern u8 D_800D2C8B[];''',
    'func_8008AA40': '''extern u32 D_8005919C;
extern void func_80039DB8(u32 v);
extern u8 D_800D366C;
extern u8 D_800C3E4C;
extern void func_8008A684(u8 v);
extern void func_8008A274(u8 v);
extern void func_8008A3EC(u8 v);''',
    'func_8009E508': '''extern u8* D_800D2DC8;
extern u8* D_800C3E34;''',
    'func_800B8D7C': '''extern void func_8002A498(u32 v);
extern void func_800B8D04(void);''',
    'func_800BCAD0': '''extern u8 D_800C37C8;
extern void func_800BC2F0(s32 v);''',
    'func_8008B108': '''extern u8* D_800D2D28;
extern u8* D_800C3EAC;
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);''',
    'func_8008BC98': '''extern u8* D_800D2D28;
extern u8* D_800C3EAC;
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);''',
    'func_8008CD28': '''extern u8* D_800D2D28;
extern u8* D_800C3EAC;
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);''',
    'func_8008C360': '''extern u8* D_800D2D28;
extern u8* D_800C3EAC;
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_8008FA60(u32 v);
extern void func_800716D8(void);
extern void func_8007765C(void);
extern void func_80077980(void);''',
    'func_8007FCE8': '''extern u8* D_800D2D28;
extern u32 D_800D367C;
extern void HeapFree(u32 p);''',
    'func_8007FDEC': '''extern u8* D_800D2D28;
extern u32 D_800C3DE8;
extern void HeapFree(u32 p);''',
    'func_80078CEC': '''extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u8 D_800D2E60[];
extern void func_800785D4(u8 a, u8 b);''',
    'func_8007893C': '''extern u8 D_800D2E5F[];
extern void func_80078658(u8 a, u8 b);
extern void func_800787E0(u8 a, u8 b);
extern void func_8007887C(u8 a);''',
    'func_800716D8': '''extern u32 D_8005917C;
extern void func_8028022C(void);
extern void func_800BE790(void);''',
    'func_800A3484': '',
    'func_800AEEEC': '',
    'func_800B3B6C': '''extern u32 D_800C3558;''',
    'func_800BCAA4': '''extern u8 D_800C37C8;
extern void func_800BC2F0(s32 v);''',
    'func_800BFD88': '''extern void func_800BFC80(u32 a0, u32 a1, u32 a2);''',
    'func_80079E18': '''extern u8* D_800D2D28;
extern u8 D_800D3725[];
extern u32 func_80089C9C(u32 a, u32 b);''',
    'func_80078C9C': '''extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u8 D_800D2E60[];
extern u16 D_800D39E0;
extern u8 D_800D2E62[];
extern void func_800785D4(u8 a, u8 b);''',
    'func_8009A7B8': '''extern u8 g_GameState[];''',
    'func_8009E3C8': '''extern u32 D_800C3D60;
extern u8 D_800C3E50;
extern u8 D_800CCE4A[];''',
    'func_8007AF5C': '''extern u8 D_800D3430[];
extern u8 D_800D3420[];''',
    'func_80079114': '''extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E61[];
extern void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);''',
    'func_800764EC': '''extern void func_8008FAD8(void);
extern void func_80073538(void);
extern void func_80073F08(void);
extern void func_8007500C(void);
extern void func_80074F70(void);
extern void func_80073FB8(void);
extern void func_80088B80(void);
extern void func_80074AB8(void);''',
    'func_8008AC00': '''extern void HeapChangeCurrentUser(s32 user, void* p);
extern void* HeapAlloc(s32 size, s32 mode);
extern s32 ArchiveDataSync(void);
extern void func_800716D8(void);''',
    'func_80076B68': '''extern void func_80076AC8(u8* p);''',
    'func_80079054': '''extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern void func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);''',
    'func_8008ABB8': '''extern void HeapChangeCurrentUser(s32 user, void* p);
extern void* HeapAlloc(s32 size, s32 mode);''',
    'func_800A22A8': '''extern void HeapFree(u32 p);''',
    'func_800A2D1C': '''extern void HeapFree(u32 p);''',
    'func_800B7364': '''extern void WorkListRemoveTask(u32 p);
extern void TimerWorkListRemoveTask(u32 p);
extern void func_80025180(u32 p);''',
    'func_800BB314': '''extern void WorkListRemoveTask(u32 p);
extern void TimerWorkListRemoveTask(u32 p);
extern void HeapFree(u32 p);''',
    'func_800B9020': '''extern void func_800245D8(u32 p, s32 v);
extern void func_80021BF8(u32 p, u32 v);''',
    'func_800764B4': '''extern u8 D_8005959C;
extern void func_8008FAD8(void);
extern void func_801DE594(void);
extern void func_80073FB8(void);''',
    'func_80076AC8': '''extern void SetSemiTrans(void* p, s32 v);
extern void SetShadeTex(void* p, s32 v);''',
    'func_8007765C': '''extern u32 D_800C3EA4;
extern void func_800716D8(void);
extern void HeapFree(u32 p);''',
    'func_8007AAB8': '''extern u8 D_800D3430[];''',
    'func_8008AC50': '''extern s32 ArchiveDataSync(void);
extern void func_800716D8(void);''',
    'func_8009AB00': '''extern u8* D_800C34B0;''',
    'func_800B7330': '''extern void DrawSync(s32 mode);
extern void HeapFree(u32 p);''',
}

ROWS = []
for p in glob.glob('asm/battle/*/*.s') + glob.glob('asm/battle/*/*/*.s'):
    m = open(p).read(300)
    mm = re.search(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ', m)
    if mm:
        ROWS.append((int(mm.group(1), 16), os.path.basename(p)[:-2]))
ROWS = sorted((a, n) for a, n in ROWS)
addr = {n: a for a, n in ROWS}
size = {n: (ROWS[i + 1][0] if i + 1 < len(ROWS) else END) - a for i, (a, n) in enumerate(ROWS)}

bodies = {}
def collect(path):
    s = open(path).read()
    i = s.find('/* Battle overlay 0x8007E8AC')
    region = s[i:] if i >= 0 else s
    for b in re.split(r'\n(?=/\* func_8[0-9A-F]+\.s)', region):
        m = re.search(r'/\* (func_[0-9A-F]+)\.s', b)
        if m and b.strip():
            bodies[m.group(1)] = b.rstrip() + '\n'
for p in sorted(glob.glob('src/battle/main*.c')):
    collect(p)
for p in sorted(glob.glob(S + 'RUN_*.c')):
    collect(p)                      # drafts are authoritative

VERIFIED = set('''
func_800BF6CC func_800BF6F8 func_800BF720
func_8008AB4C func_8008AB70 func_8008AB94
func_80089BEC func_80089C08 func_80089C24 func_80089C48 func_80089C6C func_80089C9C
func_8007E934 func_8007E954 func_8007E9D0 func_8007EA4C func_8007EAC8 func_8007EB50
func_8007EBD8 func_8007EC54 func_8007ECDC func_8007ED58 func_8007EE70 func_8007EEA8
func_8007EED0 func_8007EF44 func_8007E98C func_8007B958
func_8007E8AC func_8007E8E0 func_8007EA08 func_8007EA84 func_8007EB08 func_8007EB90
func_8007EC10 func_8007EC94 func_8007ED14 func_8007ED98 func_8007EDE0 func_8007EE28
func_8007EEE8 func_8007B914
func_8009795C func_8009F5B0 func_800B3348 func_800B3350 func_800B89F4 func_800BAF40
func_800BDD34 func_80077980 func_8009E268
func_800A577C func_800A578C func_800BF0B4 func_80079934 func_800AA788 func_800B14B8
func_800A5E9C func_800B00D0 func_800769E8 func_8007A9A8 func_80078D48 func_80080C6C func_800B168C func_8007E7C0
func_8007A900
func_8007A8B4 func_80085C88 func_8008BC40 func_8008CCCC func_8008B108
func_80077610 func_8008AA74
func_800AA760 func_8007B3B0 func_8007BA88 func_8007BAB8 func_800B6A50
func_8007D1A8 func_8008FDE4
func_800764B4 func_80076AC8 func_8007765C func_8007AAB8 func_8008AC50
func_8009AB00 func_800B7330
func_8008ABB8 func_800A22A8 func_800A2D1C func_800B7364 func_800B9020
func_8008AC00 func_80076B68 func_80076BAC func_80076BF0 func_80076C34
func_80079054
func_8007AF5C func_8007AFAC func_80079114 func_800764EC
func_80079E18 func_80079E4C func_80079E7C func_80078C9C func_8009A7B8
func_8009E3C8
func_800A3484 func_800AEEEC func_800B3B6C func_800BCAA4
func_800BFD88
func_8007FCE8 func_8007FDEC func_80078CEC func_8007893C
func_8008B168 func_8008BC98 func_8008CD28 func_8008C360 func_8008C3F0
func_8007AB30 func_8007AB68 func_8007ABA0 func_8007E674 func_8008AA40
func_8009E508 func_800B8D7C func_800BCAD0
func_80071A08 func_80085350 func_80085E78 func_800885D0 func_800B8354
func_8007AAF4 func_800B73EC
func_80085C48 func_8007B3E4 func_800879A8
func_8007A828 func_8007AFFC func_8007B040 func_8007B084 func_8007B208
func_8007B264 func_8007BC40
func_8007B0C8 func_8007B134 func_8007BB70 func_8007BBD8
func_8007B198 func_80097D08 func_8009C0E0
func_8007BB2C func_800B6BFC
func_8007C4A0 func_8007C678 func_8007C75C
func_8007C580 func_8007D6A8 func_8007D7B4
func_8007D148
func_8009AEFC
func_8009D354 func_8009CB68
func_8009A074 func_8009B098
func_80089AF8 func_8008FA60 func_80080B64
func_800800E8 func_80080BD0 func_8008A9C0
func_8008A274
func_800B6930
func_800B6990
func_800B639C
func_800B6C44 func_800B6C98
func_800B6DC0
func_800B6CEC
func_80071A38
func_80071A8C
func_800BA8F4
func_800B9B54
func_800BE11C
func_800B5924 func_800B5C18 func_800BF7C8
func_800B56E4 func_800B5DC4 func_800BDC78
func_800B64D4
func_800B73A0
func_800BF5E8 func_800B6438 func_8007ADF4 func_800B16A4
func_800B35C0
func_800B7C28 func_800BCD8C func_800BF730 func_800BC454 func_800BC3F8
func_800B8054 func_800BE108 func_800BED30 func_800B9B30
func_800B8048 func_800B7134 func_800B16F0 func_800B8068 func_800B3588
func_800B383C func_800BDCF8 func_800C0F70 func_800B397C func_800BEDE8
func_800BF3A4 func_800B3C2C func_800BB7F8 func_800BB690 func_800BC404
func_800BF354 func_800BF998 func_800BCB54 func_800BEBC4
func_800B8D04 func_800B8840 func_800BF9EC func_800BF600
func_800BEB04 func_800BFBA0 func_800B8774 func_800B9258 func_800BE0DC
func_8007D30C func_80085310 func_800AA7DC
'''.split())
landed = sorted((addr[n], n) for n in bodies if n in VERIFIED and n in addr)
LIT_FUNCS = set('''func_800B3B6C func_800BCAA4 func_800BFD88
func_800BCAD0 func_800B73EC func_800B6BFC func_800B6DC0
func_800B6CEC func_800BA8F4
func_800B9B54'''.split())


def flavour_of(n):
    # TU-wide compiler/flavour classes: `c` = CDK cc1 + addiu li,
    # `l` = PSY-Q 2.7.2 + addiu li, `d` = PSY-Q 2.7.2 + default (lui/ori) li.
    if n in CDK_SET:
        return 'c'
    if n in LIT_FUNCS:
        return 'l'
    return 'd'


runs = []
for a, n in landed:
    # the `li` expansion flag is a TU-wide assembler setting, so a run may not
    # mix the addiu-flavoured bodies with the ori-flavoured ones
    same_flavour = (runs and runs[-1][2] and
                    flavour_of(runs[-1][2][-1]) == flavour_of(n))
    if runs and runs[-1][1] == a and same_flavour:
        runs[-1][1] = a + size[n]
        runs[-1][2].append(n)
    else:
        runs.append([a, a + size[n], [n]])

def decls_for(funcs):
    head = funcs[0]
    if head in CDK_SET:
        return CDK_DECLS
    if head in T6_DECLS:
        return T6_DECLS[head]
    if head in T5A_SET:
        return T5A_DECLS
    if head in T5B_SET:
        return T5B_DECLS
    if head in T5C_SET:
        return T5C_DECLS
    if head in T5D_SET:
        return T5D_DECLS
    if head in T5E_SET:
        return T5E_DECLS
    if head in T4_SET:
        return T4_DECLS
    if head in TINY_SET:
        return TINY_DECLS
    if head in TINY2_SET:
        return TINY2_DECLS
    if head in TINY3_SET:
        return TINY3_DECLS
    if head.startswith('func_80089'):
        return R1_DECLS
    if head.startswith('func_8008AB'):
        return R2_DECLS
    if head.startswith('func_800BF'):
        return R3_DECLS
    if head.startswith('func_8007E'):
        return FAMILY_DECLS
    if head.startswith('func_8007B'):
        return B8D4_DECLS
    if head.startswith('func_8007A'):
        return A9_DECLS
    return ''

cur = open('src/battle/main.c').read()
port_blocks = re.findall(r'#ifndef XENO_PC_PORT\n#else\n.*?\n#endif\n',
                         cur[:cur.find('/* Battle overlay 0x8007E8AC')], re.S)
texts, names, prev, yaml, file_of = {}, [], 0x80070F40, [], {}
for i, (rs, rend, funcs) in enumerate(runs):
    if i == 0:
        name = 'main'
    elif funcs and funcs[0] in CDK_SET:
        name = 'mainc%d' % (i + 1)
    elif funcs and funcs[0] in LIT_FUNCS:
        name = 'mainl%d' % (i + 1)
    else:
        name = 'main%d' % (i + 1)
    stubs = [n for a, n in ROWS if prev <= a < rs and n.startswith('func_')]
    parts = ['#include "common.h"\n']
    if i == 0:
        parts.append('\n'.join(b.rstrip() for b in port_blocks) + '\n')
    if stubs:
        parts.append('\n'.join('INCLUDE_ASM("asm/battle/nonmatchings/%s", %s);' % (name, s) for s in stubs) + '\n')
    d = decls_for(funcs)
    if d:
        parts.append(d + '\n')
    body = ''.join(bodies[f] for f in funcs)
    if body:
        parts.append(body.rstrip() + '\n')
    texts[name] = '\n\n'.join(p for p in parts if p.strip())
    names.append(name)
    for f in funcs:
        file_of[f] = name
    yaml.append((prev, name))
    prev = rend
tail = 'main%d' % (len(runs) + 1)
stubs = [n for a, n in ROWS if prev <= a < END and n.startswith('func_')]
texts[tail] = ('#include "common.h"\n\n' +
               '\n'.join('INCLUDE_ASM("asm/battle/nonmatchings/%s", %s);' % (tail, s) for s in stubs) + '\n')
yaml.append((prev, tail))
names.append(tail)

for n in names:
    open('src/battle/%s.c' % n, 'w').write(texts[n])
for p in glob.glob('src/battle/main*.c'):
    b = os.path.basename(p)[:-2]
    if b not in names:
        os.unlink(p)
open(S + 'tu_names.txt', 'w').write('\n'.join(names) + '\n')
open(S + 'yaml_entries.txt', 'w').write('\n'.join('      - [0x%X, c, %s]' % (off - 0x8006FAF0, n) for off, n in yaml) + '\n')
print("landed %d bodies in %d runs; TUs: %d; stubs: %d" %
      (len(landed), len(runs), len(names), sum(t.count('INCLUDE_ASM') for t in texts.values())))
print("R1 in", file_of.get('func_80089BEC'), " R2 in", file_of.get('func_8008AB4C'), " R3 in", file_of.get('func_800BF6CC'))
