/*******************************************************************************

  ioaccess.c: IO access code for Lanner Platfomr Watchdog/Bypass program

  Lanner Platform Miscellaneous Utility
  Copyright(c) 2010 Lanner Electronics Inc.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer,
     without modification.
  2. Redistributions in binary form must reproduce at minimum a disclaimer
     similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
     redistribution must be conditioned upon including a substantially
     similar Disclaimer requirement for further binary redistribution.
  3. Neither the names of the above-listed copyright holders nor the names
     of any contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  Alternatively, this software may be distributed under the terms of the
  GNU General Public License ("GPL") version 2 as published by the Free
  Software Foundation.

  NO WARRANTY
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
  THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGES.

*******************************************************************************/



#include "../include/config.h"


#ifdef DJGPP

/* standard include file */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* For DOS DJGPP */
#include <dos.h>
#include <inlines/pc.h>

#else //DJGPP
/* For Linux */


#ifdef DIRECT_IO_ACCESS
/* For Linux direct io access code */
/* standard include file */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(LINUX_ENV)
#include <sys/io.h>
#endif

#if defined(FreeBSD_ENV)
#include <machine/cpufunc.h>
#endif


#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#define delay(x) usleep(x)
#endif


#ifdef MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/delay.h>

#undef delay
#define delay(x) mdelay(x)
#undef fprintf
#define fprintf(S, A)  printk(A)

#endif //MODULE

#ifdef KLD_MODULE

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/errno.h>

#include <machine/bus.h>
#include <machine/resource.h>

#endif

#endif

/* local include file */
#include "../include/ioaccess.h"

#if (defined(MODULE) || defined(DIRECT_IO_ACCESS) || defined(KLD_MODULE))


/*
 * Platform Depend GPIOs Interface for Watchdog and Lan bypass
 */
/*
 *------------------------------------------------------------------------------
 * MB-7537 Version V1.0
 *
 * MB-7437 embedded with Lan-bypass and HW Watchdog timer functions.
 * Set Lan bypass Enable/Disable while System-off:
 * ===============================================
 * It is able to set Lan bypass enable/disable in system off mode by SW program.
 * The IO interface for off-mode bypass is connected to Winbond SIO 83627DHG
 * GPO34,GPO35(Pair1), GPO32,GPO33(Pair2),
 * Refer to Winbond 83627 datasheet for details.
 *
 * The truth table of function is defined as below:
 *
 *	Pair 	Bypass function 	GPIO Pin 
 *      ---------------------------------------------------
 *  	1	Enable			GPO32=1 GPO33=0
 *  	1	Disable			GPO32=0 GPO33=1
 *  	2	Enable			GPO34=1 GPO34=0
 *  	2	Disable			GPO34=0 GPO35=1
 *
 * Runtime:
 * ========
 * It is able to set Lan bypass enable/disable alone, or design hybrid with 
 * watchdog timeout(WDTO#).
 * The IO interface for this function is conjunction with Winbond 83627 
 * GPO30 (Pair1), GPO31(Pair2) and WDTO#.
 * Refer to Winbond 83627 datasheet for details.
 * The truth table is defined as below:
 *
 * Below setting is to determine system behavior while watchdog timer expired.
 *
 *	GPO36	System behavior
 *  ------------------------------------------------
 *    	0      	Lan-bypass while watchdog timeout
 *    	1     	System Reset while watchdog timeout
 *
 * Below setting is to determine lan bypass in runtime mode
 *
 *	Pair	Bypass function 	GPIO Pin 
 *      -----------------------------------------------------------
 *  1	Enable			GPO30 =1
 *	1	Disable			GPO30 =0
 *  2	Enable			GPO31 =1
 *	2	Disable			GPO31 =0
 *
 *	Note: To sete runtime bypass mode, user need to set off-mode bypass
 *            enabled in order to let function activity.
 *	
 *------------------------------------------------------------------------------




 *------------------------------------------------------------------------------
 */
/*
 * Device Depend Definition : Winbond 83627DHG
 */

#define INDEX_PORT	0x2E
#define DATA_PORT	0x2F
#define SIO_GPIO_30_BIT	(1<<0)
#define SIO_GPIO_31_BIT	(1<<1)
#define SIO_GPIO_32_BIT	(1<<2)
#define SIO_GPIO_33_BIT	(1<<3)
#define SIO_GPIO_34_BIT	(1<<4)
#define SIO_GPIO_35_BIT	(1<<5)
#define SIO_GPIO_36_BIT	(1<<6)


void enter_w83627_config(void)
{
        outportb(INDEX_PORT, 0x87); // Must Do It Twice
        outportb(INDEX_PORT, 0x87);
        return;
}


void exit_w83627_config(void)
{
        outportb(INDEX_PORT, 0xAA);
        return;
}

unsigned char read_w83627_reg(int LDN, int reg)
{
        unsigned char tmp = 0;


        enter_w83627_config();
        outportb(INDEX_PORT, 0x07); // LDN Register
        outportb(DATA_PORT, LDN); // Select LDNx
        outportb(INDEX_PORT, reg); // Select Register
        tmp = inportb( DATA_PORT ); // Read Register
        exit_w83627_config();
        return tmp;
}


void write_w83627_reg(int LDN, int reg, int value)
{
        enter_w83627_config();
        outportb(INDEX_PORT, 0x07); // LDN Register
        outportb(DATA_PORT, LDN); // Select LDNx
        outportb(INDEX_PORT, reg); // Select Register
        outportb(DATA_PORT, value); // Write Register
        exit_w83627_config();
        return;
}


/*Runtime bypass definitions */
#define RUNTIME_BYPASS_PAIR1_LDN	(9)
#define RUNTIME_BYPASS_PAIR1_REG	(0xF1)
#define RUNTIME_BYPASS_PAIR1_BIT	(SIO_GPIO_30_BIT)
#define RUNTIME_BYPASS_PAIR1_ENABLE	(SIO_GPIO_30_BIT)
#define RUNTIME_BYPASS_PAIR1_DISABLE	(0)

#define RUNTIME_BYPASS_PAIR2_LDN	(9)
#define RUNTIME_BYPASS_PAIR2_REG	(0xF1)
#define RUNTIME_BYPASS_PAIR2_BIT	(SIO_GPIO_31_BIT)
#define RUNTIME_BYPASS_PAIR2_ENABLE	(SIO_GPIO_31_BIT)
#define RUNTIME_BYPASS_PAIR2_DISABLE	(0)

/*Offmode bypass definitions */
#define OFFMODE_BYPASS_PAIR1_LDN	(9)
#define OFFMODE_BYPASS_PAIR1_REG	(0xF1)
#define OFFMODE_BYPASS_PAIR1_BIT	(SIO_GPIO_34_BIT | SIO_GPIO_35_BIT)
#define OFFMODE_BYPASS_PAIR1_ENABLE	SIO_GPIO_34_BIT
#define OFFMODE_BYPASS_PAIR1_DISABLE	SIO_GPIO_35_BIT

#define OFFMODE_BYPASS_PAIR2_LDN	(9)
#define OFFMODE_BYPASS_PAIR2_REG	(0xF1)
#define OFFMODE_BYPASS_PAIR2_BIT	(SIO_GPIO_32_BIT | SIO_GPIO_33_BIT)
#define OFFMODE_BYPASS_PAIR2_ENABLE	SIO_GPIO_32_BIT
#define OFFMODE_BYPASS_PAIR2_DISABLE	SIO_GPIO_33_BIT


void start_watchdog_timer(int watchdog_time)
{
	unsigned char tmp;
	/* clear timeout value */
	write_w83627_reg(0x08, 0xf6, 0x00);

	/* set to count with second */
	tmp=read_w83627_reg(0x08, 0xF5);
	tmp &= ~(0x08);
	write_w83627_reg(0x08, 0xF5, tmp);

	/* clear status bit */
	tmp=read_w83627_reg(0x08, 0xf7);
	tmp &= ~(0x10);
	write_w83627_reg(0x08, 0xf7, tmp);
	
	/* set WDT Reset Event */
	tmp=read_w83627_reg(0x08, 0xF7);
	tmp = (0x00);
	write_w83627_reg(0x08, 0xF7, tmp);

	/* Set function enable */
	write_w83627_reg(0x08, 0x30, 1);

	/* fill in timeout value */
	write_w83627_reg(0x08, 0xf6, watchdog_time);
	
	return;	
}

void stop_watchdog_timer(void)
{

	/* stop timer */
	write_w83627_reg(0x08, 0xf6, 0);
}

int wd_gpio_init(void)
{
	unsigned char tmp;
	int ret=0;

	/* Set W83627 multiplex pin to WDTO function */
	tmp=read_w83627_reg(0x00, 0x2d);
	tmp &= ~(0x01);
	write_w83627_reg(0x00, 0x2d, tmp);
	
	/* clear timeout value */
	write_w83627_reg(0x08, 0xf6, 0x00);

	/* Enable LDN8 watchdog function */
	tmp=read_w83627_reg(0x08, 0x30);
	tmp |= 1;
	write_w83627_reg(0x08, 0x30, tmp);

	/* active GPIO3 group */
	tmp=read_w83627_reg(0x09, 0x30);
	tmp |= 2;
	write_w83627_reg(0x09, 0x30, tmp);

		
	/* Set GPIO30~36  to output mode */
	tmp=read_w83627_reg(0x09, 0xF0);
	tmp &= ~(SIO_GPIO_30_BIT+SIO_GPIO_31_BIT+SIO_GPIO_32_BIT+SIO_GPIO_33_BIT+SIO_GPIO_34_BIT+SIO_GPIO_35_BIT+SIO_GPIO_36_BIT) ;
	write_w83627_reg(0x09, 0xF0, tmp);

	return ret;

}

void set_bypass_enable_when_system_off(unsigned long pair_no)
{

	int reg_no, ldn_no;
	unsigned char bit_mask;
	unsigned char en_data;
	unsigned char tmp;

	reg_no=ldn_no=bit_mask=en_data=tmp=0;
	switch(pair_no) {
		case BYPASS_PAIR_1:
			ldn_no = OFFMODE_BYPASS_PAIR1_LDN;
			reg_no = OFFMODE_BYPASS_PAIR1_REG;
			bit_mask = OFFMODE_BYPASS_PAIR1_BIT;
			en_data = OFFMODE_BYPASS_PAIR1_ENABLE;
			break;
		case BYPASS_PAIR_2:
			ldn_no = OFFMODE_BYPASS_PAIR2_LDN;
			reg_no = OFFMODE_BYPASS_PAIR2_REG;
			bit_mask = OFFMODE_BYPASS_PAIR2_BIT;
			en_data = OFFMODE_BYPASS_PAIR2_ENABLE;
			break;
		default:
			/*un-support pair no, return */
			return;
	}
	tmp=read_w83627_reg(ldn_no, reg_no);
	tmp &= ~(bit_mask) ;
	tmp |= en_data;
	write_w83627_reg(ldn_no, reg_no, tmp);

	return;
}


void set_bypass_disable_when_system_off(unsigned long pair_no)
{

	int reg_no, ldn_no;
	unsigned char bit_mask;
	unsigned char en_data;
	unsigned char tmp;

	reg_no=ldn_no=bit_mask=en_data=tmp=0;
	switch(pair_no) {
		case BYPASS_PAIR_1:
			ldn_no = OFFMODE_BYPASS_PAIR1_LDN;
			reg_no = OFFMODE_BYPASS_PAIR1_REG;
			bit_mask = OFFMODE_BYPASS_PAIR1_BIT;
			en_data = OFFMODE_BYPASS_PAIR1_DISABLE;
			break;
		case BYPASS_PAIR_2:
			ldn_no = OFFMODE_BYPASS_PAIR2_LDN;
			reg_no = OFFMODE_BYPASS_PAIR2_REG;
			bit_mask = OFFMODE_BYPASS_PAIR2_BIT;
			en_data = OFFMODE_BYPASS_PAIR2_DISABLE;
			break;
		default:
			/*un-support pair no, return */
			return;
	}
	tmp=read_w83627_reg(ldn_no, reg_no);
	tmp &= ~(bit_mask) ;
	tmp |= en_data;
	write_w83627_reg(ldn_no, reg_no, tmp);

	return;
}

void set_runtime_bypass_enable(unsigned long pair_no)
{
	int reg_no, ldn_no;
        unsigned char tmp, bit_mask, en_data;

	reg_no=ldn_no=bit_mask=en_data=tmp=0;
/*      Note: To sete runtime bypass mode, user need to set off-mode bypass
 *            enabled in order to let function activity.
 */
        set_bypass_enable_when_system_off(pair_no);

        switch(pair_no) {
        	case BYPASS_PAIR_1:
			ldn_no = RUNTIME_BYPASS_PAIR1_LDN;
			reg_no = RUNTIME_BYPASS_PAIR1_REG;
                        bit_mask = RUNTIME_BYPASS_PAIR1_BIT;
                        en_data = RUNTIME_BYPASS_PAIR1_ENABLE;
                        break;
                case BYPASS_PAIR_2:
			ldn_no = RUNTIME_BYPASS_PAIR2_LDN;
			reg_no = RUNTIME_BYPASS_PAIR2_REG;
                        bit_mask = RUNTIME_BYPASS_PAIR2_BIT;
                        en_data = RUNTIME_BYPASS_PAIR2_ENABLE;
                        break;
		default:
			/*un-support pair no, return */
			return;

        }
	tmp=read_w83627_reg(ldn_no, reg_no);
	tmp &= ~(bit_mask) ;
	tmp |= en_data;
	write_w83627_reg(ldn_no, reg_no, tmp);

	return;
}

void set_runtime_bypass_disable(unsigned long pair_no)
{

	int reg_no, ldn_no;
	unsigned char tmp, bit_mask, en_data;

	reg_no=ldn_no=tmp=bit_mask=en_data=0;

	switch(pair_no) {
	    case BYPASS_PAIR_1:
		ldn_no = RUNTIME_BYPASS_PAIR1_LDN;
		reg_no = RUNTIME_BYPASS_PAIR1_REG;
		bit_mask = RUNTIME_BYPASS_PAIR1_BIT;
		en_data = RUNTIME_BYPASS_PAIR1_DISABLE;
		break;
	    case BYPASS_PAIR_2:
		ldn_no = RUNTIME_BYPASS_PAIR2_LDN;
		reg_no = RUNTIME_BYPASS_PAIR2_REG;
		bit_mask = RUNTIME_BYPASS_PAIR2_BIT;
		en_data = RUNTIME_BYPASS_PAIR2_DISABLE;
		break;
        }
	tmp=read_w83627_reg(ldn_no, reg_no);
	tmp &= ~(bit_mask) ;
	tmp |= en_data;
	write_w83627_reg(ldn_no, reg_no, tmp);

	return;
}

void set_wdto_state_system_reset(void)
{
	unsigned char tmp;

	/* set GPIO36=1 for reset mode */
	tmp=read_w83627_reg(0x9, 0xF1);
	tmp |= SIO_GPIO_36_BIT;
	write_w83627_reg(0x9, 0xF1, tmp);

	return;
}

void set_wdto_state_system_bypass(void)
{
	unsigned char tmp;

	/* set GPIO36=0 for bypass mode */
	tmp=read_w83627_reg(0x9, 0xF1);
	tmp &= ~SIO_GPIO_36_BIT;
	write_w83627_reg(0x9, 0xF1, tmp);

	return;
}

void show_dump()
{
	printf("runtime bypass pair 1: %s\n",
		(read_w83627_reg(RUNTIME_BYPASS_PAIR1_LDN, RUNTIME_BYPASS_PAIR1_REG) &
		 RUNTIME_BYPASS_PAIR1_ENABLE)?
		"enabled" : "disabled");
	printf("runtime bypass pair 2: %s\n",
		(read_w83627_reg(RUNTIME_BYPASS_PAIR2_LDN, RUNTIME_BYPASS_PAIR2_REG) &
		 RUNTIME_BYPASS_PAIR2_ENABLE)?
		"enabled" : "disabled");
	printf("off-mode bypass pair 1: %s\n",
		(read_w83627_reg(OFFMODE_BYPASS_PAIR1_LDN, OFFMODE_BYPASS_PAIR1_REG) &
		 OFFMODE_BYPASS_PAIR1_ENABLE)?
		"enabled" : "disabled");
	printf("off-mode bypass pair 2: %s\n",
		(read_w83627_reg(OFFMODE_BYPASS_PAIR2_LDN, OFFMODE_BYPASS_PAIR2_REG) &
		 OFFMODE_BYPASS_PAIR2_ENABLE)?
		"enabled" : "disabled");
	printf("watchdog timer state : %s\n", //bypass or reset
		(read_w83627_reg(0x9, 0xF1) & SIO_GPIO_36_BIT)?
		"reset" : "bypass");
	return;
}

#endif
