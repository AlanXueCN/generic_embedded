#include "miniutils.h"

#ifndef UART_PUTC_OFFSET
#define UART_PUTC_OFFSET  0
#endif

#define __ITOA_NEGATE_B         0
#define __ITOA_NO_ZERO_END_B    1
#define __ITOA_FILL_SPACE_B     2
#define __ITOA_FORCE_SIGN_B     3
#define __ITOA_BASE_SIG_B       4
#define __ITOA_CAPITALS_B       5

// u_itoa flags
// places a '-' before string generated by u_itoa
#define ITOA_NEGATE             (1<<__ITOA_NEGATE_B)
// omits ending string with a zero
#define ITOA_NO_ZERO_END        (1<<__ITOA_NO_ZERO_END_B)
// fills zero prefixes with ' '
#define ITOA_FILL_SPACE         (1<<__ITOA_FILL_SPACE_B)
// forces a '+' or a '-' before string
#define ITOA_FORCE_SIGN         (1<<__ITOA_FORCE_SIGN_B)
// adds base signature, e.g. "0x", "0b", etc
#define ITOA_BASE_SIG           (1<<__ITOA_BASE_SIG_B)
// generates string ascii as captials
#define ITOA_CAPITALS           (1<<__ITOA_CAPITALS_B)

#ifdef MINIUTILS_PRINT_LONGLONG
typedef unsigned long long utype_t;
typedef signed long long stype_t;
#else
typedef unsigned int utype_t;
typedef signed int stype_t;
#endif

static void u_itoa(utype_t v, char* dst, int base, int num, int flags);

#ifdef MINIUTILS_PRINT_LONGLONG
typedef unsigned long long utype_t;
typedef signed long long stype_t;
#else
typedef unsigned int utype_t;
typedef signed int stype_t;
#endif

void v_printf(long p, const char* f, va_list arg_p) {
  register const char* tmp_f = f;
  register const char* start_f = f;
  char c;
  int format = 0;
  int num = 0;
#ifdef MINIUTILS_PRINT_FLOAT
  bool fracspec = FALSE;
  int fracnum = 5;
#endif
  int lcnt = 0;
  char buf[32*2 + 4];
  int flags = ITOA_FILL_SPACE;

  while ((c = *tmp_f++) != 0) {
    if (format) {
      // formatting
      switch (c) {
      case '%': {
        PUTC(p, '%');
        break;
      }
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
#ifdef MINIUTILS_PRINT_FLOAT
        if (!fracspec) {
#endif
          if (c == '0' && num == 0) {
            flags &= ~ITOA_FILL_SPACE;
          }
          num = num * 10 + (c - '0');
          num = MIN(sizeof(buf)/2-1, num);
#ifdef MINIUTILS_PRINT_FLOAT
        } else {
          fracnum = fracnum * 10 + (c - '0');
          fracnum = MIN(sizeof(buf)/2-1, fracnum);
        }
#endif
        continue;
      case '+':
        flags |= ITOA_FORCE_SIGN;
        continue;
      case '#':
        flags |= ITOA_BASE_SIG;
        continue;
#ifdef MINIUTILS_PRINT_FLOAT
      case '.':
        fracspec = TRUE;
        fracnum = 0;
        continue;
#endif
#ifdef MINIUTILS_PRINT_LONGLONG
      case 'l':
        lcnt++;
        continue;
#endif


      case 'd':
      case 'i': {
        stype_t v;
        if (lcnt)
          v = va_arg(arg_p, stype_t);
        else
          v = va_arg(arg_p, int);
        if (v < 0) {
          v = -v;
          flags |= ITOA_NEGATE;
        }
        u_itoa(v, &buf[0], 10, num, flags);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
      case 'u': {
        utype_t v;
        if (lcnt)
          v = va_arg(arg_p, utype_t);
        else
          v = va_arg(arg_p, unsigned int);
        u_itoa(v, &buf[0], 10, num, flags);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
#ifdef MINIUTILS_PRINT_FLOAT
      case 'f': {
        double v = va_arg(arg_p, double);
        if (v < 0) {
          v = -v;
          flags |= ITOA_NEGATE;
        }
        u_itoa((int)v, &buf[0], 10, num, flags);
        int c = strlen(&buf[0]);
        buf[c] = '.';
        int mul = 1, i;
        for (i = 0; i < fracnum; i++) mul *= 10;
        u_itoa((int)(v * mul) - (int)(v), &buf[c+1], 10, fracnum, 0);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
#endif
      case 'p': {
        u_itoa(va_arg(arg_p, int), &buf[0], 16, sizeof(void *)*2, flags);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
      case 'X':
        flags |= ITOA_CAPITALS;
        // fall through
        //no break
      case 'x': {
        stype_t v;
        if (lcnt)
          v = va_arg(arg_p, stype_t);
        else
          v = va_arg(arg_p, int);
        u_itoa(v, &buf[0], 16, num, flags);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
      case 'o': {
        stype_t v;
        if (lcnt)
          v = va_arg(arg_p, stype_t);
        else
          v = va_arg(arg_p, int);
        u_itoa(v, &buf[0], 8, num, flags);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
      case 'b': {
        stype_t v;
        if (lcnt)
          v = va_arg(arg_p, stype_t);
        else
          v = va_arg(arg_p, int);
        u_itoa(v, &buf[0], 2, num, flags);
        PUTB(p, &buf[0], strlen(&buf[0]));
        break;
      }
      case 'c': {
        int d = va_arg(arg_p, int);
        PUTC(p, d);
        break;
      }
      case 's': {
        char *s = va_arg(arg_p, char*);
        PUTB(p, s, strlen(s));
        break;
      }
      default:
        PUTC(p, '?');
        break;
      }
      start_f = tmp_f;
      format = 0;
    } else {
      // not formatting
      if (c == '%') {
        if (tmp_f > start_f + 1) {
          PUTB(p, start_f, (int)(tmp_f - start_f - 1));
        }
        num = 0;
        format = 1;
        flags = ITOA_FILL_SPACE;
#ifdef MINIUTILS_PRINT_FLOAT
        fracspec = FALSE;
        fracnum = 5;
#endif
#ifdef MINIUTILS_PRINT_LONGLONG
        lcnt = 0;
#endif
      }
    }
  } // while string
  if (tmp_f > start_f + 1) {
    PUTB(p, start_f, (int)(tmp_f - start_f - 1));
  }
}

void ioprint(int io, const char* f, ...) {
  va_list arg_p;
  va_start(arg_p, f);
  v_printf(io, f, arg_p);
  va_end(arg_p);
}

#ifndef print
void print(const char* f, ...) {
  va_list arg_p;
  va_start(arg_p, f);
  v_printf(IOSTD, f, arg_p);
  va_end(arg_p);
}
#endif

void printbuf(u8_t io, u8_t *buf, u16_t len) {
  int i = 0, ix = 0;
  while (i < len) {
    for (i = ix; i < MIN(ix+32, len); i++) {
      ioprint(io, "%02x", buf[i]);
    }
    while (i++ < ix+32) {
      ioprint(io, "   ");
    }
    ioprint (io, "  ");
    for (i = ix; i < MIN(ix+32, len); i++) {
      ioprint(io, "%c", buf[i] < 32 ? '.' : buf[i]);
    }
    ix += 32;
    ioprint(io, "\n");
  }
}

void vprint(const char* f, va_list arg_p) {
  v_printf(IOSTD, f, arg_p);
}

void vioprint(int io, const char* f, va_list arg_p) {
  v_printf(io, f, arg_p);
}

void sprint(char *s, const char* f, ...) {
  va_list arg_p;
  va_start(arg_p, f);
  v_printf((long)s, f, arg_p);
  va_end(arg_p);
}

void vsprint(char *s, const char* f, va_list arg_p) {
  v_printf((long)s, f, arg_p);
}

static const char *I_BASE_ARR_L = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char *I_BASE_ARR_U = "0123456789ABCDEFGHIJKLMNOPQRTSUVWXYZ";

static void u_itoa(utype_t v, char* dst, int base, int num, int flags) {
  // check that the base if valid
  if (base < 2 || base > 36) {
    if ((flags & ITOA_NO_ZERO_END) == 0) {
      *dst = '\0';
    }
    return;
  }

  const char *arr = (flags & ITOA_CAPITALS) ? I_BASE_ARR_U : I_BASE_ARR_L;
  char* ptr = dst, *ptr_o = dst, tmp_char;
  utype_t tmp_value;
  int ix = 0;
  char zero_char = flags & ITOA_FILL_SPACE ? ' ' : '0';
  do {
    tmp_value = v;
    v /= base;
    if (tmp_value != 0 || (tmp_value == 0 && ix == 0)) {
      *ptr++ = arr[(tmp_value - v * base)];
    } else {
      *ptr++ = zero_char;
    }
    ix++;
  } while ((v && num == 0) || (num != 0 && ix < num));

  if (flags & ITOA_BASE_SIG) {
    if (base == 16) {
      *ptr++ = 'x';
      *ptr++ = '0';
    } else if (base == 8) {
      *ptr++ = '0';
    } else if (base == 2) {
      *ptr++ = 'b';
    }
  }

  // apply sign
  if (flags & ITOA_NEGATE) {
    *ptr++ = '-';
  } else if (flags & ITOA_FORCE_SIGN) {
    *ptr++ = '+';
  }

  // end
  if (flags & ITOA_NO_ZERO_END) {
    ptr--;
  } else {
    *ptr-- = '\0';
  }
  while (ptr_o < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr_o;
    *ptr_o++ = tmp_char;
  }
}

void itoa(int v, char* dst, int base) {
  if (base == 10) {
    if (v < 0) {
      u_itoa(-v, dst, base, 0, ITOA_NEGATE);
    } else {
      u_itoa(v, dst, base, 0, 0);
    }
  } else {
    u_itoa((unsigned int) v, dst, base, 0, 0);
  }
}

void itoan(int v, char* dst, int base, int num) {
  if (base == 10) {
    if (v < 0) {
      u_itoa(-v, dst, base, num, ITOA_NEGATE | ITOA_NO_ZERO_END);
    } else {
      u_itoa(v, dst, base, num, ITOA_NO_ZERO_END);
    }
  } else {
    u_itoa((unsigned int) v, dst, base, num, ITOA_NO_ZERO_END);
  }
}

int atoi(const char* s) {
  return atoin(s, 10, strlen(s));
}

int atoin(const char* s, int base, int len) {
  int val = 0;
  char negate = FALSE;
  if (*s == '-') {
    negate = TRUE;
    s++;
    len--;
  } else if (*s == '+') {
    s++;
    len--;
  }
  while (len-- > 0) {
    int b = -1;
    char c = *s++;
    if (c >= '0' && c <= '9') {
      b = c - '0';
    } else if (c >= 'a' && c <= 'z') {
      b = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
      b = c - 'A' + 10;
    }
    if (b >= base) {
      return 0;
    }
    val = val * base + b;
  }
  return negate ? -val : val;
}

int strlen(const char* c) {
  const char *s;
  for (s = c; *s; ++s)
    ;
  return (int) (s - c);
}

int strnlen(const char *c, int size) {
  const char *s;
  for (s = c; size > 0 && *s; ++s, --size)
    ;
  return size >= 0 ? (int) (s - c) : 0;
}

int strcmp(const char* s1, const char* s2) {
  char c1, c2;
  while (((c1 = *s1++) != 0) & ((c2 = *s2++) != 0)) {
    if (c1 != c2) {
      return -1;
    }
  }
  return c1 - c2;
}

int strcmpbegin(const char* prefix, const char* s) {
  char c1, c2;
  while (((c1 = *prefix++) != 0) & ((c2 = *s++) != 0)) {
    if (c1 != c2) {
      return -1;
    }
  }
  return c1 == 0 ? 0 : c1 - c2;
}

int strncmp(const char* s1, const char* s2, int len) {
  while (len > 0 && *s1++ == *s2++) {
    len--;
  }
  return len;
}

char* strncpy(char* d, const char* s, int num) {
  char* oldd = d;
  char c;
  while (num > 0 && (c = *s++) != 0) {
    *d++ = c;
    num--;
  }
  while (num-- > 0) {
    *d++ = 0;
  }
  return oldd;
}

char* strcpy(char* d, const char* s) {
  char* oldd = d;
  char c;
  do {
    c = *s++;
    *d++ = c;
  } while (c != 0);
  return oldd;
}

const char* strchr(const char* str, int ch) {
  char d;
  while ((d = *str) != 0 && d != ch) {
    str++;
  }
  if (d == 0) {
    return 0;
  } else {
    return str;
  }
}

char* strpbrk(const char* str, const char* key) {
  char c;
  while ((c = *str++) != 0) {
    if (strchr(key, c)) {
      return (char*)--str;
    }
  }
  return 0;
}

char* strnpbrk(const char* str, const char* key, int len) {
  char c;
  while (len-- > 0) {
    c = *str++;
    if (strchr(key, c)) {
      return (char*)--str;
    }
  }
  return 0;
}

char* strstr(const char* str, const char* sub) {
  int len = strlen(sub);
  int ix = 0;
  char c;
  while ((c = *str++) != 0 && ix < len) {
    if (c == sub[ix]) {
      ix++;
    } else {
      ix = 0;
    }
  }
  if (ix == len) {
    return (char*)(str - len - 1);
  } else {
    return 0;
  }
}

char* strncontainex(const char* s, const char* content, int len) {
  while (len-- > 0) {
    if (strchr(content, *s++) == 0) {
      return (char*)--s;
    }
  }
  return 0;
}

unsigned short crc_ccitt_16(unsigned short crc, unsigned char data) {
  crc  = (unsigned char)(crc >> 8) | (crc << 8);
  crc ^= data;
  crc ^= (unsigned char)(crc & 0xff) >> 4;
  crc ^= (crc << 8) << 4;
  crc ^= ((crc & 0xff) << 4) << 1;
  return crc;
}

#define TAPMASK       0x80000062U
unsigned int rand(unsigned int seed) {
  if (seed & 1) {
    seed = (1 << 31) | ((seed ^ TAPMASK) >> 1);
  } else {
    seed >>= 1;
  }
  return seed;
}

static unsigned int _rand_seed = 0;
unsigned int rand_next() {
  _rand_seed = rand(_rand_seed);
  return _rand_seed;
}

void rand_seed(unsigned int seed) {
  _rand_seed = seed;
}

void quicksort(int* orders, void** pp, int elements) {

#define  MAX_LEVELS  32

  int piv;
  int beg[MAX_LEVELS];
  int end[MAX_LEVELS];
  int i = 0;
  int L, R, swap;

  beg[0] = 0;
  end[0] = elements;

  while (i >= 0) {
    void* pivEl;
    L = beg[i];
    R = end[i] - 1;

    if (L < R) {
      pivEl = pp[L];
      piv = orders[L];

      while (L < R) {
        while (orders[R] >= piv && L < R) {
          R--;
        }
        if (L < R) {
          pp[L] = pp[R];
          orders[L++] = orders[R];
        }
        while (orders[L] <= piv && L < R) {
          L++;
        }
        if (L < R) {
          pp[R] = pp[L];
          orders[R--] = orders[L];
        }
      }

      pp[L] = pivEl;
      orders[L] = piv;
      beg[i + 1] = L + 1;
      end[i + 1] = end[i];
      end[i++] = L;

      if (end[i] - beg[i] > end[i - 1] - beg[i - 1]) {
        swap = beg[i];
        beg[i] = beg[i - 1];
        beg[i - 1] = swap;

        swap = end[i];
        end[i] = end[i - 1];
        end[i - 1] = swap;
      }
    } else {
      i--;
    }
  }
}

void quicksort_cmp(int* orders, void** pp, int elements,
    int(*orderfunc)(void* p)) {
  int i;
  for (i = 0; i < elements; i++) {
    orders[i] = orderfunc(pp[i]);
  }
  quicksort(orders, pp, elements);
}

static void c_next(cursor *c) {
  if (c->len > 0) {
    c->len--;
    c->s++;
  }
}

static void c_advance(cursor *c, int l) {
  l = MIN(l, c->len);
  c->len -= l;
  c->s += l;
}

static void c_back(cursor *c) {
  c->len++;
  c->s--;
}

static void c_skip_blanks(cursor *curs) {
  char c;
  while (curs->len > 0) {
    c = *(curs->s);
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      break;
    }
    c_next(curs);
  }
}

static void c_strnparse(cursor *c, char str_def) {
  typedef enum {
    STR, BS_FIRST, BS_OCT, BS_HEX
  } pstate;
  pstate state = STR;
  int val = -1;
  while (c->len > 0) {
    char h = *c->s;

    switch (state) {
    case STR: {
      if (h == '\\') {
        state = BS_FIRST;
      } else if (h == str_def) {
        c_next(c);
        return;
      } else {
        *c->wrk++ = h;
      }
      c_next(c);
      break;
    } // case SR

    case BS_FIRST: {
      char advance = TRUE;
      // first backslash char
      switch (h) {
      case 'a':
        *c->wrk++ = '\a';
        state = STR;
        break;
      case 'b':
        *c->wrk++ = '\b';
        state = STR;
        break;
      case 'f':
        *c->wrk++ = '\f';
        state = STR;
        break;
      case 'n':
        *c->wrk++ = '\n';
        state = STR;
        break;
      case 'r':
        *c->wrk++ = '\r';
        state = STR;
        break;
      case 't':
        *c->wrk++ = '\t';
        state = STR;
        break;
      case 'v':
        *c->wrk++ = '\v';
        state = STR;
        break;
      case '\\':
      case '?':
      case '\'':
      case '\"':
        *c->wrk++ = h;
        state = STR;
        break;
      case 'x':
      case 'X':
        val = 0;
        state = BS_HEX;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        advance = FALSE;
        val = 0;
        state = BS_OCT;
        break;
      default:
        advance = FALSE;
        state = STR;
        break;
      }
      if (advance) {
        c_next(c);
      }
      break;
    } // case BS_FIRST

    case BS_OCT: {
      if (h >= '0' && h <= '7') {
        val = val * 8 + (h-'0');
        c_next(c);
      } else {
        *c->wrk++ = val;
        state = STR;
      }
      break;
    } // case BS_OCT

    case BS_HEX: {
      if (h >= 'A' && h <= 'F') {
        h = h - 'A' + 'a';
      }
      if (h >= '0' && h <= '9') {
        val = val * 16 + (h-'0');
        c_next(c);
      } else if ( h >= 'a' && h <= 'f') {
        val = val * 16 + (h-'a') + 10;
        c_next(c);
      } else {
        *c->wrk++ = val;
        state = STR;
      }
      break;
    } // case BS_HEX
    } // switch state
  } // while
  if (state == BS_OCT || state == BS_HEX) {
    *c->wrk++ = val;
  }
}

static int _strarg_next(cursor *c, strarg* arg, const char *delim, bool string) {
  if (delim) {
    if (strchr(delim, *c->s)) {
      do {
        c_next(c);
      } while (strchr(delim, *c->s));
      if (c->len == 0) return FALSE;
//      arg->type = STR;
//      arg->len = 0;
//      arg->str = 0;
//      c_next(c);
//      return TRUE;
    }
  } else {
    c_skip_blanks(c);
  }
  if (c->len == 0) {
    // end of string
    return FALSE;
  }

  // find starting string definition
  char str_def = 0;
  if (*c->s == '"') {
    str_def = '"';
  } else if (*c->s == '\'') {
    str_def = '\'';
  }

  // is a defined string, handle further
  if (str_def) {
    arg->type = STR;
    arg->str = c->wrk;
    // skip str def char
    c_next(c);
    // fill up c->wrk buffer with string parsed string
    c_strnparse(c, str_def);
    // and zero end
    *c->wrk++ = '\0';
    // calc len
    arg->len = c->wrk - arg->str;
    return TRUE;
  }

  // no defined string, find arg end ptr
  char *arg_end = strnpbrk(c->s, delim ? delim : " \n\r\t", c->len);
  // adjust arg end if bad
  if (arg_end == 0 || arg_end - c->s > c->len) {
    arg_end = c->s + c->len;
  }
  arg->len = arg_end - c->s;

  // determine type

  // signs
  char possibly_minus = FALSE;
  char possibly_plus = FALSE;
  if (arg->len > 1) {
    possibly_minus = *c->s == '-';
    possibly_plus = *c->s == '+';
  }
  if (possibly_minus | possibly_plus) {
    c_next(c);
    arg->len--;
  }

  // determine content
  char possibly_hex = FALSE;
  char possibly_int = FALSE;
  char possibly_bin = FALSE;
  if (!string) {
    if (arg->len > 2 && *c->s == '0') {
      possibly_hex = ((*(c->s+1) == 'x' || *(c->s+1) == 'X'));
      possibly_bin = ((*(c->s+1) == 'b' || *(c->s+1) == 'B'));
    }

    if (possibly_bin) {
      possibly_bin = strncontainex(c->s + 2, "01", arg->len-2) == 0;
    }
    if (!possibly_bin && possibly_hex) {
      possibly_hex = strncontainex(c->s + 2, "0123456789abcdefABCDEF", arg->len-2) == 0;
    }
    if (!possibly_bin && !possibly_hex) {
      possibly_int = strncontainex(c->s, "0123456789", arg->len) == 0;
    }
  }

  // ok, determined
  if (possibly_bin) {
    c_advance(c,2); // adjust for 0b
    arg->type = INT;
    arg->val = possibly_minus ? -atoin(c->s, 2, arg->len-2) : atoin(c->s, 2, arg->len-2);
    c_advance(c, arg->len - 2);
  } else if (possibly_int) {
    arg->type = INT;
    arg->val = possibly_minus ? -atoin(c->s, 10, arg->len) : atoin(c->s, 10, arg->len);
    c_advance(c, arg->len+1);
  } else if (possibly_hex) {
    c_advance(c,2); // adjust for 0x
    arg->type = INT;
    arg->val = possibly_minus ? -atoin(c->s, 16, arg->len-2) : atoin(c->s, 16, arg->len-2);
    c_advance(c, arg->len - 2);
  } else {
    arg->type = STR;
    arg->str = c->wrk;
    if (possibly_minus || possibly_plus) {
      c_back(c);
      arg->len++;
    }
    strncpy(arg->str, c->s, arg->len);
    arg->str[arg->len] = 0;
    c_advance(c, arg->len + 1);
    c->wrk += arg->len + 1;
  }

  return TRUE;
}

int strarg_next(cursor *c, strarg* arg) {
  return _strarg_next(c, arg, 0, FALSE);
}

int strarg_next_delim(cursor *c, strarg* arg, const char *delim) {
  return _strarg_next(c, arg, delim, FALSE);
}

int strarg_next_str(cursor *c, strarg* arg) {
  return _strarg_next(c, arg, 0, TRUE);
}

int strarg_next_delim_str(cursor *c, strarg* arg, const char *delim) {
  return _strarg_next(c, arg, delim, TRUE);
}

void strarg_init(cursor *c, char* s, int len) {
  c->s = s;
  c->wrk = s;
  if (len) {
    c->len = len;
  } else {
    c->len = strlen(s);
  }
}

