#ifndef __ARCH_arm_ORO_ATOMIC__
#define __ARCH_arm_ORO_ATOMIC__

/* Felipe Brandão Cavalcanti - cavalkaf@gmail.com
 * LARA - Laboratório de Robótica e Automacão, UnB, Brazil
 * http://www.lara.unb.br/
 * I pretty much copied and pasted the contents from the kernel files
 * linux-2.6.24/include/asm-arm/atomic.h
 * and added the oro_ and ORO_ before each function.
 * Functions for ARMv6 and over were removed
 */

#define oro_localirq_save_hw_notrace(x)					\
	({							\
		unsigned long temp;				\
		(void) (&temp == &x);				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ oro_localirq_save_hw\n"	\
"	orr	%1, %0, #128\n"					\
"	msr	cpsr_c, %1"					\
	: "=r" (x), "=r" (temp)					\
	:							\
	: "memory", "cc");					\
	})
#define oro_localirq_save_hw(flags)	oro_localirq_save_hw_notrace(flags)

#define oro_localirq_restore_hw_notrace(x)				\
	__asm__ __volatile__(					\
	"msr	cpsr_c, %0		@ oro_localirq_restore_hw\n"	\
	:							\
	: "r" (x)						\
	: "memory", "cc")
#define oro_localirq_restore_hw(flags)	oro_localirq_restore_hw_notrace(flags)	

typedef struct { volatile int counter; } oro_atomic_t;

#define ORO_ATOMIC_INIT(i)	{ (i) }

#define oro_atomic_read(v)	((v)->counter)

#define oro_atomic_set(v,i)	(((v)->counter) = (i))

static inline int oro_atomic_add_return(int i, oro_atomic_t *v)
{
	unsigned long flags;
	int val;

	oro_localirq_save_hw(flags);
	val = v->counter;
	v->counter = val += i;
	oro_localirq_restore_hw(flags);

	return val;
}

static inline int oro_atomic_sub_return(int i, oro_atomic_t *v)
{
	unsigned long flags;
	int val;

	oro_localirq_save_hw(flags);
	val = v->counter;
	v->counter = val -= i;
	oro_localirq_restore_hw(flags);

	return val;
}

static inline int oro_atomic_cmpxchg(oro_atomic_t *v, int old, int a)
{
	int ret;
	unsigned long flags;

	oro_localirq_save_hw(flags);
	ret = v->counter;
		v->counter = a;
	oro_localirq_restore_hw(flags);

	return ret;
}

static inline void oro_atomic_clear_mask(unsigned long mask, unsigned long *addr)
{
	unsigned long flags;

	oro_localirq_save_hw(flags);
	*addr &= ~mask;
	oro_localirq_restore_hw(flags);
}

#define oro_atomic_xchg(v, new) (xchg(&((v)->counter), new))

static inline int oro_atomic_add_unless(oro_atomic_t *v, int a, int u)
{
	int c, old;

	c = oro_atomic_read(v);
	while (c != u && (old = oro_atomic_cmpxchg((v), c, c + a)) != c)
		c = old;
	return c != u;
}
#define oro_atomic_inc_not_zero(v) oro_atomic_add_unless((v), 1, 0)

#define oro_atomic_add(i, v)	(void) oro_atomic_add_return(i, v)
#define oro_atomic_inc(v)		(void) oro_atomic_add_return(1, v)
#define oro_atomic_sub(i, v)	(void) oro_atomic_sub_return(i, v)
#define oro_atomic_dec(v)		(void) oro_atomic_sub_return(1, v)

#define oro_atomic_inc_and_test(v)	(oro_atomic_add_return(1, v) == 0)
#define oro_atomic_dec_and_test(v)	(oro_atomic_sub_return(1, v) == 0)
#define oro_atomic_inc_return(v)    (oro_atomic_add_return(1, v))
#define oro_atomic_dec_return(v)    (oro_atomic_sub_return(1, v))
#define oro_atomic_sub_and_test(i, v) (oro_atomic_sub_return(i, v) == 0)

#define oro_atomic_add_negative(i,v) (oro_atomic_add_return(i, v) < 0)

/* oro_atomic operations are already serializing on ARM */
#define smp_mb__before_oro_atomic_dec()	barrier()
#define smp_mb__after_oro_atomic_dec()	barrier()
#define smp_mb__before_oro_atomic_inc()	barrier()
#define smp_mb__after_oro_atomic_inc()	barrier()

#endif // __ARCH_powerpc_ORO_atomic__
