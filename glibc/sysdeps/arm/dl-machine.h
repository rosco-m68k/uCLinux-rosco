/* Machine-dependent ELF dynamic relocation inline functions.  ARM version.
   Copyright (C) 1995,96,97,98,99,2000,2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef dl_machine_h
#define dl_machine_h

#define ELF_MACHINE_NAME "ARM"

#include <sys/param.h>

#define VALID_ELF_ABIVERSION(ver)	(ver == 0)
#define VALID_ELF_OSABI(osabi) \
  (osabi == ELFOSABI_SYSV || osabi == ELFOSABI_ARM)
#define VALID_ELF_HEADER(hdr,exp,size) \
  memcmp (hdr,exp,size-2) == 0 \
  && VALID_ELF_OSABI (hdr[EI_OSABI]) \
  && VALID_ELF_ABIVERSION (hdr[EI_ABIVERSION])

#define CLEAR_CACHE(BEG,END)						\
{									\
  register unsigned long _beg __asm ("a1") = (unsigned long)(BEG);	\
  register unsigned long _end __asm ("a2") = (unsigned long)(END);	\
  register unsigned long _flg __asm ("a3") = 0;				\
  __asm __volatile ("swi 0x9f0002		@ sys_cacheflush"	\
		    : /* no outputs */					\
		    : /* no inputs */					\
		    : "a1");						\
}

/* Return nonzero iff ELF header is compatible with the running host.  */
static inline int __attribute__ ((unused))
elf_machine_matches_host (const Elf32_Ehdr *ehdr)
{
  return ehdr->e_machine == EM_ARM;
}


/* Return the link-time address of _DYNAMIC.  Conveniently, this is the
   first element of the GOT.  This must be inlined in a function which
   uses global data.  */
static inline Elf32_Addr __attribute__ ((unused))
elf_machine_dynamic (void)
{
  register Elf32_Addr *got asm ("r10");
  return *got;
}


/* Return the run-time load address of the shared object.  */
static inline Elf32_Addr __attribute__ ((unused))
elf_machine_load_address (void)
{
  extern void __dl_start asm ("_dl_start");
  Elf32_Addr got_addr = (Elf32_Addr) &__dl_start;
  Elf32_Addr pcrel_addr;
  asm ("adr %0, _dl_start" : "=r" (pcrel_addr));
  return pcrel_addr - got_addr;
}


/* Set up the loaded object described by L so its unrelocated PLT
   entries will jump to the on-demand fixup code in dl-runtime.c.  */

static inline int __attribute__ ((unused))
elf_machine_runtime_setup (struct link_map *l, int lazy, int profile)
{
  Elf32_Addr *got;
  extern void _dl_runtime_resolve (Elf32_Word);
  extern void _dl_runtime_profile (Elf32_Word);

  if (l->l_info[DT_JMPREL] && lazy)
    {
      /* patb: this is different than i386 */
      /* The GOT entries for functions in the PLT have not yet been filled
	 in.  Their initial contents will arrange when called to push an
	 index into the .got section, load ip with &_GLOBAL_OFFSET_TABLE_[3],
	 and then jump to _GLOBAL_OFFSET_TABLE[2].  */
      got = (Elf32_Addr *) D_PTR (l, l_info[DT_PLTGOT]);
      got[1] = (Elf32_Addr) l;	/* Identify this shared object.  */

      /* The got[2] entry contains the address of a function which gets
	 called to get the address of a so far unresolved function and
	 jump to it.  The profiling extension of the dynamic linker allows
	 to intercept the calls to collect information.  In this case we
	 don't store the address in the GOT so that all future calls also
	 end in this function.  */
      if (profile)
	{
	  got[2] = (Elf32_Addr) &_dl_runtime_profile;

	  if (_dl_name_match_p (_dl_profile, l))
	    /* Say that we really want profiling and the timers are
	       started.  */
	    _dl_profile_map = l;
	}
      else
	/* This function will get called to fix up the GOT entry indicated by
	   the offset on the stack, and then jump to the resolved address.  */
	got[2] = (Elf32_Addr) &_dl_runtime_resolve;
    }
  return lazy;
}

/* This code is used in dl-runtime.c to call the `fixup' function
   and then redirect to the address it returns.  */
   // macro for handling PIC situation....
#ifdef PIC
#define CALL_ROUTINE(x) " ldr sl,0f \n\
	add 	sl, pc, sl \n\
1:	ldr	r2, 2f \n\
	mov	lr, pc \n\
	add	pc, sl, r2 \n\
	b	3f \n\
0:	.word	_GLOBAL_OFFSET_TABLE_ - 1b - 4 \n\
2:	.word " #x "(GOTOFF) \n\
3:	"
#else
#define CALL_ROUTINE(x) " bl " #x
#endif

#ifndef PROF
# define ELF_MACHINE_RUNTIME_TRAMPOLINE asm ("\n\
	.text \n\
	.globl _dl_runtime_resolve \n\
	.type _dl_runtime_resolve, #function \n\
	.align 2 \n\
_dl_runtime_resolve: \n\
	@ we get called with \n\
	@ 	stack[0] contains the return address from this call \n\
	@	ip contains &GOT[n+3] (pointer to function) \n\
	@	lr points to &GOT[2] \n\
 \n\
	@ save almost everything; lr is already on the stack \n\
	stmdb	sp!,{r0-r3,sl,fp} \n\
 \n\
	@ prepare to call fixup() \n\
	@ change &GOT[n+3] into 8*n        NOTE: reloc are 8 bytes each \n\
	sub	r1, ip, lr \n\
	sub	r1, r1, #4 \n\
	add	r1, r1, r1 \n\
 \n\
	@ get pointer to linker struct \n\
	ldr	r0, [lr, #-4] \n\
 \n\
	@ call fixup routine \n\
	" CALL_ROUTINE(fixup) " \n\
 \n\
	@ save the return \n\
	mov	ip, r0 \n\
 \n\
	@ restore the stack \n\
	ldmia	sp!,{r0-r3,sl,fp,lr} \n\
 \n\
	@ jump to the newly found address \n\
	mov	pc, ip \n\
 \n\
	.size _dl_runtime_resolve, .-_dl_runtime_resolve \n\
 \n\
	.globl _dl_runtime_profile \n\
	.type _dl_runtime_profile, #function \n\
	.align 2 \n\
_dl_runtime_profile: \n\
	@ save almost everything; lr is already on the stack \n\
	stmdb	sp!,{r0-r3,sl,fp} \n\
 \n\
	@ prepare to call fixup() \n\
	@ change &GOT[n+3] into 8*n        NOTE: reloc are 8 bytes each \n\
	sub	r1, ip, lr \n\
	sub	r1, r1, #4 \n\
	add	r1, r1, r1 \n\
 \n\
	@ get pointer to linker struct \n\
	ldr	r0, [lr, #-4] \n\
 \n\
	@ call profiling fixup routine \n\
	" CALL_ROUTINE(profile_fixup) " \n\
 \n\
	@ save the return \n\
	mov	ip, r0 \n\
 \n\
	@ restore the stack \n\
	ldmia	sp!,{r0-r3,sl,fp,lr} \n\
 \n\
	@ jump to the newly found address \n\
	mov	pc, ip \n\
 \n\
	.size _dl_runtime_resolve, .-_dl_runtime_resolve \n\
	.previous \n\
");
#else // PROF
# define ELF_MACHINE_RUNTIME_TRAMPOLINE asm ("\n\
	.text \n\
	.globl _dl_runtime_resolve \n\
	.globl _dl_runtime_profile \n\
	.type _dl_runtime_resolve, #function \n\
	.type _dl_runtime_profile, #function \n\
	.align 2 \n\
_dl_runtime_resolve: \n\
_dl_runtime_profile: \n\
	@ we get called with \n\
	@ 	stack[0] contains the return address from this call \n\
	@	ip contains &GOT[n+3] (pointer to function) \n\
	@	lr points to &GOT[2] \n\
 \n\
	@ save almost everything; return add is already on the stack \n\
	stmdb	sp!,{r0-r3,sl,fp} \n\
 \n\
	@ prepare to call fixup() \n\
	@ change &GOT[n+3] into 8*n        NOTE: reloc are 8 bytes each \n\
	sub	r1, ip, lr \n\
	sub	r1, r1, #4 \n\
	add	r1, r1, r1 \n\
 \n\
	@ get pointer to linker struct \n\
	ldr	r0, [lr, #-4] \n\
 \n\
	@ call profiling fixup routine \n\
	" CALL_ROUTINE(fixup) " \n\
 \n\
	@ save the return \n\
	mov	ip, r0 \n\
 \n\
	@ restore the stack \n\
	ldmia	sp!,{r0-r3,sl,fp,lr} \n\
 \n\
	@ jump to the newly found address \n\
	mov	pc, ip \n\
 \n\
	.size _dl_runtime_profile, .-_dl_runtime_profile \n\
	.previous \n\
");
#endif //PROF

/* Mask identifying addresses reserved for the user program,
   where the dynamic linker should not map anything.  */
#define ELF_MACHINE_USER_ADDRESS_MASK	0xf8000000UL

/* Initial entry point code for the dynamic linker.
   The C function `_dl_start' is the real entry point;
   its return value is the user program's entry point.  */

#define RTLD_START asm ("\n\
.text \n\
.globl _start \n\
.globl _dl_start_user \n\
_start: \n\
	@ at start time, all the args are on the stack \n\
	mov	r0, sp \n\
	bl	_dl_start \n\
	@ returns user entry point in r0 \n\
_dl_start_user: \n\
	mov	r6, r0 \n\
	@ we are PIC code, so get global offset table \n\
	ldr	sl, .L_GET_GOT \n\
	add	sl, pc, sl \n\
.L_GOT_GOT: \n\
	@ Store the highest stack address \n\
	ldr	r1, .L_STACK_END \n\
	ldr	r1, [sl, r1] \n\
	str	sp, [r1] \n\
	@ See if we were run as a command with the executable file \n\
	@ name as an extra leading argument. \n\
	ldr	r4, .L_SKIP_ARGS \n\
	ldr	r4, [sl, r4] \n\
	@ get the original arg count \n\
	ldr	r1, [sp] \n\
	@ subtract _dl_skip_args from it \n\
	sub	r1, r1, r4 \n\
	@ adjust the stack pointer to skip them \n\
	add	sp, sp, r4, lsl #2 \n\
	@ get the argv address \n\
	add	r2, sp, #4 \n\
	@ store the new argc in the new stack location \n\
	str	r1, [sp] \n\
	@ compute envp \n\
	add	r3, r2, r1, lsl #2 \n\
	add	r3, r3, #4 \n\
 \n\
	@ now we call _dl_init \n\
	ldr	r0, .L_LOADED \n\
	ldr	r0, [sl, r0] \n\
	ldr	r0, [r0] \n\
	@ call _dl_init \n\
	bl	_dl_init(PLT) \n\
	@ clear the startup flag \n\
	ldr	r2, .L_STARTUP_FLAG \n\
	ldr	r1, [sl, r2] \n\
	mov	r0, #0 \n\
	str	r0, [r1] \n\
	@ load the finalizer function \n\
	ldr	r0, .L_FINI_PROC \n\
	ldr	r0, [sl, r0] \n\
	@ jump to the user_s entry point \n\
	mov	pc, r6 \n\
.L_GET_GOT: \n\
	.word	_GLOBAL_OFFSET_TABLE_ - .L_GOT_GOT - 4	\n\
.L_SKIP_ARGS:					\n\
	.word	_dl_skip_args(GOTOFF)		\n\
.L_STARTUP_FLAG: \n\
	.word	_dl_starting_up(GOT) \n\
.L_FINI_PROC: \n\
	.word	_dl_fini(GOT) \n\
.L_STACK_END: \n\
	.word	__libc_stack_end(GOT) \n\
.L_LOADED: \n\
	.word	_dl_loaded(GOT) \n\
.previous\n\
");

/* ELF_RTYPE_CLASS_PLT iff TYPE describes relocation of a PLT entry, so
   PLT entries should not be allowed to define the value.
   ELF_RTYPE_CLASS_NOCOPY iff TYPE should not be allowed to resolve to one
   of the main executable's symbols, as for a COPY reloc.  */
#define elf_machine_type_class(type) \
  ((((type) == R_ARM_JUMP_SLOT) * ELF_RTYPE_CLASS_PLT)	\
   | (((type) == R_ARM_COPY) * ELF_RTYPE_CLASS_COPY))

/* A reloc type used for ld.so cmdline arg lookups to reject PLT entries.  */
#define ELF_MACHINE_JMP_SLOT	R_ARM_JUMP_SLOT

/* The ARM never uses Elf32_Rela relocations.  */
#define ELF_MACHINE_NO_RELA 1

/* We define an initialization functions.  This is called very early in
   _dl_sysdep_start.  */
#define DL_PLATFORM_INIT dl_platform_init ()

extern const char *_dl_platform;

static inline void __attribute__ ((unused))
dl_platform_init (void)
{
  if (_dl_platform != NULL && *_dl_platform == '\0')
    /* Avoid an empty string which would disturb us.  */
    _dl_platform = NULL;
}

static inline Elf32_Addr
elf_machine_fixup_plt (struct link_map *map, lookup_t t,
		       const Elf32_Rel *reloc,
		       Elf32_Addr *reloc_addr, Elf32_Addr value)
{
  return *reloc_addr = value;
}

/* Return the final value of a plt relocation.  */
static inline Elf32_Addr
elf_machine_plt_value (struct link_map *map, const Elf32_Rel *reloc,
		       Elf32_Addr value)
{
  return value;
}

#endif /* !dl_machine_h */

#ifdef RESOLVE

extern char **_dl_argv;

/* Deal with an out-of-range PC24 reloc.  */
static Elf32_Addr
fix_bad_pc24 (Elf32_Addr *const reloc_addr, Elf32_Addr value)
{
  static void *fix_page;
  static unsigned int fix_offset;
  static size_t pagesize;
  Elf32_Word *fix_address;

  if (! fix_page)
    {
      if (! pagesize)
	pagesize = getpagesize ();
      fix_page = mmap (NULL, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (! fix_page)
	assert (! "could not map page for fixup");
      fix_offset = 0;
    }

  fix_address = (Elf32_Word *)(fix_page + fix_offset);
  fix_address[0] = 0xe51ff004;	/* ldr pc, [pc, #-4] */
  fix_address[1] = value;

  fix_offset += 8;
  if (fix_offset >= pagesize)
    fix_page = NULL;

  return (Elf32_Addr)fix_address;
}

/* Perform the relocation specified by RELOC and SYM (which is fully resolved).
   MAP is the object containing the reloc.  */

static inline void
elf_machine_rel (struct link_map *map, const Elf32_Rel *reloc,
		 const Elf32_Sym *sym, const struct r_found_version *version,
		 Elf32_Addr *const reloc_addr)
{
  const unsigned int r_type = ELF32_R_TYPE (reloc->r_info);

  if (__builtin_expect (r_type == R_ARM_RELATIVE, 0))
    {
#ifndef RTLD_BOOTSTRAP
      if (map != &_dl_rtld_map) /* Already done in rtld itself.  */
#endif
	*reloc_addr += map->l_addr;
    }
#ifndef RTLD_BOOTSTRAP
  else if (__builtin_expect (r_type == R_ARM_NONE, 0))
    return;
#endif
  else
    {
      const Elf32_Sym *const refsym = sym;
      Elf32_Addr value = RESOLVE (&sym, version, r_type);
      if (sym)
	value += sym->st_value;

      switch (r_type)
	{
	case R_ARM_COPY:
	  if (sym == NULL)
	    /* This can happen in trace mode if an object could not be
	       found.  */
	    break;
	  if (sym->st_size > refsym->st_size
	      || (_dl_verbose && sym->st_size < refsym->st_size))
	    {
	      const char *strtab;

	      strtab = (const void *) D_PTR (map, l_info[DT_STRTAB]);
	      _dl_error_printf ("\
%s: Symbol `%s' has different size in shared object, consider re-linking\n",
				_dl_argv[0] ?: "<program name unknown>",
				strtab + refsym->st_name);
	    }
	  memcpy (reloc_addr, (void *) value, MIN (sym->st_size,
						   refsym->st_size));
	  break;
	case R_ARM_GLOB_DAT:
	case R_ARM_JUMP_SLOT:
#ifdef RTLD_BOOTSTRAP
	  /* Fix weak undefined references.  */
	  if (sym != NULL && sym->st_value == 0)
	    *reloc_addr = 0;
	  else
#endif
	    *reloc_addr = value;
	  break;
	case R_ARM_ABS32:
	  {
#ifndef RTLD_BOOTSTRAP
	   /* This is defined in rtld.c, but nowhere in the static
	      libc.a; make the reference weak so static programs can
	      still link.  This declaration cannot be done when
	      compiling rtld.c (i.e.  #ifdef RTLD_BOOTSTRAP) because
	      rtld.c contains the common defn for _dl_rtld_map, which
	      is incompatible with a weak decl in the same file.  */
	    weak_extern (_dl_rtld_map);
	    if (map == &_dl_rtld_map)
	      /* Undo the relocation done here during bootstrapping.
		 Now we will relocate it anew, possibly using a
		 binding found in the user program or a loaded library
		 rather than the dynamic linker's built-in definitions
		 used while loading those libraries.  */
	      value -= map->l_addr + refsym->st_value;
#endif
	    *reloc_addr += value;
	    break;
	  }
	case R_ARM_PC24:
	  {
	     Elf32_Sword addend;
	     Elf32_Addr newvalue, topbits;

	     addend = *reloc_addr & 0x00ffffff;
	     if (addend & 0x00800000) addend |= 0xff000000;

	     newvalue = value - (Elf32_Addr)reloc_addr + (addend << 2);
	     topbits = newvalue & 0xfe000000;
	     if (topbits != 0xfe000000 && topbits != 0x00000000)
	       {
		 newvalue = fix_bad_pc24(reloc_addr, value)
		   - (Elf32_Addr)reloc_addr + (addend << 2);
		 topbits = newvalue & 0xfe000000;
		 if (topbits != 0xfe000000 && topbits != 0x00000000)
		   {
		     _dl_signal_error (0, map->l_name, NULL,
				       "R_ARM_PC24 relocation out of range");
		   }
	       }
	     newvalue >>= 2;
	     value = (*reloc_addr & 0xff000000) | (newvalue & 0x00ffffff);
	     *reloc_addr = value;
	  }
	break;
	default:
	  _dl_reloc_bad_type (map, r_type, 0);
	  break;
	}
    }
}

static inline void
elf_machine_rel_relative (Elf32_Addr l_addr, const Elf32_Rel *reloc,
			  Elf32_Addr *const reloc_addr)
{
  *reloc_addr += l_addr;
}

static inline void
elf_machine_lazy_rel (struct link_map *map,
		      Elf32_Addr l_addr, const Elf32_Rel *reloc)
{
  Elf32_Addr *const reloc_addr = (void *) (l_addr + reloc->r_offset);
  const unsigned int r_type = ELF32_R_TYPE (reloc->r_info);
  /* Check for unexpected PLT reloc type.  */
  if (__builtin_expect (r_type == R_ARM_JUMP_SLOT, 1))
    *reloc_addr += l_addr;
  else
    _dl_reloc_bad_type (map, r_type, 1);
}

#endif /* RESOLVE */
