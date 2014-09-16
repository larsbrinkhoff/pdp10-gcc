#if 0

#include <stdarg.h>

asm ("SEARCH MONSYM");

#define EXECUTIVE_BASE_PAGE 0

struct dte
{
  int std;	/* PDP-10 diagnostic start address.  */
  int ddt;	/* PDP-10 DDT start address.  */
  int stl;	/* PDP-10 loader start address.  */
  int stm;	/* PDP-10 monitor start address.  */
  int flg;	/* Operation complete flag.  */
  int clk;	/* Clock interrupt flag.  */
  int ci;	/* Clock interrupt instruction.  */
  int t11;	/* 10 to 11 argument.  */
  int f11;	/* 10 from 11 argument.  */
  int cmd;	/* To 11 command word.  */
  int seq;	/* Operation sequence number.  */
  int opr;	/* Operational DTE #.  */
  int chr;	/* Last typed character.  */
  int mtd;	/* Monitor TTY output complete flag.  */
  int mti;	/* Monitor TTY input flag.  */
  int swr;	/* 10 switch register.  */
};

#define DTE (*(struct dte *)((EXECUTIVE_BASE_PAGE << 9) + 0440))

static int dte_talk (int command)
{
  DTE.cmd = command;
  DTE.seq++;
  asm ("CONO DTE,20000");
  while (DTE.flg == 0)
    ;
  DTE.flg = 0;
  return DTE.f11;
}

static void dte_init (void)
{
  DTE.flg = 0;
  DTE.clk = 0;
  DTE.ci = 0;
}

static int getchar (void)
{
  int c;

  do
    c = dte_talk (03400) & 0177;
  while (c == 0);

  return c;
}

static void putchar (int c)
{
  dte_talk (c & 0177);
}

static void puts (char *s)
{
  int c;

  if ((c = *s) == 0)
    return;

  do
    putchar (c);
  while ((c = *++s) != 0);
}

static void
output (void *ptr, int c)
{
  putchar (c);
}

int
printf (const char *format, va_list ap)
{
  extern int _vxprintf (void (*) (void *, int), void *, const char *, va_list);
  return _vxprintf (output, 0, format, ap);
}

static void
processor_id (void)
{
  int word;

  asm ("APRID %0" : "=m" (word));

  printf ("APRID = %o\n", word);

  printf ("  TOPS-20 paging:     %s\n", word & 0400000000000 ? "yes" : "no");
  printf ("  Extended addresing: %s\n", word & 0200000000000 ? "yes" : "no");
  printf ("  Exotic microcode:   %s\n", word & 0100000000000 ? "yes" : "no");
  printf ("  Microcode version:  %d\n", (word >> 18) & 0777);
  printf ("  50 Hz:              %s\n", word & 0400000 ? "yes" : "no");
  printf ("  Cache:              %s\n", word & 0200000 ? "yes" : "no");
  printf ("  Channel:            %s\n", word & 0100000 ? "yes" : "no");
  printf ("  Extended KL10:      %s\n", word & 040000 ? "yes" : "no");
  printf ("  Master oscillator:  %s\n", word & 020000 ? "yes" : "no");
  printf ("  Processor serial number: %d\n", word & 07777);
}

static void
enable_paging (void)
{
  /* Set up mapping.  */
  asm ("CONO PAG,%0" : : "i" (0660000 + EXECUTIVE_BASE_PAGE));
}
#endif

int
main (int argc, char **argv)
{
#if 0
  dte_init ();

  puts ("Ceci n'est pas un port de Unix.\r\n");

  {
    int c = getchar ();
    puts ("You typed: ");
    putchar (c);
    puts ("\r\n");
  }

  processor_id ();

  enable_paging ();
#endif

  return 0;
}
