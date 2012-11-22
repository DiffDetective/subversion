/*
 * string-test.c:  a collection of libsvn_string tests
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */

/* ====================================================================
   To add tests, look toward the bottom of this file.

*/



#include <stdio.h>
#include <string.h>

#include <apr_pools.h>
#include <apr_file_io.h>

#include "../svn_test.h"

#include "svn_io.h"
#include "svn_error.h"
#include "svn_string.h"   /* This includes <apr_*.h> */
#include "private/svn_string_private.h"

/* A quick way to create error messages.  */
static svn_error_t *
fail(apr_pool_t *pool, const char *fmt, ...)
{
  va_list ap;
  char *msg;

  va_start(ap, fmt);
  msg = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  return svn_error_create(SVN_ERR_TEST_FAILED, 0, msg);
}


/* Some of our own global variables, for simplicity.  Yes,
   simplicity. */
svn_stringbuf_t *a = NULL, *b = NULL, *c = NULL;
const char *phrase_1 = "hello, ";
const char *phrase_2 = "a longish phrase of sorts, longer than 16 anyway";




static svn_error_t *
test1(apr_pool_t *pool)
{
  a = svn_stringbuf_create(phrase_1, pool);

  /* Test that length, data, and null-termination are correct. */
  if ((a->len == strlen(phrase_1)) && ((strcmp(a->data, phrase_1)) == 0))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test2(apr_pool_t *pool)
{
  b = svn_stringbuf_ncreate(phrase_2, 16, pool);

  /* Test that length, data, and null-termination are correct. */
  if ((b->len == 16) && ((strncmp(b->data, phrase_2, 16)) == 0))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test3(apr_pool_t *pool)
{
  char *tmp;
  size_t old_len;

  a = svn_stringbuf_create(phrase_1, pool);
  b = svn_stringbuf_ncreate(phrase_2, 16, pool);

  tmp = apr_palloc(pool, (a->len + b->len + 1));
  strcpy(tmp, a->data);
  strcat(tmp, b->data);
  old_len = a->len;
  svn_stringbuf_appendstr(a, b);

  /* Test that length, data, and null-termination are correct. */
  if ((a->len == (old_len + b->len)) && ((strcmp(a->data, tmp)) == 0))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test4(apr_pool_t *pool)
{
  a = svn_stringbuf_create(phrase_1, pool);
  svn_stringbuf_appendcstr(a, "new bytes to append");

  /* Test that length, data, and null-termination are correct. */
  if (svn_stringbuf_compare
      (a, svn_stringbuf_create("hello, new bytes to append", pool)))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test5(apr_pool_t *pool)
{
  a = svn_stringbuf_create(phrase_1, pool);
  svn_stringbuf_appendbytes(a, "new bytes to append", 9);

  /* Test that length, data, and null-termination are correct. */
  if (svn_stringbuf_compare
      (a, svn_stringbuf_create("hello, new bytes", pool)))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test6(apr_pool_t *pool)
{
  a = svn_stringbuf_create(phrase_1, pool);
  b = svn_stringbuf_create(phrase_2, pool);
  c = svn_stringbuf_dup(a, pool);

  /* Test that length, data, and null-termination are correct. */
  if ((svn_stringbuf_compare(a, c)) && (! svn_stringbuf_compare(b, c)))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test7(apr_pool_t *pool)
{
  char *tmp;
  size_t tmp_len;

  c = svn_stringbuf_create(phrase_2, pool);

  tmp_len = c->len;
  tmp = apr_palloc(pool, c->len + 1);
  strcpy(tmp, c->data);

  svn_stringbuf_chop(c, 11);

  if ((c->len == (tmp_len - 11))
      && (strncmp(tmp, c->data, c->len) == 0)
      && (c->data[c->len] == '\0'))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test8(apr_pool_t *pool)
{
  c = svn_stringbuf_create(phrase_2, pool);

  svn_stringbuf_setempty(c);

  if ((c->len == 0) && (c->data[0] == '\0'))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test9(apr_pool_t *pool)
{
  a = svn_stringbuf_create(phrase_1, pool);

  svn_stringbuf_fillchar(a, '#');

  if ((strcmp(a->data, "#######") == 0)
      && ((strncmp(a->data, "############", a->len - 1)) == 0)
      && (a->data[(a->len - 1)] == '#')
      && (a->data[(a->len)] == '\0'))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}



static svn_error_t *
test10(apr_pool_t *pool)
{
  svn_stringbuf_t *s, *t;
  size_t len_1 = 0;
  size_t block_len_1 = 0;
  size_t block_len_2 = 0;

  s = svn_stringbuf_create("a small string", pool);
  len_1       = (s->len);
  block_len_1 = (s->blocksize);

  t = svn_stringbuf_create(", plus a string more than twice as long", pool);
  svn_stringbuf_appendstr(s, t);
  block_len_2 = (s->blocksize);

  /* Test that:
   *   - The initial block was at least the right fit.
   *   - The initial block was not excessively large.
   *   - The block more than doubled (because second string so long).
   */
  if ((len_1 <= (block_len_1 - 1))
      && ((block_len_1 - len_1) <= APR_ALIGN_DEFAULT(1))
        && ((block_len_2 / block_len_1) > 2))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}


static svn_error_t *
test11(apr_pool_t *pool)
{
  svn_stringbuf_t *s;

  s = svn_stringbuf_createf(pool,
                            "This %s is used in test %d.",
                            "string",
                            12);

  if (strcmp(s->data, "This string is used in test 12.") == 0)
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}

static svn_error_t *
check_string_contents(svn_stringbuf_t *string,
                      const char *ftext,
                      apr_size_t ftext_len,
                      int repeat,
                      apr_pool_t *pool)
{
  const char *data;
  apr_size_t len;
  int i;

  data = string->data;
  len = string->len;
  for (i = 0; i < repeat; ++i)
    {
      if (len < ftext_len || memcmp(ftext, data, ftext_len))
        return fail(pool, "comparing failed");
      data += ftext_len;
      len -= ftext_len;
    }
  if (len < 1 || memcmp(data, "\0", 1))
    return fail(pool, "comparing failed");
  data += 1;
  len -= 1;
  for (i = 0; i < repeat; ++i)
    {
      if (len < ftext_len || memcmp(ftext, data, ftext_len))
        return fail(pool, "comparing failed");
      data += ftext_len;
      len -= ftext_len;
    }

  if (len)
    return fail(pool, "comparing failed");

  return SVN_NO_ERROR;
}


static svn_error_t *
test12(apr_pool_t *pool)
{
  svn_stringbuf_t *s;
  const char fname[] = "string-test.tmp";
  apr_file_t *file;
  apr_status_t status;
  apr_size_t len;
  int i, repeat;
  const char ftext[] =
    "Just some boring text. Avoiding newlines 'cos I don't know"
    "if any of the Subversion platfoms will mangle them! There's no"
    "need to test newline handling here anyway, it's not relevant.";

  status = apr_file_open(&file, fname, APR_WRITE | APR_TRUNCATE | APR_CREATE,
                         APR_OS_DEFAULT, pool);
  if (status)
    return fail(pool, "opening file");

  repeat = 100;

  /* Some text */
  for (i = 0; i < repeat; ++i)
    {
      status = apr_file_write_full(file, ftext, sizeof(ftext) - 1, &len);
      if (status)
        return fail(pool, "writing file");
    }

  /* A null byte, I don't *think* any of our platforms mangle these */
  status = apr_file_write_full(file, "\0", 1, &len);
  if (status)
    return fail(pool, "writing file");

  /* Some more text */
  for (i = 0; i < repeat; ++i)
    {
      status = apr_file_write_full(file, ftext, sizeof(ftext) - 1, &len);
      if (status)
        return fail(pool, "writing file");
    }

  status = apr_file_close(file);
  if (status)
    return fail(pool, "closing file");

  SVN_ERR(svn_stringbuf_from_file(&s, fname, pool));
  SVN_ERR(check_string_contents(s, ftext, sizeof(ftext) - 1, repeat, pool));

  /* Reset to avoid false positives */
  s = NULL;

  status = apr_file_open(&file, fname, APR_READ, APR_OS_DEFAULT, pool);
  if (status)
    return fail(pool, "opening file");

  SVN_ERR(svn_stringbuf_from_aprfile(&s, file, pool));
  SVN_ERR(check_string_contents(s, ftext, sizeof(ftext) - 1, repeat, pool));

  status = apr_file_close(file);
  if (status)
    return fail(pool, "closing file");

  status = apr_file_remove(fname, pool);
  if (status)
    return fail(pool, "removing file");

  return SVN_NO_ERROR;
}

/* Helper function for checking correctness of find_char_backward */
static svn_error_t *
test_find_char_backward(const char* data,
                        apr_size_t len,
                        char ch,
                        apr_size_t pos,
                        apr_pool_t *pool)
{
  apr_size_t i;

  a = svn_stringbuf_create(data, pool);
  i = svn_stringbuf_find_char_backward(a, ch);

  if (i == pos)
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}

static svn_error_t *
test13(apr_pool_t *pool)
{
  a = svn_stringbuf_create("test, test", pool);

  return test_find_char_backward(a->data, a->len, ',', 4, pool);
}

static svn_error_t *
test14(apr_pool_t *pool)
{
  a = svn_stringbuf_create(",test test", pool);

  return test_find_char_backward(a->data, a->len, ',', 0, pool);
}

static svn_error_t *
test15(apr_pool_t *pool)
{
  a = svn_stringbuf_create("testing,", pool);

  return test_find_char_backward(a->data,
                                 a->len,
                                 ',',
                                 a->len - 1,
                                 pool);
}

static svn_error_t *
test16(apr_pool_t *pool)
{
  a = svn_stringbuf_create_empty(pool);

  return test_find_char_backward(a->data, a->len, ',', 0, pool);
}

static svn_error_t *
test17(apr_pool_t *pool)
{
  a = svn_stringbuf_create("test test test", pool);

  return test_find_char_backward(a->data,
                                 a->len,
                                 ',',
                                 a->len,
                                 pool);
}

static svn_error_t *
test_first_non_whitespace(const char *str,
                          const apr_size_t pos,
                          apr_pool_t *pool)
{
  apr_size_t i;

  a = svn_stringbuf_create(str, pool);

  i = svn_stringbuf_first_non_whitespace(a);

  if (i == pos)
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}

static svn_error_t *
test18(apr_pool_t *pool)
{
  return test_first_non_whitespace("   \ttest", 4, pool);
}

static svn_error_t *
test19(apr_pool_t *pool)
{
  return test_first_non_whitespace("test", 0, pool);
}

static svn_error_t *
test20(apr_pool_t *pool)
{
  return test_first_non_whitespace("   ", 3, pool);
}

static svn_error_t *
test21(apr_pool_t *pool)
{
  a = svn_stringbuf_create("    \ttest\t\t  \t  ", pool);
  b = svn_stringbuf_create("test", pool);

  svn_stringbuf_strip_whitespace(a);

  if (svn_stringbuf_compare(a, b) == TRUE)
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}

static svn_error_t *
test_stringbuf_unequal(const char* str1,
                       const char* str2,
                       apr_pool_t *pool)
{
  a = svn_stringbuf_create(str1, pool);
  b = svn_stringbuf_create(str2, pool);

  if (svn_stringbuf_compare(a, b))
    return fail(pool, "test failed");
  else
    return SVN_NO_ERROR;
}

static svn_error_t *
test22(apr_pool_t *pool)
{
  return test_stringbuf_unequal("abc", "abcd", pool);
}

static svn_error_t *
test23(apr_pool_t *pool)
{
  return test_stringbuf_unequal("abc", "abb", pool);
}

static svn_error_t *
test24(apr_pool_t *pool)
{
  char buffer[SVN_INT64_BUFFER_SIZE];
  apr_size_t length;

  length = svn__i64toa(buffer, 0);
  SVN_TEST_ASSERT(length == 1);
  SVN_TEST_STRING_ASSERT(buffer, "0");

  length = svn__i64toa(buffer, 0x8000000000000000ll);
  SVN_TEST_ASSERT(length == 20);
  SVN_TEST_STRING_ASSERT(buffer, "-9223372036854775808");

  length = svn__i64toa(buffer, 0x7fffffffffffffffll);
  SVN_TEST_ASSERT(length == 19);
  SVN_TEST_STRING_ASSERT(buffer, "9223372036854775807");

  length = svn__ui64toa(buffer, 0ull);
  SVN_TEST_ASSERT(length == 1);
  SVN_TEST_STRING_ASSERT(buffer, "0");

  length = svn__ui64toa(buffer, 0xffffffffffffffffull);
  SVN_TEST_ASSERT(length == 20);
  SVN_TEST_STRING_ASSERT(buffer, "18446744073709551615");

  return test_stringbuf_unequal("abc", "abb", pool);
}

static svn_error_t *
expect_stringbuf_equal(const svn_stringbuf_t* str1,
                       const char* str2,
                       apr_pool_t *pool)
{
  if (svn_stringbuf_compare(str1, svn_stringbuf_create(str2, pool)))
    return SVN_NO_ERROR;
  else
    return fail(pool, "test failed");
}

static svn_error_t *
test_stringbuf_insert(apr_pool_t *pool)
{
  a = svn_stringbuf_create("st , ", pool);

  svn_stringbuf_insert(a, 0, "teflon", 2);
  SVN_TEST_STRING_ASSERT(a->data, "test , ");

  svn_stringbuf_insert(a, 5, "hllo", 4);
  SVN_TEST_STRING_ASSERT(a->data, "test hllo, ");

  svn_stringbuf_insert(a, 6, a->data + 1, 1);
  SVN_TEST_STRING_ASSERT(a->data, "test hello, ");
  
  svn_stringbuf_insert(a, 12, "world class", 5);
  SVN_TEST_STRING_ASSERT(a->data, "test hello, world");

  svn_stringbuf_insert(a, 1200, "!", 1);
  return expect_stringbuf_equal(a, "test hello, world!", pool);
}

static svn_error_t *
test_stringbuf_remove(apr_pool_t *pool)
{
  a = svn_stringbuf_create("test hello, world!", pool);

  svn_stringbuf_remove(a, 0, 2);
  SVN_TEST_STRING_ASSERT(a->data, "st hello, world!");

  svn_stringbuf_remove(a, 2, 2);
  SVN_TEST_STRING_ASSERT(a->data, "stello, world!");

  svn_stringbuf_remove(a, 5, 200);
  SVN_TEST_STRING_ASSERT(a->data, "stell");

  svn_stringbuf_remove(a, 1200, 393);
  return expect_stringbuf_equal(a, "stell", pool);
}

static svn_error_t *
test_stringbuf_replace(apr_pool_t *pool)
{
  a = svn_stringbuf_create("odd with some world?", pool);

  svn_stringbuf_replace(a, 0, 3, "tester", 4);
  SVN_TEST_STRING_ASSERT(a->data, "test with some world?");

  svn_stringbuf_replace(a, 5, 10, "hllo, coder", 6);
  SVN_TEST_STRING_ASSERT(a->data, "test hllo, world?");

  svn_stringbuf_replace(a, 6, 0, a->data + 1, 1);
  SVN_TEST_STRING_ASSERT(a->data, "test hello, world?");

  svn_stringbuf_replace(a, 17, 10, "!", 1);
  SVN_TEST_STRING_ASSERT(a->data, "test hello, world!");

  svn_stringbuf_replace(a, 1200, 199, "!!", 2);

  return expect_stringbuf_equal(a, "test hello, world!!!", pool);
}

static svn_error_t *
test_string_similarity(apr_pool_t *pool)
{
  const struct sim_score_test_t
  {
    const char *stra;
    const char *strb;
    apr_size_t lcs;
    int score;
  } tests[] =
      {
#define SCORE(lcs, len) ((2000 * (lcs) + (len)/2) / (len))

        /* Equality */
        {"",       "",          0, 1000},
        {"quoth",  "quoth",     5, SCORE(5, 5+5)},

        /* Deletion at start */
        {"quoth",  "uoth",      4, SCORE(4, 5+4)},
        {"uoth",   "quoth",     4, SCORE(4, 4+5)},

        /* Deletion at end */
        {"quoth",  "quot",      4, SCORE(4, 5+4)},
        {"quot",   "quoth",     4, SCORE(4, 4+5)},

        /* Insertion at start */
        {"quoth",  "Xquoth",    5, SCORE(5, 5+6)},
        {"Xquoth", "quoth",     5, SCORE(5, 6+5)},

        /* Insertion at end */
        {"quoth",  "quothX",    5, SCORE(5, 5+6)},
        {"quothX", "quoth",     5, SCORE(5, 6+5)},

        /* Insertion in middle */
        {"quoth",  "quoXth",    5, SCORE(5, 5+6)},
        {"quoXth", "quoth",     5, SCORE(5, 6+5)},

        /* Transposition at start */
        {"quoth",  "uqoth",     4, SCORE(4, 5+5)},
        {"uqoth",  "quoth",     4, SCORE(4, 5+5)},

        /* Transposition at end */
        {"quoth",  "quoht",     4, SCORE(4, 5+5)},
        {"quoht",  "quoth",     4, SCORE(4, 5+5)},

        /* Transposition in middle */
        {"quoth",  "qutoh",     4, SCORE(4, 5+5)},
        {"qutoh",  "quoth",     4, SCORE(4, 5+5)},

        /* Difference */
        {"quoth",  "raven",     0, SCORE(0, 5+5)},
        {"raven",  "quoth",     0, SCORE(0, 5+5)},
        {"x",      "",          0, SCORE(0, 1+0)},
        {"",       "x",         0, SCORE(0, 0+1)},
        {"",       "quoth",     0, SCORE(0, 0+5)},
        {"quoth",  "",          0, SCORE(0, 5+0)},
        {"quoth",  "the raven", 2, SCORE(2, 5+9)},
        {"the raven",  "quoth", 2, SCORE(2, 5+9)},
        {NULL, NULL}
      };

  const struct sim_score_test_t *t;
  svn_membuf_t buffer;

  svn_membuf__create(&buffer, 0, pool);
  for (t = tests; t->stra; ++t)
    {
      apr_size_t lcs;
      const unsigned int score =
        svn_cstring__similarity(t->stra, t->strb, &buffer, &lcs);
      /*
      fprintf(stderr,
              "lcs %s ~ %s score %.3f (%"APR_SIZE_T_FMT
              ") expected %.3f (%"APR_SIZE_T_FMT"))\n",
              t->stra, t->strb, score/1000.0, lcs, t->score/1000.0, t->lcs);
      */
      if (score != t->score)
        return fail(pool, "%s ~ %s score %.3f <> expected %.3f",
                    t->stra, t->strb, score/1000.0, t->score/1000.0);

      if (lcs != t->lcs)
        return fail(pool,
                    "%s ~ %s lcs %"APR_SIZE_T_FMT
                    " <> expected "APR_SIZE_T_FMT,
                    t->stra, t->strb, lcs, t->lcs);
    }

  return SVN_NO_ERROR;
}

/*
   ====================================================================
   If you add a new test to this file, update this array.

   (These globals are required by our included main())
*/

/* An array of all test functions */
struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test1,
                   "make svn_stringbuf_t from cstring"),
    SVN_TEST_PASS2(test2,
                   "make svn_stringbuf_t from substring of cstring"),
    SVN_TEST_PASS2(test3,
                   "append svn_stringbuf_t to svn_stringbuf_t"),
    SVN_TEST_PASS2(test4,
                   "append C string to svn_stringbuf_t"),
    SVN_TEST_PASS2(test5,
                   "append bytes, then compare two strings"),
    SVN_TEST_PASS2(test6,
                   "dup two strings, then compare"),
    SVN_TEST_PASS2(test7,
                   "chopping a string"),
    SVN_TEST_PASS2(test8,
                   "emptying a string"),
    SVN_TEST_PASS2(test9,
                   "fill string with hashmarks"),
    SVN_TEST_PASS2(test10,
                   "block initialization and growth"),
    SVN_TEST_PASS2(test11,
                   "formatting strings from varargs"),
    SVN_TEST_PASS2(test12,
                   "create string from file"),
    SVN_TEST_PASS2(test13,
                   "find_char_backward; middle case"),
    SVN_TEST_PASS2(test14,
                   "find_char_backward; 0 case"),
    SVN_TEST_PASS2(test15,
                   "find_char_backward; strlen - 1 case"),
    SVN_TEST_PASS2(test16,
                   "find_char_backward; len = 0 case"),
    SVN_TEST_PASS2(test17,
                   "find_char_backward; no occurence case"),
    SVN_TEST_PASS2(test18,
                   "check whitespace removal; common case"),
    SVN_TEST_PASS2(test19,
                   "check whitespace removal; no whitespace case"),
    SVN_TEST_PASS2(test20,
                   "check whitespace removal; all whitespace case"),
    SVN_TEST_PASS2(test21,
                   "check that whitespace will be stripped correctly"),
    SVN_TEST_PASS2(test22,
                   "compare stringbufs; different lengths"),
    SVN_TEST_PASS2(test23,
                   "compare stringbufs; same length, different content"),
    SVN_TEST_PASS2(test24,
                   "verify i64toa"),
    SVN_TEST_PASS2(test_stringbuf_insert,
                   "check inserting into svn_stringbuf_t"),
    SVN_TEST_PASS2(test_stringbuf_remove,
                   "check deletion from svn_stringbuf_t"),
    SVN_TEST_PASS2(test_stringbuf_replace,
                   "check replacement in svn_stringbuf_t"),
    SVN_TEST_PASS2(test_string_similarity,
                   "test string similarity scores"),
    SVN_TEST_NULL
  };
