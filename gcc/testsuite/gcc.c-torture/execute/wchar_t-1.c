typedef __WCHAR_TYPE__ wchar_t;
wchar_t x[] = L"�";
wchar_t y = L'�';
extern void abort (void);
extern void exit (int);

int main (void)
{
  if (sizeof (x) / sizeof (wchar_t) != 2)
    abort ();
  if (x[0] != L'�' || x[1] != L'\0')
    abort ();
  if (y != L'�')
    abort ();
  exit (0);
}
