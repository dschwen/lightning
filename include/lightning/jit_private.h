/*
 * Copyright (C) 2012  Free Software Foundation, Inc.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *	Paulo Cesar Pereira de Andrade
 */

#ifndef _jit_private_h
#define _jit_private_h

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <gmp.h>

#if defined(__GNUC__)
#  define maybe_unused		__attribute__ ((unused))
#  define unlikely(exprn)	__builtin_expect(!!(exprn), 0)
#  define likely(exprn)		__builtin_expect(!!(exprn), 1)
#  if (__GNUC__ >= 4)
#    define PUBLIC		__attribute__ ((visibility("default")))
#    define HIDDEN		__attribute__ ((visibility("hidden")))
#  else
#    define PUBLIC		/**/
#    define HIDDEN		/**/
#  endif
#else
#  define maybe_unused		/**/
#  define unlikely(exprn)	exprn
#  define likely(exprn)		exprn
#  define PUBLIC		/**/
#  define HIDDEN		/**/
#endif

#define jit_size(vector)	(sizeof(vector) / sizeof((vector)[0]))

/*
 * Private jit_class bitmasks
 */
#define jit_class_nospill	0x00800000	/* hint to fail if need spill */
#define jit_class_sft		0x01000000	/* not a hardware register */
#define jit_class_rg8		0x04000000	/* x86 8 bits */
#define jit_class_xpr		0x80000000	/* float / vector */
#define jit_regno_patch		0x00008000	/* this is a register
						 * returned by a "user" call
						 * to jit_get_reg() */

#define jit_kind_register	1
#define jit_kind_code		2
#define jit_kind_word		3
#define jit_kind_float32	4
#define jit_kind_float64	5

#define jit_cc_a0_reg		0x00000001	/* arg0 is a register */
#define jit_cc_a0_chg		0x00000002	/* arg0 is modified */
#define jit_cc_a0_jmp		0x00000004	/* arg0 is a jump target */
#define jit_cc_a0_int		0x00000010	/* arg0 is immediate word */
#define jit_cc_a0_flt		0x00000020	/* arg0 is immediate float */
#define jit_cc_a0_dbl		0x00000040	/* arg0 is immediate double */
#define jit_cc_a1_reg		0x00000100	/* arg1 is a register */
#define jit_cc_a1_chg		0x00000200	/* arg1 is modified */
#define jit_cc_a1_int		0x00001000	/* arg1 is immediate word */
#define jit_cc_a1_flt		0x00002000	/* arg1 is immediate float */
#define jit_cc_a1_dbl		0x00004000	/* arg1 is immediate double */
#define jit_cc_a2_reg		0x00010000	/* arg2 is a register */
#define jit_cc_a2_chg		0x00020000	/* arg2 is modified */
#define jit_cc_a2_int		0x00100000	/* arg2 is immediate word */
#define jit_cc_a2_flt		0x00200000	/* arg2 is immediate float */
#define jit_cc_a2_dbl		0x00400000	/* arg2 is immediate double */

#define jit_regset_com(u, v)	((u) = ~(v))
#define jit_regset_and(u, v, w)	((u) = (v) & (w))
#define jit_regset_ior(u, v, w)	((u) = (v) | (w))
#define jit_regset_xor(u, v, w)	((u) = (v) ^ (w))
#define jit_regset_set(u, v)	((u) = (v))
#define jit_regset_cmp_ui(u, v)	((u) != (v))
#define jit_regset_set_ui(u, v)	((u) = (v))
#define jit_regset_set_p(set)	(set)
#if DEBUG
#  define jit_regset_clrbit(set, bit)					\
	(assert(bit >= 0 && bit < (sizeof(jit_regset_t) << 3)),		\
	 (set) &= ~(1LL << (bit)))
#  define jit_regset_setbit(set, bit)					\
	(assert(bit >= 0 && bit < (sizeof(jit_regset_t) << 3)),		\
	 (set) |= 1LL << (bit))
#  define jit_regset_tstbit(set, bit)					\
	(assert(bit >= 0 && bit < (sizeof(jit_regset_t) << 3)),		\
	 (set) & (1LL << (bit)))
#else
#  define jit_regset_clrbit(set, bit)	((set) &= ~(1LL << (bit)))
#  define jit_regset_setbit(set, bit)	((set) |= 1LL << (bit))
#  define jit_regset_tstbit(set, bit)	((set) & (1LL << (bit)))
#endif
#define jit_regset_new(set)	((set) = 0)
#define jit_regset_del(set)	((set) = 0)
extern unsigned long
jit_regset_scan1(jit_regset_t, jit_int32_t);

#define jit_reglive_setup()						\
    do {								\
	jit_regset_set_ui(_jit->reglive, 0);				\
	jit_regset_set_ui(_jit->regmask, 0);				\
    } while (0)

/*
 * Types
 */
typedef union jit_data		jit_data_t;
typedef struct jit_block	jit_block_t;
typedef struct jit_value	jit_value_t;
typedef struct jit_function	jit_function_t;
typedef struct jit_register	jit_register_t;
#if __arm__
#  if DISASSEMBLER
typedef struct jit_data_info	jit_data_info_t;
#  endif
#endif

union jit_data {
    struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	jit_int32_t	l;
	jit_int32_t	h;
#else
	jit_int32_t	h;
	jit_int32_t	l;
#endif
    } pair;
    jit_word_t		 w;
    jit_float32_t	 f;
    jit_float64_t	 d;
    jit_pointer_t	 p;
    jit_node_t		*n;
};

struct jit_node {
    jit_node_t		*next;
    jit_code_t		 code;
    jit_int32_t		 flag;
    jit_data_t		 u;
    jit_data_t		 v;
    jit_data_t		 w;
    jit_node_t		*link;
};

struct jit_block {
    jit_node_t		*label;
    jit_regset_t	 reglive;
    jit_regset_t	 regmask;
};

struct jit_value {
    jit_int32_t		kind;
    jit_code_t		code;
    jit_data_t		base;
    jit_data_t		disp;
};

typedef struct {
#if __arm__
    jit_word_t		 kind;
#endif
    jit_word_t		 inst;
    jit_node_t		*node;
} jit_patch_t;

#if __arm__ && DISASSEMBLER
struct jit_data_info {
    jit_uword_t		  code;		/* pointer in code buffer */
    jit_word_t		  length;	/* length of constant vector */
};
#endif

struct jit_function {
    struct {
	jit_int32_t	 argi;
	jit_int32_t	 argf;
	jit_int32_t	 size;
	jit_int32_t	 aoff;
	jit_int32_t	 alen;
    } self;
    struct {
	jit_int32_t	 argi;
	jit_int32_t	 argf;
	jit_int32_t	 size;
	jit_int32_t	 kind;
    } call;
    jit_node_t		*prolog;
    jit_node_t		*epilog;
    jit_int32_t		*regoff;
    jit_regset_t	 regset;
    jit_int32_t		 stack;
};

struct jit_state {
    union {
	jit_uint8_t	 *uc;
	jit_uint16_t	 *us;
	jit_uint32_t	 *ui;
	jit_uint64_t	 *ul;
	jit_word_t	  w;
    } pc;
    jit_node_t		 *head;
    jit_node_t		 *tail;
    jit_uint32_t	  emit	: 1;	/* emit state entered */
    jit_uint32_t	  again	: 1;	/* start over emiting function */
    jit_int32_t		  reglen;	/* number of registers */
    jit_regset_t	  regarg;	/* cannot allocate */
    jit_regset_t	  regsav;	/* automatic spill only once */
    jit_regset_t	  reglive;	/* known live registers at some point */
    jit_regset_t	  regmask;	/* register mask to update reglive */
    mpz_t		  blockmask;	/* mask of visited basic blocks */
    struct {
	jit_uint8_t	 *ptr;
	jit_word_t	  length;
    } code;
    struct {
	jit_uint8_t	 *ptr;		/* constant pool */
	jit_node_t	**table;	/* very simple hash table */
	jit_word_t	  size;		/* number of vectors in table */
	jit_word_t	  count;	/* number of hash table entries */
	jit_word_t	  offset;	/* offset in bytes in ptr */
	jit_word_t	  length;	/* length in bytes of ptr */
    } data;
    jit_node_t		**spill;
    jit_int32_t		 *gen;		/* ssa like "register version" */
    jit_value_t		 *values;	/* temporary jit_value_t vector */
    struct {
	jit_block_t	 *ptr;
	jit_word_t	  offset;
	jit_word_t	  length;
    } blocks;				/* basic blocks */
    struct {
	jit_patch_t	 *ptr;
	jit_word_t	  offset;
	jit_word_t	  length;
    } patches;				/* forward patch information */
    jit_function_t	 *function;	/* current function */
    struct {
	jit_function_t	 *ptr;
	jit_word_t	  offset;
	jit_word_t	  length;
    } functions;			/* prolog/epilogue offsets in code */
    struct {
	jit_node_t	**ptr;
	jit_word_t	  offset;
	jit_word_t	  length;
    } pool;
    jit_node_t		 *list;
#if __arm__
#  if DISASSEMBLER
    struct {
	jit_data_info_t	 *ptr;
	it_word_t	  offset;
	jit_word_t	  length;
    } data_info;			/* constant pools information */
#  endif
    struct {
	jit_uint8_t	 *data;		/* pointer to code */
	jit_word_t	  size;		/* size data */
	jit_word_t	  offset;	/* pending patches */
	jit_word_t	  length;	/* number of pending constants */
	jit_int32_t	  values[1024];	/* pending constants */
	jit_word_t	  patches[2048];
    } consts;
#endif
};

struct jit_register {
    jit_reg_t		 spec;
    char		*name;
};

/*
 * Prototypes
 */
extern void jit_get_cpu(void);

#define jit_init()			_jit_init(_jit)
extern void _jit_init(jit_state_t*);

#define jit_new_node_no_link(u)		_jit_new_node_no_link(_jit, u)
extern jit_node_t *_jit_new_node_no_link(jit_state_t*, jit_code_t);

#define jit_link_node(u)		_jit_link_node(_jit, u)
extern void _jit_link_node(jit_state_t*, jit_node_t*);

#define jit_link_label(l)	_jit_link_label(_jit,l)
extern void
_jit_link_label(jit_state_t*,jit_node_t*);

#define jit_reglive(node)	_jit_reglive(_jit, node)
extern void
_jit_reglive(jit_state_t*, jit_node_t*);

#define jit_regarg_set(n,v)	_jit_regarg_set(_jit,n,v)
extern void
_jit_regarg_set(jit_state_t*, jit_node_t*, jit_int32_t);

#define jit_regarg_clr(n,v)	_jit_regarg_clr(_jit,n,v)
extern void
_jit_regarg_clr(jit_state_t*, jit_node_t*, jit_int32_t);

#define jit_get_reg(s)		_jit_get_reg(_jit,s)
extern jit_int32_t
_jit_get_reg(jit_state_t*, jit_int32_t);

#define jit_unget_reg(r)	_jit_unget_reg(_jit,r)
extern void
_jit_unget_reg(jit_state_t*, jit_int32_t);

#define jit_save(reg)		_jit_save(_jit, reg)
extern void
_jit_save(jit_state_t*, jit_int32_t);

#define jit_load(reg)		_jit_load(_jit, reg)
extern void
_jit_load(jit_state_t*, jit_int32_t);

#define jit_optimize()		_jit_optimize(_jit)
extern void
_jit_optimize(jit_state_t*);

#define jit_classify(code)	_jit_classify(_jit, code)
extern jit_int32_t
_jit_classify(jit_state_t*, jit_code_t);

#define jit_regarg_p(n, r)	_jit_regarg_p(_jit, n, r)
extern jit_bool_t
_jit_regarg_p(jit_state_t*, jit_node_t*, jit_int32_t);

#define emit_ldxi(r0, r1, i0)	_emit_ldxi(_jit, r0, r1, i0)
extern void
_emit_ldxi(jit_state_t*, jit_int32_t, jit_int32_t, jit_word_t);

#define emit_stxi(i0, r0, r1)	_emit_stxi(_jit, i0, r0, r1)
extern void
_emit_stxi(jit_state_t*, jit_word_t, jit_int32_t, jit_int32_t);

#define emit_ldxi_d(r0, r1, i0)	_emit_ldxi_d(_jit, r0, r1, i0)
extern void
_emit_ldxi_d(jit_state_t*, jit_int32_t, jit_int32_t, jit_word_t);

#define emit_stxi_d(i0, r0, r1)	_emit_stxi(_jit, i0, r0, r1)
extern void
_emit_stxi_d(jit_state_t*, jit_word_t, jit_int32_t, jit_int32_t);

extern void jit_init_debug();
extern void jit_finish_debug();

/*
 * Externs
 */
extern jit_register_t	 _rvs[];

#endif /* _jit_private_h */