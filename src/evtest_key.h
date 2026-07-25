#ifndef EVTEST_KEY_H
#define EVTEST_KEY_H
#ifdef __cplusplus
extern "C" {
#endif

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

const char *get_key_name(int);

#ifdef __cplusplus
}
#endif
#endif // EVTEST_KEY_H
