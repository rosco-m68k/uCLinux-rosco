#include <linux/module.h>
#include <linux/string.h>

extern int active_ds;

/*
 * libgcc functions - functions that are used internally by the
 * compiler...  (prototypes are not correct though, but that
 * doesn't really matter since they're not versioned).
 */
extern void __ashrdi3(void);
extern void udivmodsi4(void);
extern void __divsi3(void);
extern void __modsi3(void);
extern void __mulsi3(void);
extern void __umodsi3(void);
extern void __udivsi3(void);
extern void __mulhi3(void);


static struct symbol_table arch_symbol_table = {
#include <linux/symtab_begin.h>
	/* platform dependent support */
	X(active_ds),
	/* The following are from string.h, which should be optimized
	as arch-specific, inline functions in asm-niosnommu/string.h. 
	They are put here before optimized ----wentao*/
	X(strcpy),
	X(strncpy),
	X(strcat),
	X(strncat),
	X(strcmp),
	X(strncmp),
	X(strchr),
	X(strrchr),
	X(strlen),
	X(strnlen),
	X(strspn),
	X(strpbrk),
	X(strtok),
	X(memset),
	X(memcpy),
	X(memmove),
	X(memcmp),
	X(memscan),
	X(strstr),
	/* The following are special because they're not called
	   explicitly (the C compiler generates them).  Fortunately,
	   their interface isn't gonna change any time soon now, so
	   it's OK to leave it out of version control.  */
	XNOVERS(__ashrdi3),
	XNOVERS(udivmodsi4),
	XNOVERS(__divsi3),
	XNOVERS(__modsi3),
	XNOVERS(__udivsi3),
	XNOVERS(__umodsi3),
	XNOVERS(__mulsi3),
	XNOVERS(__mulhi3),


#include <linux/symtab_end.h>
};

void arch_syms_export(void)
{
	register_symtab(&arch_symbol_table);
}

