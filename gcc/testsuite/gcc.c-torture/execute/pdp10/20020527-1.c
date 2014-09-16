#define base_00377777   b1
#define base_00400000   b2
#define base_00777777   b3
#define base_01000000   b4
#define index__00400001 i1
#define index__00400000 i2
#define index__00000001 i3
#define index_00000000  i4
#define index_00000001  i5
#define index_00377777  i6
#define index_00400000  i7
#define index_00777777  i8
#define index_01000000  i9

#define BASE(base) \
static int base_##base (int index) { return ((int *)base)[index]; }

#define INDEX(name, index) \
static int index_##name (int *base) { return base[index]; }
#define INDEX_PLUS(index)  INDEX (index, index)
#define INDEX_MINUS(index) INDEX (_##index, -index)

#define TEST(name, index)			\
  if (index_##name (pointer - (index)) != 42)	\
    abort ()
#define TEST_PLUS(index) TEST (index, index)
#define TEST_MINUS(index) TEST (_##index, -index)

BASE (00377777)
BASE (00400000)
BASE (00777777)
BASE (01000000)

INDEX_MINUS (01000000)
INDEX_MINUS (00400001)
INDEX_MINUS (00400000)
INDEX_MINUS (00000001)
INDEX_PLUS  (00000000)
INDEX_PLUS  (00000001)
INDEX_PLUS  (00377777)
INDEX_PLUS  (00400000)
INDEX_PLUS  (00777777)
INDEX_PLUS  (01000000)

static int array[1] = { 42 };
static int *pointer = array;

int
main ()
{
#if 1
  TEST_MINUS (01000000);
  TEST_MINUS (00400001);
#endif
  TEST_MINUS (00400000);
  TEST_MINUS (00000001);
  TEST_PLUS  (00000000);
  TEST_PLUS  (00000001);
  TEST_PLUS  (00377777);
#if 1
  TEST_PLUS  (00400000);
  TEST_PLUS  (00777777);
#endif
  TEST_PLUS  (01000000);
  exit (0);
}
