#ifndef _LINUX_LINKAGE_H
#define _LINUX_LINKAGE_H

#ifdef __cplusplus
#define asmlinkage extern "C"
#else
#define asmlinkage
#endif

#if defined(__ELF__) || defined(__COFF__)
#define SYMBOL_NAME_STR(X) #X
#define SYMBOL_NAME(X) X
#ifdef __STDC__
#define SYMBOL_NAME_LABEL(X) X##:
#else
#define SYMBOL_NAME_LABEL(X) X/**/:
#endif
#else
#define SYMBOL_NAME_STR(X) "_"#X
#ifdef __STDC__
#define SYMBOL_NAME(X) _##X
#define SYMBOL_NAME_LABEL(X) _##X##:
#else
#define SYMBOL_NAME(X) _/**/X
#define SYMBOL_NAME_LABEL(X) _/**/X/**/:
#endif
#endif

#ifdef __mc68000__
#if defined(__ELF__) || defined(__COFF__)
#define __ALIGN .align 4
#define __ALIGN_STR ".align 4"
#else
#define __ALIGN .align 2
#define __ALIGN_STR ".align 2"
#endif
#else /* !m68k */
#if !defined(__i486__) && !defined(__i586__)
#ifdef __ELF__
#define __ALIGN .align 4,0x90
#define __ALIGN_STR ".align 4,0x90"
#else  /* __ELF__ */
#define __ALIGN .align 2,0x90
#define __ALIGN_STR ".align 2,0x90"
#endif /* __ELF__ */
#else  /* __i486__/__i586__ */
#ifdef __ELF__
#define __ALIGN .align 16,0x90
#define __ALIGN_STR ".align 16,0x90"
#else  /* __ELF__ */
#define __ALIGN .align 4,0x90
#define __ALIGN_STR ".align 4,0x90"
#endif /* __ELF__ */
#endif /* __i486__/__i586__ */
#endif

#define ALIGN_STR __ALIGN_STR

#ifdef __ASSEMBLY__

#define ALIGN __ALIGN

#define ENTRY(name) \
  .globl SYMBOL_NAME(name); \
  ALIGN; \
  SYMBOL_NAME_LABEL(name)

#endif

#endif
