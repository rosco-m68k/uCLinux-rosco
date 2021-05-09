#ifndef _ASM_OR32_SIGCONTEXT_H
#define _ASM_OR32_SIGCONTEXT_H

struct sigcontext_struct {
        unsigned long   _unused[4];
        int             signal;
        unsigned long   handler;
        unsigned long   oldmask;
        struct pt_regs  *regs;
};
#endif
