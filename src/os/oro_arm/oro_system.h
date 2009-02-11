
/* Felipe Brandão Cavalcanti - cavalkaf@gmail.com
 * LARA - Laboratório de Robótica e Automacão, UnB, Brazil
 * http://www.lara.unb.br/
 * I pretty much copied and pasted the contents from the kernel files
 * linux-2.6.24/include/asm-generic/cmpxchg.h and cmpxchg-local.h
 * and added the oro_ and ORO_ before each function.
 * Since ARM does not have a native cmpxchg function, generic ones are used
 */

#define oro_raw_local_irq_save(flags)	oro_local_irq_save_hw_notrace(flags)
#define oro_raw_local_irq_restore(flags)	oro_local_irq_restore_hw_notrace(flags)
#define oro_likely(x)       __builtin_expect((x),1)

#define oro_local_irq_save_hw_notrace(x)					\
	({							\
		unsigned long temp;				\
		(void) (&temp == &x);				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_save_hw\n"	\
"	orr	%1, %0, #128\n"					\
"	msr	cpsr_c, %1"					\
	: "=r" (x), "=r" (temp)					\
	:							\
	: "memory", "cc");					\
	})

#define oro_local_irq_restore_hw_notrace(x)				\
	__asm__ __volatile__(					\
	"msr	cpsr_c, %0		@ local_irq_restore_hw\n"	\
	:							\
	: "r" (x)						\
	: "memory", "cc")

#define oro_cmpxchg(ptr, old, new)			\
({						\
	unsigned long ____flags;		\
	__typeof__ (ptr) ____p = (ptr);		\
	__typeof__(*ptr) ____old = (old);	\
	__typeof__(*ptr) ____new = (new);	\
	__typeof__(*ptr) ____res;		\
	oro_raw_local_irq_save(____flags);		\
	____res = *____p;			\
	if (oro_likely(____res == (____old)))	\
		*____p = (____new);		\
	oro_raw_local_irq_restore(____flags);	\
	____res;				\
})
