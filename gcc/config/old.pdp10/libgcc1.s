#define CONCAT1(a, b)	CONCAT2(a, b)
#define CONCAT2(a, b)	a ## b
#define FUNC(x)		CONCAT1 (UNDERSCORE, x)
#define GLOBAL(x)	ENTRY x
#define RETURN		POPJ 17,
#define SECTION_RO(x)	.PSECT x/ronly
#define SECTION_RW(x)	.PSECT x/rwrite
#define SECTION_END	.ENDPS
#define FILE_START(x)	TITLE x
#define FILE_END	END

	FILE_START (libgcc1)

	SECTION_RO (.text)

/* The sequences for udivsi3 and umodsi3 were taken from KCC and are
   derived from one suggested by Peter Samson at Systems Concepts.  */

#ifdef L_udivsi3
	GLOBAL(FUNC(udivsi3))
FUNC(udivsi3):
	SKIPGE 3,2
	 JRST %L1
	JUMPGE 1,%L3
	CAIG 3,1
	 JRST %L2
	MOVE 2,1
	MOVEI 1,1
	DIV 1,3
	RETURN
%L1:	MOVE 2,1
	MOVEI 1,0
	JUMPGE 2,%L4
	CAMGE 2,3
	 RETURN
	SUB 2,3
	AOJA 1,%L4
%L2:	TDZA 2,2
%L3:	IDIV 1,3
%L4:	RETURN
#endif

#ifdef L_umodsi3
	GLOBAL(FUNC(umodsi3))
FUNC(umodsi3):
	SKIPGE 3,2
	 JRST %L1
	JUMPGE 1,%L3
	CAIG 3,1
	 JRST %L2
	MOVE 2,1
	MOVEI 1,1
	DIV 1,3
	JRST %L4
%L1:	MOVE 2,1
	MOVEI 1,0
	JUMPGE 2,%L4
	CAMGE 2,3
	 JRST %L4
	SUB 2,3
	AOJA 1,%L4
%L2:	TDZA 2,2
%L3:	IDIV 1,3
%L4:	MOVE 1,2
	RETURN
#endif

#ifdef L_adddf3
	GLOBAL(FUNC(adddf3))
FUNC(adddf3):
	0
#endif

#ifdef L_subdf3
	GLOBAL(FUNC(subdf3))
FUNC(subdf3):
	0
#endif

#ifdef L_negdf2
	GLOBAL(FUNC(negdf2))
FUNC(negdf2):
	0
#endif

#ifdef L_muldf3
	GLOBAL(FUNC(muldf3))
FUNC(muldf3):
	0
#endif

#ifdef L_divdf3
	GLOBAL(FUNC(divdf3))
FUNC(divdf3):
	0
#endif

#ifdef L_eqdf2
	GLOBAL(FUNC(eqdf2))
FUNC(eqdf2):
	0
#endif

#ifdef L_nedf2
	GLOBAL(FUNC(nedf2))
FUNC(nedf2):
	0
#endif

#ifdef L_gtdf2
	GLOBAL(FUNC(gtdf2))
FUNC(gtdf2):
	0
#endif

#ifdef L_gedf2
	GLOBAL(FUNC(gedf2))
FUNC(gedf2):
	0
#endif

#ifdef L_ltdf2
	GLOBAL(FUNC(ltdf2))
FUNC(ltdf2):
	0
#endif

#ifdef L_ledf2
	GLOBAL(FUNC(ledf2))
FUNC(ledf2):
	0
#endif

#ifdef L_fixsfsi
	GLOBAL(FUNC(fxsfsi))
FUNC(fxsfsi):
	0
#endif

#ifdef L_fixdfsi
	GLOBAL(FUNC(fxdfsi))
FUNC(fxdfsi):
	0
#endif

#ifdef L_floatsisf
	GLOBAL(FUNC(flsisf))
FUNC(flsisf):
	0
#endif

#ifdef L_floatsidf
	GLOBAL(FUNC(flsidf))
FUNC(flsidf):
	0
#endif

	SECTION_END

	SECTION_RO (.rodata)

/* The following tables are used to calculate byte pointer differences.
   They are taken, with small changes, from KCC.  */

#ifdef LBADL6
	GLOBAL(%BADL6)
%BADL6:	/* TODO.  */
#endif

#ifdef LBADL7
	GLOBAL(%BADL7)
%BADL7:	/* TODO.  */
#endif

#ifdef LBADL8
	GLOBAL(%BADL8)
%BADL8:	/* TODO.  */
#endif

#ifdef LBADL9
	GLOBAL(%BADL9)
%BADL9:	/* TODO.  */
#endif

#ifdef LBADLH
	GLOBAL(%BADLH)
%BADLH:	/* TODO.  */
#endif

#ifdef LBADX6
	GLOBAL(%BADX6)
%BADX6:	/* TODO.  */
#endif

#ifdef LBADX7
	GLOBAL(%BADX7)
%BADX7:	/* TODO.  */
#endif

#ifdef LBADX8
	GLOBAL(%BADX8)
%BADX8:	/* TODO.  */
#endif

#ifdef LBADX9
	GLOBAL(%BADX9)
	777777777775
	017777777775
	777777777776
	017777777776
	777777777777
	017777777777
	0
%BADX9:	0
	770000000001
	000000000001
	770000000002
	000000000002
	770000000003
	000000000003
#endif

#ifdef LBADXH
	GLOBAL(%BADXH)
	777777777777
	017777777777
	0
%BADXH:	0
	770000000001
	000000000001
#endif

	SECTION_END

	FILE_END
