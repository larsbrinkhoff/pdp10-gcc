#include <stdio.h>
#include <stdlib.h>

typedef struct
{
  const char *rtl;
  const char *mode;
  const char *predicate;
  const char *constraint;
} op;

static void
define_insn (op *dest, op *src, const char *template)
{
  char buf[100];

  fputs ("(define_insn \"\"\n  [(set ", stdout);
  snprintf (buf, sizeof buf, "(match_operand:%s 0 \"%s\" \"%s\")",
	    dest->mode, dest->predicate, dest->constraint);
  printf (dest->rtl, buf);
  fputs ("\n        ", stdout);
  snprintf (buf, sizeof buf, "(match_operand:%s 1 \"%s\" \"%s\")",
	    src->mode, src->predicate, src->constraint);
  printf (src->rtl, buf);
  fputs (")]\n  \"\"\n  \"", stdout);
  fputs (template, stdout);
  fputs ("\")\n\n", stdout);
}

int
main (void)
{
  op dest[] =
  {
    { "(zero_extract:SI %s\n\t\t\t (const_int 18)\n\t\t\t (const_int 0))",
      "SI", "reg_or_mem_operand", "+r,m" },
    { "(zero_extract:SI %s\n\t\t\t (const_int 18)\n\t\t\t (const_int 18))",
      "SI", "reg_or_mem_operand", "+r,m" },
    { "(subreg:SI %s)",
      "HI", "reg_or_mem_operand", "=r,m" },
  };
  op src[] =
  {
    { "(zero_extract:SI %s\n\t\t\t (const_int 18)\n\t\t\t (const_int 0))",
      "SI", "reg_or_mem_operand", "rm,r" },
    { "(zero_extract:SI %s\n\t\t\t (const_int 18)\n\t\t\t (const_int 18))",
      "SI", "reg_or_mem_operand", "rm,r" },
    { "(sign_extract:SI %s\n\t\t\t (const_int 18)\n\t\t\t (const_int 0))",
      "SI", "reg_or_mem_operand", "rm,r" },
    { "(sign_extract:SI %s\n\t\t\t (const_int 18)\n\t\t\t (const_int 18))",
      "SI", "reg_or_mem_operand", "rm,r" },
    { "(subreg:SI %s)",
      "HI", "reg_or_mem_operand", "rm,r" },
    { "(and:SI %s\n\t\t(const_int RIGHT_HALF))",
      "SI", "reg_or_mem_operand", "rm,r" },
    { "(lshiftrt:SI %s\n\t\t     (const_int 18))",
      "SI", "reg_or_mem_operand", "rm,r" },
    { "(ashiftrt:SI %s\n\t\t     (const_int 18))",
      "SI", "reg_or_mem_operand", "rm,r" },
  };
  int i, j;

  for (i = 0; i < sizeof dest / sizeof dest[0]; i ++)
    for (j = 0; j < sizeof src / sizeof src[0]; j ++)
      define_insn (&dest[i], &src[j],
		   "*return pdp10_output_halfword_move (insn);");

  exit (0);
}
