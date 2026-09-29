local ffi = require("ffi")
local mem = PCSX.getMemPtr()

local function off(addr) return bit.band(addr, 0x001fffff) end
local function ptr(addr) return mem + off(addr) end
local function s32(addr) return tonumber(ffi.cast("int32_t*", ptr(addr))[0]) end
local function w32(addr, value) ffi.cast("uint32_t*", ptr(addr))[0] = value end
local function reg(name) return tonumber(PCSX.getRegisters().GPR.n[name]) end

local statePath = os.getenv("XENO_RETAIL_WORLD_STATE")
local frameLimit = tonumber(os.getenv("XENO_RETAIL_TRACE_FRAMES") or "5")
local loaded = false
local frame = 0
local branchCount = 0
local seeded = false
local slot1Calls = 0
local slot2Calls = 0
local driverExitCalls = 0
local slot2Returned = false
local decisionSeen = false

local function fail(message)
    print("W34N24_RETAIL FAIL " .. message)
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
    print(string.format("W34N24_RETAIL SLOT1_CALL target=%08x", target))
    if slot1Calls ~= 1 then return fail("slot1.count") end
    if target ~= 0x80072238 then return fail("slot1.target") end
end)

PCSX.addBreakpoint(0x8007130c, "Exec", 4, "frame head", function()
    if not loaded then return end
    frame = frame + 1
    local d554 = s32(0x8009d554)
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N24_RETAIL FRAME_HEAD frame=%d D554=%d D7CC=%d",
        frame, d554, d7cc))
    if frame ~= branchCount + 1 then return fail("frame_head.count") end
    if d554 ~= 1 then return fail("frame_head.d554") end
    if d7cc ~= 2 then return fail("frame_head.d7cc") end
end)

-- This is the sole disclosed seed. It executes immediately before retail's
-- own `lw D_8009D554` at 0x800719C0, so the retail branch, driver epilogue,
-- slot-2 dispatch, and session decision remain unmodified.
PCSX.addBreakpoint(0x800719c0, "Exec", 4, "frame latch load", function()
    if not loaded or frame ~= frameLimit or seeded then return end
    local before = s32(0x8009d554)
    print(string.format(
        "W34N24_RETAIL SEED_D554 frame=%d before=%d after=0",
        frame, before))
    if before ~= 1 then return fail("seed.before") end
    w32(0x8009d554, 0)
    seeded = true
end)

PCSX.addBreakpoint(0x800719c8, "Exec", 4, "frame branch", function()
    if not loaded then return end
    branchCount = branchCount + 1
    local d554 = s32(0x8009d554)
    local taken = d554 ~= 0 and 1 or 0
    print(string.format("W34N24_RETAIL FRAME_BRANCH frame=%d D554=%d taken=%d",
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
    local d554 = s32(0x8009d554)
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N24_RETAIL DRIVER_EXIT D554=%d D7CC=%d",
        d554, d7cc))
    if not seeded then return fail("driver_exit.unseeded") end
    if driverExitCalls ~= 1 then return fail("driver_exit.count") end
    if d554 ~= 0 then return fail("driver_exit.d554") end
    if d7cc ~= 2 then return fail("driver_exit.d7cc") end
end)

PCSX.addBreakpoint(0x800710c4, "Exec", 4, "slot2 call", function()
    if not loaded then return end
    slot2Calls = slot2Calls + 1
    local target = reg("v0")
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N24_RETAIL SLOT2_CALL target=%08x D7CC=%d",
        target, d7cc))
    if driverExitCalls ~= 1 then return fail("slot2.before_driver_exit") end
    if slot2Calls ~= 1 then return fail("slot2.count") end
    if target ~= 0x8007299c then return fail("slot2.target") end
    if d7cc ~= 2 then return fail("slot2.d7cc") end
end)

PCSX.addBreakpoint(0x800710cc, "Exec", 4, "slot2 return", function()
    if not loaded then return end
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N24_RETAIL SLOT2_RETURN D7CC=%d", d7cc))
    if slot2Calls ~= 1 then return fail("slot2_return.without_call") end
    if d7cc ~= 2 then return fail("slot2_return.d7cc") end
    slot2Returned = true
end)

PCSX.addBreakpoint(0x800710dc, "Exec", 4, "session decision", function()
    if not loaded then return end
    local d7cc = s32(0x8009d7cc)
    local repeatSession = d7cc >= 2 and 1 or 0
    print(string.format("W34N24_RETAIL SESSION_DECISION D7CC=%d repeat=%d",
        d7cc, repeatSession))
    if not slot2Returned then return fail("decision.before_slot2_return") end
    if d7cc ~= 2 then return fail("decision.d7cc") end
    if repeatSession ~= 1 then return fail("decision.repeat") end
    decisionSeen = true
end)

PCSX.addBreakpoint(0x80071034, "Exec", 4, "next session", function()
    if not loaded or not seeded then return end
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N24_RETAIL NEXT_SESSION_HEAD D7CC=%d", d7cc))
    if not decisionSeen then return fail("next_session.before_decision") end
    if slot1Calls ~= 1 then return fail("summary.slot1") end
    if slot2Calls ~= 1 then return fail("summary.slot2") end
    if d7cc ~= 2 then return fail("next_session.d7cc") end
    print(string.format(
        "W34N24_RETAIL PASS frames=%d slot1=%d slot2=%d repeat=1",
        frame, slot1Calls, slot2Calls))
    PCSX.quit(0)
end)

PCSX.Events.createEventListener("ExecutionFlow::SaveStateLoaded", function()
    loaded = true
    print("W34N24_RETAIL STATE_LOADED")
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

print("W34N24_RETAIL READY")
