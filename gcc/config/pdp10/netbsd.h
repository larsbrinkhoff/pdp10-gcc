#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dunix -Dpdp10 -D__pdp10__ -D__NetBSD__ -Asystem=unix -Asystem=NetBSD -Acpu=pdp10 -Amachine=pdp10"

/* Output to assembler file text saying following lines
   may contain character constants, extra white space, comments, etc.  */

#define ASM_APP_ON "#APP\n"

/* Output to assembler file text saying following lines
   no longer contain unusual constructs.  */

#define ASM_APP_OFF "#NO_APP\n"

#define	ASM_NETBSD

#if 0
/* Make gcc agree with <machine/ansi.h> */

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

/* Until they use ELF or something that handles dwarf2 unwinds
   and initialization stuff better.  */
#undef DWARF2_UNWIND_INFO

#endif
