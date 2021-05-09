/*
 * arch/arm/mach-ixp425/ixp425-io.c 
 *
 * PCI I/O routines for IXP425.  IXP425 does not have an outbound 
 * I/O window, so we need to manually convert each operation into
 * a set of register acceses to configure the PCI byye lanes
 * that we want enabled, and then do the transaction.
 *
 * Copyright (C) 2002 Intel Corporation.
 *
 * Maintainer: Deepak Saxena <dsaxena@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * TODO: We probably want to at least inline these, and maybe
 * even ix425_pci_write?
 */

#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/io.h>

void ixp425_outb(u8 v, u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP425_PCI_NP_CBE_BESL;
	data = v << (8*n);
	ixp425_pci_write(p, byte_enables | NP_CMD_IOWRITE, data);
}

void ixp425_outw(u16 v, u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP425_PCI_NP_CBE_BESL;
	data = v << (8*n);
	ixp425_pci_write(p, byte_enables | NP_CMD_IOWRITE, data);
}

void ixp425_outl(u32 v, u32 p)
{
	ixp425_pci_write(p, NP_CMD_IOWRITE, v);
}

u8 ixp425_inb(u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP425_PCI_NP_CBE_BESL;
	if (ixp425_pci_read(p, byte_enables | NP_CMD_IOREAD, &data))
		return 0xff;

	return data >> (8*n);
}

u16 ixp425_inw(u32 p)
{
	u32 n, byte_enables, data;
	n = p % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP425_PCI_NP_CBE_BESL;
	if (ixp425_pci_read(p, byte_enables | NP_CMD_IOREAD, &data))
		return 0xffff;

	return data>>(8*n);
}

u32 ixp425_inl(u32 p)
{
	u32 data;
	if (ixp425_pci_read(p, NP_CMD_IOREAD, &data))
		return 0xffffffff;

	return data;
}

void ixp425_outsb(u32 p, u8 *addr, u32 count)
{
	while (count--)
		outb(*addr++, p);
}

void ixp425_outsw(u32 p, u16 *addr, u32 count)
{
	while (count--)
		outw(*addr++, p);
}

void ixp425_outsl(u32 p, u32 *addr, u32 count)
{
	while (count--)
		outl(*addr++, p);
}

void ixp425_insb(u32 p, u8 *addr, u32 count)
{
	while (count--)
		*addr++ = inb(p);
}

void ixp425_insw(u32 p, u16 *addr, u32 count)
{
	while (count--)
		*addr++ = inw(p);
}

void ixp425_insl(u32 p, u32 *addr, u32 count)
{
	while (count--)
		*addr++ = inl(p);
}

EXPORT_SYMBOL(ixp425_outb);
EXPORT_SYMBOL(ixp425_outw);
EXPORT_SYMBOL(ixp425_outl);
EXPORT_SYMBOL(ixp425_inb);
EXPORT_SYMBOL(ixp425_inw);
EXPORT_SYMBOL(ixp425_inl);
EXPORT_SYMBOL(ixp425_outsb);
EXPORT_SYMBOL(ixp425_outsw);
EXPORT_SYMBOL(ixp425_outsl);
EXPORT_SYMBOL(ixp425_insb);
EXPORT_SYMBOL(ixp425_insw);
EXPORT_SYMBOL(ixp425_insl);

