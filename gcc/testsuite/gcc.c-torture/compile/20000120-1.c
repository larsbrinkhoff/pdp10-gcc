extern char lets[26+1];
char let;
int letnum;
char lets[] = "AbCdefghiJklmNopQrStuVwXyZ";

static void
pad_home1 ()
{
  let = lets[letnum = lets[letnum + 1] ? letnum + 1 : 0];
}

