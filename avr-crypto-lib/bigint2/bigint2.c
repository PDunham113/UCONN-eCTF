/*
 * bigint2.c
 *
 *  Created on: 09.04.2014
 *      Author: bg
 */

#include <bigint2.h>
#include <bigint2_io.h>
#include <stdio.h>
#include <string.h>

#define CHECKS 1

#define E_PARAM -1;
#define E_OOM   -2;
#define E_VALUE -3;


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

void *(*int_realloc)(void *ptr, size_t size);
void (*int_free)(void *ptr);

/**
 * \brief used to check if bigint is large enough
 * Checks if the memory of a is large enough to hold min_size
 * words, and if this is not the case tries to increase the allocated memory
 * amount (conserving the memory content).
 * If the allocated memory insufficient and the buffer could not be enlarged
 * E_OOM is returned, otherwise 0 is returned.
 */
static
int check_size(bigint_t *a, bigint_length_t min_size) {
    bigint_word_t *tmp;
    if (a->allocated_W >= min_size) {
        return 0;
    }
    tmp = int_realloc(a->wordv, min_size * sizeof(bigint_word_t));
    if (!tmp) {
        return E_OOM;
    }
    a->wordv = tmp;
    a->allocated_W = min_size;
    return 0;
}

int bigint_copy(bigint_t *dest, const bigint_t *a) {
    int r;
    if (dest == a) {
        return 0;
    }
    r = check_size(dest, a->length_W);
    if (r) {
        return r;
    }
    dest->info = a->info;
    dest->length_W = a->length_W;
    memcpy(dest->wordv, a->wordv, a->length_W * sizeof(bigint_word_t));

    return 0;
}

int bigint_free(bigint_t *a) {
    if(a->allocated_W) {
        int_free(a->wordv);
    }
    memset(a, 0, sizeof(bigint_t));
    return 0;
}

/**
 * \brief dest = |a| + |b|
 * Adds the bigints a and b and stores the result in dest.
 * Signs are ignored.
 */
int bigint_add_u(bigint_t *dest, const bigint_t *a, const bigint_t *b) {
    bigint_word_t t, c1, c2, c = 0;
    bigint_length_t i, j;
    int r;
#if CHECKS
    if (!dest || !a || !b) {
        return E_PARAM;
    }
#endif
#if CHECKS
    if ((r = check_size(dest, a->length_W))) {
        return r;
    }
#endif
    j = MIN(a->length_W, b->length_W);
    for (i = 0; i < j; ++i) {
        t = a->wordv[i] + b->wordv[i];
        c1 = t < a->wordv[i];
        dest->wordv[i] = t + c;
        c2 = dest->wordv[i] < t;
        c = c1 | c2;
    }
    for (; i < a->length_W; ++i) {
        t = a->wordv[i];
        dest->wordv[i] = t + c;
        c = dest->wordv[i] < t;
    }
    dest->length_W = i;
    return 0;
}

/** UNSAFE!!!
 * \brief dest = |a| + |b|
 * Adds the bigints a and b and stores the result in dest.
 * Signs are ignored.
 */
int bigint_add_auto_u(bigint_t *dest, const bigint_t *a, const bigint_t *b) {
    bigint_word_t t, c1, c2, c = 0;
    bigint_length_t i, j;
    int r;
#if CHECKS
    if (!dest || !a || !b) {
        return E_PARAM;
    }
#endif
    if (a->length_W < b->length_W) {
        const bigint_t *t;
        t = a;
        a = b;
        b = t;
    }
#if CHECKS
    if ((r = check_size(dest, a->length_W + 1))) {
        return r;
    }
#endif
    j = MIN(a->length_W, b->length_W);
    for (i = 0; i < j; ++i) {
        t = a->wordv[i] + b->wordv[i];
        c1 = t < a->wordv[i];
        dest->wordv[i] = t + c;
        c2 = dest->wordv[i] < t;
        c = c1 | c2;
    }
    for (; i < a->length_W; ++i) {
        t = a->wordv[i];
        dest->wordv[i] = t + c;
        c = dest->wordv[i] < t;
    }
    if (c) {
        dest->wordv[i++] = c;
    }
    dest->length_W = i;
    return 0;
}


/**
 * \brief dest = ~a
 */
int bigint_invert(bigint_t *dest, const bigint_t *a) {
    bigint_length_t i;
    int r;
#if CHECKS
    if ((r = check_size(dest, a->length_W))) {
        return r;
    }
#endif
    i = a->length_W;
    while(i--) {
        dest->wordv[i] = ~a->wordv[i];
    }
    dest->length_W = a->length_W;
    return 0;
}

/**
 * \brief dest = a + 1
 */
int bigint_add_one(bigint_t *dest, const bigint_t *a) {
    bigint_t one = {
        .wordv = (bigint_word_t*)"\x01",
        .length_W = 1,
        .allocated_W = 1,
        .info = 0
    };
    return bigint_add_u(dest, a, &one);
}

/**
 * \brief dest = -a
 */
int bigint_negate(bigint_t *dest, const bigint_t *a) {
    int r = 0;
#if CHECKS
    if ((r = check_size(dest, a->length_W))) {
        return r;
    }
#endif
    r |= bigint_invert(dest, a);
    r |= bigint_add_one(dest, dest);
    return r;
}

/**
 * \brief dest = |a| - |b|
 * Subtracts b from a and stores the result in dest.
 * Signs are ignored
 */
int bigint_sub_u(bigint_t *dest, const bigint_t *a, const bigint_t *b) {
    bigint_length_t i, j;
    bigint_word_t t, c1, c2, c = 0;
    int r;
#if CHECKS
    if (!dest || !a || !b) {
        return E_PARAM;
    }
#endif
#if CHECKS
    if ((r = check_size(dest, MAX(a->length_W, b->length_W)))) {
        return r;
    }

#endif
    j = MIN(a->length_W, b->length_W);
    for (i = 0; i < j; ++i) {
        t = a->wordv[i] - b->wordv[i];
        c1 = t > a->wordv[i];
        dest->wordv[i] = t - c;
        c2 = dest->wordv[i] > t;
        c = c1 | c2;
    }
    for (; i < a->length_W; ++i) {
        t = a->wordv[i];
        dest->wordv[i] = t - c;
        c = dest->wordv[i] > t;
    }
    dest->length_W = a->length_W;
    return 0;
}

/**
 * \brief a <<= 1
 */
int bigint_shiftleft_words(bigint_t *dest, const bigint_t *a, bigint_length_t s) {
#if CHECKS
    if (!a) {
        return E_PARAM;
    }
#endif
    if (s == 0) {
        bigint_copy(dest, a);
        return 0;
    }
    int r;
    if ((r = check_size(dest, a->length_W + s))) {
        return r;
    }
    memmove(&dest->wordv[s], &a->wordv[0], a->length_W * sizeof(bigint_word_t));
    memset(&dest->wordv[0], 0, s * sizeof(bigint_word_t));
    dest->length_W = a->length_W + s;
    return 0;
}


/**
 * \brief a <<= 1
 */
int bigint_shiftleft_1bit(bigint_t *a) {
    uint8_t c1 = 0, c2;
    bigint_word_t t;
    bigint_length_t i;
#if CHECKS
    if (!a) {
        return E_PARAM;
    }
#endif

    for(i = 0; i < a->length_W; ++i) {
        t = a->wordv[i];
        c2 = t >> (BIGINT_WORD_SIZE - 1);
        t <<= 1;
        t |= c1;
        a->wordv[i] = t;
        c1 = c2;
    }

    return 0;
}

/**
 * \brief a <<= s
 */
static
int bigint_shiftleft_small(bigint_t *dest, const bigint_t *a, uint_fast8_t s) {
    bigint_wordplus_t t = 0;
    bigint_length_t i, l;
#if CHECKS
    if (!a) {
        return E_PARAM;
    }
#endif
    l = bigint_get_first_set_bit(a) + BIGINT_WORD_SIZE;
#if CHECKS
    int r;
    if ((r = check_size(dest, (l + s ) / BIGINT_WORD_SIZE))) {
        return r;
    }

#endif
    dest->length_W = (l + s ) / BIGINT_WORD_SIZE;
    l /= BIGINT_WORD_SIZE;
    for(i = 0; i < l; ++i) {
        t |= (bigint_wordplus_t)a->wordv[i] << s;
        dest->wordv[i] = t;
        t >>= BIGINT_WORD_SIZE;
    }
    if (t) {
        dest->wordv[i] = t;
    }
    return 0;
}

/**
 * \brief dest = a << s
 */
int bigint_shiftleft(bigint_t *dest, const bigint_t *a, bigint_length_t s) {
    int r;
    if((r = bigint_shiftleft_small(dest, a, s % BIGINT_WORD_SIZE))) {
        return r;
    }
    if ((r = bigint_shiftleft_words(dest, dest, s / BIGINT_WORD_SIZE))) {
        return r;
    }
    return 0;
}

/**
 * \brief a >>= 1
 */
int bigint_shiftright_1bit(bigint_t *a) {
    uint8_t c1 = 0, c2;
    bigint_word_t t;
    bigint_length_t i;
#if CHECKS
    if (!a) {
        return E_PARAM;
    }
#endif

    for(i = a->length_W; i != 0; --i) {
        t = a->wordv[i - 1];
        c2 = t & 1;
        t >>= 1;
        t |= c1 << (BIGINT_WORD_SIZE - 1);
        a->wordv[i - 1] = t;
        c1 = c2;
    }

    return 0;
}

/**
 * \brief dest = a ** 2
 */
int bigint_square(bigint_t *dest, const bigint_t *a) {
    bigint_word_t c1, c2;
    bigint_wordplus_t uv;
    bigint_word_t q;
    bigint_length_t i, j;
#if CHECKS
    int r;
    if (!dest || !a) {
        return E_PARAM;
    }
    if ((r = check_size(dest, a->length_W * 2))) {
        return r;
    }
#endif
    if (dest == a) {
        bigint_t t = {NULL, 0, 0, 0};
        bigint_copy(&t, a);
        r = bigint_square(dest, &t);
        bigint_free(&t);
        return r;
    }
    memset(dest->wordv, 0, 2 * a->length_W * sizeof(bigint_word_t));
    for(i = 0; i < a->length_W; ++i) {
        uv =   (bigint_wordplus_t)dest->wordv[2 * i]
             + (bigint_wordplus_t)a->wordv[i]
             * (bigint_wordplus_t)a->wordv[i];
        dest->wordv[2 * i] = uv;
        c1 = uv >> BIGINT_WORD_SIZE;
        c2 = 0;
        for (j = i + 1; j < a->length_W; ++j) {
            uv =   (bigint_wordplus_t)a->wordv[i]
                 * (bigint_wordplus_t)a->wordv[j];
            q = uv >> (2 * BIGINT_WORD_SIZE - 1);
            uv <<= 1;
            uv += dest->wordv[i + j];
            q += (uv < dest->wordv[i + j]); /* this might be dangerous! XXX */
            uv += c1;
            q += (uv < c1); /* this might be dangerous! XXX */
            dest->wordv[i + j] = uv;
            c1 = c2 + (uv >> BIGINT_WORD_SIZE);
            c2 = q + (c1 < c2); /* this might be dangerous! XXX */
        }
        dest->wordv[i + a->length_W] += c1;
        dest->wordv[i + a->length_W + 1] = c2 + (dest->wordv[i + a->length_W] < c1);  /* this might be dangerous! XXX */
    }
    dest->length_W = a->length_W * 2;
    return 0;
}


/**
 * \brief dest = |a * b|
 * unsigned multiply a bigint (a) by a word (b)
 */
int bigint_mul_word(bigint_t *dest, const bigint_t *a, const bigint_word_t b) {
    bigint_wordplus_t c1 = 0;
    bigint_length_t i;
#if CHECKS
    int r;
    if (!dest || !a || !b) {
        return E_PARAM;
    }
    if ((r = check_size(dest, a->length_W + 1))) {
        return r;
    }
#endif
    for(i = 0; i < a->length_W; ++i) {
        c1 += (bigint_wordplus_t)b * (bigint_wordplus_t)a->wordv[i];
        dest->wordv[i] = c1;
        c1 >>= BIGINT_WORD_SIZE;
    }
    dest->wordv[i++] = c1;
    dest->length_W = i;
    BIGINT_SET_POS(dest);
    return 0;
}

/**
 * \brief dest = a * b
 */
int bigint_mul_schoolbook(bigint_t *dest, const bigint_t *a, const bigint_t *b) {
    bigint_wordplus_t v;
    bigint_word_t c;
    bigint_length_t i, j;
    int r;
#if CHECKS
    if (!dest || !a || !b) {
        return E_PARAM;
    }
    if ((r = check_size(dest, a->length_W + b->length_W))) {
        return r;
    }
#endif
    if (dest == a) {
        bigint_t t = {NULL, 0, 0, 0};
        bigint_copy(&t, a);
        r = bigint_mul_schoolbook(dest, &t, b);
        bigint_free(&t);
        return r;
    }
    if (dest == b) {
        bigint_t t = {NULL, 0, 0, 0};
        bigint_copy(&t, b);
        r = bigint_mul_schoolbook(dest, a, &t);
        bigint_free(&t);
        return r;
    }
    memset(dest->wordv, 0, (a->length_W + b->length_W) * sizeof(bigint_word_t));
    for(i = 0; i < b->length_W; ++i) {
        c = 0;
        for(j = 0; j < a->length_W; ++j) {
            v =   (bigint_wordplus_t)a->wordv[j]
                * (bigint_wordplus_t)b->wordv[i];
            v += (bigint_wordplus_t)dest->wordv[i + j];
            v += (bigint_wordplus_t)c;
            dest->wordv[i + j] = v;
            c = v >> BIGINT_WORD_SIZE;
        }
        dest->wordv[i + j] = c;
    }

    dest->length_W = a->length_W + b->length_W;
    dest->info &= ~BIGINT_SIGN_MASK;
    dest->info |= (a->info ^ b->info) & BIGINT_SIGN_MASK;
    return 0;
}

/*
 * UNSAFE!!!
 */
int8_t bigint_cmp_u(const bigint_t *a, const bigint_t *b) {
    int8_t r = 1;
    bigint_length_t i;
    if (a->length_W == 0 && b->length_W == 0) {
        return 0;
    }
    if (b->length_W == 0) {
        return 1;
    }
    if (a->length_W == 0) {
        return -1;
    }
    if (a->length_W < b->length_W) {
        const bigint_t *t;
        t = a;
        a = b;
        b = t;
        r = -1;
    }
    for (i = a->length_W - 1; i > b->length_W - 1; --i) {
        if (a->wordv[i]) {
            return r;
        }
    }
    ++i;
    do {
        --i;
        if (a->wordv[i] != b->wordv[i]) {
            return a->wordv[i] > b->wordv[i] ? r : -r;
        }
    } while (i);
    return 0;
}

int8_t bigint_cmp_s(const bigint_t *a, const bigint_t *b) {
    if ( bigint_get_first_set_bit(a) == (bigint_length_t)-1 &&
         bigint_get_first_set_bit(b) == (bigint_length_t)-1 ) {
        return 0;
    }
    if ((a->info & BIGINT_SIGN_MASK) ^ (b->info & BIGINT_SIGN_MASK)) {
        return (a->info & BIGINT_SIGN_MASK) ? -1 : 1;
    } else {
        int r;
        r = bigint_cmp_u(a, b);
        return (a->info & BIGINT_SIGN_MASK) ? -r : r;
    }
    return 0;
}

/*
 * UNSAFE!!!
 * a <=> b * (B ** s)
 */
int8_t bigint_cmp_scale(const bigint_t *a, const bigint_t *b, bigint_length_t s) {
    bigint_length_t i;
    if (s == 0) {
        return bigint_cmp_u(a, b);
    }
    if (a->length_W == 0 && b->length_W == 0) {
        return 0;
    }
    if (b->length_W == 0) {
        return 1;
    }
    if (a->length_W == 0) {
        return -1;
    }
    if (s > a->length_W) {
        return -1;
    }
    for (i = a->length_W - 1; i > b->length_W + s - 1; --i) {
        if (a->wordv[i]) {
            return 1;
        }
    }
    for (; i > s - 1; --i) {
        if (a->wordv[i] != b->wordv[i + s]){
            return a->wordv[i] > b->wordv[i + s] ? 1 : -1;
        }
    }
        ++i;
    do {
        --i;
        if (a->wordv[i]) {
            return 1;
        }
    } while (i);
    return 0;
}

/**
 * XXXXXXXXXXXX
 * \brief dest = |a| - |b| * B ** s
 * Subtracts b from a and stores the result in dest.
 * Signs are ignored
 */
int bigint_sub_scale(bigint_t *dest, const bigint_t *a, const bigint_t *b, bigint_length_t s) {
    bigint_length_t i, j;
    bigint_word_t t, c1, c2, c = 0;
    int r;
#if CHECKS
    if (!dest || !a || !b) {
        return E_PARAM;
    }
#endif
#if CHECKS
    if ((r = check_size(dest, MAX(a->length_W, b->length_W)))) {
        return r;
    }

#endif
    j = MIN(a->length_W, b->length_W);
    for (i = 0; i < j; ++i) {
        t = a->wordv[i + s] - b->wordv[i];
        c1 = t > a->wordv[i + s];
        dest->wordv[i + s] = t - c;
        c2 = dest->wordv[i + s] > t;
        c = c1 | c2;
    }
    for (; i < a->length_W; ++i) {
        t = a->wordv[i + s];
        dest->wordv[i + s] = t - c;
        c = dest->wordv[i + s] > t;
    }
    dest->length_W = a->length_W;
    return 0;
}

/*
 * UNSAFE!!!
 */
int bigint_adjust_length(bigint_t *a) {
#if CHECKS
    if (!a) {
        return E_PARAM;
    }
#endif
    while (a->length_W) {
        if (a->wordv[a->length_W - 1]) {
            return 0;
        }
        a->length_W--;
    }
    return 0;
}

bigint_length_t bigint_get_first_set_bit(const bigint_t *a) {
    bigint_length_t r;
    bigint_word_t t;
    if (!a) {
        return (bigint_length_t)-1;
    }
    if (a->length_W == 0) {
        return (bigint_length_t)-1;
    }
    r = a->length_W - 1;
    while (r && a->wordv[r] == 0) {
        --r;
    }
    t = a->wordv[r];
    if (!t) {
        return (bigint_length_t)-1;
    }
    r *= BIGINT_WORD_SIZE;
    t >>= 1;
    while (t) {
        t >>= 1;
        ++r;
    }
    return r;
}

/*
 * UNSAFE!!!
 * a = q * b + r
 */
int bigint_divide(bigint_t *q, bigint_t *r, const bigint_t *a, const bigint_t *b) {
    bigint_t a_ = {NULL, 0, 0, 0}, x_ = {NULL, 0, 0, 0};
    bigint_length_t i, la, lb;
    int ret;
#if CHECKS
    if (!a || !b || (!q && !r)) {
        return E_PARAM;
    }
#endif
    la = bigint_get_first_set_bit(a);
    lb = bigint_get_first_set_bit(b);
    if (lb == (bigint_length_t)-1) {
        return E_VALUE;
    }
    if (la == (bigint_length_t)-1) {
        if (q) {
            q->length_W = 0;
        }
        if (r) {
            r->length_W = 0;
        }
        return 0;
    }
    if (la < lb) {
        if (q) {
            q->length_W = 0;
        }
        if (r) {
            bigint_copy(r, a);
        }
        return 0;
    }
    i = la - lb;
    if (q) {
        if ((ret = check_size(q, (i + BIGINT_WORD_SIZE) / BIGINT_WORD_SIZE))) {
            return ret;
        }
        q->length_W = (i + BIGINT_WORD_SIZE) / BIGINT_WORD_SIZE;
        memset(q->wordv, 0, q->allocated_W * sizeof(bigint_word_t));
    }
    if (r) {
        if ((ret = check_size(q, (lb + BIGINT_WORD_SIZE - 1) / BIGINT_WORD_SIZE))) {
            return ret;
        }
    }
    if ((ret = bigint_copy(&a_, a))) {
        return ret;
    }
    if ((ret = bigint_shiftleft(&x_, b, i))) {
        bigint_free(&a_);
        return ret;
    }
#if 0
    printf("DBG: x' = ");
    bigint_print_hex(&x_);
    printf("; b = ");
    bigint_print_hex(b);
    printf("; la = %d; lb = %d; i = %d\n", la, lb, i);
#endif
    do {
        if (bigint_cmp_u(&a_, &x_) >= 0) {
            bigint_sub_u(&a_, &a_, &x_);
            if (q) {
                q->wordv[i / BIGINT_WORD_SIZE] |= 1 << (i % BIGINT_WORD_SIZE);
            }
        }
        bigint_shiftright_1bit(&x_);
    } while (i--);
    if (r) {
        bigint_copy(r, &a_);
    }
    bigint_free(&x_);
    bigint_free(&a_);
    return 0;
}

/**
 * UNSAFE!!!
 */
int bigint_sub_s(bigint_t *dest, const bigint_t *a, const bigint_t *b) {
    int8_t s;
    int r;
    if (a->length_W == 0) {
        bigint_copy(dest, b);
        dest->info ^= BIGINT_SIGN_MASK;
        return 0;
    }
    if ((a->info ^ b->info) & BIGINT_SIGN_MASK) {
        /* different signs */
        if ((r = bigint_add_auto_u(dest, a, b))) {
            return r;
        }
        dest->info = a->info;
    } else {
        s = bigint_cmp_u(a, b);
        if (s >= 0) {
            if (( r = bigint_sub_u(dest, a, b))) {
                return r;
            }
            dest->info = a->info;
        } else {
            if (( r = bigint_sub_u(dest, b, a))) {
                return r;
            }
            dest->info = (~a->info) & BIGINT_SIGN_MASK;
        }
    }
    return 0;
}

/**
 * UNSAFE!!!
 */
int bigint_add_s(bigint_t *dest, const bigint_t *a, const bigint_t *b) {
    int8_t s;
    int r;
    if ((a->info ^ b->info) & BIGINT_SIGN_MASK) {
        /* different signs */
        s = bigint_cmp_u(a, b);
        if (s >= 0) {
            if (( r = bigint_sub_u(dest, a, b))) {
                return r;
            }
            dest->info = a->info;
        } else {
            if (( r = bigint_sub_u(dest, b, a))) {
                return r;
            }
            dest->info = (~a->info) & BIGINT_SIGN_MASK;
        }
    } else {
        if ((r = bigint_add_auto_u(dest, a, b))) {
            return r;
        }
        dest->info = a->info;
    }
    return 0;
}

int8_t bigint_is_even(const bigint_t *a) {
    if (!a) {
        return E_VALUE;
    }
    if (a->length_W == 0 || (a->wordv[0] & 1) == 0) {
        return 1;
    }
    return 0;
}

int8_t bigint_is_odd(const bigint_t *a) {
    if (!a) {
        return E_VALUE;
    }
    if (a->length_W > 0 && (a->wordv[0] & 1) == 1) {
        return 1;
    }
    return 0;
}

/**
 * UNSAFE!!!
 * (a, b) -> (x, y, v)
 * v = gcd(a,b)
 * x * a + y * b = v
 */
int bigint_gcdext(bigint_t *gcd, bigint_t *x, bigint_t *y, const bigint_t *a, const bigint_t *b) {
    bigint_length_t g = 0;
    bigint_t u = {NULL, 0, 0, 0}, v = {NULL, 0, 0, 0};
    bigint_t x_ = {NULL, 0, 0, 0}, y_ = {NULL, 0, 0, 0};
    bigint_t A = {NULL, 0, 0, 0}, B = {NULL, 0, 0, 0};
    bigint_t C = {NULL, 0, 0, 0}, D = {NULL, 0, 0, 0};
    int r;
    if ((r = bigint_copy(&x_, a))) {
        return r;
    }
    if ((r = bigint_copy(&y_, b))) {
        bigint_free(&x_);
        return r;
    }
    bigint_adjust_length(&x_);
    bigint_adjust_length(&y_);
    if (x_.length_W == 0 || y_.length_W == 0) {
        bigint_free(&y_);
        bigint_free(&x_);
        return E_VALUE;
    }
    while (((x_.wordv[0] | y_.wordv[0]) & 1) == 0) {
        ++g;
        bigint_shiftright_1bit(&x_);
        bigint_shiftright_1bit(&y_);
    }
    if ((r = check_size(&A, 1 + (bigint_get_first_set_bit(&y_) + BIGINT_WORD_SIZE) / BIGINT_WORD_SIZE))) {
        bigint_free(&y_);
        bigint_free(&x_);
        return r;
    }
    if ((r = check_size(&C, 1 + (bigint_get_first_set_bit(&y_) + BIGINT_WORD_SIZE) / BIGINT_WORD_SIZE))) {
        bigint_free(&A);
        bigint_free(&y_);
        bigint_free(&x_);
        return r;
    }
    if ((r = check_size(&B, 1 + (bigint_get_first_set_bit(&x_) + BIGINT_WORD_SIZE) / BIGINT_WORD_SIZE))) {
        bigint_free(&C);
        bigint_free(&A);
        bigint_free(&y_);
        bigint_free(&x_);
        return r;
    }
    if ((r = check_size(&D, 1 + (bigint_get_first_set_bit(&x_) + BIGINT_WORD_SIZE) / BIGINT_WORD_SIZE))) {
        bigint_free(&B);
        bigint_free(&C);
        bigint_free(&A);
        bigint_free(&y_);
        bigint_free(&x_);
        return r;
    }
    if ((r = bigint_copy(&u, &x_))) {
        bigint_free(&D);
        bigint_free(&B);
        bigint_free(&C);
        bigint_free(&A);
        bigint_free(&y_);
        bigint_free(&x_);
        return r;
    }
    if ((r = bigint_copy(&v, &y_))) {
        bigint_free(&u);
        bigint_free(&D);
        bigint_free(&B);
        bigint_free(&C);
        bigint_free(&A);
        bigint_free(&y_);
        bigint_free(&x_);
        return r;
    }
    A.wordv[0] = D.wordv[0] = 1;
    A.length_W = D.length_W = 1;
    C.length_W = B.length_W = 0;

    do {
        while ((u.wordv[0] & 1) == 0) {
            bigint_shiftright_1bit(&u);
            if (bigint_is_odd(&A) || bigint_is_odd(&B)) {
                bigint_add_s(&A, &A, &y_);
                bigint_sub_s(&B, &B, &x_);
            }
            bigint_shiftright_1bit(&A);
            bigint_shiftright_1bit(&B);
        }
        while ((v.wordv[0] & 1) == 0) {
            bigint_shiftright_1bit(&v);
            if (bigint_is_odd(&C) || bigint_is_odd(&D)) {
                bigint_add_s(&C, &C, &y_);
                bigint_sub_s(&D, &D, &x_);
            }
            bigint_shiftright_1bit(&C);
            bigint_shiftright_1bit(&D);
        }
        if (bigint_cmp_s(&u, &v) >= 0) {
            bigint_sub_s(&u, &u, &v);
            bigint_sub_s(&A, &A, &C);
            bigint_sub_s(&B, &B, &D);
        } else {
            bigint_sub_s(&v, &v, &u);
            bigint_sub_s(&C, &C, &A);
            bigint_sub_s(&D, &D, &B);
        }
        bigint_adjust_length(&u);
    } while (u.length_W != 0);
    if (gcd) {
        bigint_shiftleft(gcd, &v, g);
    }
    if (x) {
        bigint_copy(x, &C);
    }
    if (y) {
        bigint_copy(y, &D);
    }
    bigint_free(&v);
    bigint_free(&u);
    bigint_free(&D);
    bigint_free(&B);
    bigint_free(&C);
    bigint_free(&A);
    bigint_free(&y_);
    bigint_free(&x_);
    return 0;
}
