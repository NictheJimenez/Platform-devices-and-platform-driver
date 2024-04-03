//MODULE_AUTHOR("Nicthe Jimenez");
//MODULE_DESCRIPTION("A pseudo character platform driver which handles n devices with Beaglebone rev C");
#include<linux/module.h>
#include<linux/platform_device.h>
#include"platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

/* Create 2 platform data */
struct cdev_platform_data cdev_pdata[] = {
	[0] = {.size = 512,  .perm = DRWR,   .serial_number = "CDEVPLATF01"},
	[1] = {.size = 1024, .perm = RDONLY, .serial_number = "CDEVPLATF02"},
	[2] = {.size = 128,  .perm = WDONLY, .serial_number = "CDEVPLATF03"}
};

/* Release API */
void cdev_release(struct device *dev)
{
	pr_info("Device release\n");
}

/* Create n platform devices */

struct platform_device  platform_cdev1 = {
	.name = "cplatform_deviceA1x",
	.id = 0,
	.dev = { 
		.platform_data = &cdev_pdata[0],
		.release = cdev_release
	}
        	
};

struct platform_device  platform_cdev2 = {
	.name = "cplatform_deviceB1x",
	.id = 1,
	.dev = {
		.platform_data = &cdev_pdata[1],
		.release = cdev_release
	}	
};

struct platform_device  platform_cdev3 = {
        .name = "cplatform_deviceC1x",
        .id = 2,
        .dev = {
                .platform_data = &cdev_pdata[2],
                .release = cdev_release
        }
};

struct platform_device *pplatform_cdevs[] = {
	&platform_cdev1,
	&platform_cdev2,
	&platform_cdev3
};

static int __init ncdev_platform_init(void)
{
	/* Register platform device*/
	/*
	platform_device_register(&platform_cdev1);
	platform_device_register(&platform_cdev2);
	*/
	platform_add_devices(pplatform_cdevs,ARRAY_SIZE(pplatform_cdevs));
	pr_info("Device setup module loaded/n");
	return 0;
}

static void __exit ncdev_platform_cleanup(void)
{
	/* Unregister platform device*/
	platform_device_unregister(&platform_cdev1);
	platform_device_unregister(&platform_cdev2);
	platform_device_unregister(&platform_cdev3);
	pr_info("Device setup module unloaded\n");
}

module_init(ncdev_platform_init);
module_exit(ncdev_platform_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module which n platform devices");
MODULE_AUTHOR("Nicthe Jimenez");
