typedef struct
{
  unsigned char a __attribute__((packed));
  unsigned short b __attribute__((packed));
} three_char_t;

unsigned char
myseta (void)
{
  return 0xab;
}

unsigned short
mysetb (void)
{
  return 0x1234;
}

main ()
{
  three_char_t three_char;

  three_char.a = myseta ();
  three_char.b = mysetb ();
  if (three_char.a != 0xab || three_char.b != 0x1234)
    abort ();
  exit (0);
}

