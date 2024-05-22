#include <service/imap/rfc2047.h>

#include <cstring>
#include <stdint.h>
#include <ctype.h>

// Adapted from: https://github.com/mobizt/ESP-Mail-Client/blob/master/src/extras/RFC2047.h

#define strfcpy(A, B, C) strncpy(A, B, C), *(A + (C)-1) = 0

enum
{
  ENCOTHER,
  ENC7BIT,
  ENC8BIT,
  ENCQUOTEDPRINTABLE,
  ENCBASE64,
  ENCBINARY
};

__attribute__((used)) static const char *Charset = "utf-8";

__attribute__((used)) static int Index_hex[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

__attribute__((used)) static int Index_64[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

#define IsPrint(c) (isprint((unsigned char)(c)) || \
                    ((unsigned char)(c) >= 0xa0))

#define hexval(c) Index_hex[(unsigned int)(c)]
#define base64val(c) Index_64[(unsigned int)(c)]

extern void rfc2047DecodeWord(char *d, const char *s, size_t dlen);
extern char *safe_strdup(const char *s);

void rfc2047_decode(char *d, const char *s, size_t dlen)
{
  const char *p, *q;
  size_t n;
  int found_encoded = 0;

  dlen--; /* save room for the terminal nul */

  while (*s && dlen > 0)
  {
    if ((p = strstr(s, "=?")) == NULL ||
        (q = strchr(p + 2, '?')) == NULL ||
        (q = strchr(q + 1, '?')) == NULL ||
        (q = strstr(q + 1, "?=")) == NULL)
    {
      /* no encoded words */
      if (d != s)
        strfcpy(d, s, dlen + 1);
      return;
    }

    if (p != s)
    {
      n = (size_t)(p - s);
      /* ignore spaces between encoded words */
      if (!found_encoded || strspn(s, " \t\r\n") != n)
      {
        if (n > dlen)
          n = dlen;
        if (d != s)
          memcpy(d, s, n);
        d += n;
        dlen -= n;
      }
    }

    rfc2047DecodeWord(d, p, dlen);
    found_encoded = 1;
    s = q + 2;
    n = strlen(d);
    dlen -= n;
    d += n;
  }
  *d = 0;
}

void rfc2047DecodeWord(char *d, const char *s, size_t dlen)
{
  char *p = safe_strdup(s);
  char *pp = p;
  char *end = p;
  char *pd = d;
  size_t len = dlen;
  int enc = 0, filter = 0, count = 0, c1, c2, c3, c4;

  while (pp != NULL)
  {
    // See RFC2047.h
    strsep(&end, "?");
    count++;
    switch (count)
    {
    case 2:
      if (strcasecmp(pp, Charset) != 0)
      {
        filter = 1;
      }
      break;
    case 3:
      if (toupper(*pp) == 'Q')
        enc = ENCQUOTEDPRINTABLE;
      else if (toupper(*pp) == 'B')
        enc = ENCBASE64;
      else
        return;
      break;
    case 4:
      if (enc == ENCQUOTEDPRINTABLE)
      {
        while (*pp && len > 0)
        {
          if (*pp == '_')
          {
            *pd++ = ' ';
            len--;
          }
          else if (*pp == '=')
          {
            *pd++ = (hexval(pp[1]) << 4) | hexval(pp[2]);
            len--;
            pp += 2;
          }
          else
          {
            *pd++ = *pp;
            len--;
          }
          pp++;
        }
        *pd = 0;
      }
      else if (enc == ENCBASE64)
      {
        while (*pp && len > 0)
        {
          c1 = base64val(pp[0]);
          c2 = base64val(pp[1]);
          *pd++ = (c1 << 2) | ((c2 >> 4) & 0x3);
          if (--len == 0)
            break;

          if (pp[2] == '=')
            break;

          c3 = base64val(pp[2]);
          *pd++ = ((c2 & 0xf) << 4) | ((c3 >> 2) & 0xf);
          if (--len == 0)
            break;

          if (pp[3] == '=')
            break;

          c4 = base64val(pp[3]);
          *pd++ = ((c3 & 0x3) << 6) | c4;
          if (--len == 0)
            break;

          pp += 4;
        }
        *pd = 0;
      }
      break;
    }

    pp = end;
  }

  free(p);

  if (filter)
  {
    pd = d;
    while (*pd)
    {
      if (!IsPrint(*pd))
        *pd = '?';
      pd++;
    }
  }
  return;
}

char * safe_strdup(const char *s)
{
  char *p;
  size_t l;

  if (!s || !*s)
    return 0;
  l = strlen(s) + 1;
  p = (char *)malloc(l);
  memcpy(p, s, l);
  return p;
}
