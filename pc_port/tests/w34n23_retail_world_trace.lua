local ffi = require("ffi")
local mem = PCSX.getMemPtr()

local function off(addr) return bit.band(addr, 0x001fffff) end
local function ptr(addr) return mem + off(addr) end
local function s32(addr) return tonumber(ffi.cast("int32_t*", ptr(addr))[0]) end
local function reg(name) return tonumber(PCSX.getRegisters().GPR.n[name]) end

local statePath = os.getenv("XENO_RETAIL_WORLD_STATE")
local frameLimit = tonumber(os.getenv("XENO_RETAIL_TRACE_FRAMES") or "5")
local loaded = false
local sequence = 0
local slot1Calls = 0
local driverCalls = 0
local slot2Calls = 0
local frameHeads = 0
local frameBranches = 0

local function fail(message)
    print("W34N23_RETAIL FAIL " .. message)
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
    print(string.format("W34N23_RETAIL SLOT1_CALL target=%08x", target))
    if sequence ~= 0 then return fail("slot1.sequence") end
    if slot1Calls ~= 1 then return fail("slot1.count") end
    if target ~= 0x80072238 then return fail("slot1.target") end
    sequence = 1
end)

PCSX.addBreakpoint(0x80071064, "Exec", 4, "slot1 return", function()
    if not loaded then return end
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N23_RETAIL SLOT1_RETURN D7CC=%d", d7cc))
    if sequence ~= 1 then return fail("slot1_return.sequence") end
    if d7cc ~= 2 then return fail("slot1_return.d7cc") end
    sequence = 2
end)

PCSX.addBreakpoint(0x800712d0, "Exec", 4, "driver entry", function()
    if not loaded then return end
    driverCalls = driverCalls + 1
    local d554 = s32(0x8009d554)
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N23_RETAIL DRIVER_ENTRY D554=%d D7CC=%d",
        d554, d7cc))
    if sequence ~= 2 then return fail("driver.sequence") end
    if driverCalls ~= 1 then return fail("driver.count") end
    if d554 ~= 0 then return fail("driver.d554") end
    if d7cc ~= 2 then return fail("driver.d7cc") end
    sequence = 3
end)

PCSX.addBreakpoint(0x8007130c, "Exec", 4, "frame head", function()
    if not loaded then return end
    frameHeads = frameHeads + 1
    local d554 = s32(0x8009d554)
    local d7cc = s32(0x8009d7cc)
    print(string.format("W34N23_RETAIL FRAME_HEAD frame=%d D554=%d D7CC=%d",
        frameHeads, d554, d7cc))
    if sequence ~= 3 then return fail("frame_head.sequence") end
    if frameHeads ~= frameBranches + 1 then return fail("frame_head.count") end
    if d554 ~= 1 then return fail("frame_head.d554") end
    if d7cc ~= 2 then return fail("frame_head.d7cc") end
    sequence = 4
end)

PCSX.addBreakpoint(0x800719c8, "Exec", 4, "frame branch", function()
    if not loaded then return end
    frameBranches = frameBranches + 1
    local d554 = s32(0x8009d554)
    local taken = d554 ~= 0 and 1 or 0
    print(string.format("W34N23_RETAIL FRAME_BRANCH frame=%d D554=%d taken=%d",
        frameHeads, d554, taken))
    if sequence ~= 4 then return fail("frame_branch.sequence") end
    if frameBranches ~= frameHeads then return fail("frame_branch.count") end
    if taken ~= 1 then return fail("frame_branch.taken") end
    sequence = 3
    if frameBranches >= frameLimit then
        if slot1Calls ~= 1 then return fail("summary.slot1") end
        if driverCalls ~= 1 then return fail("summary.driver") end
        if slot2Calls ~= 0 then return fail("summary.slot2") end
        print(string.format(
            "W34N23_RETAIL PASS frames=%d slot1=%d driver=%d slot2=%d",
            frameBranches, slot1Calls, driverCalls, slot2Calls))
        PCSX.quit(0)
    end
end)

PCSX.addBreakpoint(0x800710c4, "Exec", 4, "shared slot0/slot2 call", function()
    if not loaded then return end
    slot2Calls = slot2Calls + 1
    print(string.format("W34N23_RETAIL SLOT2_OR_SHARED_CALL count=%d",
        slot2Calls))
    fail("unexpected.slot2_or_shared_call")
end)

PCSX.Events.createEventListener("ExecutionFlow::SaveStateLoaded", function()
    loaded = true
    print("W34N23_RETAIL STATE_LOADED")
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

print("W34N23_RETAIL READY")
