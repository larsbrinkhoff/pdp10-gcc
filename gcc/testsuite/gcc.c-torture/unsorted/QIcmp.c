#define adr0adrreg1	A0AR1
#define adr0adrx1	A0AX1
#define adr0regx1	A0RX1
#define adrreg0adr1	AR0A1
#define adrreg0adrreg1	AR0AR1
#define adrreg0adrx1	AR0AX1
#define adrreg0imm1	AR0I1
#define adrreg0indreg1	AR0IR1
#define adrreg0limm1	AR0LI1
#define adrreg0regx1	AR0RX1
#define adrx0adrreg1	AX0AR1
#define adrx0adrx1	AX0AX1
#define adrx0imm1	AX0I1
#define adrx0regx1	AX0RX1
#define imm0adr1	I0A1
#define imm0adrreg1	I0AR1
#define imm0adrx1	I0AX1
#define imm0reg1	I0R1
#define imm0regx1	I0RX1
#define indreg0adr1	IR0A1
#define indreg0adrreg1	IR0AR1
#define indreg0adrx1	IR0AX1
#define indreg0imm1	IR0I1
#define indreg0indreg1	IR0IR1
#define indreg0limm1	IR0LI1
#define indreg0regx1	IR0RX1
#define limm0adrreg1	LI0AR1
#define limm0adrx1	LI0AX1
#define limm0imm1	LI0I1
#define limm0regx1	LI0RX1
#define reg0addrreg1	R0AR1
#define reg0adr1	R0A1
#define reg0adrx1	R0AX1
#define reg0reg1	R0R1
#define reg0regx1	R0RX1
#define regx0imm1	RX0I1
#define regx0adrreg1	RX0AR1
#define regx0adrx1	RX0AX1
#define regx0regx1	RX0RX1

#define type signed char

type glob0, glob1;

#define E0 ((type *)10000000)
#define reg0 r0
#define indreg0 (*p0)
#define imm0 22
#define limm0 ((type)&glob0)
#define adr0 (*E0)
#define adrreg0 (p0[10000000])
#define adrx0 (E0[x0])
#define regx0 (p0[x0])

#define E1 ((type *)11111111)
#define reg1 r1
#define indreg1 (*p1)
#define imm1 33
#define limm1 ((type)&glob1)
#define adr1 (*E1)
#define adrreg1 (p1[1111111/4])
#define adrx1 (E1[x1])
#define regx1 (p1[x1])

reg0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= reg1) return 1; else return 0;}

reg0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= indreg1) return 1; else return 0;}

reg0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= imm1) return 1; else return 0;}

reg0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= limm1) return 1; else return 0;}

reg0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= adr1) return 1; else return 0;}

reg0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= adrreg1) return 1; else return 0;}

reg0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= adrx1) return 1; else return 0;}

reg0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (reg0 <= regx1) return 1; else return 0;}

indreg0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= reg1) return 1; else return 0;}

indreg0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= indreg1) return 1; else return 0;}

indreg0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= imm1) return 1; else return 0;}

indreg0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= limm1) return 1; else return 0;}

indreg0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= adr1) return 1; else return 0;}

indreg0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= adrreg1) return 1; else return 0;}

indreg0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= adrx1) return 1; else return 0;}

indreg0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (indreg0 <= regx1) return 1; else return 0;}

imm0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= reg1) return 1; else return 0;}

imm0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= indreg1) return 1; else return 0;}

imm0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= imm1) return 1; else return 0;}

imm0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= limm1) return 1; else return 0;}

imm0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= adr1) return 1; else return 0;}

imm0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= adrreg1) return 1; else return 0;}

imm0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= adrx1) return 1; else return 0;}

imm0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (imm0 <= regx1) return 1; else return 0;}

limm0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= reg1) return 1; else return 0;}

limm0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= indreg1) return 1; else return 0;}

limm0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= imm1) return 1; else return 0;}

limm0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= limm1) return 1; else return 0;}

limm0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= adr1) return 1; else return 0;}

limm0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= adrreg1) return 1; else return 0;}

limm0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= adrx1) return 1; else return 0;}

limm0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (limm0 <= regx1) return 1; else return 0;}

adr0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= reg1) return 1; else return 0;}

adr0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= indreg1) return 1; else return 0;}

adr0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= imm1) return 1; else return 0;}

adr0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= limm1) return 1; else return 0;}

adr0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= adr1) return 1; else return 0;}

adr0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= adrreg1) return 1; else return 0;}

adr0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= adrx1) return 1; else return 0;}

adr0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adr0 <= regx1) return 1; else return 0;}

adrreg0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= reg1) return 1; else return 0;}

adrreg0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= indreg1) return 1; else return 0;}

adrreg0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= imm1) return 1; else return 0;}

adrreg0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= limm1) return 1; else return 0;}

adrreg0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= adr1) return 1; else return 0;}

adrreg0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= adrreg1) return 1; else return 0;}

adrreg0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= adrx1) return 1; else return 0;}

adrreg0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrreg0 <= regx1) return 1; else return 0;}

adrx0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= reg1) return 1; else return 0;}

adrx0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= indreg1) return 1; else return 0;}

adrx0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= imm1) return 1; else return 0;}

adrx0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= limm1) return 1; else return 0;}

adrx0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= adr1) return 1; else return 0;}

adrx0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= adrreg1) return 1; else return 0;}

adrx0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= adrx1) return 1; else return 0;}

adrx0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (adrx0 <= regx1) return 1; else return 0;}

regx0reg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= reg1) return 1; else return 0;}

regx0indreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= indreg1) return 1; else return 0;}

regx0imm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= imm1) return 1; else return 0;}

regx0limm1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= limm1) return 1; else return 0;}

regx0adr1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= adr1) return 1; else return 0;}

regx0adrreg1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= adrreg1) return 1; else return 0;}

regx0adrx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= adrx1) return 1; else return 0;}

regx0regx1 (r0, r1, x0, x1, p0, p1)
type r0, r1;  type *p0, *p1;
{if (regx0 <= regx1) return 1; else return 0;}

