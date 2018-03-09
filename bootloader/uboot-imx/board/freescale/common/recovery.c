/*
 * Freescale Android Recovery mode checking routing
 *
 * Copyright (C) 2010-2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include <common.h>
#include <malloc.h>
#include <recovery.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <part.h>
#include <fat.h>
#include <malloc.h>

#define CONFIG_MXC_KPD	0

#ifdef CONFIG_MXC_KPD
#include <mxc_keyb.h>
#endif

extern int check_recovery_cmd_file(void);
extern enum boot_device get_boot_device(void);

#ifdef CONFIG_MXC_KPD

#define PRESSED_VOL_DOWN	0x01
#define PRESSED_POWER	    0x02
#define RECOVERY_KEY_MASK (PRESSED_VOL_DOWN | PRESSED_POWER)

#define SD3_CD	IMX_GPIO_NR(7, 0)

inline int test_key(int value, struct kpp_key_info *ki)
{
	return (ki->val == value) && (ki->evt == KDepress);
}

//add ben
unsigned short checksum(unsigned short * data, int len)    
{
	unsigned long sum=0;
    
    for(;len>1;len-=2)
    {
		sum+= *data ++;
        if(sum & 0x80000000)
			sum=(sum & 0xffff) + (sum >>16);  

	}

   if(len ==1)    /*如果是奇数的话*/
   {
		u_short i=0;
        *(unsigned char *) (&i) = *(unsigned char *)data;
        sum+=i;
	}

    while(sum >>16)
    sum=(sum & 0xffff)  +(sum>>16);
    
	return (sum ==0xffff) ? sum: ~sum;
}

int check_key_pressing(void)
{
#if 0	
	struct kpp_key_info *key_info = NULL;
	int state = 0, keys, i;

	int ret = 0;

	mxc_kpp_init();
	/* due to glitch suppression circuit,
	   wait sometime to let all keys scanned. */
	udelay(1000);
	keys = mxc_kpp_getc(&key_info);

	printf("Detecting VOL_DOWN+POWER key for recovery(%d:%d) ...\n",
		keys, keys ? key_info->val : 0);
	if (keys > 1) {
		for (i = 0; i < keys; i++) {
			if (test_key(CONFIG_POWER_KEY, &key_info[i]))
				state |= PRESSED_POWER;
			else if (test_key(CONFIG_VOL_DOWN_KEY, &key_info[i]))
				state |= PRESSED_VOL_DOWN;
		}
	}
	if ((state & RECOVERY_KEY_MASK) == RECOVERY_KEY_MASK)
		ret = 1;
	if (key_info)
		free(key_info);
	return ret;
#endif
	int cd;
	int ret;
	int filelen;
	block_dev_desc_t *dev_desc = NULL;
	struct mmc *mmc = find_mmc_device(2);	
	disk_partition_t info;
	unsigned char buff[300*1024*1024];
	
	gpio_direction_input(SD3_CD);
	cd = gpio_get_value(SD3_CD);
	if (cd == 0)
	{
		printf("sd3_cd ==0 \n");
		//buff = malloc(300*1024*1024);

		//if (buff == NULL)
		//{	
		//	printf("buff malloc error \n");
		//	return 0;
		//}
		
		//mmc dev 2
		dev_desc = get_dev("mmc", 2);
		if (NULL == dev_desc) {
			puts("** Block device MMC 0 not supported\n");
			return 0;
		}

		mmc_init(mmc);
		if (get_partition_info(dev_desc, 1, &info))
		{	
			printf("** Bad partition  **\n");
			return 0;
		}

		if (fat_register_device(dev_desc, 1)!=0) {
			printf ("\n** Unable to use for fatls **\n");
			return 0;
		}

		#if 0
		filelen = file_fat_read("update.zip", &buff, 300*1024*1024);
		printf("update.zip filelen=%d \n", filelen);
		#endif
		
		#if 0
		//checksum
		printf("update.zip checksum =%d \n", checksum(buff, filelen));
		#endif
		
		#if 1
		filelen = do_fat_read("/", NULL, 0, LS_YES);
		#endif
		if (filelen == 100)
		{
			printf("update.zip does  exist %d \n", filelen);
			ret = 1;
		}
		else
		{
			printf("update.zip does not exist \n");
			ret = 0;
		}
	}
	else
	{
		printf("sd3_cd ==1 \n");
		ret = 0;
	}	

	return ret;
	//return 0;
}
#else
/* If not using mxc keypad, currently we will detect power key on board */
int check_key_pressing(void)
{
	return 0;
}
#endif

extern struct reco_envs supported_reco_envs[];

void setup_recovery_env(void)
{
	char *env, *boot_cmd;
	int bootdev = get_boot_device();

	printf("recovery on bootdev: %d\n", bootdev);
	boot_cmd = supported_reco_envs[bootdev].cmd;

	if (boot_cmd == NULL) {
		printf("Unsupported bootup device for recovery: dev: %d\n", bootdev);
		return;
	}

	printf("setup env for recovery..\n");

	env = getenv("bootcmd_android_recovery");
	if (!env)
		setenv("bootcmd_android_recovery", boot_cmd);
	setenv("bootcmd", "run bootcmd_android_recovery");
}

/* export to lib_arm/board.c */
void check_recovery_mode(void)
{
	if (check_key_pressing())
		setup_recovery_env();
	else if (check_recovery_cmd_file()) {
		puts("Recovery command file founded!\n");
		setup_recovery_env();
	}
}
