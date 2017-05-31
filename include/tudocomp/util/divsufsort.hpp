/*
 * This file integrates customized parts of divsufsort into tudocomp.
 * divsufsort is licensed under the MIT License, which follows.
 *
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <tudocomp/util/divsufsort/divsufsort_def.hpp>
#include <tudocomp/util/divsufsort/divsufsort_private.hpp>
#include <tudocomp/util/divsufsort/divsufsort_ssort.hpp>
#include <tudocomp/util/divsufsort/divsufsort_trsort.hpp>
#include <tudocomp/util/divsufsort/divsufsort_bufwrapper.hpp>

#include <tudocomp/ds/IntVector.hpp>

namespace tdc {
namespace libdivsufsort {

// from divsufsort.c
/* Sorts suffixes of type B*. */
template<typename buffer_t>
inline saidx_t sort_typeBstar(
    const sauchar_t *T, buffer_t& SA,
          saidx_t *bucket_A, saidx_t *bucket_B,
          saidx_t n) {

  saidx_t PAb, ISAb, buf;
  saidx_t i, j, k, t, m, bufsize;
  saint_t c0, c1;

  /* Initialize bucket arrays. */
  for(i = 0; i < BUCKET_A_SIZE; ++i) { bucket_A[i] = 0; }
  for(i = 0; i < BUCKET_B_SIZE; ++i) { bucket_B[i] = 0; }

  /* Count the number of occurrences of the first one or two characters of each
     type A, B and B* suffix. Moreover, store the beginning position of all
     type B* suffixes into the array SA. */
  for(i = n - 1, m = n, c0 = T[n - 1]; 0 <= i;) {
    /* type A suffix. */
    do { ++BUCKET_A(c1 = c0); } while((0 <= --i) && ((c0 = T[i]) >= c1));
    if(0 <= i) {
      /* type B* suffix. */
      ++BUCKET_BSTAR(c0, c1);
      SA[--m] = i;
      /* type B suffix. */
      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) {
        ++BUCKET_B(c0, c1);
      }
    }
  }
  m = n - m;
/*
note:
  A type B* suffix is lexicographically smaller than a type B suffix that
  begins with the same first two characters.
*/

  /* Calculate the index of start/end point of each bucket. */
  for(c0 = 0, i = 0, j = 0; c0 < ALPHABET_SIZE; ++c0) {
    t = i + BUCKET_A(c0);
    BUCKET_A(c0) = i + j; /* start point */
    i = t + BUCKET_B(c0, c0);
    for(c1 = c0 + 1; c1 < ALPHABET_SIZE; ++c1) {
      j += BUCKET_BSTAR(c0, c1);
      BUCKET_BSTAR(c0, c1) = j; /* end point */
      i += BUCKET_B(c0, c1);
    }
  }

  if(0 < m) {
    /* Sort the type B* suffixes by their first two characters. */
    PAb = n - m; ISAb = m;
    for(i = m - 2; 0 <= i; --i) {
      t = SA[PAb + i], c0 = T[t], c1 = T[t + 1];
      SA[--BUCKET_BSTAR(c0, c1)] = i;
    }
    t = SA[PAb + m - 1], c0 = T[t], c1 = T[t + 1];
    SA[--BUCKET_BSTAR(c0, c1)] = m - 1;

    /* Sort the type B* substrings using sssort. */
    buf = m, bufsize = n - (2 * m);
    for(c0 = ALPHABET_SIZE - 2, j = m; 0 < j; --c0) {
      for(c1 = ALPHABET_SIZE - 1; c0 < c1; j = i, --c1) {
        i = BUCKET_BSTAR(c0, c1);
        if(1 < (j - i)) {
          sssort(T, SA, PAb, i, j,
                 buf, bufsize, 2, n, SA[i] == (m - 1));
        }
      }
    }

    /* Compute ranks of type B* substrings. */
    for(i = m - 1; 0 <= i; --i) {
      if(0 <= SA[i]) {
        j = i;
        do { SA[ISAb + SA[i]] = i; } while((0 <= --i) && (0 <= SA[i]));
        SA[i + 1] = i - j;
        if(i <= 0) { break; }
      }
      j = i;
      do { SA[i] = ~SA[i]; SA[ISAb + SA[i]] = j; } while(SA[--i] < 0);
      SA[ISAb + SA[i]] = j;
    }

    /* Construct the inverse suffix array of type B* suffixes using trsort. */
    trsort(SA, ISAb, 0, m, 1);

    /* Set the sorted order of tyoe B* suffixes. */
    for(i = n - 1, j = m, c0 = T[n - 1]; 0 <= i;) {
      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) >= c1); --i, c1 = c0) { }
      if(0 <= i) {
        t = i;
        for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) { }
        SA[SA[ISAb + (--j)]] = ((t == 0) || (1 < (t - i))) ? t : ~t;
      }
    }

    /* Calculate the index of start/end point of each bucket. */
    BUCKET_B(ALPHABET_SIZE - 1, ALPHABET_SIZE - 1) = n; /* end point */
    for(c0 = ALPHABET_SIZE - 2, k = m - 1; 0 <= c0; --c0) {
      i = BUCKET_A(c0 + 1) - 1;
      for(c1 = ALPHABET_SIZE - 1; c0 < c1; --c1) {
        t = i - BUCKET_B(c0, c1);
        BUCKET_B(c0, c1) = i; /* end point */

        /* Move all type B* suffixes to the correct position. */
        for(i = t, j = BUCKET_BSTAR(c0, c1);
            j <= k;
            --i, --k) { SA[i] = SA[k]; }
      }
      BUCKET_BSTAR(c0, c0 + 1) = i - BUCKET_B(c0, c0) + 1; /* start point */
      BUCKET_B(c0, c0) = i; /* end point */
    }
  }

  return m;
}

// from divsufsort.c
/* Constructs the suffix array by using the sorted order of type B* suffixes. */
template<typename buffer_t>
inline void construct_SA(
        const sauchar_t *T, buffer_t& SA,
              saidx_t *bucket_A, saidx_t *bucket_B,
              saidx_t n, saidx_t m) {

  saidx_t i, j, k;
  saidx_t s;
  saint_t c0, c1, c2;

  if(0 < m) {
    /* Construct the sorted order of type B suffixes by using
       the sorted order of type B* suffixes. */
    for(c1 = ALPHABET_SIZE - 2; 0 <= c1; --c1) {
      /* Scan the suffix array from right to left. */
      for(i = BUCKET_BSTAR(c1, c1 + 1),
          j = BUCKET_A(c1 + 1) - 1, k = -1, c2 = -1;
          i <= j;
          --j) {
        if(0 < (s = SA[j])) {
          assert(T[s] == c1);
          assert(((s + 1) < n) && (T[s] <= T[s + 1]));
          assert(T[s - 1] <= T[s]);
          SA[j] = ~s;
          c0 = T[--s];
          if((0 < s) && (T[s - 1] > c0)) { s = ~s; }
          if(c0 != c2) {
            if(0 <= c2) { BUCKET_B(c2, c1) = k; }

            k = BUCKET_B(c2 = c0, c1);
          }
          assert(k < j);
          SA[k--] = s;
        } else {
          assert(((s == 0) && (T[s] == c1)) || (s < 0));
          SA[j] = ~s;
        }
      }
    }
  }

  /* Construct the suffix array by using
     the sorted order of type B suffixes. */
  k = BUCKET_A(c2 = T[n - 1]);
  SA[k++] = (T[n - 2] < c2) ? ~(n - 1) : (n - 1);
  /* Scan the suffix array from left to right. */
  for(i = 0, j = n; i < j; ++i) {
    if(0 < (s = SA[i])) {
      assert(T[s - 1] >= T[s]);
      c0 = T[--s];
      if((s == 0) || (T[s - 1] < c0)) { s = ~s; }
      if(c0 != c2) {
        BUCKET_A(c2) = k;
        k = BUCKET_A(c2 = c0);
      }
      assert(i < k);
      SA[k++] = s;
    } else {
      assert(s < 0);
      SA[i] = ~s;
    }
  }
}

// the actual divsufsort execution
template<typename buffer_t>
inline void divsufsort_run(
    const sauchar_t* T, buffer_t& SA,
    saidx_t *bucket_A, saidx_t *bucket_B, saidx_t n) {

    // sign check
    SA[0] = -1; DCHECK(SA[0] < 0) << "only signed integer buffers are supported";

    saidx_t m = sort_typeBstar(T, SA, bucket_A, bucket_B, n);
    construct_SA(T, SA, bucket_A, bucket_B, n, m);
}

// specialize for len_t vectors
template<>
inline void divsufsort_run<std::vector<index_t>>(
    const sauchar_t* T, std::vector<index_t>& SA,
    saidx_t *bucket_A, saidx_t *bucket_B, saidx_t n) {

    BufferWrapper<std::vector<index_t>> wrapSA(SA);
    divsufsort_run(T, wrapSA, bucket_A, bucket_B, n);
}

// specialize for DynamicIntVector
template<>
inline void divsufsort_run<DynamicIntVector>(
    const sauchar_t* T, DynamicIntVector& SA,
    saidx_t *bucket_A, saidx_t *bucket_B, saidx_t n) {

    BufferWrapper<DynamicIntVector> wrapSA(SA);
    divsufsort_run(T, wrapSA, bucket_A, bucket_B, n);
}

// from divsufsort.c
template<typename buffer_t>
inline saint_t divsufsort(const sauchar_t* T, buffer_t& SA, saidx_t n) {
  saidx_t *bucket_A, *bucket_B;
  saidx_t m;
  saint_t err = 0;

  /* Check arguments. */
  if((T == NULL) || (n < 0)) { return -1; }
  else if(n == 0) { return 0; }
  else if(n == 1) { SA[0] = 0; return 0; }
  else if(n == 2) { m = (T[0] < T[1]); SA[m ^ 1] = 0, SA[m] = 1; return 0; }

  bucket_A = new saidx_t[BUCKET_A_SIZE];
  bucket_B = new saidx_t[BUCKET_B_SIZE];

  /* Suffixsort. */
  if((bucket_A != NULL) && (bucket_B != NULL)) {
      divsufsort_run(T, SA, bucket_A, bucket_B, n);
  } else {
      err = -2;
  }

  delete[] bucket_B;
  delete[] bucket_A;

  return err;
}

} //ns divsufsort

using libdivsufsort::saidx_t;
using libdivsufsort::divsufsort;

} //ns tdc
