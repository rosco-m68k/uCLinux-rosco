#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/page.h>
#include <asm/machdep.h>

/* table for system interrupt handlers */
static irq_handler_t irq_list[SYS_IRQS];

static const char *default_names[SYS_IRQS] = {
	"int0", "int1", "int2", "int3",	"int4", "int5", "int6", "int7"
	"int8", "int9", "int10", "int11", "int12", "int13", "int14", "int15"
	"int16", "int17", "int18", "int19", "int20", "int21", "int22", "int23"
	"int24", "int25", "int26", "int27", "int28", "int29", "int30", "int31"
};

int pic_enable_irq(unsigned int irq)
{
	/* Enable in the IMR */
	mtspr(SPR_PICMR, mfspr(SPR_PICMR) | (0x00000001L << irq));

	return 0;
}

int pic_disable_irq(unsigned int irq)
{
	/* Disable in the IMR */
	mtspr(SPR_PICMR, mfspr(SPR_PICMR) & ~(0x00000001L << irq));

	return 0;
}

int pic_init(void)
{
	/* turn off all interrupts */
	mtspr(SPR_PICMR, 0);
	return 0;
}

int pic_do_irq(struct pt_regs *fp)
{
	int irq;
	int mask;

	unsigned long pend = mfspr(SPR_PICSR) & 0xfffffffc;

	if (pend & 0x0000ffff) {
		if (pend & 0x000000ff) {
			if (pend & 0x0000000f) {
				mask = 0x00000001;
				irq = 0;
			} else {
				mask = 0x00000010;
				irq = 4;
			}
		} else {
			if (pend & 0x00000f00) {
				mask = 0x00000100;
				irq = 8;
			} else {
				mask = 0x00001000;
				irq = 12;
			}
		}
	} else if(pend & 0xffff0000) {
		if (pend & 0x00ff0000) {
			if (pend & 0x000f0000) {
				mask = 0x00010000;
				irq = 16;
			} else {
				mask = 0x00100000;
				irq = 20;
			}
		} else {
			if (pend & 0x0f000000) {
				mask = 0x01000000;
				irq = 24;
			} else {
				mask = 0x10000000;
				irq = 28;
			}
		}
	} else {
		return -1;
	}

	while (! (mask & pend)) {
		mask <<=1;
		irq++;
	}

	mtspr(SPR_PICSR, mfspr(SPR_PICSR) & ~mask);
	return irq;
}

void init_IRQ(void)
{
	int i;

	for (i = 0; i < SYS_IRQS; i++) {
		irq_list[i].handler = NULL;
		irq_list[i].flags   = IRQ_FLG_STD;
		irq_list[i].dev_id  = NULL;
		irq_list[i].devname = default_names[i];
	}

	pic_init();
}

asmlinkage void handle_IRQ(struct pt_regs *regs)
{
	int irq;

	while((irq = pic_do_irq(regs)) >= 0) {

		if (irq_list[irq].handler)
			irq_list[irq].handler(irq, irq_list[irq].dev_id, regs);
		else
			panic("No interrupt handler for autovector %d\n", irq);
	}
}

int request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
                unsigned long flags, const char *devname, void *dev_id)
{
	if (irq >= SYS_IRQS) {
		printk("%s: Incorrect IRQ %d from %s\n", __FUNCTION__, irq, devname);
		return -ENXIO;
	}

	if (!(irq_list[irq].flags & IRQ_FLG_STD)) {
		if (irq_list[irq].flags & IRQ_FLG_LOCK) {
			printk("%s: IRQ %d from %s is not replaceable\n",
			       __FUNCTION__, irq, irq_list[irq].devname);
			return -EBUSY;
		}
		if (flags & IRQ_FLG_REPLACE) {
			printk("%s: %s can't replace IRQ %d from %s\n",
			       __FUNCTION__, devname, irq, irq_list[irq].devname);
			return -EBUSY;
		}
	}
	irq_list[irq].handler = handler;
	irq_list[irq].flags   = flags;
	irq_list[irq].dev_id  = dev_id;
	irq_list[irq].devname = devname;

  pic_enable_irq(irq);
	
	return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
	if (irq >= SYS_IRQS) {
		printk("%s: Incorrect IRQ %d\n", __FUNCTION__, irq);
		return;
	}

  pic_disable_irq(irq);

	irq_list[irq].handler = NULL;
	irq_list[irq].flags   = IRQ_FLG_STD;
	irq_list[irq].dev_id  = NULL;
	irq_list[irq].devname = default_names[irq];
}

unsigned long probe_irq_on (void)
{
	return 0;
}

int probe_irq_off (unsigned long irqs)
{
	return 0;
}

void enable_irq(unsigned int irq)
{
	if (irq >= SYS_IRQS) {
		printk("%s: Incorrect IRQ %d\n", __FUNCTION__, irq);
		return;
	}
	pic_enable_irq(irq);
}

void disable_irq(unsigned int irq)
{
	if (irq >= SYS_IRQS) {
		printk("%s: Incorrect IRQ %d\n", __FUNCTION__, irq);
		return;
	}
	pic_disable_irq(irq);
}

int get_irq_list(char *buf)
{
	int i, len = 0;

	/* autovector interrupts */
	for (i = 0; i < SYS_IRQS; i++) {
		if (irq_list[i].handler) {
			if (irq_list[i].flags & IRQ_FLG_LOCK)
				len += sprintf(buf+len, "L ");
			else
				len += sprintf(buf+len, "  ");
			if (irq_list[i].flags & IRQ_FLG_PRI_HI)
				len += sprintf(buf+len, "H ");
			else
				len += sprintf(buf+len, "L ");
			len += sprintf(buf+len, "%s\n", irq_list[i].devname);
		}
	}

	return len;
}

void dump(struct pt_regs *fp)
{
	unsigned long	*sp;
	unsigned char	*tp;
	int		i;

	printk("\nCURRENT PROCESS:\n\n");
	printk("COMM=%s PID=%d\n", current->comm, current->pid);
	if (current->mm) {
		printk("TEXT=%08x-%08x DATA=%08x-%08x BSS=%08x-%08x\n",
			(int) current->mm->start_code,
			(int) current->mm->end_code,
			(int) current->mm->start_data,
			(int) current->mm->end_data,
			(int) current->mm->end_data,
			(int) current->mm->brk);
		printk("USER-STACK=%08x  KERNEL-STACK=%08x\n\n",
			(int) current->mm->start_stack,
			(int) current->kernel_stack_page);
	}
	printk("PC: %08lx  Status: %08lx\n",
	       fp->pc, fp->sr);
	printk("R0 : %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	0L,             fp->sp,      fp->gprs[0], fp->gprs[1], 
		fp->gprs[2], fp->gprs[3], fp->gprs[4], fp->gprs[5]);
	printk("R8 : %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	fp->gprs[6], fp->gprs[7], fp->gprs[8], fp->gprs[9], 
		fp->gprs[10], fp->gprs[11], fp->gprs[12], fp->gprs[13]);
	printk("R16: %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	fp->gprs[14], fp->gprs[15], fp->gprs[16], fp->gprs[17], 
		fp->gprs[18], fp->gprs[19], fp->gprs[20], fp->gprs[21]);
	printk("R24: %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	fp->gprs[22], fp->gprs[23], fp->gprs[24], fp->gprs[25], 
		fp->gprs[26], fp->gprs[27], fp->gprs[28], fp->gprs[29]);

	printk("\nUSP: %08lx   TRAPFRAME: %08x\n",
		fp->sp, (unsigned int) fp);

	printk("\nCODE:");
	tp = ((unsigned char *) fp->pc) - 0x20;
	for (sp = (unsigned long *) tp, i = 0; (i < 0x40);  i += 4) {
		if ((i % 0x10) == 0)
			printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n");

	printk("\nKERNEL STACK:");
	tp = ((unsigned char *) fp) - 0x40;
	for (sp = (unsigned long *) tp, i = 0; (i < 0xc0); i += 4) {
		if ((i % 0x10) == 0)
			printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n");
	if (STACK_MAGIC != *(unsigned long *)current->kernel_stack_page)
                printk("(Possibly corrupted stack page??)\n");
	printk("\n");

	printk("\nUSER STACK:");
	tp = (unsigned char *) (fp->sp - 0x10);
	for (sp = (unsigned long *) tp, i = 0; (i < 0x80); i += 4) {
		if ((i % 0x10) == 0)
			printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n\n");
}
