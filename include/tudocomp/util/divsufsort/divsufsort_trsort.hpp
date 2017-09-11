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

/// \cond INTERNAL
namespace tdc {
namespace libdivsufsort {

/*---------------------------------------------------------------------------*/

/* Simple insertionsort for small size groups. */
template<typename buffer_t>
inline void tr_insertionsort(buffer_t& B, const saidx_t ISAd, saidx_t first, saidx_t last) {
  saidx_t a, b;
  saidx_t t, r;

  for(a = first + 1; a < last; ++a) {
    for(t = B[a], b = a - 1; 0 > (r = B[ISAd + t] - B[ISAd + B[b]]);) {
      do { B[b + 1] = B[b]; } while((first <= --b) && (B[b] < 0));
      if(b < first) { break; }
    }
    if(r == 0) { B[b] = ~B[b]; }
    B[b + 1] = t;
  }
}


/*---------------------------------------------------------------------------*/

template<typename buffer_t>
inline void tr_fixdown(buffer_t& B, const saidx_t ISAd, saidx_t SA, saidx_t i, saidx_t size) {
  saidx_t j, k;
  saidx_t v;
  saidx_t c, d, e;

  for(v = B[SA + i], c = B[ISAd + v]; (j = 2 * i + 1) < size; B[SA + i] = B[SA + k], i = k) {
    k = j++;
    d = B[ISAd + B[SA + k]];
    if(d < (e = B[ISAd + B[SA + j]])) { k = j; d = e; }
    if(d <= c) { break; }
  }
  B[SA + i] = v;
}

/* Simple top-down heapsort. */
template<typename buffer_t>
inline void tr_heapsort(buffer_t& B, const saidx_t ISAd, saidx_t SA, saidx_t size) {
  saidx_t i, m;
  saidx_t t;

  m = size;
  if((size % 2) == 0) {
    m--;
    if(B[ISAd + B[SA + m / 2]] < B[ISAd + B[SA + m]]) { SWAP(B[SA + m], B[SA + m / 2]); }
  }

  for(i = m / 2 - 1; 0 <= i; --i) { tr_fixdown(B, ISAd, SA, i, m); }
  if((size % 2) == 0) { SWAP(B[SA + 0], B[SA + m]); tr_fixdown(B, ISAd, SA, 0, m); }
  for(i = m - 1; 0 < i; --i) {
    t = B[SA + 0], B[SA + 0] = B[SA + i];
    tr_fixdown(B, ISAd, SA, 0, i);
    B[SA + i] = t;
  }
}


/*---------------------------------------------------------------------------*/

/* Returns the median of three elements. */
template<typename buffer_t>
inline saidx_t tr_median3(buffer_t& B, const saidx_t ISAd, saidx_t v1, saidx_t v2, saidx_t v3) {
  saidx_t t;
  if(B[ISAd + B[v1]] > B[ISAd + B[v2]]) { SWAP(v1, v2); }
  if(B[ISAd + B[v2]] > B[ISAd + B[v3]]) {
    if(B[ISAd + B[v1]] > B[ISAd + B[v3]]) { return v1; }
    else { return v3; }
  }
  return v2;
}

/* Returns the median of five elements. */
template<typename buffer_t>
inline saidx_t tr_median5(buffer_t& B, const saidx_t ISAd,
           saidx_t v1, saidx_t v2, saidx_t v3, saidx_t v4, saidx_t v5) {
  saidx_t t;
  if(B[ISAd + B[v2]] > B[ISAd + B[v3]]) { SWAP(v2, v3); }
  if(B[ISAd + B[v4]] > B[ISAd + B[v5]]) { SWAP(v4, v5); }
  if(B[ISAd + B[v2]] > B[ISAd + B[v4]]) { SWAP(v2, v4); SWAP(v3, v5); }
  if(B[ISAd + B[v1]] > B[ISAd + B[v3]]) { SWAP(v1, v3); }
  if(B[ISAd + B[v1]] > B[ISAd + B[v4]]) { SWAP(v1, v4); SWAP(v3, v5); }
  if(B[ISAd + B[v3]] > B[ISAd + B[v4]]) { return v4; }
  return v3;
}

/* Returns the pivot element. */
template<typename buffer_t>
inline saidx_t tr_pivot(buffer_t& B, const saidx_t ISAd, saidx_t first, saidx_t last) {
  saidx_t middle;
  saidx_t t;

  t = last - first;
  middle = first + t / 2;

  if(t <= 512) {
    if(t <= 32) {
      return tr_median3(B, ISAd, first, middle, last - 1);
    } else {
      t >>= 2;
      return tr_median5(B, ISAd, first, first + t, middle, last - 1 - t, last - 1);
    }
  }
  t >>= 3;
  first  = tr_median3(B, ISAd, first, first + t, first + (t << 1));
  middle = tr_median3(B, ISAd, middle - t, middle, middle + t);
  last   = tr_median3(B, ISAd, last - 1 - (t << 1), last - 1 - t, last - 1);
  return tr_median3(B, ISAd, first, middle, last);
}


/*---------------------------------------------------------------------------*/

typedef struct _trbudget_t trbudget_t;
struct _trbudget_t {
  saidx_t chance;
  saidx_t remain;
  saidx_t incval;
  saidx_t count;
};

inline void trbudget_init(trbudget_t *budget, saidx_t chance, saidx_t incval) {
  budget->chance = chance;
  budget->remain = budget->incval = incval;
}

inline saint_t trbudget_check(trbudget_t *budget, saidx_t size) {
  if(size <= budget->remain) { budget->remain -= size; return 1; }
  if(budget->chance == 0) { budget->count += size; return 0; }
  budget->remain += budget->incval - size;
  budget->chance -= 1;
  return 1;
}


/*---------------------------------------------------------------------------*/

template<typename buffer_t>
inline void tr_partition(buffer_t& B, const saidx_t ISAd,
             saidx_t first, saidx_t middle, saidx_t last,
             saidx_t *pa, saidx_t *pb, saidx_t v) {
  saidx_t a, b, c, d, e, f;
  saidx_t t, s;
  saidx_t x = 0;

  for(b = middle - 1; (++b < last) && ((x = B[ISAd + B[b]]) == v);) { }
  if(((a = b) < last) && (x < v)) {
    for(; (++b < last) && ((x = B[ISAd + B[b]]) <= v);) {
      if(x == v) { SWAP(B[b], B[a]); ++a; }
    }
  }
  for(c = last; (b < --c) && ((x = B[ISAd + B[c]]) == v);) { }
  if((b < (d = c)) && (x > v)) {
    for(; (b < --c) && ((x = B[ISAd + B[c]]) >= v);) {
      if(x == v) { SWAP(B[c], B[d]); --d; }
    }
  }
  for(; b < c;) {
    SWAP(B[b], B[c]);
    for(; (++b < c) && ((x = B[ISAd + B[b]]) <= v);) {
      if(x == v) { SWAP(B[b], B[a]); ++a; }
    }
    for(; (b < --c) && ((x = B[ISAd + B[c]]) >= v);) {
      if(x == v) { SWAP(B[c], B[d]); --d; }
    }
  }

  if(a <= d) {
    c = b - 1;
    if((s = a - first) > (t = b - a)) { s = t; }
    for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { SWAP(B[e], B[f]); }
    if((s = d - c) > (t = last - d - 1)) { s = t; }
    for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { SWAP(B[e], B[f]); }
    first += (b - a), last -= (d - c);
  }
  *pa = first, *pb = last;
}

template<typename buffer_t>
inline void tr_copy(buffer_t& B, saidx_t ISA, const saidx_t SA,
        saidx_t first, saidx_t a, saidx_t b, saidx_t last,
        saidx_t depth) {
  /* sort suffixes of middle partition
     by using sorted order of suffixes of left and right partition. */
  saidx_t c, d, e;
  saidx_t s, v;

  v = b - SA - 1;
  for(c = first, d = a - 1; c <= d; ++c) {
    if((0 <= (s = B[c] - depth)) && (B[ISA + s] == v)) {
      B[++d] = s;
      B[ISA + s] = d - SA;
    }
  }
  for(c = last - 1, e = d + 1, d = b; e < d; --c) {
    if((0 <= (s = B[c] - depth)) && (B[ISA + s] == v)) {
      B[--d] = s;
      B[ISA + s] = d - SA;
    }
  }
}

template<typename buffer_t>
inline void tr_partialcopy(buffer_t& B, saidx_t ISA, const saidx_t SA,
               saidx_t first, saidx_t a, saidx_t b, saidx_t last,
               saidx_t depth) {
  saidx_t c, d, e;
  saidx_t s, v;
  saidx_t rank, lastrank, newrank = -1;

  v = b - SA - 1;
  lastrank = -1;
  for(c = first, d = a - 1; c <= d; ++c) {
    if((0 <= (s = B[c] - depth)) && (B[ISA + s] == v)) {
      B[++d] = s;
      rank = B[ISA + s + depth];
      if(lastrank != rank) { lastrank = rank; newrank = d - SA; }
      B[ISA + s] = newrank;
    }
  }

  lastrank = -1;
  for(e = d; first <= e; --e) {
    rank = B[ISA + B[e]];
    if(lastrank != rank) { lastrank = rank; newrank = e - SA; }
    if(newrank != rank) { B[ISA + B[e]] = newrank; }
  }

  lastrank = -1;
  for(c = last - 1, e = d + 1, d = b; e < d; --c) {
    if((0 <= (s = B[c] - depth)) && (B[ISA + s] == v)) {
      B[--d] = s;
      rank = B[ISA + s + depth];
      if(lastrank != rank) { lastrank = rank; newrank = d - SA; }
      B[ISA + s] = newrank;
    }
  }
}

template<typename buffer_t>
inline void tr_introsort(buffer_t& B, saidx_t ISA, saidx_t ISAd,
             saidx_t SA, saidx_t first, saidx_t last,
             trbudget_t *budget) {
#define STACK_SIZE TR_STACKSIZE
  struct { saidx_t a, b, c; saint_t d, e; }stack[STACK_SIZE];
  saidx_t a, b, c;
  saidx_t t;
  saidx_t v, x = 0;
  saidx_t incr = ISAd - ISA;
  saint_t limit, next;
  saint_t ssize, trlink = -1;

  for(ssize = 0, limit = ilg<saidx_t>(last - first);;) {

    if(limit < 0) {
      if(limit == -1) {
        /* tandem repeat partition */
        tr_partition(B, ISAd - incr, first, first, last, &a, &b, last - SA - 1);

        /* update ranks */
        if(a < last) {
          for(c = first, v = a - SA - 1; c < a; ++c) { B[ISA + B[c]] = v; }
        }
        if(b < last) {
          for(c = a, v = b - SA - 1; c < b; ++c) { B[ISA + B[c]] = v; }
        }

        /* push */
        if(1 < (b - a)) {
          STACK_PUSH5(-1, a, b, 0, 0); //TODO: is -1 instead of NULL good?
          STACK_PUSH5(ISAd - incr, first, last, -2, trlink);
          trlink = ssize - 2;
        }
        if((a - first) <= (last - b)) {
          if(1 < (a - first)) {
            STACK_PUSH5(ISAd, b, last, ilg<saidx_t>(last - b), trlink);
            last = a, limit = ilg<saidx_t>(a - first);
          } else if(1 < (last - b)) {
            first = b, limit = ilg<saidx_t>(last - b);
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        } else {
          if(1 < (last - b)) {
            STACK_PUSH5(ISAd, first, a, ilg<saidx_t>(a - first), trlink);
            first = b, limit = ilg<saidx_t>(last - b);
          } else if(1 < (a - first)) {
            last = a, limit = ilg<saidx_t>(a - first);
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        }
      } else if(limit == -2) {
        /* tandem repeat copy */
        a = stack[--ssize].b, b = stack[ssize].c;
        if(stack[ssize].d == 0) {
          tr_copy(B, ISA, SA, first, a, b, last, ISAd - ISA);
        } else {
          if(0 <= trlink) { stack[trlink].d = -1; }
          tr_partialcopy(B, ISA, SA, first, a, b, last, ISAd - ISA);
        }
        STACK_POP5(ISAd, first, last, limit, trlink);
      } else {
        /* sorted partition */
        if(0 <= B[first]) {
          a = first;
          do { B[ISA + B[a]] = a - SA; } while((++a < last) && (0 <= B[a]));
          first = a;
        }
        if(first < last) {
          a = first; do { B[a] = ~B[a]; } while(B[++a] < 0);
          next = (B[ISA + B[a]] != B[ISAd + B[a]]) ? ilg<saidx_t>(a - first + 1) : -1;
          if(++a < last) { for(b = first, v = a - SA - 1; b < a; ++b) { B[ISA + B[b]] = v; } }

          /* push */
          if(trbudget_check(budget, a - first)) {
            if((a - first) <= (last - a)) {
              STACK_PUSH5(ISAd, a, last, -3, trlink);
              ISAd += incr, last = a, limit = next;
            } else {
              if(1 < (last - a)) {
                STACK_PUSH5(ISAd + incr, first, a, next, trlink);
                first = a, limit = -3;
              } else {
                ISAd += incr, last = a, limit = next;
              }
            }
          } else {
            if(0 <= trlink) { stack[trlink].d = -1; }
            if(1 < (last - a)) {
              first = a, limit = -3;
            } else {
              STACK_POP5(ISAd, first, last, limit, trlink);
            }
          }
        } else {
          STACK_POP5(ISAd, first, last, limit, trlink);
        }
      }
      continue;
    }

    if((last - first) <= TR_INSERTIONSORT_THRESHOLD) {
      tr_insertionsort(B, ISAd, first, last);
      limit = -3;
      continue;
    }

    if(limit-- == 0) {
      tr_heapsort(B, ISAd, first, last - first);
      for(a = last - 1; first < a; a = b) {
        for(x = B[ISAd + B[a]], b = a - 1; (first <= b) && (B[ISAd + B[b]] == x); --b) { B[b] = ~B[b]; }
      }
      limit = -3;
      continue;
    }

    /* choose pivot */
    a = tr_pivot(B, ISAd, first, last);
    SWAP(B[first], B[a]);
    v = B[ISAd + B[first]];

    /* partition */
    tr_partition(B, ISAd, first, first + 1, last, &a, &b, v);
    if((last - first) != (b - a)) {
      next = (B[ISA + B[a]] != v) ? ilg<saidx_t>(b - a) : -1;

      /* update ranks */
      for(c = first, v = a - SA - 1; c < a; ++c) { B[ISA + B[c]] = v; }
      if(b < last) { for(c = a, v = b - SA - 1; c < b; ++c) { B[ISA + B[c]] = v; } }

      /* push */
      if((1 < (b - a)) && (trbudget_check(budget, b - a))) {
        if((a - first) <= (last - b)) {
          if((last - b) <= (b - a)) {
            if(1 < (a - first)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              STACK_PUSH5(ISAd, b, last, limit, trlink);
              last = a;
            } else if(1 < (last - b)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              first = b;
            } else {
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else if((a - first) <= (b - a)) {
            if(1 < (a - first)) {
              STACK_PUSH5(ISAd, b, last, limit, trlink);
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              last = a;
            } else {
              STACK_PUSH5(ISAd, b, last, limit, trlink);
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else {
            STACK_PUSH5(ISAd, b, last, limit, trlink);
            STACK_PUSH5(ISAd, first, a, limit, trlink);
            ISAd += incr, first = a, last = b, limit = next;
          }
        } else {
          if((a - first) <= (b - a)) {
            if(1 < (last - b)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              STACK_PUSH5(ISAd, first, a, limit, trlink);
              first = b;
            } else if(1 < (a - first)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              last = a;
            } else {
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else if((last - b) <= (b - a)) {
            if(1 < (last - b)) {
              STACK_PUSH5(ISAd, first, a, limit, trlink);
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              first = b;
            } else {
              STACK_PUSH5(ISAd, first, a, limit, trlink);
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else {
            STACK_PUSH5(ISAd, first, a, limit, trlink);
            STACK_PUSH5(ISAd, b, last, limit, trlink);
            ISAd += incr, first = a, last = b, limit = next;
          }
        }
      } else {
        if((1 < (b - a)) && (0 <= trlink)) { stack[trlink].d = -1; }
        if((a - first) <= (last - b)) {
          if(1 < (a - first)) {
            STACK_PUSH5(ISAd, b, last, limit, trlink);
            last = a;
          } else if(1 < (last - b)) {
            first = b;
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        } else {
          if(1 < (last - b)) {
            STACK_PUSH5(ISAd, first, a, limit, trlink);
            first = b;
          } else if(1 < (a - first)) {
            last = a;
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        }
      }
    } else {
      if(trbudget_check(budget, last - first)) {
        limit = ilg<saidx_t>(last - first), ISAd += incr;
      } else {
        if(0 <= trlink) { stack[trlink].d = -1; }
        STACK_POP5(ISAd, first, last, limit, trlink);
      }
    }
  }
#undef STACK_SIZE
}



/*---------------------------------------------------------------------------*/

/*- Function -*/

/* Tandem repeat sort */
template<typename buffer_t>
inline void trsort(buffer_t& B, saidx_t ISA, saidx_t SA, saidx_t n, saidx_t depth) {
  saidx_t ISAd;
  saidx_t first, last;
  trbudget_t budget;
  saidx_t t, skip, unsorted;

  trbudget_init(&budget, ilg<saidx_t>(n) * 2 / 3, n);
  for(ISAd = ISA + depth; -n < B[SA]; ISAd += ISAd - ISA) {
    first = SA;
    skip = 0;
    unsorted = 0;
    do {
      if((t = B[first]) < 0) { first -= t; skip += t; }
      else {
        if(skip != 0) { B[first + skip] = skip; skip = 0; }
        last = SA + B[ISA + t] + 1;
        if(1 < (last - first)) {
          budget.count = 0;
          tr_introsort(B, ISA, ISAd, SA, first, last, &budget);
          if(budget.count != 0) { unsorted += budget.count; }
          else { skip = first - last; }
        } else if((last - first) == 1) {
          skip = -1;
        }
        first = last;
      }
    } while(first < (SA + n));
    if(skip != 0) { B[first + skip] = skip; }
    if(unsorted == 0) { break; }
  }
}

}} //ns
///\endcond

