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
local stateCalls = 0
local clearCalls = 0
local drawSyncCalls = 0
local commonSeen = false
local syncCalls = 0

local function fail(message)
    print("W34N31_RETAIL FAIL " .. message)
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
    print(string.format("W34N31_RETAIL SLOT1_CALL target=%08x", reg("v0")))
    if slot1Calls ~= 1 or reg("v0") ~= 0x80072238 then
        return fail("slot1")
    end
end)

PCSX.addBreakpoint(0x8007130c, "Exec", 4, "frame head", function()
    if not loaded then return end
    frame = frame + 1
    print(string.format("W34N31_RETAIL FRAME_HEAD frame=%d D554=%d D7CC=%d",
        frame, s32(0x8009d554), s32(0x8009d7cc)))
    if frame ~= branches + 1 then return fail("frame_head.count") end
    if s32(0x8009d554) ~= 1 or s32(0x8009d7cc) ~= 2 then
        return fail("frame_head.state")
    end
end)

PCSX.addBreakpoint(0x800719c0, "Exec", 4, "frame latch load", function()
    if not loaded or frame ~= frameLimit or seededD554 then return end
    local before = s32(0x8009d554)
    print(string.format("W34N31_RETAIL SEED_D554 frame=%d before=%d after=0",
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
    print(string.format("W34N31_RETAIL FRAME_BRANCH frame=%d D554=%d taken=%d",
        frame, value, taken))
    if branches ~= frame then return fail("frame_branch.count") end
    if frame < frameLimit and taken ~= 1 then return fail("early_exit") end
    if frame == frameLimit and taken ~= 0 then return fail("seeded_exit") end
end)

PCSX.addBreakpoint(0x800719d0, "Exec", 4, "driver exit", function()
    if not loaded then return end
    driverExits = driverExits + 1
    print(string.format("W34N31_RETAIL DRIVER_EXIT D554=%d D7CC=%d",
        s32(0x8009d554), s32(0x8009d7cc)))
    if driverExits ~= 1 or not seededD554 then return fail("driver_exit") end
end)

-- Disclosed signed-default seed after the driver and before slot 2.
PCSX.addBreakpoint(0x8007109c, "Exec", 4, "default state seed", function()
    if not loaded then return end
    local before = s32(0x8009d7cc)
    print(string.format("W34N31_RETAIL SEED_D7CC before=%d after=-1", before))
    if driverExits ~= 1 or before ~= 2 then return fail("seed_d7cc") end
    w32(0x8009d7cc, 0xffffffff)
    seededD7cc = true
end)

PCSX.addBreakpoint(0x800710c4, "Exec", 4, "slot2 call", function()
    if not loaded then return end
    slot2Calls = slot2Calls + 1
    print(string.format("W34N31_RETAIL SLOT2_CALL target=%08x D7CC=%d",
        reg("v0"), s32(0x8009d7cc)))
    if not seededD7cc or slot2Calls ~= 1 or reg("v0") ~= 0x8007299c or
       s32(0x8009d7cc) ~= -1 then return fail("slot2") end
end)

PCSX.addBreakpoint(0x800710cc, "Exec", 4, "slot2 return", function()
    if not loaded then return end
    print(string.format("W34N31_RETAIL SLOT2_RETURN D7CC=%d",
        s32(0x8009d7cc)))
    if slot2Calls ~= 1 or s32(0x8009d7cc) ~= -1 then
        return fail("slot2_return")
    end
end)

PCSX.addBreakpoint(0x80071264, "Exec", 4, "default lane entry", function()
    if not loaded then return end
    terminalSeen = true
    print(string.format(
        "W34N31_RETAIL TERMINAL_DECISION D7CC=%d lane=default",
        s32(0x8009d7cc)))
    if s32(0x8009d7cc) ~= -1 then return fail("terminal.d7cc") end
end)

PCSX.addBreakpoint(0x8001996c, "Exec", 4, "ChangeGameState entry", function()
    if not terminalSeen then return end
    stateCalls = stateCalls + 1
    print(string.format("W34N31_RETAIL CHANGE_STATE call=%d a0=%d",
        stateCalls, reg("a0")))
    if stateCalls ~= 1 or reg("a0") ~= 0 then return fail("state") end
end)

PCSX.addBreakpoint(0x80044764, "Exec", 4, "ClearImage entry", function()
    if not terminalSeen then return end
    clearCalls = clearCalls + 1
    local rect = reg("a0")
    print(string.format(
        "W34N31_RETAIL CLEAR_IMAGE call=%d rect=%d,%d,%d,%d rgb=%d,%d,%d",
        clearCalls, u16(rect), u16(rect + 2), u16(rect + 4), u16(rect + 6),
        reg("a1"), reg("a2"), reg("a3")))
    if stateCalls ~= 1 or clearCalls ~= 1 or u16(rect) ~= 0 or
       u16(rect + 2) ~= 0 or u16(rect + 4) ~= 319 or
       u16(rect + 6) ~= 431 or reg("a1") ~= 0 or reg("a2") ~= 0 or
       reg("a3") ~= 64 then return fail("clear_image") end
end)

PCSX.addBreakpoint(0x800445d0, "Exec", 4, "DrawSync entry", function()
    if not terminalSeen then return end
    -- The common 0x800762FC helper performs its own DrawSync calls.  This
    -- profile certifies the direct default-lane call before that helper.
    if commonSeen then return end
    drawSyncCalls = drawSyncCalls + 1
    print(string.format("W34N31_RETAIL DRAW_SYNC call=%d a0=%d",
        drawSyncCalls, reg("a0")))
    if clearCalls ~= 1 or drawSyncCalls ~= 1 or reg("a0") ~= 0 then
        return fail("draw_sync")
    end
end)

PCSX.addBreakpoint(0x800712a0, "Exec", 4, "common epilogue", function()
    if not terminalSeen then return end
    commonSeen = true
    print(string.format("W34N31_RETAIL COMMON_EPILOGUE BYTE591AE_BEFORE=%02x",
        u8(0x800591ae)))
    if drawSyncCalls ~= 1 then return fail("common.order") end
end)

PCSX.addBreakpoint(0x800762fc, "Exec", 4, "sync entry", function()
    if not terminalSeen then return end
    syncCalls = syncCalls + 1
    print(string.format("W34N31_RETAIL SYNC_762FC call=%d BYTE591AE=%02x",
        syncCalls, u8(0x800591ae)))
    if not commonSeen or syncCalls ~= 1 or u8(0x800591ae) ~= 0 then
        return fail("sync")
    end
end)

PCSX.addBreakpoint(0x80019acc, "Exec", 4, "MainLoop entry", function()
    if not terminalSeen then return end
    print(string.format("W34N31_RETAIL MAIN_LOOP a0=%d frames=%d",
        reg("a0"), frame))
    if syncCalls ~= 1 or reg("a0") ~= 0 or frame ~= frameLimit then
        return fail("main_loop")
    end
    print(string.format(
        "W34N31_RETAIL PASS frames=%d slot1=%d slot2=%d default=1",
        frame, slot1Calls, slot2Calls))
    PCSX.quit(0)
end)

PCSX.Events.createEventListener("ExecutionFlow::SaveStateLoaded", function()
    loaded = true
    print("W34N31_RETAIL STATE_LOADED")
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

print("W34N31_RETAIL READY")
