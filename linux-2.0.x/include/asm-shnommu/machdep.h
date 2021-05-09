#ifndef _SH7615_MACHDEP_H
#define _SH7615_MACHDEP_H

#define VULP(x) (*(volatile unsigned long*)(x))
#define VUSP(x) (*(volatile unsigned short*)(x))
#define VUCP(x) (*(volatile unsigned char*)(x))

struct pt_regs;
struct kbd_repeat;
struct mktime;
struct hwclk_time;
struct gendisk;
struct buffer_head;

extern void config_BSP(char *commandp, int size);

extern void (*mach_sched_init) (void (*handler)(int, void *, struct pt_regs *));
extern void (*mach_tick)(void);
/* machine dependent keyboard functions */
extern int (*mach_keyb_init) (void);
extern int (*mach_kbdrate) (struct kbd_repeat *);
extern void (*mach_kbd_leds) (unsigned int);
/* machine dependent irq functions */
/*extern void (*mach_init_IRQ) (void);
extern void (*(*mach_default_handler)[]) (int, void *, struct pt_regs *);
extern int (*mach_request_irq) (unsigned int irq, void (*handler)(int, void *,  struct pt_regs *),
				unsigned long flags, const char *devname, void *dev_id);
extern int (*mach_free_irq) (unsigned int irq, void *dev_id);
extern void (*mach_enable_irq) (unsigned int irq);
extern void (*mach_disable_irq) (unsigned int irq);
extern int (*mach_get_irq_list) (char *buf);
extern int (*mach_process_int) (int irq, struct pt_regs *fp);*/
extern void (*mach_trap_init) (void);
/* machine dependent timer functions */
extern unsigned long (*mach_gettimeoffset)(void);
extern void (*mach_gettod)(int *year, int *mon, int *day, int *hour,
                          int *min, int *sec);
extern int (*mach_hwclk)(int, struct hwclk_time*);
extern int (*mach_set_clock_mmss)(unsigned long);
extern void (*mach_mksound)( unsigned int count, unsigned int ticks );
extern void (*mach_reset)( void );
extern int (*mach_floppy_init) (void);
extern unsigned long (*mach_hd_init) (unsigned long, unsigned long);
extern void (*mach_hd_setup)(char *, int *);
extern void (*waitbut)(void);
extern struct fb_info *(*mach_fb_init)(long *);
extern long mach_max_dma_address;
extern void (*mach_debug_init)(void);
extern void (*mach_video_setup)(char *, int *);
extern void (*mach_floppy_setup)(char *, int *);
extern void (*mach_floppy_eject)(void);

#endif /* _SH7615_MACHDEP_H */
