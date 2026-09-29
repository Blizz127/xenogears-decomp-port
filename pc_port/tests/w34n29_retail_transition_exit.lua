local ffi = require("ffi")
local mem = PCSX.getMemPtr()
local rawPrint = print
local livePath = os.getenv("XENO_RETAIL_TRACE_LIVE")
local liveFile = livePath ~= nil and livePath ~= "" and
    assert(io.open(livePath, "w")) or nil

local function emit(message)
    rawPrint(message)
    if liveFile ~= nil then
        liveFile:write(message .. "\n")
        liveFile:flush()
    end
end
print = emit

local function off(addr) return bit.band(addr, 0x001fffff) end
local function ptr(addr) return mem + off(addr) end
local function u8(addr) return tonumber(ffi.cast("uint8_t*", ptr(addr))[0]) end
local function u16(addr) return tonumber(ffi.cast("uint16_t*", ptr(addr))[0]) end
local function u32(addr) return tonumber(ffi.cast("uint32_t*", ptr(addr))[0]) end
local function s32(addr) return tonumber(ffi.cast("int32_t*", ptr(addr))[0]) end
local function w32(addr, value) ffi.cast("uint32_t*", ptr(addr))[0] = value end
local function reg(name) return tonumber(PCSX.getRegisters().GPR.n[name]) end

local statePath = os.getenv("XENO_RETAIL_WORLD_STATE")
local frameLimit = tonumber(os.getenv("XENO_RETAIL_TRACE_FRAMES") or "5")
local loaded = false
local frame = 0
local branches = 0
local seededD554 = false
local seededD7cc = false
local slot1Calls = 0
local slot2Calls = 0
local driverExits = 0
local terminalSeen = false
local overlayCalls = 0
local stateCalls = 0
local soundCleanupCalls = 0
local audioDone = false
local sizeCalls = 0
local sizeValue = 0
local copyCalls = 0
local managerCreateCalls = 0
local managerConfigureCalls = 0
local commonSeen = false
local syncCalls = 0

local function fail(message)
    print("W34N29_RETAIL FAIL " .. message)
    PCSX.quit(1)
end

if statePath == nil or statePath == "" then
    error("XENO_RETAIL_WORLD_STATE is required")
end
if frameLimit == nil or frameLimit < 1 then
    error("XENO_RETAIL_TRACE_FRAMES must be a positive integer")
end

PCSX.addBreakpoint(0x8007105c, "Exec", 4, "slot1 call", function()
    if not loaded then return end
    slot1Calls = slot1Calls + 1
    print(string.format("W34N29_RETAIL SLOT1_CALL target=%08x", reg("v0")))
    if slot1Calls ~= 1 or reg("v0") ~= 0x80072238 then
        return fail("slot1")
    end
end)

PCSX.addBreakpoint(0x8007130c, "Exec", 4, "frame head", function()
    if not loaded then return end
    frame = frame + 1
    print(string.format("W34N29_RETAIL FRAME_HEAD frame=%d D554=%d D7CC=%d",
        frame, s32(0x8009d554), s32(0x8009d7cc)))
    if frame ~= branches + 1 then return fail("frame_head.count") end
    if s32(0x8009d554) ~= 1 or s32(0x8009d7cc) ~= 2 then
        return fail("frame_head.state")
    end
end)

PCSX.addBreakpoint(0x800719c0, "Exec", 4, "frame latch load", function()
    if not loaded or frame ~= frameLimit or seededD554 then return end
    local before = s32(0x8009d554)
    print(string.format("W34N29_RETAIL SEED_D554 frame=%d before=%d after=0",
        frame, before))
    if before ~= 1 then return fail("seed_d554.before") end
    w32(0x8009d554, 0)
    seededD554 = true
end)

PCSX.addBreakpoint(0x800719c8, "Exec", 4, "frame branch", function()
    if not loaded then return end
    branches = branches + 1
    local value = s32(0x8009d554)
    local taken = value ~= 0 and 1 or 0
    print(string.format("W34N29_RETAIL FRAME_BRANCH frame=%d D554=%d taken=%d",
        frame, value, taken))
    if branches ~= frame then return fail("frame_branch.count") end
    if frame < frameLimit and taken ~= 1 then return fail("early_exit") end
    if frame == frameLimit and taken ~= 0 then return fail("seeded_exit") end
end)

PCSX.addBreakpoint(0x800719d0, "Exec", 4, "driver exit", function()
    if not loaded then return end
    driverExits = driverExits + 1
    print(string.format("W34N29_RETAIL DRIVER_EXIT D554=%d D7CC=%d",
        s32(0x8009d554), s32(0x8009d7cc)))
    if driverExits ~= 1 or not seededD554 then return fail("driver_exit") end
end)

-- Disclosed transition-state seed, after the driver and before slot 2.  Slot
-- 2 therefore executes its real D7CC==1 snapshot/teardown behavior.
PCSX.addBreakpoint(0x8007109c, "Exec", 4, "transition state seed", function()
    if not loaded then return end
    local before = s32(0x8009d7cc)
    print(string.format("W34N29_RETAIL SEED_D7CC before=%d after=1", before))
    if driverExits ~= 1 or before ~= 2 then return fail("seed_d7cc") end
    w32(0x8009d7cc, 1)
    seededD7cc = true
end)

PCSX.addBreakpoint(0x800710c4, "Exec", 4, "slot2 call", function()
    if not loaded then return end
    slot2Calls = slot2Calls + 1
    print(string.format("W34N29_RETAIL SLOT2_CALL target=%08x D7CC=%d",
        reg("v0"), s32(0x8009d7cc)))
    if not seededD7cc or slot2Calls ~= 1 or reg("v0") ~= 0x8007299c or
       s32(0x8009d7cc) ~= 1 then return fail("slot2") end
end)

PCSX.addBreakpoint(0x800710cc, "Exec", 4, "slot2 return", function()
    if not loaded then return end
    print(string.format(
        "W34N29_RETAIL SLOT2_RETURN D7CC=%d F954=%04x SNAPSHOT_HEAD=%08x",
        s32(0x8009d7cc), u16(0x8006f954), u32(0x8005a4e4)))
    if slot2Calls ~= 1 or s32(0x8009d7cc) ~= 1 then
        return fail("slot2_return")
    end
end)

PCSX.addBreakpoint(0x800711b0, "Exec", 4, "transition lane entry", function()
    if not loaded then return end
    terminalSeen = true
    print("W34N29_RETAIL TERMINAL_DECISION D7CC=1 lane=transition")
    if s32(0x8009d7cc) ~= 1 then return fail("terminal.d7cc") end
end)

PCSX.addBreakpoint(0x800199cc, "Exec", 4, "LoadGameStateOverlay entry", function()
    if not terminalSeen then return end
    overlayCalls = overlayCalls + 1
    print(string.format("W34N29_RETAIL LOAD_OVERLAY call=%d a0=%d",
        overlayCalls, reg("a0")))
    if overlayCalls ~= 1 or reg("a0") ~= 2 then return fail("overlay") end
end)

PCSX.addBreakpoint(0x8001996c, "Exec", 4, "ChangeGameState entry", function()
    if not terminalSeen then return end
    stateCalls = stateCalls + 1
    print(string.format("W34N29_RETAIL CHANGE_STATE call=%d a0=%d",
        stateCalls, reg("a0")))
    if stateCalls ~= 1 or reg("a0") ~= 2 then return fail("state") end
end)

PCSX.addBreakpoint(0x80039cc4, "Exec", 4, "sound cleanup entry", function()
    if not terminalSeen then return end
    soundCleanupCalls = soundCleanupCalls + 1
    print(string.format(
        "W34N29_RETAIL SOUND_CLEANUP call=%d BYTE594F8=%02x EE70=%04x EE72=%04x EE74=%04x",
        soundCleanupCalls, u8(0x800594f8), u16(0x8006ee70),
        u16(0x8006ee72), u16(0x8006ee74)))
    if overlayCalls ~= 1 or stateCalls ~= 1 or soundCleanupCalls ~= 1 then
        return fail("sound_cleanup.order")
    end
    if u8(0x800594f8) ~= 0 then return fail("sound_cleanup.byte") end
end)

PCSX.addBreakpoint(0x800711fc, "Exec", 4, "sound cleanup return", function()
    if not terminalSeen then return end
    audioDone = true
end)

PCSX.addBreakpoint(0x800288ec, "Exec", 4, "aligned-size entry", function()
    if not audioDone then return end
    sizeCalls = sizeCalls + 1
    print(string.format(
        "W34N29_RETAIL ALIGNED_SIZE call=%d a0=%08x BCC8=%08x C614=%08x",
        sizeCalls, reg("a0"), u32(0x8009bcc8), u32(0x8009c614)))
    if sizeCalls ~= 1 or reg("a0") ~= u32(0x8009bcc8) then
        return fail("aligned_size")
    end
end)

PCSX.addBreakpoint(0x80071214, "Exec", 4, "aligned-size return", function()
    if sizeCalls ~= 1 then return end
    sizeValue = reg("v0")
    print(string.format("W34N29_RETAIL ALIGNED_SIZE_RETURN size=%d", sizeValue))
end)

PCSX.addBreakpoint(0x8003f968, "Exec", 4, "memcpy entry", function()
    if sizeCalls ~= 1 then return end
    copyCalls = copyCalls + 1
    print(string.format(
        "W34N29_RETAIL COPY call=%d dst=%08x src=%08x size=%d",
        copyCalls, reg("a0"), reg("a1"), reg("a2")))
    if copyCalls ~= 1 or reg("a0") ~= 0x80062648 or
       reg("a1") ~= u32(0x8009c614) or reg("a2") ~= sizeValue then
        return fail("copy.arguments")
    end
end)

PCSX.addBreakpoint(0x80039850, "Exec", 4, "manager create entry", function()
    if copyCalls ~= 1 then return end
    managerCreateCalls = managerCreateCalls + 1
    print(string.format(
        "W34N29_RETAIL MANAGER_CREATE call=%d a0=%08x OLD62528=%08x SAVED4F2FC=%08x",
        managerCreateCalls, reg("a0"), u32(0x80062528), u32(0x8004f2fc)))
    if managerCreateCalls ~= 1 or reg("a0") ~= 0x80062648 or
       u32(0x8004f2fc) ~= u32(0x80062528) then
        return fail("manager_create")
    end
end)

PCSX.addBreakpoint(0x80039a80, "Exec", 4, "manager configure entry", function()
    if managerCreateCalls ~= 1 then return end
    managerConfigureCalls = managerConfigureCalls + 1
    print(string.format(
        "W34N29_RETAIL MANAGER_CONFIGURE call=%d a0=%08x a1=%d a2=%d NEW62528=%08x",
        managerConfigureCalls, reg("a0"), reg("a1"), reg("a2"),
        u32(0x80062528)))
    if managerConfigureCalls ~= 1 or reg("a0") ~= u32(0x80062528) or
       reg("a1") ~= 127 or reg("a2") ~= 0 then
        return fail("manager_configure")
    end
end)

PCSX.addBreakpoint(0x800712a0, "Exec", 4, "common epilogue", function()
    if not terminalSeen then return end
    commonSeen = true
    print(string.format("W34N29_RETAIL COMMON_EPILOGUE BYTE591AE_BEFORE=%02x",
        u8(0x800591ae)))
    if managerConfigureCalls ~= 1 then return fail("common.before_manager") end
end)

PCSX.addBreakpoint(0x800762fc, "Exec", 4, "sync entry", function()
    if not terminalSeen then return end
    syncCalls = syncCalls + 1
    print(string.format("W34N29_RETAIL SYNC_762FC call=%d BYTE591AE=%02x",
        syncCalls, u8(0x800591ae)))
    if not commonSeen or syncCalls ~= 1 or u8(0x800591ae) ~= 0 then
        return fail("sync")
    end
end)

PCSX.addBreakpoint(0x80019acc, "Exec", 4, "MainLoop entry", function()
    if not terminalSeen then return end
    print(string.format("W34N29_RETAIL MAIN_LOOP a0=%d frames=%d",
        reg("a0"), frame))
    if syncCalls ~= 1 or reg("a0") ~= 0 or frame ~= frameLimit then
        return fail("main_loop")
    end
    print(string.format(
        "W34N29_RETAIL PASS frames=%d slot1=%d slot2=%d transition=1",
        frame, slot1Calls, slot2Calls))
    PCSX.quit(0)
end)

PCSX.Events.createEventListener("ExecutionFlow::SaveStateLoaded", function()
    loaded = true
    print("W34N29_RETAIL STATE_LOADED")
end)

PCSX.Events.createEventListener("ExecutionFlow::ShellReached", function()
    PCSX.pauseEmulator()
    PCSX.nextTick(function()
        local stateFile = Support.File.open(statePath, "READ")
        PCSX.loadSaveState(stateFile)
        if not loaded then return fail("load_event_missing") end
        PCSX.resumeEmulator()
    end)
end)

print("W34N29_RETAIL READY")
