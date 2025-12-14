#include <stddef.h>
#include <stdio.h>

#include "config.h"
#include "cpu.h"
#include "memory.h"
#include "opcodes.h"

/*
======================================
   GENERIC OPCODE DEFINITIONS START
======================================
*/

/*
op_ld_r_r

Load an 8-bit value [src] into an 8-bit destination [dest].
Flags: - - - -
*/
void op_ld_r_n(uint8_t * dest, uint8_t src){
    *dest = src;
}

/*
op_ld_rr_d16

Load an immediate 16-bit value into the two 8-bit destinations [high] and [low].
Flags: - - - -
*/
void op_ld_rr_d16(CPU * cpu, Memory * mem, uint8_t * high, uint8_t * low){
    uint8_t lo = get_imm8(cpu, mem);
    uint8_t hi = get_imm8(cpu, mem);
    *high = hi;
    *low = lo;
}

/*
op_jr

Jump PC by the signed 8-bit value of [src] if [condition] is true.
Flags: - - - -
*/
uint8_t op_jr(CPU * cpu, uint8_t src, uint8_t condition){
    int8_t offset = (int8_t)src;
    if(condition){
        cpu -> pc += offset;
        return 12;
    }
    return 8;
}

/*
op_cp

Compare the 8-bit register a to [src].
Flags: Z 1 H CY
*/
void op_cp(CPU * cpu, uint8_t src){
    set_zero_flag(cpu, cpu -> a == src);
    set_subtract_flag(cpu, 1);
    set_half_carry_flag(cpu, (cpu -> a & 0x0F) < (src & 0x0F));
    set_carry_flag(cpu, (cpu -> a) < src);
}

/*
op_dec_r

Decrement the 8-bit value [src] by 1.
Flags: Z 1 H -
*/
void op_dec_r(CPU * cpu, uint8_t * src) {
    uint8_t old = *src;
    uint8_t res = old - 1;
    *src = res;
    set_zero_flag(cpu, res == 0);
    set_subtract_flag(cpu, 1);
    set_half_carry_flag(cpu, (old & 0x0F) == 0);
}

/*
op_inc_r

Increment the 8-bit value [src] by 1.
Flags: Z 0 H -
*/
void op_inc_r(CPU * cpu, uint8_t * src) {
    uint8_t old = *src;
    uint8_t res = old + 1;
    *src = res;
    set_zero_flag(cpu, res == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, (old & 0x0F) + 1 > 0x0F);
}

/*
op_call

If [condition] evaluates to true, push the current PC onto the stack and set PC to the immediate 16-bit value.
Flags: - - - -
*/
uint8_t op_call(CPU * cpu, Memory * mem, uint8_t condition)
{
    uint16_t addr = get_imm16(cpu, mem);

    if (!condition) {
        return 12;
    }

    push16(cpu, mem, cpu->pc);

    cpu->pc = addr;

    return 24;
}

/*
op_ret

If [condition] evaluates to true, set the PC to the 16-bit value popped off the stack.
Flags: - - - -
*/
uint8_t op_ret(CPU * cpu, Memory * mem, uint8_t condition)
{
    if (!condition) {
        return 8;
    }

    cpu->pc = pop16(cpu, mem);

    return 20;
}

/*
op_or

Set register a to the bitwise OR of a and [src].
Flags: Z 0 0 0
*/
void op_or(CPU * cpu, uint8_t src) {
    cpu -> a |= src;

    set_zero_flag(cpu, cpu -> a == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, 0);
}

/*
op_and

Set register a to the bitwise AND of a and [src].
Flags: Z 0 1 0
*/
void op_and(CPU * cpu, uint8_t src) {
    cpu -> a &= src;

    set_zero_flag(cpu, cpu -> a == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 1);
    set_carry_flag(cpu, 0);
}

/*
op_xor

Set register a to the bitwise XOR of a and [src].
Flags: Z 0 0 0
*/
void op_xor(CPU * cpu, uint8_t src) {
    cpu -> a ^= src;

    set_zero_flag(cpu, cpu -> a == 0);
    set_subtract_flag(cpu, 0);
    set_carry_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
}

/*
op_sub

Set register a to a - [src].
Flags: Z 1 H CY
*/
void op_sub(CPU * cpu, uint8_t src)
{
    uint8_t a = cpu -> a;

    cpu -> a = a - src;

    set_zero_flag(cpu, cpu -> a == 0);
    set_subtract_flag(cpu, 1);
    set_half_carry_flag(cpu, (a & 0x0F) < (src & 0x0F));
    set_carry_flag(cpu, a < src);
}

/*
op_daa

Decimal adjust register a.
Flags: Z - 0 CY
*/
void op_daa(CPU * cpu) {
    uint8_t correction = 0;
    uint8_t set_carry = 0;

    if(get_half_carry_flag(cpu) || (!get_subtract_flag(cpu) && (cpu -> a & 0x0F) > 0x09)) {
        correction |= 0x06;
    }

    if(get_carry_flag(cpu) || (!get_subtract_flag(cpu) && cpu -> a > 0x99)) {
        correction |= 0x60;
        set_carry = 1;
    }

    if(get_subtract_flag(cpu)) {
        cpu -> a -= correction;
    } else {
        cpu -> a += correction;
    }

    set_zero_flag(cpu, cpu -> a == 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, set_carry);
}

/*
op_sbc

Set register a to a - [src] - cy.
Flags: Z 1 H CY
*/
void op_sbc(CPU * cpu, uint8_t src)
{
    uint8_t a = cpu -> a;
    uint8_t carry = get_carry_flag(cpu);

    cpu -> a -= src + carry;

    set_zero_flag(cpu, cpu -> a == 0);
    set_subtract_flag(cpu, 1);
    set_half_carry_flag(cpu, (a & 0x0F) < ((src & 0x0F) + carry));
    set_carry_flag(cpu, a < (src + carry));
}

/*
op_adc

Set register a to a + [src] + cy.
Flags: Z 0 H CY
*/
void op_adc(CPU * cpu, uint8_t src)
{
    uint8_t a = cpu -> a;
    uint8_t carry = get_carry_flag(cpu);

    cpu -> a += src + carry;

    set_zero_flag(cpu, cpu->a == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, ((a & 0x0F) + (src & 0x0F) + carry) > 0x0F);
    set_carry_flag(cpu, a + src + carry > 0xFF);
}

/*
op_add

Set register a to a + [src].
Flags: Z 0 H CY
*/
void op_add(CPU * cpu, uint8_t src) {
    uint8_t a = cpu -> a;

    cpu -> a += src;

    set_zero_flag(cpu, cpu -> a == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, ((a & 0x0F) + (src & 0x0F)) > 0x0F);
    set_carry_flag(cpu, a + src > 0xFF);
}

/*
op_add_16

Set register pair hl to hl + [src].
Flags: - 0 H CY
*/
void op_add_16(CPU * cpu, uint16_t src) {
    uint16_t hl = get_hl(cpu);
    set_hl(cpu, get_hl(cpu) + src);

    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, ((hl & 0x0FFF) + (src & 0x0FFF)) > 0x0FFF);
    set_carry_flag(cpu, hl + src > 0xFFFF);
}


/*
=============================
   OPCODE DEFINITION START
=============================
*/

// NOP
uint8_t op_00(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD BC, d16
uint8_t op_01(CPU * cpu, Memory * mem) {op_ld_rr_d16(cpu, mem, &cpu -> b, &cpu -> c); return 12;}

// LD (BC), A
uint8_t op_02(CPU * cpu, Memory * mem) {mem_write8(mem, get_bc(cpu), cpu -> a); return 8;}

// INC BC
uint8_t op_03(CPU * cpu, Memory * mem) {(void)mem; set_bc(cpu, get_bc(cpu) + 1); return 8;}

// INC B
uint8_t op_04(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> b); return 4;}

// DEC B
uint8_t op_05(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> b); return 4;}

// LD B, d8
uint8_t op_06(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> b, get_imm8(cpu, mem)); return 8;}

// RLCA
uint8_t op_07(CPU * cpu, Memory * mem) {
    (void)mem;
    uint8_t msb = (cpu -> a & 0x80) >> 7;
    cpu -> a = (cpu -> a << 1) | msb;

    set_zero_flag(cpu, 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, msb);

    return 4;
}

// LD (a16), SP
uint8_t op_08(CPU * cpu, Memory * mem) {uint16_t addr = get_imm16(cpu, mem); mem_write8(mem, addr, cpu -> sp & 0x00FF); mem_write8(mem, addr + 1, (cpu -> sp & 0xFF00) >> 8); return 20;}

// ADD HL, BC
uint8_t op_09(CPU * cpu, Memory * mem) {(void)mem; op_add_16(cpu, get_bc(cpu)); return 8;}

// LD A, (BC)
uint8_t op_0A(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> a, mem_read8(mem, get_bc(cpu))); return 8;}

// DEC BC
uint8_t op_0B(CPU * cpu, Memory * mem) {(void)mem; set_bc(cpu, get_bc(cpu) - 1); return 8;}

// INC C
uint8_t op_0C(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> c); return 4;}

// DEC C
uint8_t op_0D(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> c); return 4;}

// LD C, d8
uint8_t op_0E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> c, get_imm8(cpu, mem)); return 8;}

// RRCA
uint8_t op_0F(CPU * cpu, Memory * mem) {
    (void)mem;
    uint8_t lsb = cpu -> a & 0x01;
    cpu -> a = (cpu -> a >> 1) | (lsb << 7);

    set_zero_flag(cpu, 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, lsb);

    return 4;
}

// LD DE, d16
uint8_t op_11(CPU * cpu, Memory * mem) {op_ld_rr_d16(cpu, mem, &cpu -> d, &cpu -> e); return 12;}

// LD (DE), A
uint8_t op_12(CPU * cpu, Memory * mem) {mem_write8(mem, get_de(cpu), cpu -> a); return 8;}

// INC DE
uint8_t op_13(CPU * cpu, Memory * mem) {(void)mem; set_de(cpu, get_de(cpu) + 1); return 8;}

// INC D
uint8_t op_14(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> d); return 4;}

// DEC D
uint8_t op_15(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> d); return 4;}

// LD D, d8
uint8_t op_16(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> d, get_imm8(cpu, mem)); return 8;}

// RLA
uint8_t op_17(CPU * cpu, Memory * mem) {(void)mem; uint8_t old = cpu -> a; cpu -> a = (old << 1) | get_carry_flag(cpu); set_carry_flag(cpu, old & 0x80); set_zero_flag(cpu, 0); set_subtract_flag(cpu, 0); set_half_carry_flag(cpu, 0); return 4;}

// JR s8
uint8_t op_18(CPU * cpu, Memory * mem) {return op_jr(cpu, get_imm8(cpu, mem), 1);}

// ADD HL, DE
uint8_t op_19(CPU * cpu, Memory * mem) {(void)mem; op_add_16(cpu, get_de(cpu)); return 8;}

// LD A, (DE)
uint8_t op_1A(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> a, mem_read8(mem, get_de(cpu))); return 8;}

// DEC DE
uint8_t op_1B(CPU * cpu, Memory * mem) {(void)mem; set_de(cpu, get_de(cpu) - 1); return 8;}

// INC E
uint8_t op_1C(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> e); return 4;}

// DEC E
uint8_t op_1D(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> e); return 4;}

// LD E, d8
uint8_t op_1E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> e, get_imm8(cpu, mem)); return 8;}

// RRA
uint8_t op_1F(CPU * cpu, Memory * mem) {(void)mem; uint8_t old = cpu -> a; cpu -> a = (old >> 1) | (get_carry_flag(cpu) << 7); set_carry_flag(cpu, old & 0x01); set_zero_flag(cpu, 0); set_subtract_flag(cpu, 0); set_half_carry_flag(cpu, 0); return 4;}

// JR NZ, s8
uint8_t op_20(CPU * cpu, Memory * mem) {return op_jr(cpu, get_imm8(cpu, mem), !get_zero_flag(cpu));}

// LD HL, d16
uint8_t op_21(CPU * cpu, Memory * mem) {op_ld_rr_d16(cpu, mem, &cpu -> h, &cpu -> l); return 12;}

// LD (HL+), A
uint8_t op_22(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl_plus(cpu),  cpu -> a); return 8;}

// INC HL
uint8_t op_23(CPU * cpu, Memory * mem) {(void)mem; set_hl(cpu, get_hl(cpu) + 1); return 8;}

// INC H
uint8_t op_24(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> h); return 4;}

// DEC H
uint8_t op_25(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> h); return 4;}

// LD H, d8
uint8_t op_26(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> h, get_imm8(cpu, mem)); return 8;}

// DAA
uint8_t op_27(CPU * cpu, Memory * mem) {(void)mem; op_daa(cpu); return 4;}

// JR Z, s8
uint8_t op_28(CPU * cpu, Memory * mem) {return op_jr(cpu, get_imm8(cpu, mem), get_zero_flag(cpu));}

// ADD HL, HL
uint8_t op_29(CPU * cpu, Memory * mem) {(void)mem; op_add_16(cpu, get_hl(cpu)); return 8;}

// LD A, (HL+)
uint8_t op_2A(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, mem_read8(mem, get_hl_plus(cpu))); return 8;}

// DEC HL
uint8_t op_2B(CPU * cpu, Memory * mem) {(void)mem; set_hl(cpu, get_hl(cpu) - 1); return 8;}

// INC L
uint8_t op_2C(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> l); return 4;}

// DEC L
uint8_t op_2D(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> l); return 4;}

// LD L, d8
uint8_t op_2E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> l, get_imm8(cpu, mem)); return 8;}

// CPL A
uint8_t op_2F(CPU * cpu, Memory * mem) {(void)mem; cpu -> a = ~cpu -> a; set_subtract_flag(cpu, 1); set_half_carry_flag(cpu, 1); return 4;}

// JR NC, s8
uint8_t op_30(CPU * cpu, Memory * mem) {return op_jr(cpu, get_imm8(cpu, mem), !get_carry_flag(cpu));}

// LD SP, d16
uint8_t op_31(CPU * cpu, Memory * mem) {cpu -> sp = get_imm16(cpu, mem); return 12;}

// LD (HL-), A
uint8_t op_32(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl_minus(cpu), cpu -> a); return 8;}

// INC SP
uint8_t op_33(CPU * cpu, Memory * mem) {(void)mem; cpu -> sp++; return 8;}

// INC (HL)
uint8_t op_34(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); op_inc_r(cpu, &val); mem_write8(mem, addr, val); return 12;}

// DEC (HL)
uint8_t op_35(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); op_dec_r(cpu, &val); mem_write8(mem, addr, val); return 12;}

// LD (HL), d8
uint8_t op_36(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), get_imm8(cpu, mem)); return 12;}

// SCF
uint8_t op_37(CPU * cpu, Memory * mem) {(void)mem; set_subtract_flag(cpu, 0); set_half_carry_flag(cpu, 0); set_carry_flag(cpu, 1); return 4;}

// JR C, s8
uint8_t op_38(CPU * cpu, Memory * mem) {return op_jr(cpu, get_imm8(cpu, mem), get_carry_flag(cpu));}

// ADD HL, SP
uint8_t op_39(CPU * cpu, Memory * mem) {(void)mem; op_add_16(cpu, cpu -> sp); return 8;}

// LD A, (HL-)
uint8_t op_3A(CPU *cpu, Memory *mem) {uint16_t addr = get_hl(cpu); cpu -> a = mem_read8(mem, addr); set_hl(cpu, addr - 1); return 8;}

// DEC SP
uint8_t op_3B(CPU * cpu, Memory * mem) {(void)mem; cpu -> sp--; return 8;}

// INC A
uint8_t op_3C(CPU * cpu, Memory * mem) {(void)mem; op_inc_r(cpu, &cpu -> a); return 4;}

// DEC A
uint8_t op_3D(CPU * cpu, Memory * mem) {(void)mem; op_dec_r(cpu, &cpu -> a); return 4;}

// LD A, d8
uint8_t op_3E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> a, get_imm8(cpu, mem)); return 8;}

// CCF
uint8_t op_3F(CPU * cpu, Memory * mem) {(void)mem; set_subtract_flag(cpu, 0); set_half_carry_flag(cpu, 0); set_carry_flag(cpu, !get_carry_flag(cpu)); return 4;}

// LD B, B
uint8_t op_40(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD B, C
uint8_t op_41(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> b, cpu -> c); return 4;}

// LD B, D
uint8_t op_42(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> b, cpu -> d); return 4;}

// LD B, E
uint8_t op_43(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> b, cpu -> e); return 4;}

// LD B, H
uint8_t op_44(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> b, cpu -> h); return 4;}

// LD B, L
uint8_t op_45(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> b, cpu -> l); return 4;}

// LD B, (HL)
uint8_t op_46(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> b, mem_read8(mem, get_hl(cpu))); return 8;}

// LD B, A
uint8_t op_47(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> b, cpu -> a); return 4;}

// LD C, B
uint8_t op_48(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> c, cpu -> b); return 4;}

// LD C, C
uint8_t op_49(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD C, D
uint8_t op_4A(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> c, cpu -> d); return 4;}

// LD C, E
uint8_t op_4B(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> c, cpu -> e); return 4;}

// LD C, H
uint8_t op_4C(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> c, cpu -> h); return 4;}

// LD C, L
uint8_t op_4D(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> c, cpu -> l); return 4;}

// LD C, (HL)
uint8_t op_4E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> c, mem_read8(mem, get_hl(cpu))); return 8;}

// LD C, A
uint8_t op_4F(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> c, cpu -> a); return 4;}

// LD D, B
uint8_t op_50(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> d, cpu -> b); return 4;}

// LD D, C
uint8_t op_51(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> d, cpu -> c); return 4;}

// LD D, D
uint8_t op_52(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD D, E
uint8_t op_53(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> d, cpu -> e); return 4;}

// LD D, H
uint8_t op_54(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> d, cpu -> h); return 4;}

// LD D, L
uint8_t op_55(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> d, cpu -> l); return 4;}

// LD D, (HL)
uint8_t op_56(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> d, mem_read8(mem, get_hl(cpu))); return 8;}

// LD D, A
uint8_t op_57(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> d, cpu -> a); return 4;}

// LD E, B
uint8_t op_58(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> e, cpu -> b); return 4;}

// LD E, C
uint8_t op_59(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> e, cpu -> c); return 4;}

// LD E, D
uint8_t op_5A(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> e, cpu -> d); return 4;}

// LD E, E
uint8_t op_5B(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD E, H
uint8_t op_5C(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> e, cpu -> h); return 4;}

// LD E, L
uint8_t op_5D(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> e, cpu -> l); return 4;}

// LD E, (HL)
uint8_t op_5E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> e, mem_read8(mem, get_hl(cpu))); return 8;}

// LD E, A
uint8_t op_5F(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> e, cpu -> a); return 4;}

// LD H, B
uint8_t op_60(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> h, cpu -> b); return 4;}

// LD H, C
uint8_t op_61(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> h, cpu -> c); return 4;}

// LD H, D
uint8_t op_62(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> h, cpu -> d); return 4;}

// LD H, E
uint8_t op_63(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> h, cpu -> e); return 4;}

// LD H, H
uint8_t op_64(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD H, L
uint8_t op_65(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> h, cpu -> l); return 4;}

// LD H, (HL)
uint8_t op_66(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> h, mem_read8(mem, get_hl(cpu))); return 8;}

// LD H, A
uint8_t op_67(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> h, cpu -> a); return 4;}

// LD L, B
uint8_t op_68(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> l, cpu -> b); return 4;}

// LD L, C
uint8_t op_69(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> l, cpu -> c); return 4;}

// LD L, D
uint8_t op_6A(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> l, cpu -> d); return 4;}

// LD L, E
uint8_t op_6B(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> l, cpu -> e); return 4;}

// LD L, H
uint8_t op_6C(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> l, cpu -> h); return 4;}

// LD L, L
uint8_t op_6D(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// LD L, (HL)
uint8_t op_6E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> l, mem_read8(mem, get_hl(cpu))); return 8;}

// LD L, A
uint8_t op_6F(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> l, cpu -> a); return 4;}

// LD (HL), B
uint8_t op_70(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> b); return 8;}

// LD (HL), C
uint8_t op_71(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> c); return 8;}

// LD (HL), D
uint8_t op_72(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> d); return 8;}

// LD (HL), E
uint8_t op_73(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> e); return 8;}

// LD (HL), H
uint8_t op_74(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> h); return 8;}

// LD (HL), L
uint8_t op_75(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> l); return 8;}

// HALT
uint8_t op_76(CPU * cpu, Memory * mem) {(void)mem; cpu -> halted = 1; return 4;}

// LD (HL), A
uint8_t op_77(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cpu -> a); return 8;}

// LD A, B
uint8_t op_78(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, cpu -> b); return 4;}

// LD A, C
uint8_t op_79(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, cpu -> c); return 4;}

// LD A, D
uint8_t op_7A(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, cpu -> d); return 4;}

// LD A, E
uint8_t op_7B(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, cpu -> e); return 4;}

// LD A, H
uint8_t op_7C(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, cpu -> h); return 4;}

// LD A, L
uint8_t op_7D(CPU * cpu, Memory * mem) {(void)mem; op_ld_r_n(&cpu -> a, cpu -> l); return 4;}

// LD A, (HL)
uint8_t op_7E(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> a, mem_read8(mem, get_hl(cpu))); return 8;}

// LD A, A
uint8_t op_7F(CPU * cpu, Memory * mem) {(void)cpu; (void)mem; return 4;}

// ADD A, B
uint8_t op_80(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> b); return 4;}

// ADD A, C
uint8_t op_81(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> c); return 4;}

// ADD A, D
uint8_t op_82(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> d); return 4;}

// ADD A, E
uint8_t op_83(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> e); return 4;}

// ADD A, H
uint8_t op_84(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> h); return 4;}

// ADD A, L
uint8_t op_85(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> l); return 4;}

// ADD A, (HL)
uint8_t op_86(CPU * cpu, Memory * mem) {op_add(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// ADD A, A
uint8_t op_87(CPU * cpu, Memory * mem) {(void)mem; op_add(cpu, cpu -> a); return 4;}

// ADC A, B
uint8_t op_88(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> b); return 4;}

// ADC A, C
uint8_t op_89(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> c); return 4;}

// ADC A, D
uint8_t op_8A(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> d); return 4;}

// ADC A, E
uint8_t op_8B(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> e); return 4;}

// ADC A, H
uint8_t op_8C(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> h); return 4;}

// ADC A, L
uint8_t op_8D(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> l); return 4;}

// ADC A, (HL)
uint8_t op_8E(CPU * cpu, Memory * mem) {op_adc(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// ADC A, A
uint8_t op_8F(CPU * cpu, Memory * mem) {(void)mem; op_adc(cpu, cpu -> a); return 4;}

// SUB B
uint8_t op_90(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> b); return 4;}

// SUB C
uint8_t op_91(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> c); return 4;}

// SUB D
uint8_t op_92(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> d); return 4;}

// SUB E
uint8_t op_93(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> e); return 4;}

// SUB H
uint8_t op_94(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> h); return 4;}

// SUB L
uint8_t op_95(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> l); return 4;}

// SUB (HL)
uint8_t op_96(CPU * cpu, Memory * mem) {op_sub(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// SUB A
uint8_t op_97(CPU * cpu, Memory * mem) {(void)mem; op_sub(cpu, cpu -> a); return 4;}

// SBC A, B
uint8_t op_98(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> b); return 4;}

// SBC A, C
uint8_t op_99(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> c); return 4;}

// SBC A, D
uint8_t op_9A(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> d); return 4;}

// SBC A, E
uint8_t op_9B(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> e); return 4;}

// SBC A, H
uint8_t op_9C(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> h); return 4;}

// SBC A, L
uint8_t op_9D(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> l); return 4;}

// SBC A, (HL)
uint8_t op_9E(CPU * cpu, Memory * mem) {op_sbc(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// SBC A, A
uint8_t op_9F(CPU * cpu, Memory * mem) {(void)mem; op_sbc(cpu, cpu -> a); return 4;}

// AND B
uint8_t op_A0(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> b); return 4;}

// AND C
uint8_t op_A1(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> c); return 4;}

// AND D
uint8_t op_A2(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> d); return 4;}

// AND E
uint8_t op_A3(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> e); return 4;}

// AND H
uint8_t op_A4(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> h); return 4;}

// AND L
uint8_t op_A5(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> l); return 4;}

// AND (HL)
uint8_t op_A6(CPU * cpu, Memory * mem) {op_and(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// AND A
uint8_t op_A7(CPU * cpu, Memory * mem) {(void)mem; op_and(cpu, cpu -> a); return 4;}

// XOR B
uint8_t op_A8(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> b); return 4;}

// XOR C
uint8_t op_A9(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> c); return 4;}

// XOR D
uint8_t op_AA(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> d); return 4;}

// XOR E
uint8_t op_AB(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> e); return 4;}

// XOR H
uint8_t op_AC(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> h); return 4;}

// XOR L
uint8_t op_AD(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> l); return 4;}

// XOR (HL)
uint8_t op_AE(CPU * cpu, Memory * mem) {op_xor(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// XOR A
uint8_t op_AF(CPU * cpu, Memory * mem) {(void)mem; op_xor(cpu, cpu -> a); return 4;}

// OR B
uint8_t op_B0(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> b); return 4;}

// OR C
uint8_t op_B1(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> c); return 4;}

// OR D
uint8_t op_B2(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> d); return 4;}

// OR E
uint8_t op_B3(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> e); return 4;}

// OR H
uint8_t op_B4(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> h); return 4;}

// OR L
uint8_t op_B5(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> l); return 4;}

// OR (HL)
uint8_t op_B6(CPU * cpu, Memory * mem) {op_or(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// OR A
uint8_t op_B7(CPU * cpu, Memory * mem) {(void)mem; op_or(cpu, cpu -> a); return 4;}

// CP B
uint8_t op_B8(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> b); return 4;}

// CP C
uint8_t op_B9(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> c); return 4;}

// CP D
uint8_t op_BA(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> d); return 4;}

// CP E
uint8_t op_BB(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> e); return 4;}

// CP H
uint8_t op_BC(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> h); return 4;}

// CP L
uint8_t op_BD(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> l); return 4;}

// CP (HL)
uint8_t op_BE(CPU * cpu, Memory * mem) {op_cp(cpu, mem_read8(mem, get_hl(cpu))); return 8;}

// CP A
uint8_t op_BF(CPU * cpu, Memory * mem) {(void)mem; op_cp(cpu, cpu -> a); return 4;}

// RET NZ
uint8_t op_C0(CPU * cpu, Memory * mem) {(void)mem; return op_ret(cpu, mem, !get_zero_flag(cpu));}

// POP BC
uint8_t op_C1(CPU * cpu, Memory * mem) {set_bc(cpu, pop16(cpu, mem)); return 12;}

// JP NZ, a16
uint8_t op_C2(CPU * cpu, Memory * mem) {uint16_t addr = get_imm16(cpu, mem); if(!get_zero_flag(cpu)) {cpu -> pc = addr; return 16;} return 12;}

// JP a16
uint8_t op_C3(CPU * cpu, Memory * mem) {uint16_t addr = get_imm16(cpu, mem); cpu -> pc = addr; return 16;}

// CALL NZ, a16
uint8_t op_C4(CPU * cpu, Memory * mem) {return op_call(cpu, mem, !get_zero_flag(cpu));}

// PUSH BC
uint8_t op_C5(CPU * cpu, Memory * mem) {push16(cpu, mem, get_bc(cpu)); return 16;}

// ADD A, d8
uint8_t op_C6(CPU * cpu, Memory * mem) {op_add(cpu, get_imm8(cpu, mem)); return 8;}

// RST 0
uint8_t op_C7(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x00; return 4;}

// RET Z
uint8_t op_C8(CPU * cpu, Memory * mem) {return op_ret(cpu, mem, get_zero_flag(cpu));}

// RET
uint8_t op_C9(CPU * cpu, Memory * mem) {cpu -> pc = pop16(cpu, mem); return 16;}

// JP Z, a16
uint8_t op_CA(CPU * cpu, Memory * mem) {uint16_t addr = get_imm16(cpu, mem); if(get_zero_flag(cpu)) {cpu -> pc = addr; return 16;} return 12;}

// CALL Z, a16
uint8_t op_CC(CPU * cpu, Memory * mem) {return op_call(cpu, mem, get_zero_flag(cpu));}

// CALL a16
uint8_t op_CD(CPU * cpu, Memory * mem) {return op_call(cpu, mem, 1);}

// ADC A, d8
uint8_t op_CE(CPU * cpu, Memory * mem) {op_adc(cpu, get_imm8(cpu, mem)); return 8;}

// RST 1
uint8_t op_CF(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x08; return 16;}

// RET NC
uint8_t op_D0(CPU * cpu, Memory * mem) {return op_ret(cpu, mem, !get_carry_flag(cpu));}

// POP DE
uint8_t op_D1(CPU * cpu, Memory * mem) {set_de(cpu, pop16(cpu, mem)); return 12;}

// JP NC, a16
uint8_t op_D2(CPU * cpu, Memory * mem) {uint16_t addr = get_imm16(cpu, mem); if(!get_carry_flag(cpu)) {cpu -> pc = addr; return 16;} return 12;}

// CALL NC, a16
uint8_t op_D4(CPU * cpu, Memory * mem) {return op_call(cpu, mem, !get_carry_flag(cpu));}

// PUSH DE
uint8_t op_D5(CPU * cpu, Memory * mem) {push16(cpu, mem, get_de(cpu)); return 16;}

// SUB d8
uint8_t op_D6(CPU * cpu, Memory * mem) {op_sub(cpu, get_imm8(cpu, mem)); return 8;}

// RST 2
uint8_t op_D7(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x10; return 4;}

// RET C
uint8_t op_D8(CPU * cpu, Memory * mem) {return op_ret(cpu, mem, get_carry_flag(cpu));}

// RETI
uint8_t op_D9(CPU * cpu, Memory * mem) {cpu -> pc = pop16(cpu, mem); cpu -> ime = 1; return 16;}

// JP C, a16
uint8_t op_DA(CPU * cpu, Memory * mem) {uint16_t addr = get_imm16(cpu, mem); if(get_carry_flag(cpu)) {cpu -> pc = addr; return 16;} return 12;}

// CALL C, a16
uint8_t op_DC(CPU * cpu, Memory * mem) {(void)mem; return op_call(cpu, mem, get_carry_flag(cpu));}

// SBC A, d8
uint8_t op_DE(CPU * cpu, Memory * mem) {op_sbc(cpu, get_imm8(cpu, mem)); return 8;}

// RST 3
uint8_t op_DF(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x18; return 16;}

// LD (a8), A
uint8_t op_E0(CPU * cpu, Memory * mem) {mem_write8(mem, 0xFF00 | get_imm8(cpu, mem), cpu -> a); return 12;}

// POP HL
uint8_t op_E1(CPU * cpu, Memory * mem) {set_hl(cpu, pop16(cpu, mem)); return 12;}

// LD (C), A
uint8_t op_E2(CPU * cpu, Memory * mem) {mem_write8(mem, 0xFF00 | cpu->c, cpu->a); return 8;}

// PUSH HL
uint8_t op_E5(CPU * cpu, Memory * mem) {push16(cpu, mem, get_hl(cpu)); return 16;}

// AND d8
uint8_t op_E6(CPU * cpu, Memory * mem) {op_and(cpu, get_imm8(cpu, mem)); return 8;}

// RST 4
uint8_t op_E7(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x20; return 4;}

// ADD SP, s8
uint8_t op_E8(CPU *cpu, Memory *mem) {
    int8_t src = (int8_t)get_imm8(cpu, mem);
    uint16_t sp = cpu->sp;

    uint16_t result = sp + src;
    cpu -> sp = result;
    uint8_t sp_lo = sp & 0xFF;
    uint8_t u = (uint8_t)src;

    set_zero_flag(cpu, 0);
    set_subtract_flag(cpu, 0);

    set_half_carry_flag(cpu, ((sp_lo & 0x0F) + (u & 0x0F)) > 0x0F);
    set_carry_flag(cpu, (sp_lo + u) > 0xFF);

    return 16;
}

// JP HL
uint8_t op_E9(CPU * cpu, Memory * mem) {(void)mem; cpu -> pc = get_hl(cpu); return 4;}

// LD (a16), A
uint8_t op_EA(CPU * cpu, Memory * mem) {mem_write8(mem, get_imm16(cpu, mem), cpu -> a); return 16;}

// XOR d8
uint8_t op_EE(CPU * cpu, Memory * mem) {op_xor(cpu, get_imm8(cpu, mem)); return 8;}

// RST 5
uint8_t op_EF(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x28; return 16;}

// LD A, (a8)
uint8_t op_F0(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> a, mem_read8(mem, 0xFF00 | get_imm8(cpu, mem))); return 12;}

// POP AF
uint8_t op_F1(CPU * cpu, Memory * mem) {set_af(cpu, pop16(cpu, mem)); return 12;}

// LD A, (C)
uint8_t op_F2(CPU * cpu, Memory * mem) {cpu -> a = mem_read8(mem, 0xFF00 | cpu -> c); return 8;}

// DI
uint8_t op_F3(CPU * cpu, Memory * mem) {(void)mem; cpu -> ime = 0; cpu -> ime_delay = 0; return 4;}

// PUSH AF
uint8_t op_F5(CPU * cpu, Memory * mem) {push16(cpu, mem, get_af(cpu)); return 16;}

// OR d8
uint8_t op_F6(CPU * cpu, Memory * mem) {op_or(cpu, get_imm8(cpu, mem)); return 8;}

// RST 6
uint8_t op_F7(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x30; return 4;}

// LD HL, SP+r8
uint8_t op_F8(CPU * cpu, Memory * mem) {
    int8_t offset = (int8_t)get_imm8(cpu, mem);
    uint16_t sp = cpu->sp;
    uint16_t result = sp + offset;

    set_hl(cpu, result);
    set_zero_flag(cpu, 0);
    set_subtract_flag(cpu, 0);

    uint16_t uoff = (uint16_t)(int16_t)offset;
    set_half_carry_flag(cpu, ((sp & 0x0F) + (uoff & 0x0F)) > 0x0F);
    set_carry_flag(cpu, ((sp & 0xFF) + (uoff & 0xFF)) > 0xFF);

    return 12;
}

// LD SP, HL
uint8_t op_F9(CPU * cpu, Memory * mem) {(void)mem; cpu -> sp = get_hl(cpu); return 8;}

// LD A, (a16)
uint8_t op_FA(CPU * cpu, Memory * mem) {op_ld_r_n(&cpu -> a, mem_read8(mem, get_imm16(cpu, mem))); return 16;}

// EI
uint8_t op_FB(CPU * cpu, Memory * mem) {(void)mem; cpu -> ime_delay = 1; return 4;}

// CP d8
uint8_t op_FE(CPU * cpu, Memory * mem) {op_cp(cpu, get_imm8(cpu, mem)); return 8;}

// RST 7
uint8_t op_FF(CPU * cpu, Memory * mem) {push16(cpu, mem, cpu -> pc); cpu -> pc = 0x38; return 16;}


/*
=========================================
   GENERIC CB OPCODE DEFINITIONS START
=========================================
*/

/*
cb_rlc

Rotate [src] to the left. Bit 7 of [src] is copied to CY and bit 0 of [src].
Flags: Z 0 0 src.7
*/
uint8_t cb_rlc(CPU *cpu, uint8_t src) {
    uint8_t result = (src << 1) | (src >> 7);
    
    set_zero_flag(cpu, result == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, src & 0x80);
    return result;
}

/*
cb_rrc

Rotate [src] to the right. Bit 0 of [src] is copied to CY and bit 7 of [src].
Flags: Z 0 0 src.0
*/
uint8_t cb_rrc(CPU *cpu, uint8_t src) {
    uint8_t result = (src << 7) | (src >> 1);
    
    set_zero_flag(cpu, result == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, src & 0x01);
    return result;
}

/*
cb_rl

Rotate [src] to the left. CY is copied to bit 0 of [src], and bit 7 of [src] is copied to CY.
Flags: Z 0 0 src.7
*/
void cb_rl(CPU *cpu, uint8_t * src)
{
    uint8_t old_carry = get_carry_flag(cpu);
    uint8_t new_carry = (*src >> 7) & 0x01;

    *src = (*src << 1) | old_carry;

    set_zero_flag(cpu, *src == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, new_carry);
}

/*
cb_rr

Rotate [src] to the right. CY is copied to bit 7 of [src], and bit 0 of [src] is copied to CY.
Flags: Z 0 0 src.0
*/
void cb_rr(CPU * cpu, uint8_t * src)
{
    uint8_t old_carry = get_carry_flag(cpu);
    uint8_t new_carry = *src & 0x01;

    *src = (old_carry << 7) | (*src >> 1);

    set_zero_flag(cpu, *src == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, new_carry);
}

/*
cb_sla

Copy bit 7 of [src] to CY and left shift the contents of [src] by 1.
Flags: Z 0 0 src.7
*/
void cb_sla(CPU * cpu, uint8_t * src) {
    uint8_t msb = (*src & 0x80) >> 7;
    *src <<= 1;

    set_zero_flag(cpu, *src == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, msb);
}

/*
cb_sra

Copy bit 0 of [src] to CY and right shift the contents of [src] by 1, keeping bit 7 intact.
Flags: Z 0 0 src.0
*/
void cb_sra(CPU * cpu, uint8_t * src) {
    uint8_t lsb = *src & 0x01;
    uint8_t msb = *src & 0x80;

    *src = msb | (*src >> 1);

    set_zero_flag(cpu, *src == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, lsb);
}

/*
cb_swap

Swap the low and high nibbles of [src].
Flags: Z 0 0 0
*/
void cb_swap(CPU * cpu, uint8_t * src) {
    *src = (*src & 0xF0) >> 4 | (*src & 0x0F) << 4;

    set_zero_flag(cpu, *src == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, 0);
}

/*
cb_srl

Copy bit 0 of [src] to CY and right shift the contents of [src] by 1, setting bit 7 to 0.
Flags: Z 0 0 src.0
*/
void cb_srl(CPU * cpu, uint8_t * src) {
    uint8_t lsb = *src & 0x01;
    *src >>= 1;

    set_zero_flag(cpu, *src == 0);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 0);
    set_carry_flag(cpu, lsb);
}

/*
cb_bit

Set the zero flag to the complement of bit [n] of [src].
Flags: !src.n 0 1 -
*/
void cb_bit(CPU * cpu, uint8_t src, uint8_t n) {
    uint8_t bit = (src >> n) & 1;
    set_zero_flag(cpu, !bit);
    set_subtract_flag(cpu, 0);
    set_half_carry_flag(cpu, 1);
}

/*
cb_res

Reset bit [n] of [src] to 0.
Flags: - - - -
*/
void cb_res(uint8_t * src, uint8_t n) {
    *src &= ~(1 << n);
}

/*
cb_set

Set bit [n] of [src] to 1.
Flags: - - - -
*/
void cb_set(uint8_t * src, uint8_t n) {
    *src |= (1 << n);
}

/*
=========================================
   CB-PREFIXED OPCODE DEFINITION START
=========================================
*/

// RLC B
uint8_t cb_00(CPU * cpu, Memory * mem) {(void)mem; cpu -> b = cb_rlc(cpu, cpu -> b); return 8;}

// RLC C
uint8_t cb_01(CPU * cpu, Memory * mem) {(void)mem; cpu -> c = cb_rlc(cpu, cpu -> c); return 8;}

// RLC D
uint8_t cb_02(CPU * cpu, Memory * mem) {(void)mem; cpu -> d = cb_rlc(cpu, cpu -> d); return 8;}

// RLC E
uint8_t cb_03(CPU * cpu, Memory * mem) {(void)mem; cpu -> e = cb_rlc(cpu, cpu -> e); return 8;}

// RLC H
uint8_t cb_04(CPU * cpu, Memory * mem) {(void)mem; cpu -> h = cb_rlc(cpu, cpu -> h); return 8;}

// RLC L
uint8_t cb_05(CPU * cpu, Memory * mem) {(void)mem; cpu -> l = cb_rlc(cpu, cpu -> l); return 8;}

// RLC (HL)
uint8_t cb_06(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cb_rlc(cpu, mem_read8(mem, get_hl(cpu)))); return 16;}

// RLC A
uint8_t cb_07(CPU * cpu, Memory * mem) {(void)mem; cpu -> a = cb_rlc(cpu, cpu -> a); return 8;}

// RRC B
uint8_t cb_08(CPU * cpu, Memory * mem) {(void)mem; cpu -> b = cb_rrc(cpu, cpu -> b); return 8;}

// RRC C
uint8_t cb_09(CPU * cpu, Memory * mem) {(void)mem; cpu -> c = cb_rrc(cpu, cpu -> c); return 8;}

// RRC D
uint8_t cb_0A(CPU * cpu, Memory * mem) {(void)mem; cpu -> d = cb_rrc(cpu, cpu -> d); return 8;}

// RRC E
uint8_t cb_0B(CPU * cpu, Memory * mem) {(void)mem; cpu -> e = cb_rrc(cpu, cpu -> e); return 8;}

// RRC H
uint8_t cb_0C(CPU * cpu, Memory * mem) {(void)mem; cpu -> h = cb_rrc(cpu, cpu -> h); return 8;}

// RRC L
uint8_t cb_0D(CPU * cpu, Memory * mem) {(void)mem; cpu -> l = cb_rrc(cpu, cpu -> l); return 8;}

// RRC (HL)
uint8_t cb_0E(CPU * cpu, Memory * mem) {mem_write8(mem, get_hl(cpu), cb_rrc(cpu, mem_read8(mem, get_hl(cpu)))); return 16;}

// RRC A
uint8_t cb_0F(CPU * cpu, Memory * mem) {(void)mem; cpu -> a = cb_rrc(cpu, cpu -> a); return 8;}

// RL B
uint8_t cb_10(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> b); return 8;}

// RL C
uint8_t cb_11(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> c); return 8;}

// RL D
uint8_t cb_12(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> d); return 8;}

// RL E
uint8_t cb_13(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> e); return 8;}

// RL H
uint8_t cb_14(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> h); return 8;}

// RL L
uint8_t cb_15(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> l); return 8;}

// RL (HL)
uint8_t cb_16(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_rl(cpu, &val); mem_write8(mem, addr, val); return 16;}

// RL A
uint8_t cb_17(CPU * cpu, Memory * mem) {(void)mem; cb_rl(cpu, &cpu -> a); return 8;}

// RR B
uint8_t cb_18(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> b); return 8;}

// RR C
uint8_t cb_19(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> c); return 8;}

// RR D
uint8_t cb_1A(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> d); return 8;}

// RR E
uint8_t cb_1B(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> e); return 8;}

// RR H
uint8_t cb_1C(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> h); return 8;}

// RR L
uint8_t cb_1D(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> l); return 8;}

// RR (HL)
uint8_t cb_1E(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_rr(cpu, &val); mem_write8(mem, addr, val); return 16;}

// RR A
uint8_t cb_1F(CPU * cpu, Memory * mem) {(void)mem; cb_rr(cpu, &cpu -> a); return 8;}

// SLA B
uint8_t cb_20(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> b); return 8;}

// SLA C
uint8_t cb_21(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> c); return 8;}

// SLA D
uint8_t cb_22(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> d); return 8;}

// SLA E
uint8_t cb_23(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> e); return 8;}

// SLA H
uint8_t cb_24(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> h); return 8;}

// SLA L
uint8_t cb_25(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> l); return 8;}

// SLA (HL)
uint8_t cb_26(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_sla(cpu, &val); mem_write8(mem, addr, val); return 16;}

// SLA A
uint8_t cb_27(CPU * cpu, Memory * mem) {(void)mem; cb_sla(cpu, &cpu -> a); return 8;}

// SRA B
uint8_t cb_28(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> b); return 8;}

// SRA C
uint8_t cb_29(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> c); return 8;}

// SRA D
uint8_t cb_2A(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> d); return 8;}

// SRA E
uint8_t cb_2B(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> e); return 8;}

// SRA H
uint8_t cb_2C(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> h); return 8;}

// SRA L
uint8_t cb_2D(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> l); return 8;}

// SRA (HL)
uint8_t cb_2E(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_sra(cpu, &val); mem_write8(mem, addr, val); return 16;}

// SRA A
uint8_t cb_2F(CPU * cpu, Memory * mem) {(void)mem; cb_sra(cpu, &cpu -> a); return 8;}

// SWAP B
uint8_t cb_30(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> b); return 8;}

// SWAP C
uint8_t cb_31(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> c); return 8;}

// SWAP D
uint8_t cb_32(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> d); return 8;}

// SWAP E
uint8_t cb_33(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> e); return 8;}

// SWAP H
uint8_t cb_34(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> h); return 8;}

// SWAP L
uint8_t cb_35(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> l); return 8;}

// SWAP (HL)
uint8_t cb_36(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_swap(cpu, &val); mem_write8(mem, addr, val); return 16;}

// SWAP A
uint8_t cb_37(CPU * cpu, Memory * mem) {(void)mem; cb_swap(cpu, &cpu -> a); return 8;}

// SRL B
uint8_t cb_38(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> b); return 8;}

// SRL C
uint8_t cb_39(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> c); return 8;}

// SRL D
uint8_t cb_3A(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> d); return 8;}

// SRL E
uint8_t cb_3B(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> e); return 8;}

// SRL H
uint8_t cb_3C(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> h); return 8;}

// SRL L
uint8_t cb_3D(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> l); return 8;}

// SRL (HL)
uint8_t cb_3E(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_srl(cpu, &val); mem_write8(mem, addr, val); return 16;}

// SRL A
uint8_t cb_3F(CPU * cpu, Memory * mem) {(void)mem; cb_srl(cpu, &cpu -> a); return 8;}

// BIT 0, B
uint8_t cb_40(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 0); return 8;}

// BIT 0, C
uint8_t cb_41(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 0); return 8;}

// BIT 0, D
uint8_t cb_42(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 0); return 8;}

// BIT 0, E
uint8_t cb_43(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 0); return 8;}

// BIT 0, H
uint8_t cb_44(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 0); return 8;}

// BIT 0, L
uint8_t cb_45(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 0); return 8;}

// BIT 0, (HL)
uint8_t cb_46(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 0); return 12;}

// BIT 0, A
uint8_t cb_47(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 0); return 8;}

// BIT 1, B
uint8_t cb_48(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 1); return 8;}

// BIT 1, C
uint8_t cb_49(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 1); return 8;}

// BIT 1, D
uint8_t cb_4A(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 1); return 8;}

// BIT 1, E
uint8_t cb_4B(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 1); return 8;}

// BIT 1, H
uint8_t cb_4C(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 1); return 8;}

// BIT 1, L
uint8_t cb_4D(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 1); return 8;}

// BIT 1, (HL)
uint8_t cb_4E(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 1); return 12;}

// BIT 1, A
uint8_t cb_4F(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 1); return 8;}

// BIT 2, B
uint8_t cb_50(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 2); return 8;}

// BIT 2, C
uint8_t cb_51(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 2); return 8;}

// BIT 2, D
uint8_t cb_52(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 2); return 8;}

// BIT 2, E
uint8_t cb_53(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 2); return 8;}

// BIT 2, H
uint8_t cb_54(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 2); return 8;}

// BIT 2, L
uint8_t cb_55(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 2); return 8;}

// BIT 2, (HL)
uint8_t cb_56(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 2); return 12;}

// BIT 2, A
uint8_t cb_57(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 2); return 8;}

// BIT 3, B
uint8_t cb_58(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 3); return 8;}

// BIT 3, C
uint8_t cb_59(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 3); return 8;}

// BIT 3, D
uint8_t cb_5A(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 3); return 8;}

// BIT 3, E
uint8_t cb_5B(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 3); return 8;}

// BIT 3, H
uint8_t cb_5C(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 3); return 8;}

// BIT 3, L
uint8_t cb_5D(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 3); return 8;}

// BIT 3, (HL)
uint8_t cb_5E(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 3); return 12;}

// BIT 3, A
uint8_t cb_5F(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 3); return 8;}

// BIT 4, B
uint8_t cb_60(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 4); return 8;}

// BIT 4, C
uint8_t cb_61(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 4); return 8;}

// BIT 4, D
uint8_t cb_62(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 4); return 8;}

// BIT 4, E
uint8_t cb_63(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 4); return 8;}

// BIT 4, H
uint8_t cb_64(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 4); return 8;}

// BIT 4, L
uint8_t cb_65(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 4); return 8;}

// BIT 4, (HL)
uint8_t cb_66(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 4); return 12;}

// BIT 4, A
uint8_t cb_67(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 4); return 8;}

// BIT 5, B
uint8_t cb_68(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 5); return 8;}

// BIT 5, C
uint8_t cb_69(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 5); return 8;}

// BIT 5, D
uint8_t cb_6A(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 5); return 8;}

// BIT 5, E
uint8_t cb_6B(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 5); return 8;}

// BIT 5, H
uint8_t cb_6C(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 5); return 8;}

// BIT 5, L
uint8_t cb_6D(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 5); return 8;}

// BIT 5, (HL)
uint8_t cb_6E(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 5); return 12;}

// BIT 5, A
uint8_t cb_6F(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 5); return 8;}

// BIT 6, B
uint8_t cb_70(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 6); return 8;}

// BIT 6, C
uint8_t cb_71(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 6); return 8;}

// BIT 6, D
uint8_t cb_72(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 6); return 8;}

// BIT 6, E
uint8_t cb_73(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 6); return 8;}

// BIT 6, H
uint8_t cb_74(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 6); return 8;}

// BIT 6, L
uint8_t cb_75(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 6); return 8;}

// BIT 6, (HL)
uint8_t cb_76(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 6); return 12;}

// BIT 6, A
uint8_t cb_77(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 6); return 8;}

// BIT 7, B
uint8_t cb_78(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> b, 7); return 8;}

// BIT 7, C
uint8_t cb_79(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> c, 7); return 8;}

// BIT 7, D
uint8_t cb_7A(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> d, 7); return 8;}

// BIT 7, E
uint8_t cb_7B(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> e, 7); return 8;}

// BIT 7, H
uint8_t cb_7C(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> h, 7); return 8;}

// BIT 7, L
uint8_t cb_7D(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> l, 7); return 8;}

// BIT 7, (HL)
uint8_t cb_7E(CPU * cpu, Memory * mem) {uint8_t val = mem_read8(mem, get_hl(cpu)); cb_bit(cpu, val, 7); return 12;}

// BIT 7, A
uint8_t cb_7F(CPU * cpu, Memory * mem) {(void)mem; cb_bit(cpu, cpu -> a, 7); return 8;}

// RES 0, B
uint8_t cb_80(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 0); return 8;}

// RES 0, C
uint8_t cb_81(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 0); return 8;}

// RES 0, D
uint8_t cb_82(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 0); return 8;}

// RES 0, E
uint8_t cb_83(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 0); return 8;}

// RES 0, H
uint8_t cb_84(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 0); return 8;}

// RES 0, L
uint8_t cb_85(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 0); return 8;}

// RES 0, (HL)
uint8_t cb_86(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 0); mem_write8(mem, addr, val); return 16;}

// RES 0, A
uint8_t cb_87(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 0); return 8;}

// RES 1, B
uint8_t cb_88(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 1); return 8;}

// RES 1, C
uint8_t cb_89(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 1); return 8;}

// RES 1, D
uint8_t cb_8A(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 1); return 8;}

// RES 1, E
uint8_t cb_8B(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 1); return 8;}

// RES 1, H
uint8_t cb_8C(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 1); return 8;}

// RES 1, L
uint8_t cb_8D(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 1); return 8;}

// RES 1, (HL)
uint8_t cb_8E(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 1); mem_write8(mem, addr, val); return 16;}

// RES 1, A
uint8_t cb_8F(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 1); return 8;}

// RES 2, B
uint8_t cb_90(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 2); return 8;}

// RES 2, C
uint8_t cb_91(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 2); return 8;}

// RES 2, D
uint8_t cb_92(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 2); return 8;}

// RES 2, E
uint8_t cb_93(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 2); return 8;}

// RES 2, H
uint8_t cb_94(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 2); return 8;}

// RES 2, L
uint8_t cb_95(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 2); return 8;}

// RES 2, (HL)
uint8_t cb_96(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 2); mem_write8(mem, addr, val); return 16;}

// RES 2, A
uint8_t cb_97(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 2); return 8;}

// RES 3, B
uint8_t cb_98(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 3); return 8;}

// RES 3, C
uint8_t cb_99(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 3); return 8;}

// RES 3, D
uint8_t cb_9A(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 3); return 8;}

// RES 3, E
uint8_t cb_9B(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 3); return 8;}

// RES 3, H
uint8_t cb_9C(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 3); return 8;}

// RES 3, L
uint8_t cb_9D(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 3); return 8;}

// RES 3, (HL)
uint8_t cb_9E(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 3); mem_write8(mem, addr, val); return 16;}

// RES 3, A
uint8_t cb_9F(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 3); return 8;}

// RES 4, B
uint8_t cb_A0(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 4); return 8;}

// RES 4, C
uint8_t cb_A1(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 4); return 8;}

// RES 4, D
uint8_t cb_A2(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 4); return 8;}

// RES 4, E
uint8_t cb_A3(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 4); return 8;}

// RES 4, H
uint8_t cb_A4(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 4); return 8;}

// RES 4, L
uint8_t cb_A5(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 4); return 8;}

// RES 4, (HL)
uint8_t cb_A6(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 4); mem_write8(mem, addr, val); return 16;}

// RES 4, A
uint8_t cb_A7(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 4); return 8;}

// RES 5, B
uint8_t cb_A8(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 5); return 8;}

// RES 5, C
uint8_t cb_A9(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 5); return 8;}

// RES 5, D
uint8_t cb_AA(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 5); return 8;}

// RES 5, E
uint8_t cb_AB(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 5); return 8;}

// RES 5, H
uint8_t cb_AC(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 5); return 8;}

// RES 5, L
uint8_t cb_AD(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 5); return 8;}

// RES 5, (HL)
uint8_t cb_AE(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 5); mem_write8(mem, addr, val); return 16;}

// RES 5, A
uint8_t cb_AF(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 5); return 8;}

// RES 6, B
uint8_t cb_B0(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 6); return 8;}

// RES 6, C
uint8_t cb_B1(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 6); return 8;}

// RES 6, D
uint8_t cb_B2(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 6); return 8;}

// RES 6, E
uint8_t cb_B3(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 6); return 8;}

// RES 6, H
uint8_t cb_B4(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 6); return 8;}

// RES 6, L
uint8_t cb_B5(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 6); return 8;}

// RES 6, (HL)
uint8_t cb_B6(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 6); mem_write8(mem, addr, val); return 16;}

// RES 6, A
uint8_t cb_B7(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 6); return 8;}

// RES 7, B
uint8_t cb_B8(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> b, 7); return 8;}

// RES 7, C
uint8_t cb_B9(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> c, 7); return 8;}

// RES 7, D
uint8_t cb_BA(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> d, 7); return 8;}

// RES 7, E
uint8_t cb_BB(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> e, 7); return 8;}

// RES 7, H
uint8_t cb_BC(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> h, 7); return 8;}

// RES 7, L
uint8_t cb_BD(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> l, 7); return 8;}

// RES 7, (HL)
uint8_t cb_BE(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_res(&val, 7); mem_write8(mem, addr, val); return 16;}

// RES 7, A
uint8_t cb_BF(CPU * cpu, Memory * mem) {(void)mem; cb_res(&cpu -> a, 7); return 8;}

// SET 0, B
uint8_t cb_C0(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 0); return 8;}

// SET 0, C
uint8_t cb_C1(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 0); return 8;}

// SET 0, D
uint8_t cb_C2(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 0); return 8;}

// SET 0, E
uint8_t cb_C3(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 0); return 8;}

// SET 0, H
uint8_t cb_C4(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 0); return 8;}

// SET 0, L
uint8_t cb_C5(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 0); return 8;}

// SET 0, (HL)
uint8_t cb_C6(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 0); mem_write8(mem, addr, val); return 16;}

// SET 0, A
uint8_t cb_C7(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 0); return 8;}

// SET 1, B
uint8_t cb_C8(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 1); return 8;}

// SET 1, C
uint8_t cb_C9(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 1); return 8;}

// SET 1, D
uint8_t cb_CA(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 1); return 8;}

// SET 1, E
uint8_t cb_CB(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 1); return 8;}

// SET 1, H
uint8_t cb_CC(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 1); return 8;}

// SET 1, L
uint8_t cb_CD(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 1); return 8;}

// SET 1, (HL)
uint8_t cb_CE(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 1); mem_write8(mem, addr, val); return 16;}

// SET 1, A
uint8_t cb_CF(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 1); return 8;}

// SET 2, B
uint8_t cb_D0(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 2); return 8;}

// SET 2, C
uint8_t cb_D1(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 2); return 8;}

// SET 2, D
uint8_t cb_D2(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 2); return 8;}

// SET 2, E
uint8_t cb_D3(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 2); return 8;}

// SET 2, H
uint8_t cb_D4(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 2); return 8;}

// SET 2, L
uint8_t cb_D5(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 2); return 8;}

// SET 2, (HL)
uint8_t cb_D6(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 2); mem_write8(mem, addr, val); return 16;}

// SET 2, A
uint8_t cb_D7(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 2); return 8;}

// SET 3, B
uint8_t cb_D8(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 3); return 8;}

// SET 3, C
uint8_t cb_D9(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 3); return 8;}

// SET 3, D
uint8_t cb_DA(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 3); return 8;}

// SET 3, E
uint8_t cb_DB(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 3); return 8;}

// SET 3, H
uint8_t cb_DC(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 3); return 8;}

// SET 3, L
uint8_t cb_DD(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 3); return 8;}

// SET 3, (HL)
uint8_t cb_DE(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 3); mem_write8(mem, addr, val); return 16;}

// SET 3, A
uint8_t cb_DF(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 3); return 8;}

// SET 4, B
uint8_t cb_E0(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 4); return 8;}

// SET 4, C
uint8_t cb_E1(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 4); return 8;}

// SET 4, D
uint8_t cb_E2(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 4); return 8;}

// SET 4, E
uint8_t cb_E3(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 4); return 8;}

// SET 4, H
uint8_t cb_E4(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 4); return 8;}

// SET 4, L
uint8_t cb_E5(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 4); return 8;}

// SET 4, (HL)
uint8_t cb_E6(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 4); mem_write8(mem, addr, val); return 16;}

// SET 4, A
uint8_t cb_E7(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 4); return 8;}

// SET 5, B
uint8_t cb_E8(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 5); return 8;}

// SET 5, C
uint8_t cb_E9(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 5); return 8;}

// SET 5, D
uint8_t cb_EA(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 5); return 8;}

// SET 5, E
uint8_t cb_EB(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 5); return 8;}

// SET 5, H
uint8_t cb_EC(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 5); return 8;}

// SET 5, L
uint8_t cb_ED(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 5); return 8;}

// SET 5, (HL)
uint8_t cb_EE(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 5); mem_write8(mem, addr, val); return 16;}

// SET 5, A
uint8_t cb_EF(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 5); return 8;}

// SET 6, B
uint8_t cb_F0(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 6); return 8;}

// SET 6, C
uint8_t cb_F1(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 6); return 8;}

// SET 6, D
uint8_t cb_F2(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 6); return 8;}

// SET 6, E
uint8_t cb_F3(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 6); return 8;}

// SET 6, H
uint8_t cb_F4(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 6); return 8;}

// SET 6, L
uint8_t cb_F5(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 6); return 8;}

// SET 6, (HL)
uint8_t cb_F6(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 6); mem_write8(mem, addr, val); return 16;}

// SET 6, A
uint8_t cb_F7(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 6); return 8;}

// SET 7, B
uint8_t cb_F8(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> b, 7); return 8;}

// SET 7, C
uint8_t cb_F9(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> c, 7); return 8;}

// SET 7, D
uint8_t cb_FA(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> d, 7); return 8;}

// SET 7, E
uint8_t cb_FB(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> e, 7); return 8;}

// SET 7, H
uint8_t cb_FC(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> h, 7); return 8;}

// SET 7, L
uint8_t cb_FD(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> l, 7); return 8;}

// SET 7, (HL)
uint8_t cb_FE(CPU * cpu, Memory * mem) {uint16_t addr = get_hl(cpu); uint8_t val = mem_read8(mem, addr); cb_set(&val, 7); mem_write8(mem, addr, val); return 16;}

// SET 7, A
uint8_t cb_FF(CPU * cpu, Memory * mem) {(void)mem; cb_set(&cpu -> a, 7); return 8;}

/*
=================================
   OPCODE INITIALIZATION START
=================================
*/

opcode_fn opcode_table[NUM_OPCODES] = 
{
    op_00, op_01, op_02, op_03, op_04, op_05, op_06, op_07, op_08, op_09, op_0A, op_0B, op_0C, op_0D, op_0E, op_0F,
    NULL , op_11, op_12, op_13, op_14, op_15, op_16, op_17, op_18, op_19, op_1A, op_1B, op_1C, op_1D, op_1E, op_1F,
    op_20, op_21, op_22, op_23, op_24, op_25, op_26, op_27, op_28, op_29, op_2A, op_2B, op_2C, op_2D, op_2E, op_2F,
    op_30, op_31, op_32, op_33, op_34, op_35, op_36, op_37, op_38, op_39, op_3A, op_3B, op_3C, op_3D, op_3E, op_3F,
    op_40, op_41, op_42, op_43, op_44, op_45, op_46, op_47, op_48, op_49, op_4A, op_4B, op_4C, op_4D, op_4E, op_4F,
    op_50, op_51, op_52, op_53, op_54, op_55, op_56, op_57, op_58, op_59, op_5A, op_5B, op_5C, op_5D, op_5E, op_5F,
    op_60, op_61, op_62, op_63, op_64, op_65, op_66, op_67, op_68, op_69, op_6A, op_6B, op_6C, op_6D, op_6E, op_6F,
    op_70, op_71, op_72, op_73, op_74, op_75, op_76, op_77, op_78, op_79, op_7A, op_7B, op_7C, op_7D, op_7E, op_7F,
    op_80, op_81, op_82, op_83, op_84, op_85, op_86, op_87, op_88, op_89, op_8A, op_8B, op_8C, op_8D, op_8E, op_8F,
    op_90, op_91, op_92, op_93, op_94, op_95, op_96, op_97, op_98, op_99, op_9A, op_9B, op_9C, op_9D, op_9E, op_9F,
    op_A0, op_A1, op_A2, op_A3, op_A4, op_A5, op_A6, op_A7, op_A8, op_A9, op_AA, op_AB, op_AC, op_AD, op_AE, op_AF,
    op_B0, op_B1, op_B2, op_B3, op_B4, op_B5, op_B6, op_B7, op_B8, op_B9, op_BA, op_BB, op_BC, op_BD, op_BE, op_BF,
    op_C0, op_C1, op_C2, op_C3, op_C4, op_C5, op_C6, op_C7, op_C8, op_C9, op_CA, NULL , op_CC, op_CD, op_CE, op_CF,
    op_D0, op_D1, op_D2, NULL , op_D4, op_D5, op_D6, op_D7, op_D8, op_D9, op_DA, NULL , op_DC, NULL , op_DE, op_DF,
    op_E0, op_E1, op_E2, NULL , NULL , op_E5, op_E6, op_E7, op_E8, op_E9, op_EA, NULL , NULL , NULL , op_EE, op_EF,
    op_F0, op_F1, op_F2, op_F3, NULL , op_F5, op_F6, op_F7, op_F8, op_F9, op_FA, op_FB, NULL , NULL , op_FE, op_FF
};

opcode_fn cb_opcode_table[NUM_OPCODES] = 
{
    cb_00, cb_01, cb_02, cb_03, cb_04, cb_05, cb_06, cb_07, cb_08, cb_09, cb_0A, cb_0B, cb_0C, cb_0D, cb_0E, cb_0F,
    cb_10, cb_11, cb_12, cb_13, cb_14, cb_15, cb_16, cb_17, cb_18, cb_19, cb_1A, cb_1B, cb_1C, cb_1D, cb_1E, cb_1F,
    cb_20, cb_21, cb_22, cb_23, cb_24, cb_25, cb_26, cb_27, cb_28, cb_29, cb_2A, cb_2B, cb_2C, cb_2D, cb_2E, cb_2F,
    cb_30, cb_31, cb_32, cb_33, cb_34, cb_35, cb_36, cb_37, cb_38, cb_39, cb_3A, cb_3B, cb_3C, cb_3D, cb_3E, cb_3F,
    cb_40, cb_41, cb_42, cb_43, cb_44, cb_45, cb_46, cb_47, cb_48, cb_49, cb_4A, cb_4B, cb_4C, cb_4D, cb_4E, cb_4F,
    cb_50, cb_51, cb_52, cb_53, cb_54, cb_55, cb_56, cb_57, cb_58, cb_59, cb_5A, cb_5B, cb_5C, cb_5D, cb_5E, cb_5F,
    cb_60, cb_61, cb_62, cb_63, cb_64, cb_65, cb_66, cb_67, cb_68, cb_69, cb_6A, cb_6B, cb_6C, cb_6D, cb_6E, cb_6F,
    cb_70, cb_71, cb_72, cb_73, cb_74, cb_75, cb_76, cb_77, cb_78, cb_79, cb_7A, cb_7B, cb_7C, cb_7D, cb_7E, cb_7F,
    cb_80, cb_81, cb_82, cb_83, cb_84, cb_85, cb_86, cb_87, cb_88, cb_89, cb_8A, cb_8B, cb_8C, cb_8D, cb_8E, cb_8F,
    cb_90, cb_91, cb_92, cb_93, cb_94, cb_95, cb_96, cb_97, cb_98, cb_99, cb_9A, cb_9B, cb_9C, cb_9D, cb_9E, cb_9F,
    cb_A0, cb_A1, cb_A2, cb_A3, cb_A4, cb_A5, cb_A6, cb_A7, cb_A8, cb_A9, cb_AA, cb_AB, cb_AC, cb_AD, cb_AE, cb_AF,
    cb_B0, cb_B1, cb_B2, cb_B3, cb_B4, cb_B5, cb_B6, cb_B7, cb_B8, cb_B9, cb_BA, cb_BB, cb_BC, cb_BD, cb_BE, cb_BF,
    cb_C0, cb_C1, cb_C2, cb_C3, cb_C4, cb_C5, cb_C6, cb_C7, cb_C8, cb_C9, cb_CA, cb_CB, cb_CC, cb_CD, cb_CE, cb_CF,
    cb_D0, cb_D1, cb_D2, cb_D3, cb_D4, cb_D5, cb_D6, cb_D7, cb_D8, cb_D9, cb_DA, cb_DB, cb_DC, cb_DD, cb_DE, cb_DF,
    cb_E0, cb_E1, cb_E2, cb_E3, cb_E4, cb_E5, cb_E6, cb_E7, cb_E8, cb_E9, cb_EA, cb_EB, cb_EC, cb_ED, cb_EE, cb_EF,
    cb_F0, cb_F1, cb_F2, cb_F3, cb_F4, cb_F5, cb_F6, cb_F7, cb_F8, cb_F9, cb_FA, cb_FB, cb_FC, cb_FD, cb_FE, cb_FF
};
