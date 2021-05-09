/******************************************************************************
*
*     Author: Xilinx, Inc.
*     
*     
*     This program is free software; you can redistribute it and/or modify it
*     under the terms of the GNU General Public License as published by the
*     Free Software Foundation; either version 2 of the License, or (at your
*     option) any later version.
*     
*     
*     XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
*     COURTESY TO YOU. BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
*     ONE POSSIBLE IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD,
*     XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE
*     FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING
*     ANY THIRD PARTY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
*     XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
*     THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY
*     WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM
*     CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND
*     FITNESS FOR A PARTICULAR PURPOSE.
*     
*     
*     Xilinx hardware products are not intended for use in life support
*     appliances, devices, or systems. Use in such applications is
*     expressly prohibited.
*     
*     
*     (c) Copyright 2002-2003 Xilinx Inc.
*     All rights reserved.
*     
*     
*     You should have received a copy of the GNU General Public License along
*     with this program; if not, write to the Free Software Foundation, Inc.,
*     675 Mass Ave, Cambridge, MA 02139, USA.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xemac_phy.c
*
* Contains functions to read and write the PHY through the Ethernet MAC MII
* registers.  These assume an MII-compliant PHY.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a rpm  07/31/01 First release
* 1.00b rpm  02/20/02 Repartitioned files and functions
* 1.00c rpm  12/05/02 New version includes support for simple DMA
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xbasic_types.h"
#include "xemac_i.h"
#include "xio.h"

/************************** Constant Definitions *****************************/

#define XEM_MAX_PHY_ADDR    32	/* Maximum PHY address */
#define XEM_MAX_PHY_REG     32	/* Maximum PHY register number */

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/*****************************************************************************/
/**
*
* Read the current value of the PHY register indicated by the PhyAddress and
* the RegisterNum parameters. The MAC provides the driver with the ability to
* talk to a PHY that adheres to the Media Independent Interface (MII) as
* defined in the IEEE 802.3 standard.
*
* @param InstancePtr is a pointer to the XEmac instance to be worked on.
* @param PhyAddress is the address of the PHY to be read (supports multiple
*        PHYs)
* @param RegisterNum is the register number, 0-31, of the specific PHY register
*        to read
* @param PhyDataPtr is an output parameter, and points to a 16-bit buffer into
*        which the current value of the register will be copied.
*
* @return
*
* - XST_SUCCESS if the PHY was read from successfully
* - XST_NO_FEATURE if the device is not configured with MII support
* - XST_EMAC_MII_BUSY if there is another PHY operation in progress
* - XST_EMAC_MII_READ_ERROR if a read error occurred between the MAC and the PHY
*
* @note
*
* This function is not thread-safe. The user must provide mutually exclusive
* access to this function if there are to be multiple threads that can call it.
* <br><br>
* There is the possibility that this function will not return if the hardware
* is broken (i.e., it never sets the status bit indicating that the read is
* done). If this is of concern to the user, the user should provide protection
* from this problem - perhaps by using a different timer thread to monitor the
* PhyRead thread.
*
******************************************************************************/
XStatus
XEmac_PhyRead(XEmac * InstancePtr, u32 PhyAddress,
	      u32 RegisterNum, u16 * PhyDataPtr)
{
	u32 MiiControl;
	u32 MiiData;

	XASSERT_NONVOID(InstancePtr != NULL);
	XASSERT_NONVOID(PhyAddress < XEM_MAX_PHY_ADDR);
	XASSERT_NONVOID(RegisterNum < XEM_MAX_PHY_REG);
	XASSERT_NONVOID(PhyDataPtr != NULL);
	XASSERT_NONVOID(InstancePtr->IsReady == XCOMPONENT_IS_READY);

	/*
	 * Make sure the device has the management interface
	 */
	if (!InstancePtr->HasMii) {
		return XST_NO_FEATURE;
	}

	/*
	 * Verify that there is no operation in progress already
	 */
	MiiControl = XIo_In32(InstancePtr->BaseAddress + XEM_MGTCR_OFFSET);
	if (MiiControl & XEM_MGTCR_START_MASK) {
		/* operation in progress */
		return XST_EMAC_MII_BUSY;
	}

	/*
	 * Set up the MII control register first.  We set up a control word with
	 * the PHY address and register number, then indicate the direction (read),
	 * then start the operation.
	 */
	MiiControl = PhyAddress << XEM_MGTCR_PHY_ADDR_SHIFT;
	MiiControl |= (RegisterNum << XEM_MGTCR_REG_ADDR_SHIFT);
	MiiControl |= (XEM_MGTCR_RW_NOT_MASK | XEM_MGTCR_START_MASK |
		       XEM_MGTCR_MII_ENABLE_MASK);

	XIo_Out32(InstancePtr->BaseAddress + XEM_MGTCR_OFFSET, MiiControl);

	/*
	 * Wait for the operation to complete
	 */
	do {
		MiiControl =
		    XIo_In32(InstancePtr->BaseAddress + XEM_MGTCR_OFFSET);
	}
	while (MiiControl & XEM_MGTCR_START_MASK);

	/*
	 * Now read the resulting MII data register.  First check to see if
	 * an error occurred before reading and returning the value in
	 * the MII data register.
	 */
	if (MiiControl & XEM_MGTCR_RD_ERROR_MASK) {
		/*
		 * MII read error occurred.  Upper layer will need to retry.
		 */
		return XST_EMAC_MII_READ_ERROR;
	}

	/*
	 * Retrieve the data from the 32-bit register, then copy it to
	 * the 16-bit output parameter.
	 */
	MiiData = XIo_In32(InstancePtr->BaseAddress + XEM_MGTDR_OFFSET);

	*PhyDataPtr = (u16) MiiData;

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* Write data to the specified PHY register. The Ethernet driver does not
* require the device to be stopped before writing to the PHY.  Although it is
* probably a good idea to stop the device, it is the responsibility of the
* application to deem this necessary. The MAC provides the driver with the
* ability to talk to a PHY that adheres to the Media Independent Interface
* (MII) as defined in the IEEE 802.3 standard.
*
* @param InstancePtr is a pointer to the XEmac instance to be worked on.
* @param PhyAddress is the address of the PHY to be written (supports multiple
*        PHYs)
* @param RegisterNum is the register number, 0-31, of the specific PHY register
*        to write
* @param PhyData is the 16-bit value that will be written to the register
*
* @return
*
* - XST_SUCCESS if the PHY was written to successfully. Since there is no error
*   status from the MAC on a write, the user should read the PHY to verify the
*   write was successful.
* - XST_NO_FEATURE if the device is not configured with MII support
* - XST_EMAC_MII_BUSY if there is another PHY operation in progress
*
* @note
*
* This function is not thread-safe. The user must provide mutually exclusive
* access to this function if there are to be multiple threads that can call it.
* <br><br>
* There is the possibility that this function will not return if the hardware
* is broken (i.e., it never sets the status bit indicating that the write is
* done). If this is of concern to the user, the user should provide protection
* from this problem - perhaps by using a different timer thread to monitor the
* PhyWrite thread.
*
******************************************************************************/
XStatus
XEmac_PhyWrite(XEmac * InstancePtr, u32 PhyAddress,
	       u32 RegisterNum, u16 PhyData)
{
	u32 MiiControl;

	XASSERT_NONVOID(InstancePtr != NULL);
	XASSERT_NONVOID(PhyAddress < XEM_MAX_PHY_ADDR);
	XASSERT_NONVOID(RegisterNum < XEM_MAX_PHY_REG);
	XASSERT_NONVOID(InstancePtr->IsReady == XCOMPONENT_IS_READY);

	/*
	 * Make sure the device has the management interface
	 */
	if (!InstancePtr->HasMii) {
		return XST_NO_FEATURE;
	}

	/*
	 * Verify that there is no operation in progress already
	 */
	MiiControl = XIo_In32(InstancePtr->BaseAddress + XEM_MGTCR_OFFSET);
	if (MiiControl & XEM_MGTCR_START_MASK) {
		/* operation in progress */
		return XST_EMAC_MII_BUSY;
	}

	/*
	 * Set up the MII data register first.  Write the 16-bit input
	 * value to the 32-bit data register.
	 */
	XIo_Out32(InstancePtr->BaseAddress + XEM_MGTDR_OFFSET, (u32) PhyData);

	/*
	 * Now set up the MII control register.  We set up a control
	 * word with the PHY address and register number, then indicate
	 * the direction (write), then start the operation.
	 */
	MiiControl = PhyAddress << XEM_MGTCR_PHY_ADDR_SHIFT;
	MiiControl |= (RegisterNum << XEM_MGTCR_REG_ADDR_SHIFT);
	MiiControl |= (XEM_MGTCR_START_MASK | XEM_MGTCR_MII_ENABLE_MASK);

	XIo_Out32(InstancePtr->BaseAddress + XEM_MGTCR_OFFSET, MiiControl);

	/*
	 * Wait for the operation to complete
	 */
	do {
		MiiControl =
		    XIo_In32(InstancePtr->BaseAddress + XEM_MGTCR_OFFSET);
	}
	while (MiiControl & XEM_MGTCR_START_MASK);

	/*
	 * There is no status indicating whether the operation was
	 * successful or not.
	 */
	return XST_SUCCESS;
}
