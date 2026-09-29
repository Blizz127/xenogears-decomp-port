#include "battle_mips_adapter.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct PendingLoad {
    uint32_t value;
    uint32_t mask;
    unsigned reg;
    int valid;
} PendingLoad;

static int cpu_error(PcPortMipsCpu *cpu, int code, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(cpu->error, sizeof(cpu->error), fmt, ap);
    va_end(ap);
    return code;
}

static int mem_read(PcPortMipsCpu *cpu, uint32_t address, unsigned width,
                    uint32_t *value)
{
    if (cpu->bus.read == NULL ||
        cpu->bus.read(cpu->bus.opaque, address, width, value) != 0) {
        return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                         "read%u fault at 0x%08x (pc=0x%08x)",
                         width * 8u, address, cpu->pc);
    }
    return 0;
}

static int mem_write(PcPortMipsCpu *cpu, uint32_t address, unsigned width,
                     uint32_t value)
{
    if (cpu->bus.write == NULL ||
        cpu->bus.write(cpu->bus.opaque, address, width, value) != 0) {
        return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                         "write%u fault at 0x%08x (pc=0x%08x)",
                         width * 8u, address, cpu->pc);
    }
    return 0;
}

static void write_reg(PcPortMipsCpu *cpu, unsigned reg, uint32_t value,
                      uint32_t *written_mask)
{
    if (reg != 0) {
        cpu->gpr[reg] = value;
        *written_mask |= 1u << reg;
    }
}

static void schedule_load(PendingLoad *next, unsigned reg, uint32_t value,
                          uint32_t mask)
{
    if (reg == 0)
        return;
    next->valid = 1;
    next->reg = reg;
    next->value = value;
    next->mask = mask;
}

static int signed_add_overflow(int32_t a, int32_t b, int32_t *out)
{
    int64_t value = (int64_t)a + (int64_t)b;
    *out = (int32_t)value;
    return value > INT32_MAX || value < INT32_MIN;
}

static int signed_sub_overflow(int32_t a, int32_t b, int32_t *out)
{
    int64_t value = (int64_t)a - (int64_t)b;
    *out = (int32_t)value;
    return value > INT32_MAX || value < INT32_MIN;
}

void PcPortMipsCpuInit(PcPortMipsCpu *cpu, const PcPortMipsBus *bus)
{
    memset(cpu, 0, sizeof(*cpu));
    if (bus != NULL)
        cpu->bus = *bus;
}

static int execute_one(PcPortMipsCpu *cpu, uint32_t instruction,
                       PendingLoad *next_load, uint32_t *written_mask)
{
    static const uint32_t lwl_mask[4] = {
        0x00ffffffu, 0x0000ffffu, 0x000000ffu, 0u
    };
    static const unsigned lwl_shift[4] = {24, 16, 8, 0};
    static const uint32_t lwr_mask[4] = {
        0u, 0xff000000u, 0xffff0000u, 0xffffff00u
    };
    static const unsigned lwr_shift[4] = {0, 8, 16, 24};
    static const uint32_t swl_mask[4] = {
        0xffffff00u, 0xffff0000u, 0xff000000u, 0u
    };
    static const unsigned swl_shift[4] = {24, 16, 8, 0};
    static const uint32_t swr_mask[4] = {
        0u, 0x000000ffu, 0x0000ffffu, 0x00ffffffu
    };
    static const unsigned swr_shift[4] = {0, 8, 16, 24};
    unsigned op = instruction >> 26;
    unsigned rs = (instruction >> 21) & 31u;
    unsigned rt = (instruction >> 16) & 31u;
    unsigned rd = (instruction >> 11) & 31u;
    unsigned shamt = (instruction >> 6) & 31u;
    unsigned funct = instruction & 63u;
    int32_t simm = (int16_t)instruction;
    uint32_t uimm = instruction & 0xffffu;
    uint32_t address;
    uint32_t value;
    int32_t signed_value;
    int rc;

    switch (op) {
    case 0x00:
        switch (funct) {
        case 0x00: write_reg(cpu, rd, cpu->gpr[rt] << shamt, written_mask); break;
        case 0x02: write_reg(cpu, rd, cpu->gpr[rt] >> shamt, written_mask); break;
        case 0x03: write_reg(cpu, rd, (uint32_t)((int32_t)cpu->gpr[rt] >> shamt), written_mask); break;
        case 0x04: write_reg(cpu, rd, cpu->gpr[rt] << (cpu->gpr[rs] & 31u), written_mask); break;
        case 0x06: write_reg(cpu, rd, cpu->gpr[rt] >> (cpu->gpr[rs] & 31u), written_mask); break;
        case 0x07: write_reg(cpu, rd, (uint32_t)((int32_t)cpu->gpr[rt] >> (cpu->gpr[rs] & 31u)), written_mask); break;
        case 0x08: cpu->next_pc = cpu->gpr[rs]; break;
        case 0x09:
            write_reg(cpu, rd ? rd : 31u, cpu->pc + 8u, written_mask);
            cpu->next_pc = cpu->gpr[rs];
            break;
        case 0x0c:
        case 0x0d:
            return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                             "%s at pc=0x%08x",
                             funct == 0x0c ? "syscall" : "break", cpu->pc);
        case 0x10: write_reg(cpu, rd, cpu->hi, written_mask); break;
        case 0x11: cpu->hi = cpu->gpr[rs]; break;
        case 0x12: write_reg(cpu, rd, cpu->lo, written_mask); break;
        case 0x13: cpu->lo = cpu->gpr[rs]; break;
        case 0x18: {
            int64_t product = (int64_t)(int32_t)cpu->gpr[rs] *
                              (int64_t)(int32_t)cpu->gpr[rt];
            cpu->lo = (uint32_t)product;
            cpu->hi = (uint32_t)((uint64_t)product >> 32);
            break;
        }
        case 0x19: {
            uint64_t product = (uint64_t)cpu->gpr[rs] * cpu->gpr[rt];
            cpu->lo = (uint32_t)product;
            cpu->hi = (uint32_t)(product >> 32);
            break;
        }
        case 0x1a: {
            int32_t dividend = (int32_t)cpu->gpr[rs];
            int32_t divisor = (int32_t)cpu->gpr[rt];
            if (divisor == 0) {
                cpu->lo = dividend >= 0 ? 0xffffffffu : 1u;
                cpu->hi = (uint32_t)dividend;
            } else if (dividend == INT32_MIN && divisor == -1) {
                cpu->lo = (uint32_t)INT32_MIN;
                cpu->hi = 0;
            } else {
                cpu->lo = (uint32_t)(dividend / divisor);
                cpu->hi = (uint32_t)(dividend % divisor);
            }
            break;
        }
        case 0x1b:
            if (cpu->gpr[rt] == 0) {
                cpu->lo = 0xffffffffu;
                cpu->hi = cpu->gpr[rs];
            } else {
                cpu->lo = cpu->gpr[rs] / cpu->gpr[rt];
                cpu->hi = cpu->gpr[rs] % cpu->gpr[rt];
            }
            break;
        case 0x20:
            if (signed_add_overflow((int32_t)cpu->gpr[rs],
                                    (int32_t)cpu->gpr[rt], &signed_value))
                return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                                 "add overflow at pc=0x%08x", cpu->pc);
            write_reg(cpu, rd, (uint32_t)signed_value, written_mask);
            break;
        case 0x21: write_reg(cpu, rd, cpu->gpr[rs] + cpu->gpr[rt], written_mask); break;
        case 0x22:
            if (signed_sub_overflow((int32_t)cpu->gpr[rs],
                                    (int32_t)cpu->gpr[rt], &signed_value))
                return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                                 "sub overflow at pc=0x%08x", cpu->pc);
            write_reg(cpu, rd, (uint32_t)signed_value, written_mask);
            break;
        case 0x23: write_reg(cpu, rd, cpu->gpr[rs] - cpu->gpr[rt], written_mask); break;
        case 0x24: write_reg(cpu, rd, cpu->gpr[rs] & cpu->gpr[rt], written_mask); break;
        case 0x25: write_reg(cpu, rd, cpu->gpr[rs] | cpu->gpr[rt], written_mask); break;
        case 0x26: write_reg(cpu, rd, cpu->gpr[rs] ^ cpu->gpr[rt], written_mask); break;
        case 0x27: write_reg(cpu, rd, ~(cpu->gpr[rs] | cpu->gpr[rt]), written_mask); break;
        case 0x2a: write_reg(cpu, rd, (int32_t)cpu->gpr[rs] < (int32_t)cpu->gpr[rt], written_mask); break;
        case 0x2b: write_reg(cpu, rd, cpu->gpr[rs] < cpu->gpr[rt], written_mask); break;
        default:
            return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                             "SPECIAL funct 0x%02x instruction 0x%08x at pc=0x%08x",
                             funct, instruction, cpu->pc);
        }
        break;

    case 0x01: {
        int take = 0;
        int link = 0;
        switch (rt) {
        case 0x00: take = (int32_t)cpu->gpr[rs] < 0; break;
        case 0x01: take = (int32_t)cpu->gpr[rs] >= 0; break;
        case 0x10: take = (int32_t)cpu->gpr[rs] < 0; link = 1; break;
        case 0x11: take = (int32_t)cpu->gpr[rs] >= 0; link = 1; break;
        default:
            return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                             "REGIMM rt 0x%02x at pc=0x%08x", rt, cpu->pc);
        }
        if (link)
            write_reg(cpu, 31, cpu->pc + 8u, written_mask);
        if (take)
            cpu->next_pc = cpu->pc + 4u + ((uint32_t)simm << 2);
        break;
    }
    case 0x02:
        cpu->next_pc = ((cpu->pc + 4u) & 0xf0000000u) |
                       ((instruction & 0x03ffffffu) << 2);
        break;
    case 0x03:
        write_reg(cpu, 31, cpu->pc + 8u, written_mask);
        cpu->next_pc = ((cpu->pc + 4u) & 0xf0000000u) |
                       ((instruction & 0x03ffffffu) << 2);
        break;
    case 0x04:
        if (cpu->gpr[rs] == cpu->gpr[rt])
            cpu->next_pc = cpu->pc + 4u + ((uint32_t)simm << 2);
        break;
    case 0x05:
        if (cpu->gpr[rs] != cpu->gpr[rt])
            cpu->next_pc = cpu->pc + 4u + ((uint32_t)simm << 2);
        break;
    case 0x06:
        if ((int32_t)cpu->gpr[rs] <= 0)
            cpu->next_pc = cpu->pc + 4u + ((uint32_t)simm << 2);
        break;
    case 0x07:
        if ((int32_t)cpu->gpr[rs] > 0)
            cpu->next_pc = cpu->pc + 4u + ((uint32_t)simm << 2);
        break;
    case 0x08:
        if (signed_add_overflow((int32_t)cpu->gpr[rs], simm, &signed_value))
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "addi overflow at pc=0x%08x", cpu->pc);
        write_reg(cpu, rt, (uint32_t)signed_value, written_mask);
        break;
    case 0x09: write_reg(cpu, rt, cpu->gpr[rs] + (uint32_t)simm, written_mask); break;
    case 0x0a: write_reg(cpu, rt, (int32_t)cpu->gpr[rs] < simm, written_mask); break;
    case 0x0b: write_reg(cpu, rt, cpu->gpr[rs] < (uint32_t)simm, written_mask); break;
    case 0x0c: write_reg(cpu, rt, cpu->gpr[rs] & uimm, written_mask); break;
    case 0x0d: write_reg(cpu, rt, cpu->gpr[rs] | uimm, written_mask); break;
    case 0x0e: write_reg(cpu, rt, cpu->gpr[rs] ^ uimm, written_mask); break;
    case 0x0f: write_reg(cpu, rt, uimm << 16, written_mask); break;

    case 0x10: /* COP0 */
        switch (rs) {
        case 0x00: schedule_load(next_load, rt, cpu->cp0[rd], 0); break;
        case 0x04: cpu->cp0[rd] = cpu->gpr[rt]; break;
        case 0x10:
            if (funct != 0x10)
                return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                                 "COP0 instruction 0x%08x at pc=0x%08x",
                                 instruction, cpu->pc);
            cpu->cp0[12] = (cpu->cp0[12] & ~0x3fu) |
                           ((cpu->cp0[12] >> 2) & 0x0fu);
            break;
        default:
            return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                             "COP0 rs 0x%02x at pc=0x%08x", rs, cpu->pc);
        }
        break;
    case 0x12: /* COP2 */
        if (instruction & 0x02000000u) {
            if (cpu->bus.cop2_command == NULL ||
                cpu->bus.cop2_command(cpu->bus.opaque, instruction) != 0)
                return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                                 "COP2 command 0x%08x failed at pc=0x%08x",
                                 instruction, cpu->pc);
        } else {
            switch (rs) {
            case 0x00:
            case 0x02:
                if (cpu->bus.cop2_read == NULL)
                    return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                                     "COP2 read unavailable at pc=0x%08x", cpu->pc);
                schedule_load(next_load, rt,
                              cpu->bus.cop2_read(cpu->bus.opaque, rs == 2, rd), 0);
                break;
            case 0x04:
            case 0x06:
                if (cpu->bus.cop2_write == NULL)
                    return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                                     "COP2 write unavailable at pc=0x%08x", cpu->pc);
                cpu->bus.cop2_write(cpu->bus.opaque, rs == 6, rd, cpu->gpr[rt]);
                break;
            default:
                return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                                 "COP2 rs 0x%02x at pc=0x%08x", rs, cpu->pc);
            }
        }
        break;

    case 0x20: /* LB */
    case 0x24: /* LBU */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if ((rc = mem_read(cpu, address, 1, &value)) != 0) return rc;
        schedule_load(next_load, rt,
                      op == 0x20 ? (uint32_t)(int32_t)(int8_t)value : value, 0);
        break;
    case 0x21: /* LH */
    case 0x25: /* LHU */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if (address & 1u)
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "unaligned half load at 0x%08x pc=0x%08x",
                             address, cpu->pc);
        if ((rc = mem_read(cpu, address, 2, &value)) != 0) return rc;
        schedule_load(next_load, rt,
                      op == 0x21 ? (uint32_t)(int32_t)(int16_t)value : value, 0);
        break;
    case 0x22: /* LWL */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if ((rc = mem_read(cpu, address & ~3u, 4, &value)) != 0) return rc;
        schedule_load(next_load, rt, value << lwl_shift[address & 3u],
                      lwl_mask[address & 3u]);
        break;
    case 0x23: /* LW */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if (address & 3u)
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "unaligned word load at 0x%08x pc=0x%08x",
                             address, cpu->pc);
        if ((rc = mem_read(cpu, address, 4, &value)) != 0) return rc;
        schedule_load(next_load, rt, value, 0);
        break;
    case 0x26: /* LWR */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if ((rc = mem_read(cpu, address & ~3u, 4, &value)) != 0) return rc;
        schedule_load(next_load, rt, value >> lwr_shift[address & 3u],
                      lwr_mask[address & 3u]);
        break;
    case 0x28: /* SB */
    case 0x29: /* SH */
    case 0x2b: /* SW */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if ((op == 0x29 && (address & 1u)) ||
            (op == 0x2b && (address & 3u)))
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "unaligned store at 0x%08x pc=0x%08x",
                             address, cpu->pc);
        return mem_write(cpu, address, op == 0x28 ? 1u : op == 0x29 ? 2u : 4u,
                         cpu->gpr[rt]);
    case 0x2a: /* SWL */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if ((rc = mem_read(cpu, address & ~3u, 4, &value)) != 0) return rc;
        value = (value & swl_mask[address & 3u]) |
                (cpu->gpr[rt] >> swl_shift[address & 3u]);
        return mem_write(cpu, address & ~3u, 4, value);
    case 0x2e: /* SWR */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if ((rc = mem_read(cpu, address & ~3u, 4, &value)) != 0) return rc;
        value = (value & swr_mask[address & 3u]) |
                (cpu->gpr[rt] << swr_shift[address & 3u]);
        return mem_write(cpu, address & ~3u, 4, value);
    case 0x32: /* LWC2 */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if (address & 3u)
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "unaligned LWC2 at 0x%08x pc=0x%08x",
                             address, cpu->pc);
        if ((rc = mem_read(cpu, address, 4, &value)) != 0) return rc;
        if (cpu->bus.cop2_write == NULL)
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "COP2 write unavailable at pc=0x%08x", cpu->pc);
        cpu->bus.cop2_write(cpu->bus.opaque, 0, rt, value);
        break;
    case 0x3a: /* SWC2 */
        address = cpu->gpr[rs] + (uint32_t)simm;
        if (address & 3u)
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "unaligned SWC2 at 0x%08x pc=0x%08x",
                             address, cpu->pc);
        if (cpu->bus.cop2_read == NULL)
            return cpu_error(cpu, PC_PORT_MIPS_FAULT,
                             "COP2 read unavailable at pc=0x%08x", cpu->pc);
        return mem_write(cpu, address, 4,
                         cpu->bus.cop2_read(cpu->bus.opaque, 0, rt));
    default:
        return cpu_error(cpu, PC_PORT_MIPS_UNSUPPORTED,
                         "opcode 0x%02x instruction 0x%08x at pc=0x%08x",
                         op, instruction, cpu->pc);
    }
    return 0;
}

int PcPortMipsRun(PcPortMipsCpu *cpu, uint32_t entry, uint32_t halt_pc,
                  uint64_t max_steps)
{
    uint64_t start_steps = cpu->steps;

    cpu->pc = entry;
    cpu->next_pc = entry + 4u;
    cpu->error[0] = '\0';

    while (cpu->steps - start_steps < max_steps) {
        PendingLoad old_load;
        PendingLoad next_load;
        uint32_t instruction;
        uint32_t current_pc;
        uint32_t following_pc;
        uint32_t written_mask = 0;
        int bridge_rc;
        int rc;

        if (cpu->pc == halt_pc)
            return PC_PORT_MIPS_HALTED;

        if (cpu->bus.bridge != NULL) {
            bridge_rc = cpu->bus.bridge(cpu->bus.opaque, cpu, cpu->pc);
            if (bridge_rc < 0)
                return cpu_error(cpu, PC_PORT_MIPS_UNRESOLVED_CALL,
                                 "unresolved call target 0x%08x", cpu->pc);
            if (bridge_rc > 0) {
                cpu->pc = cpu->gpr[31];
                cpu->next_pc = cpu->pc + 4u;
                continue;
            }
        }

        if (mem_read(cpu, cpu->pc, 4, &instruction) != 0)
            return PC_PORT_MIPS_FAULT;

        old_load.valid = cpu->load_valid;
        old_load.reg = cpu->load_reg;
        old_load.value = cpu->load_value;
        old_load.mask = cpu->load_mask;
        memset(&next_load, 0, sizeof(next_load));

        current_pc = cpu->pc;
        following_pc = cpu->next_pc;
        cpu->next_pc = following_pc + 4u;
        cpu->steps++;

        /* execute_one reports the address of the current instruction through
         * cpu->pc, so temporarily expose it while retaining the pipeline PCs. */
        cpu->pc = current_pc;
        rc = execute_one(cpu, instruction, &next_load, &written_mask);
        cpu->pc = following_pc;
        if (rc != 0)
            return rc;

        if (old_load.valid && old_load.reg != 0 &&
            !(written_mask & (1u << old_load.reg))) {
            cpu->gpr[old_load.reg] =
                (cpu->gpr[old_load.reg] & old_load.mask) | old_load.value;
        }
        cpu->load_valid = (uint8_t)next_load.valid;
        cpu->load_reg = (uint8_t)next_load.reg;
        cpu->load_value = next_load.value;
        cpu->load_mask = next_load.mask;
        cpu->gpr[0] = 0;
    }

    return cpu_error(cpu, PC_PORT_MIPS_BUDGET,
                     "instruction budget exhausted at pc=0x%08x after %llu steps",
                     cpu->pc, (unsigned long long)(cpu->steps - start_steps));
}
