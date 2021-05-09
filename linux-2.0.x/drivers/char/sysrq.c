/*****************************************************************************/

/*
 *	linux/drivers/char/sysrq.c
 *
 *	Copyright (C) 1999, Greg Ungerer (gerg@snapgear.com)
 *	Copyright (C) 2000  Lineo Inc. (www.lineo.com)
 *
 *	Process System Request Magic Keys. Used for debugging...
 */

/*****************************************************************************/

/*
 *	Key that marks start of magic key sequence.
 *	Default is CTRL-X. When using magic system request key mode,
 *	any of these characters will disappear from the data stream.
 *	So choose carefully!
 */
int magic_sysrq_master = 24;

/*
 *	Mode we are in. Is next char expected to be a command.
 */
int magic_sysrq_mode = 0;

/*****************************************************************************/

int magic_sysrq_key(int ch)
{
	if (magic_sysrq_mode == 0) {
		if (ch == magic_sysrq_master) {
			magic_sysrq_mode++;
			return(1);
		}
		return(0);
	}

	switch (ch) {
	case 'a':
		printk("Show Free Areas:\n");
		show_free_areas();
		break;
	case 'b':
		printk("Show Buffers:\n");
		show_buffers();
		break;
#if 0
	case 'r':
		printk("Show Registers:\n");
		if (pt_regs)
			show_regs(pt_regs);
		break;
#endif
	case 't':
		printk("Show State:\n");
		show_state();
		break;
	case 'm':
		printk("Show Memory:\n");
		show_mem();
		break;
	case 'n':
		printk("Show Net Buffers:\n");
		show_net_buffers();
		break;
	default:
		/* Unkown command?? */
		break;
	}

	magic_sysrq_mode = 0;
	return(1);
}

/*****************************************************************************/
