/* XKL test code.  (C) XKL.  */

#ifdef	TOPS20
#pragma	no_covert_voidp
typedef	unsigned	byte8	__attribute__ ((size(8)));
#else
typedef	unsigned char	byte8;
#endif

typedef	unsigned char	qword;
typedef	unsigned short	hword;
typedef	unsigned long	fword;

int	item, base, line, code;
int	xv, xp8, xpv8, xp9, xpv9, xph, xpvh, xpf;
char*	text;

byte8 b, *bP = &b;
qword q, *qP = &q;
hword h, *hP = &h;
fword f, *fP = &f;

byte8 ba[48], *baP = &ba[0];
qword qa[48], *qaP = &qa[0];
hword ha[48], *haP = &ha[0];
fword fa[48], *faP = &fa[0];

struct	{ fword wd6[6]; byte8 b; fword wd1; byte8 ba[48]; } bs, *bsP = &bs;
struct	{ fword wd6[6]; qword q; fword wd1; qword qa[48]; } qs, *qsP = &qs;
struct	{ fword wd6[6]; hword h; fword wd1; hword ha[48]; } hs, *hsP = &hs;
struct	{ fword wd6[6]; fword f; fword wd1; fword fa[48]; } fs, *fsP = &fs;

union	{ fword wd6[6]; byte8 b; fword wd1; byte8 ba[48]; } bu, *buP = &bu;
union	{ fword wd6[6]; qword q; fword wd1; qword qa[48]; } qu, *quP = &qu;
union	{ fword wd6[6]; hword h; fword wd1; hword ha[48]; } hu, *huP = &hu;
union	{ fword wd6[6]; fword f; fword wd1; fword fa[48]; } fu, *fuP = &fu;

void* vbyte8() { byte8* p = &ba[0]; *p = base; return p; }
void* vqword() { qword* p = &qa[0]; *p = base; return p; }
void* vhword() { hword* p = &ha[0]; *p = base; return p; }
void* vfword() { fword* p = &fa[0]; *p = base; return p; }
void* vpassv(void* p)			     { return p; }

fail(int test)
{
#ifndef	TOPS20
  if ((code % 10) == 1)
  printf("%13s%s  %9X *8 %3X *v8 %3X *9 %3X *v9 %3X *h %5X *vh %5X\r\n",
	 text, item > 0 ? "[j]" : item == 0 ? "[0]" : "   ",
	 xpf, xp8, xpv8, xp9, xpv9, xph, xpvh);
  if (test) exit(line);
#else
  if (test)
    {
      printf ("Failed at line %d, code = %d\n", line, code);
      abort();
    }
#endif
}

extract(int lineno, char* string, int j, int size,
	fword x, byte8* p8, qword* p9, hword* ph, void* pv)
{
  printf ("extract (lineno=%d, string=%s, j=%d, size=%d, x=%d, p8=%p, p9=%p, ph=%p, pv=%p\n",
	  lineno, string, j, size, x, p8, p9, ph, pv);

  item = j;
  line = lineno;
  code = lineno * 10;
  text = string;

  if (item > 0) xv = base + (item * size); else xv = base;
  if (*string == 's') if (item >= 0) xv += 32; else xv += 24;
  printf ("item = %d, base = %d, size = %d, xv = %d\n", item, base, size, xv);

  xp8  = *p8;
  xpv8 = *(byte8*)pv;
  xp9  = *p9;
  xpv9 = *(qword*)pv;
  xph  = *ph;
  xpvh = *(hword*)pv;
  xpf  = x;

  if (item >= -1)
    {
      printf ("xp8 = %d\n", xp8);
      code++; fail(xv != xp8		);		/* 1 */
      /*printf ("xpv8 = %d\n", xpv8);*/
      code++; /*fail(xv != xpv8		);*/		/* 2 */
      printf ("xp9 = %d\n", xp9);
      code++; fail(xv != xp9		);		/* 3 */
      code++; /*fail(xv != xpv9		);*/		/* 4 */
#if 1
      printf ("xph = %d, xph >> 8 = %d\n", xph, xph >> 8);
      code++; fail(xv != (xph  >> 8)	);		/* 5 */
      printf ("xpvh = %d, xpvh >> 8 = %d\n", xpvh, xpvh >> 8);
      code++; fail(xv != (xpvh >> 8)	);		/* 6 */
      printf ("xpf = %d, xpf >> 24 = %d\n", xpf, xpf >> 24);
      code++; fail(xv != (xpf  >> 24)	);		/* 7 */
#else
      printf ("xph = %d, xph & 255 = %d\n", xph, xph & 255);
      code++; fail(xv != (xph  & 255)	);
      code++; fail(xv != (xpvh & 255)	);
      printf ("xpf = %d, xpf & 255 = %d\n", xpf, xpf & 255);
      code++; fail(xv != (xpf  & 255)	);
#endif
    }
}

void fill(int p, int n, int v)
{
  if (n > 256) exit(255);
  for (v += base; -- n >= 0; *((char *)p)++ = v++);
}

#define	FILL8(a,v)	fill(((int)(byte8*)&(a)),sizeof(a),v)
#define	FILLQ(a,v)	fill(((int)(qword*)&(a)),sizeof(a),v)
#define	VAL(s,j,v)	extract(__LINE__,s,j,sizeof(v),v,\
			(byte8*)&v,(qword*)&v,(hword*)&v,&v)
#define	PTR(s,j,v)	extract(__LINE__,s,j,sizeof(*v),*v,\
			(byte8*)v,(qword*)v,(hword*)v,v)

void run(int j)
{
  FILL8(b,  0);  FILLQ(q,  0);  FILLQ(h,  0);  FILLQ(f,  0);
  FILL8(ba, 0);  FILLQ(qa, 0);  FILLQ(ha, 0);  FILLQ(fa, 0);
  FILL8(bs, 0);  FILLQ(qs, 0);  FILLQ(hs, 0);  FILLQ(fs, 0);
  FILL8(bu, 0);  FILLQ(qu, 0);  FILLQ(hu, 0);  FILLQ(fu, 0);

  VAL(" byte8", -1, b);
  VAL(" qword", -1, q);
  VAL(" hword", -1, h);
  VAL(" fword", -1, f);
#if 0
  PTR("*byte8", -1, bP);
  printf ("qP = %p\n", qP);
  PTR("*qword", -1, qP);
  PTR("*hword", -1, hP);
  PTR("*fword", -1, fP);
#endif
  VAL(" byte8", 0, ba[0]);
  VAL(" qword", 0, qa[0]);
  VAL(" hword", 0, ha[0]);
  VAL(" fword", 0, fa[0]);
#if 0
  VAL(" byte8", j, ba[j]);
  VAL(" qword", j, qa[j]);
#endif
  VAL(" hword", j, ha[j]);
  VAL(" fword", j, fa[j]);
  VAL("*byte8", 0, baP[0]);
  VAL("*qword", 0, qaP[0]);
  VAL("*hword", 0, haP[0]);
  VAL("*fword", 0, faP[0]);
  VAL("*byte8", j, baP[j]);
  VAL("*qword", j, qaP[j]);
  VAL("*hword", j, haP[j]);
  VAL("*fword", j, faP[j]);

  VAL("union   byte8", -1, bu.b);
  VAL("union   qword", -1, qu.q);
  VAL("union   hword", -1, hu.h);
  VAL("union   fword", -1, fu.f);
  VAL("union   byte8",  0, bu.ba[0]);
  VAL("union   qword",  0, qu.qa[0]);
  VAL("union   hword",  0, hu.ha[0]);
  VAL("union   fword",  0, fu.fa[0]);
  VAL("union   byte8",  j, bu.ba[j]);
  VAL("union   qword",  j, qu.qa[j]);
  VAL("union   hword",  j, hu.ha[j]);
  VAL("union   fword",  j, fu.fa[j]);
  VAL("union  *byte8", -1, buP->b);
  VAL("union  *qword", -1, quP->q);
  VAL("union  *hword", -1, huP->h);
  VAL("union  *fword", -1, fuP->f);
  VAL("union  *byte8",  0, buP->ba[0]);
  VAL("union  *qword",  0, quP->qa[0]);
  VAL("union  *hword",  0, huP->ha[0]);
  VAL("union  *fword",  0, fuP->fa[0]);
  VAL("union  *byte8",  j, buP->ba[j]);
  VAL("union  *qword",  j, quP->qa[j]);
  VAL("union  *hword",  j, huP->ha[j]);
  VAL("union  *fword",  j, fuP->fa[j]);

  VAL("struct  byte8", -1, bs.b);
  VAL("struct  qword", -1, qs.q);
  VAL("struct  hword", -1, hs.h);
  VAL("struct  fword", -1, fs.f);
  VAL("struct  byte8",  0, bs.ba[0]);
  VAL("struct  qword",  0, qs.qa[0]);
  VAL("struct  hword",  0, hs.ha[0]);
  VAL("struct  fword",  0, fs.fa[0]);
  VAL("struct  byte8",  j, bs.ba[j]);
  VAL("struct  qword",  j, qs.qa[j]);
  VAL("struct  hword",  j, hs.ha[j]);
  VAL("struct  fword",  j, fs.fa[j]);
  VAL("struct *byte8", -1, bsP->b);
  VAL("struct *qword", -1, qsP->q);
  VAL("struct *hword", -1, hsP->h);
  VAL("struct *fword", -1, fsP->f);
  VAL("struct *byte8",  0, bsP->ba[0]);
  VAL("struct *qword",  0, qsP->qa[0]);
  VAL("struct *hword",  0, hsP->ha[0]);
  VAL("struct *fword",  0, fsP->fa[0]);
  VAL("struct *byte8",  j, bsP->ba[j]);
  VAL("struct *qword",  j, qsP->qa[j]);
  VAL("struct *hword",  j, hsP->ha[j]);
  VAL("struct *fword",  j, fsP->fa[j]);
}

main()
{
  line = __LINE__; fail(vpassv(bP) != bP);
  line = __LINE__; fail(vpassv(qP) != qP);
  line = __LINE__; fail(vpassv(hP) != hP);
  line = __LINE__; fail(vpassv(fP) != fP);

  for (base = 0; base < 48 ; base += 3)
  {
    line = __LINE__; fail(((*(bP = vbyte8())) & 255) != base);
    line = __LINE__; fail(((*(qP = vbyte8())) & 255) != base);
    line = __LINE__; fail(((*(hP = vbyte8())) & 255) != base);
    line = __LINE__; fail(((*(fP = vbyte8())) & 255) != base);
    line = __LINE__; fail(((*(bP = vqword())) & 255) != base);
    line = __LINE__; fail(((*(qP = vqword())) & 255) != base);
    line = __LINE__; fail(((*(hP = vqword())) & 255) != base);
    line = __LINE__; fail(((*(fP = vqword())) & 255) != base);
    line = __LINE__; fail(((*(bP = vhword())) & 255) != base);
    line = __LINE__; fail(((*(qP = vhword())) & 255) != base);
    line = __LINE__; fail(((*(hP = vhword())) & 255) != base);
    line = __LINE__; fail(((*(fP = vhword())) & 255) != base);
    line = __LINE__; fail(((*(bP = vfword())) & 255) != base);
    line = __LINE__; fail(((*(qP = vfword())) & 255) != base);
    line = __LINE__; fail(((*(hP = vfword())) & 255) != base);
    line = __LINE__; fail(((*(fP = vfword())) & 255) != base);

    run(1); run(3); run(4); run(5); run(7); run(8);
  }

#ifdef TOPS20
  exit(0);
#else
  exit(256);
#endif
}
