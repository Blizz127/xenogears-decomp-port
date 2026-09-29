# W18I strict forbidden-target instrumentation for world-map GDB harnesses.
#
# Rule: a forbidden target is proven absent only when its instrumentation was
# successfully registered and remained active for the entire measured
# interval, and its measured hit count is zero.
#
# Every target ends in exactly one status:
#   ZERO_VERIFIED         registered, active throughout, hits == 0
#   HIT                   registered, hits > 0 (or expectation violated)
#   NOT_INSTRUMENTED      registration failed / symbol absent / no method
#   INSTRUMENTATION_ERROR registered but lost, disabled, deleted, or
#                         counter state became inconsistent mid-interval
#
# Targets are never initialized to a numeric zero; hit_count is undefined
# until registration is proven.

import gdb

STATUS_UNATTEMPTED = "UNATTEMPTED"
STATUS_REGISTERED = "REGISTERED"
STATUS_REG_FAILED = "REGISTRATION_FAILED"

FINAL_ZERO_VERIFIED = "ZERO_VERIFIED"
FINAL_HIT = "HIT"
FINAL_NOT_INSTRUMENTED = "NOT_INSTRUMENTED"
FINAL_INSTRUMENTATION_ERROR = "INSTRUMENTATION_ERROR"

METHOD_SYMBOL_BP = "native_symbol_bp"
METHOD_COUNTER = "native_route_counter"
METHOD_ATTR_BP = "shared_symbol_attr_bp"
METHOD_WINDOW = "gdb_window_counter"
METHOD_NONE = "none"


def _sanitize(text):
    out = str(text).replace("=", ":")
    out = "_".join(out.split())
    return out or "none"


class Target(object):
    def __init__(self, label, method, symbol=None, required=True,
                 expect="zero", reason_declared=None, bp_factory=None):
        self.label = label
        self.method = method
        self.symbol = symbol
        self.required = required
        # expect: "zero" | "hit_exact:N" | "hit_min:N"
        self.expect = expect
        self.reason_declared = reason_declared
        # optional gdb.Breakpoint subclass factory: bp_factory(target)
        self.bp_factory = bp_factory
        # registration state
        self.registration_status = STATUS_UNATTEMPTED
        self.registration_error = None
        self.bp = None
        self.bp_number = None
        self.bp_enabled = None
        self.bp_resolved = None
        self.counter_addr = None
        self.baseline = None
        # interval state
        self.hit_count = None          # undefined until registered
        self.allowed_count = 0         # attribution-classified allowed hits
        self.lost_reason = None
        self.final_status = None

    def expect_desc(self):
        return self.expect


class ForbiddenInstrumentation(object):
    """Manifest + registration proof + interval bookkeeping + strict summary."""

    def __init__(self, harness_tag):
        self.tag = harness_tag
        self.targets = []
        self.by_label = {}
        self.interval_started = False
        self.interval_verified = False

    # ---------------- manifest construction ----------------

    def _add(self, target):
        if target.label in self.by_label:
            raise ValueError("duplicate target label: %s" % target.label)
        self.targets.append(target)
        self.by_label[target.label] = target
        return target

    def symbol_target(self, label, symbol, required=True, expect="zero",
                      bp_factory=None):
        """Breakpoint on a native symbol (stub or real function)."""
        return self._add(Target(label, METHOD_SYMBOL_BP, symbol=symbol,
                                required=required, expect=expect,
                                bp_factory=bp_factory))

    def counter_target(self, label, required=True, expect="zero",
                       counter_expr=None):
        """Native diagnostic counter read through the exported registry
        (default) or through an explicit counter symbol expression."""
        return self._add(Target(label, METHOD_COUNTER, symbol=counter_expr,
                                required=required, expect=expect))

    def attribution_target(self, label, symbol, required=True, expect="zero",
                           bp_factory=None):
        """Breakpoint on a shared native function; each hit is classified by
        backtrace attribution into forbidden (world-route) vs allowed."""
        return self._add(Target(label, METHOD_ATTR_BP, symbol=symbol,
                                required=required, expect=expect,
                                bp_factory=bp_factory))

    def window_target(self, label, required=True, expect="zero"):
        """Harness-side accumulator gated by a window the harness itself
        delimits (registration proof = the window delimiters)."""
        return self._add(Target(label, METHOD_WINDOW, required=required,
                                expect=expect))

    def uninstrumented_target(self, label, reason, required=True):
        """Declared target with no registration method. Always ends
        NOT_INSTRUMENTED; required ones fail the run."""
        return self._add(Target(label, METHOD_NONE, required=required,
                                reason_declared=reason))

    # ---------------- registration (pre-run) ----------------

    def register_all(self):
        for t in self.targets:
            if t.method == METHOD_SYMBOL_BP or t.method == METHOD_ATTR_BP:
                self._register_bp(t)
            elif t.method == METHOD_COUNTER:
                self._register_counter(t)
            elif t.method == METHOD_WINDOW:
                t.registration_status = STATUS_REGISTERED
                t.hit_count = 0
            elif t.method == METHOD_NONE:
                t.registration_status = STATUS_REG_FAILED
                t.registration_error = t.reason_declared or "no method"
            else:
                t.registration_status = STATUS_REG_FAILED
                t.registration_error = "unknown method %s" % t.method
        self._print_manifest_note()

    def _register_bp(self, t):
        # Pending breakpoints are not accepted as registration proof; the
        # harness must run with `set breakpoint pending off`.
        try:
            if t.bp_factory is not None:
                t.bp = t.bp_factory(t)
            else:
                t.bp = gdb.Breakpoint(t.symbol, internal=True)
        except Exception as exc:
            t.registration_status = STATUS_REG_FAILED
            t.registration_error = str(exc)
            return
        t.bp_number = t.bp.number
        t.bp_enabled = bool(t.bp.enabled)
        t.registration_status = STATUS_REGISTERED
        # Resolution (locations) is confirmed at the interval start once the
        # inferior is live; a still-pending required breakpoint fails there.

    def _register_counter(self, t):
        # Resolve the counter address symbolically before the run; the first
        # actual memory read happens at interval start.
        try:
            if t.symbol:
                expr = "&(%s)" % t.symbol
            else:
                expr = self._registry_counter_expr(t.label)
                if expr is None:
                    t.registration_status = STATUS_REG_FAILED
                    t.registration_error = (
                        "label %s absent from g_wm_forbidden_targets" % t.label)
                    return
            value = gdb.parse_and_eval(expr)
            t.counter_addr = int(value.cast(gdb.lookup_type("uintptr_t")))
        except Exception as exc:
            t.registration_status = STATUS_REG_FAILED
            t.registration_error = str(exc)
            return
        t.registration_status = STATUS_REGISTERED

    def _registry_counter_expr(self, label):
        try:
            count = int(gdb.parse_and_eval("g_wm_forbidden_target_count"))
        except Exception:
            return None
        for i in range(count):
            try:
                entry = gdb.parse_and_eval("g_wm_forbidden_targets[%d]" % i)
                if entry["label"].string() == label:
                    # .hits is an int*; its value is the counter address.
                    return "g_wm_forbidden_targets[%d].hits" % i
            except Exception:
                return None
        return None

    def _print_manifest_note(self):
        for t in self.targets:
            extra = ""
            if t.bp_number is not None:
                extra = " bp_number=%d bp_enabled=%d" % (
                    t.bp_number, int(bool(t.bp_enabled)))
            if t.counter_addr is not None:
                extra = " counter_addr=0x%x" % t.counter_addr
            print("%s_REG label=%s method=%s required=%d status=%s "
                  "symbol=%s%s" %
                  (self.tag, t.label, t.method, int(t.required),
                   t.registration_status, t.symbol or "-", extra))

    # ---------------- interval bookkeeping ----------------

    def _read_counter(self, t):
        if t.counter_addr is None:
            raise RuntimeError("counter address unresolved")
        mem = gdb.selected_inferior().read_memory(t.counter_addr, 4)
        return int.from_bytes(bytes(mem), "little")

    def start_interval(self):
        """Call once after the inferior is live, before the measured window.
        Completes registration proof: breakpoints must be resolved and
        enabled; counters must be readable (baseline captured)."""
        for t in self.targets:
            if t.final_status is not None:
                continue
            if t.registration_status == STATUS_REG_FAILED:
                continue
            if t.method in (METHOD_SYMBOL_BP, METHOD_ATTR_BP):
                try:
                    locs = list(t.bp.locations)
                    enabled = bool(t.bp.enabled)
                except Exception as exc:
                    self._fail_registration(t, "bp_state_error", exc)
                    continue
                if not enabled:
                    self._fail_registration(t, "bp_disabled_before_interval")
                    continue
                if not locs:
                    self._fail_registration(t, "bp_pending_unresolved")
                    continue
                t.bp_resolved = True
                t.bp_enabled = True
                t.hit_count = 0 if t.hit_count is None else t.hit_count
            elif t.method == METHOD_COUNTER:
                try:
                    t.baseline = self._read_counter(t)
                    t.hit_count = 0
                except Exception as exc:
                    self._fail_registration(t, "counter_read_failed", exc)
                    continue
        self.interval_started = True
        print("%s_INTERVAL_START targets=%d registered=%d failed=%d" %
              (self.tag, len(self.targets),
               sum(1 for t in self.targets
                   if t.registration_status == STATUS_REGISTERED),
               sum(1 for t in self.targets
                   if t.registration_status == STATUS_REG_FAILED)))

    def _fail_registration(self, t, reason, exc=None):
        t.registration_status = STATUS_REG_FAILED
        if exc is not None:
            reason = "%s:%s" % (reason, _sanitize(exc))
        t.registration_error = reason

    def check_interval_health(self):
        """Cheap per-vsync liveness probe: detects deleted/disabled
        instrumentation during the interval."""
        if not self.interval_started:
            return
        for t in self.targets:
            if t.final_status is not None or t.lost_reason is not None:
                continue
            if t.registration_status != STATUS_REGISTERED:
                # Already failed registration with a more precise reason.
                continue
            if t.method in (METHOD_SYMBOL_BP, METHOD_ATTR_BP):
                try:
                    _ = t.bp.number
                    enabled = bool(t.bp.enabled)
                except Exception:
                    t.lost_reason = "bp_deleted_during_interval"
                    continue
                if not enabled:
                    t.lost_reason = "bp_disabled_during_interval"

    def record_hit(self, label, allowed=False):
        t = self.by_label.get(label)
        if t is None:
            return
        if allowed:
            t.allowed_count += 1
            return
        if t.hit_count is None:
            t.hit_count = 0
        t.hit_count += 1

    def end_interval(self):
        self.check_interval_health()
        for t in self.targets:
            if t.lost_reason is not None:
                t.final_status = FINAL_INSTRUMENTATION_ERROR
                t.registration_error = t.lost_reason
                continue
            if t.registration_status == STATUS_REG_FAILED:
                t.final_status = FINAL_NOT_INSTRUMENTED
                continue
            if t.method == METHOD_COUNTER:
                try:
                    final = self._read_counter(t)
                except Exception as exc:
                    t.final_status = FINAL_INSTRUMENTATION_ERROR
                    t.registration_error = "counter_reread_failed:%s" % (
                        _sanitize(exc))
                    continue
                if final < t.baseline:
                    t.final_status = FINAL_INSTRUMENTATION_ERROR
                    t.registration_error = (
                        "counter_inconsistent final=%d baseline=%d" %
                        (final, t.baseline))
                    continue
                t.hit_count = final - t.baseline
            if t.hit_count is None:
                t.final_status = FINAL_INSTRUMENTATION_ERROR
                t.registration_error = t.registration_error or "hits_undefined"
                continue
            t.final_status = self._classify(t)
        self.interval_verified = True

    def _classify(self, t):
        hits = t.hit_count
        if t.expect == "zero":
            return FINAL_ZERO_VERIFIED if hits == 0 else FINAL_HIT
        if t.expect.startswith("hit_exact:"):
            want = int(t.expect.split(":", 1)[1])
            return FINAL_HIT if hits == want else FINAL_INSTRUMENTATION_ERROR
        if t.expect.startswith("hit_min:"):
            want = int(t.expect.split(":", 1)[1])
            return FINAL_HIT if hits >= want else FINAL_INSTRUMENTATION_ERROR
        return FINAL_INSTRUMENTATION_ERROR

    # ---------------- reporting ----------------

    def emit_summary(self):
        if not self.interval_verified:
            self.end_interval()
        required_labels = [t.label for t in self.targets if t.required]
        print("%s_FORBIDDEN_SUMMARY_BEGIN" % self.tag)
        print("required=%s" % (",".join(required_labels) or "-"))
        for t in self.targets:
            hits = "undefined" if t.hit_count is None else str(t.hit_count)
            registered = int(t.registration_status == STATUS_REGISTERED)
            reason = "none"
            if t.registration_error:
                reason = _sanitize(t.registration_error)
            elif t.final_status == FINAL_NOT_INSTRUMENTED:
                reason = "no_registration_method"
            if t.method == METHOD_ATTR_BP:
                allowed_note = " allowed=%d" % t.allowed_count
            else:
                allowed_note = ""
            print("target label=%s status=%s registered=%d hits=%s "
                  "method=%s reason=%s%s" %
                  (t.label, t.final_status, registered, hits, t.method,
                   reason, allowed_note))
        print("%s_FORBIDDEN_SUMMARY_END" % self.tag)

    def verdict(self):
        """Returns (exit_code, verdict_string). Exit 0 only when every
        required target is ZERO_VERIFIED and every optional target avoided
        INSTRUMENTATION_ERROR."""
        if not self.interval_verified:
            self.end_interval()
        problems = []
        zero_verified = hit = not_inst = inst_err = 0
        for t in self.targets:
            if t.final_status == FINAL_ZERO_VERIFIED:
                zero_verified += 1
            elif t.final_status == FINAL_HIT:
                hit += 1
            elif t.final_status == FINAL_NOT_INSTRUMENTED:
                not_inst += 1
            else:
                inst_err += 1
            ok = t.final_status in (FINAL_ZERO_VERIFIED, FINAL_HIT)
            if t.required and t.final_status != FINAL_ZERO_VERIFIED:
                # Required forbidden targets pass only on ZERO VERIFIED;
                # positive expectations (hit_exact/hit_min) must also match.
                if t.expect != "zero" and t.final_status == FINAL_HIT:
                    continue
                problems.append("%s=%s" % (t.label, t.final_status))
            elif not t.required and t.final_status == FINAL_INSTRUMENTATION_ERROR:
                problems.append("%s=%s" % (t.label, t.final_status))
        exit_code = 1 if problems else 0
        verdict = "PASS" if exit_code == 0 else "FAIL"
        print("%s_FORBIDDEN_VERDICT verdict=%s exit=%d required_problems=%s "
              "zero_verified=%d hit=%d not_instrumented=%d "
              "instrumentation_error=%d" %
              (self.tag, verdict, exit_code,
               ",".join(problems) if problems else "none",
               zero_verified, hit, not_inst, inst_err))
        return exit_code, verdict


# ---------------- caller attribution for shared functions ----------------

# World update/draw family: any ArchiveCdDataSync or DrawOTag call whose
# backtrace contains one of these frames is a world-route residual hit.
# Placeholder presentation (PcPort_WorldMapPlaceholderMain) and field code
# are deliberately NOT world residuals (W18E3R attribution evidence).
WORLD_RESIDUAL_CALLERS = frozenset((
    "wm_800712D0_should_not_run",
    "wm_80072238_should_not_run",
    "wm_8007299C_should_not_run",
    "wm_80074E58_should_not_run",
    "wm_80075030_should_not_run",
    "wm_800739B8_should_not_run",
    "wm_80088F64_should_not_run",
    "wm_80037FD8_should_not_run",
    "wm_world_DrawOTag_should_not_run",
    "wm_world_ArchiveCdDataSync_should_not_run",
))

WORLD_RESIDUAL_PREFIXES = (
    "wm_world_update",
    "wm_world_loop",
    "wm_world_draw",
)


def backtrace_is_world_residual(max_frames=12):
    frame = gdb.newest_frame()
    depth = 0
    while frame is not None and depth < max_frames:
        name = frame.name()
        if name:
            if name in WORLD_RESIDUAL_CALLERS:
                return True
            for prefix in WORLD_RESIDUAL_PREFIXES:
                if name.startswith(prefix):
                    return True
        frame = frame.older()
        depth += 1
    return False
