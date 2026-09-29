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
local function s16(addr) return tonumber(ffi.cast("int16_t*", ptr(addr))[0]) end
local function u32(addr) return tonumber(ffi.cast("uint32_t*", ptr(addr))[0]) end
local function s32(addr) return tonumber(ffi.cast("int32_t*", ptr(addr))[0]) end
local function w32(addr, value) ffi.cast("uint32_t*", ptr(addr))[0] = value end
local function reg(name) return tonumber(PCSX.getRegisters().GPR.n[name]) end

local statePath = os.getenv("XENO_RETAIL_WORLD_STATE")
local frameLimit = tonumber(os.getenv("XENO_RETAIL_TRACE_FRAMES") or "5")
local loaded = false
local frame = 0
local branchCount = 0
local d554Seeded = false
local d7ccSeeded = false
local slot1Calls = 0
local slot2Calls = 0
local driverExitCalls = 0
local terminalDecisionSeen = false
local overlayCalls = 0
local stateCalls = 0
local guardSeen = false
local guardBbc4 = 0
local guardType = 0
local helperCalls = 0
local helperReturnSeen = false
local publicationsSeen = false
local ef68Seen = false
local commonSeen = false
local syncCalls = 0

local function fail(message)
    print("W34N27_RETAIL FAIL " .. message)
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
    local target = reg("v0")
    print(string.format("W34N27_RETAIL SLOT1_CALL target=%08x", target))
    if slot1Calls ~= 1 then return fail("slot1.count") end
    if target ~= 0x80072238 then return fail("slot1.target") end
end)

PCSX.addBreakpoint(0x8007130c, "Exec", 4, "frame head", function()
    if not loaded then return end
    frame = frame + 1
    local d554 = s32(0x8009d554)
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N27_RETAIL FRAME_HEAD frame=%d D554=%d D7CC=%d",
        frame, d554, d7cc))
    if frame ~= branchCount + 1 then return fail("frame_head.count") end
    if d554 ~= 1 then return fail("frame_head.d554") end
    if d7cc ~= 2 then return fail("frame_head.d7cc") end
end)

-- Seed one driver exit at retail's own D554 latch load.  This is identical to
-- W34N24 and leaves the driver exit and slot-2 teardown unmodified.
PCSX.addBreakpoint(0x800719c0, "Exec", 4, "frame latch load", function()
    if not loaded or frame ~= frameLimit or d554Seeded then return end
    local before = s32(0x8009d554)
    print(string.format(
        "W34N27_RETAIL SEED_D554 frame=%d before=%d after=0",
        frame, before))
    if before ~= 1 then return fail("seed_d554.before") end
    w32(0x8009d554, 0)
    d554Seeded = true
end)

PCSX.addBreakpoint(0x800719c8, "Exec", 4, "frame branch", function()
    if not loaded then return end
    branchCount = branchCount + 1
    local d554 = s32(0x8009d554)
    local taken = d554 ~= 0 and 1 or 0
    print(string.format("W34N27_RETAIL FRAME_BRANCH frame=%d D554=%d taken=%d",
        frame, d554, taken))
    if branchCount ~= frame then return fail("frame_branch.count") end
    if frame < frameLimit and taken ~= 1 then
        return fail("frame_branch.early_exit")
    end
    if frame == frameLimit and taken ~= 0 then
        return fail("frame_branch.seeded_exit")
    end
end)

PCSX.addBreakpoint(0x800719d0, "Exec", 4, "driver exit", function()
    if not loaded then return end
    driverExitCalls = driverExitCalls + 1
    print(string.format("W34N27_RETAIL DRIVER_EXIT D554=%d D7CC=%d",
        s32(0x8009d554), s32(0x8009d7cc)))
    if not d554Seeded then return fail("driver_exit.unseeded") end
    if driverExitCalls ~= 1 then return fail("driver_exit.count") end
end)

PCSX.addBreakpoint(0x800710c4, "Exec", 4, "slot2 call", function()
    if not loaded then return end
    slot2Calls = slot2Calls + 1
    local target = reg("v0")
    print(string.format("W34N27_RETAIL SLOT2_CALL target=%08x D7CC=%d",
        target, s32(0x8009d7cc)))
    if driverExitCalls ~= 1 then return fail("slot2.before_driver_exit") end
    if slot2Calls ~= 1 then return fail("slot2.count") end
    if target ~= 0x8007299c then return fail("slot2.target") end
end)

-- Additional terminal-state seed: immediately after the retail frame driver
-- returns and before retail selects/calls slot 2.  A natural D7CC==0 exit
-- would present this value to teardown.  The replay has BBC4=0 with no current
-- region (D7D8=0xFFFFFFFF), an impossible combination for the zero lane, so
-- BBC4 is disclosedly seeded nonzero to exercise retail's guarded skip path.
PCSX.addBreakpoint(0x8007109c, "Exec", 4, "terminal state seed", function()
    if not loaded then return end
    local before = s32(0x8009d7cc)
    local d7d8 = u32(0x8009d7d8)
    local bbc4 = s32(0x8009bbc4)
    print(string.format(
        "W34N27_RETAIL SEED_TERMINAL D7CC_BEFORE=%d D7CC_AFTER=0 D7D8=%08x BBC4_BEFORE=%d BBC4_AFTER=1",
        before, d7d8, bbc4))
    if driverExitCalls ~= 1 then return fail("seed_d7cc.before_driver_exit") end
    if slot2Calls ~= 0 then return fail("seed_d7cc.after_slot2") end
    if before ~= 2 then return fail("seed_d7cc.before") end
    w32(0x8009d7cc, 0)
    w32(0x8009bbc4, 1)
    d7ccSeeded = true
end)

PCSX.addBreakpoint(0x800710cc, "Exec", 4, "slot2 return", function()
    if not loaded then return end
    print(string.format("W34N27_RETAIL SLOT2_RETURN D7CC=%d D7D8=%08x",
        s32(0x8009d7cc), u32(0x8009d7d8)))
    if not d7ccSeeded then return fail("slot2_return.unseeded") end
    if slot2Calls ~= 1 then return fail("slot2_return.without_call") end
end)

PCSX.addBreakpoint(0x800710e4, "Exec", 4, "terminal decision", function()
    if not loaded then return end
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N27_RETAIL TERMINAL_DECISION D7CC=%d lane=zero", d7cc))
    if not d7ccSeeded then return fail("terminal.unseeded") end
    if d7cc ~= 0 then return fail("terminal.d7cc") end
    terminalDecisionSeen = true
end)

PCSX.addBreakpoint(0x800199cc, "Exec", 4, "LoadGameStateOverlay entry", function()
    if not terminalDecisionSeen then return end
    overlayCalls = overlayCalls + 1
    print(string.format("W34N27_RETAIL LOAD_OVERLAY call=%d a0=%d",
        overlayCalls, reg("a0")))
    if overlayCalls ~= 1 then return fail("overlay.count") end
    if reg("a0") ~= 1 then return fail("overlay.argument") end
end)

PCSX.addBreakpoint(0x8001996c, "Exec", 4, "ChangeGameState entry", function()
    if not terminalDecisionSeen then return end
    stateCalls = stateCalls + 1
    print(string.format("W34N27_RETAIL CHANGE_STATE call=%d a0=%d",
        stateCalls, reg("a0")))
    if stateCalls ~= 1 then return fail("change_state.count") end
    if reg("a0") ~= 1 then return fail("change_state.argument") end
end)

PCSX.addBreakpoint(0x80071124, "Exec", 4, "terminal region guard", function()
    if not terminalDecisionSeen then return end
    local bbc4 = s32(0x8009bbc4)
    local d7d8 = u32(0x8009d7d8)
    local recordType
    if bbc4 ~= 0 then
        guardSeen = true
        guardBbc4 = bbc4
        print(string.format(
            "W34N27_RETAIL REGION_GUARD BBC4=%d D7D8=%08x TYPE=SKIPPED helper_expected=0",
            bbc4, d7d8))
        if overlayCalls ~= 1 then return fail("guard.before_overlay") end
        if stateCalls ~= 1 then return fail("guard.before_state") end
        return
    end
    if d7d8 < 0x80000000 or d7d8 >= 0x80200000 then
        print(string.format(
            "W34N27_RETAIL REGION_GUARD BBC4=%d D7D8=%08x TYPE=INVALID helper_expected=0",
            bbc4, d7d8))
        return fail("guard.d7d8_domain")
    end
    recordType = s16(d7d8 + 0x0e)
    guardSeen = true
    guardBbc4 = bbc4
    guardType = recordType
    print(string.format(
        "W34N27_RETAIL REGION_GUARD BBC4=%d D7D8=%08x TYPE=%d helper_expected=%d",
        bbc4, d7d8, recordType,
        (bbc4 == 0 and recordType == 3) and 1 or 0))
    if overlayCalls ~= 1 then return fail("guard.before_overlay") end
    if stateCalls ~= 1 then return fail("guard.before_state") end
end)

PCSX.addBreakpoint(0x80094364, "Exec", 4, "typed region helper entry", function()
    if not terminalDecisionSeen then return end
    helperCalls = helperCalls + 1
    print(string.format(
        "W34N27_RETAIL REGION_HELPER call=%d a0=%08x a1=%d a2=%d D7D8_BEFORE=%08x",
        helperCalls, reg("a0"), reg("a1"), reg("a2"), u32(0x8009d7d8)))
    if not guardSeen then return fail("helper.before_guard") end
    if guardBbc4 ~= 0 or guardType ~= 3 then
        return fail("helper.guard_mismatch")
    end
    if reg("a0") ~= 0x8009d55c then return fail("helper.a0") end
    if reg("a1") ~= 3 then return fail("helper.a1") end
    if reg("a2") ~= u16(0x8006ef64) then return fail("helper.a2") end
end)

PCSX.addBreakpoint(0x80071160, "Exec", 4, "typed region helper return seam", function()
    if not terminalDecisionSeen then return end
    helperReturnSeen = true
    print(string.format(
        "W34N27_RETAIL REGION_AFTER helper_calls=%d D7D8_AFTER=%08x",
        helperCalls, u32(0x8009d7d8)))
    local expected = (guardBbc4 == 0 and guardType == 3) and 1 or 0
    if helperCalls ~= expected then return fail("helper.expected_count") end
end)

PCSX.addBreakpoint(0x80071190, "Exec", 4, "terminal publications complete", function()
    if not terminalDecisionSeen then return end
    publicationsSeen = true
    print(string.format(
        "W34N27_RETAIL REGION_OUTPUTS writes=%d F950=%04x F94E=%04x F954=%04x",
        guardBbc4 == 0 and 1 or 0,
        u16(0x8006f950), u16(0x8006f94e), u16(0x8006f954)))
    if not guardSeen then return fail("publications.before_guard") end
    if guardBbc4 == 0 and not helperReturnSeen then
        return fail("publications.before_region_seam")
    end
end)

PCSX.addBreakpoint(0x800711a8, "Exec", 4, "terminal ef68 complete", function()
    if not terminalDecisionSeen then return end
    local bd0c = u32(0x8009bd0c)
    local expected = bit.band(bd0c + 0x400, 0xffff)
    local actual = u16(0x8006ef68)
    ef68Seen = true
    print(string.format(
        "W34N27_RETAIL EF68 BD0C=%08x expected=%04x actual=%04x",
        bd0c, expected, actual))
    if actual ~= expected then return fail("ef68.value") end
end)

PCSX.addBreakpoint(0x800712a0, "Exec", 4, "common terminal epilogue", function()
    if not terminalDecisionSeen then return end
    commonSeen = true
    print(string.format("W34N27_RETAIL COMMON_EPILOGUE BYTE591AE_BEFORE=%02x",
        u8(0x800591ae)))
    if not publicationsSeen then return fail("common.before_publications") end
    if not ef68Seen then return fail("common.before_ef68") end
end)

PCSX.addBreakpoint(0x800762fc, "Exec", 4, "terminal sync helper entry", function()
    if not terminalDecisionSeen then return end
    syncCalls = syncCalls + 1
    print(string.format("W34N27_RETAIL SYNC_762FC call=%d BYTE591AE=%02x",
        syncCalls, u8(0x800591ae)))
    if not commonSeen then return fail("sync.before_common") end
    if syncCalls ~= 1 then return fail("sync.count") end
    if u8(0x800591ae) ~= 0 then return fail("sync.byte591ae") end
end)

PCSX.addBreakpoint(0x80019acc, "Exec", 4, "MainLoop entry", function()
    if not terminalDecisionSeen then return end
    local errorCode = reg("a0")
    print(string.format(
        "W34N27_RETAIL MAIN_LOOP a0=%d BYTE591AE=%02x frames=%d helper_calls=%d",
        errorCode, u8(0x800591ae), frame, helperCalls))
    if syncCalls ~= 1 then return fail("main_loop.before_sync") end
    if errorCode ~= 0 then return fail("main_loop.argument") end
    if slot1Calls ~= 1 then return fail("summary.slot1") end
    if slot2Calls ~= 1 then return fail("summary.slot2") end
    if frame ~= frameLimit then return fail("summary.frames") end
    print(string.format(
        "W34N27_RETAIL PASS frames=%d slot1=%d slot2=%d terminal=0 helper_calls=%d",
        frame, slot1Calls, slot2Calls, helperCalls))
    PCSX.quit(0)
end)

PCSX.Events.createEventListener("ExecutionFlow::SaveStateLoaded", function()
    loaded = true
    print("W34N27_RETAIL STATE_LOADED")
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

print("W34N27_RETAIL READY")
