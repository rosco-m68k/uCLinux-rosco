/*
 * arch/arm/mach-ixp425/coyote-pci.c 
 *
 * ADI Coyote PCI initialization
 *
 * Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/config.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <asm/mach/pci.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/arch/pci.h>


/* PCI controller pin mappings */
#define INTA_PIN	IXP425_GPIO_PIN_11
#define INTB_PIN	IXP425_GPIO_PIN_6

#define IXP425_PCI_RESET_GPIO   IXP425_GPIO_PIN_12
#define IXP425_PCI_CLK_PIN      IXP425_GPIO_CLK_0
#define IXP425_PCI_CLK_ENABLE   IXP425_GPIO_CLK0_ENABLE
#define IXP425_PCI_CLK_TC_LSH   IXP425_GPIO_CLK0TC_LSH
#define IXP425_PCI_CLK_DC_LSH   IXP425_GPIO_CLK0DC_LSH

void __init coyote_pci_init(void *sysdata)
{
	gpio_line_config(INTA_PIN, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);
	gpio_line_config(INTB_PIN, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);

	gpio_line_isr_clear(INTA_PIN);
	gpio_line_isr_clear(INTB_PIN);

	ixp425_pci_init(sysdata);
}

static int __init coyote_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	if(slot == 15)
		return IRQ_COYOTE_PCI_INTA;
	else if(slot == 14)
		return IRQ_COYOTE_PCI_INTB;
	else return -1;
}

struct hw_pci coyote_pci __initdata = {
	init:		coyote_pci_init,
	swizzle:	common_swizzle,
	map_irq:	coyote_map_irq,
};

