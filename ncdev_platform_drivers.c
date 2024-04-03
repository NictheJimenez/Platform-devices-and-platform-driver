//MODULE_AUTHOR("Nicthe Jimenez");
//MODULE_DESCRIPTION("A pseudo character platform driver which handles n devices with Beaglebone rev C");
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include"platform.h"

#define NUM_DEVICES 10 

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

enum pcdev_names
{
	CDEVPLATFORMA1X,
	CDEVPLATFORMB1X,
	CDEVPLATFORMC1X
};

struct device_config
{
	int config_item1;
	int config_item2;
};

/* Configuration data of the driver for devices */
struct device_config cdev_config[] =
{
	[CDEVPLATFORMA1X] = {.config_item1 = 60, .config_item2 = 21},
	[CDEVPLATFORMB1X] = {.config_item1 = 50, .config_item2 = 22},
	[CDEVPLATFORMC1X] = {.config_item1 = 40, .config_item2 = 23}
};

/* Device private data structure */
struct cdev_private_data
{
	struct cdev_platform_data pdata;
	char *pbuffer;
	dev_t dev_num;
	struct cdev cdev;
};

/* Driver private data structure */
struct cdrv_private_data
{
	int total_devices;
	dev_t dev_num_base;
	struct class *pclass;
	struct device *pdevice;
};

struct cdrv_private_data cdrv_data;
int check_permission(int dev_perm, int acc_mode)
{
	if(dev_perm == DRWR)
		return 0;

	/* Ensures readonly access */
	if((dev_perm == RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;
	if((dev_perm == WDONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
                return 0;
	return -EPERM;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	struct cdev_private_data *pcdev_data = (struct cdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	loff_t temp;
	pr_info("Lseek requested \n");
	pr_info("Current value of the file position = %lld\n",filp->f_pos);
	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + offset;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	pr_info("New value of the file position = %lld\n",filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct cdev_private_data *pcdev_data = (struct cdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	pr_info("Read requested for %zu bytes \n",count);
	pr_info("Current file position = %lld\n",*f_pos);
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;
	/* Copy to user */
	if(copy_to_user(buff,pcdev_data->pbuffer+(*f_pos),count)){
		return -EFAULT;
	}
	/* Update the current file postion */
	*f_pos += count;
	pr_info("Number of bytes successfully read = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);
	/* Return number of bytes which have been successfully read */
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	struct cdev_private_data *pcdev_data = (struct cdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;
	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}
	/* Copy from user */
	if(copy_from_user(pcdev_data->pbuffer+(*f_pos),buff,count)){
		return -EFAULT;
	}
	/* Update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully written = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);
	/* Return number of bytes which have been successfully written */
	return count;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	int ret;
	int minor_n;
	struct cdev_private_data *pcdev_data;
	/* Find out on which device file open was attempted by the user space */
	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n",minor_n);
	/* Get device's private data structure */
	pcdev_data = container_of(inode->i_cdev,struct cdev_private_data,cdev);
	/* To supply device private data to other methods of the driver */
	filp->private_data = pcdev_data;	
	/* Check permission */
	ret = check_permission(pcdev_data->pdata.perm,filp->f_mode);
	(!ret)?pr_info("Open was successful\n"):pr_info("Open was unsuccessful\n");
	return ret;
}

int pcd_release(struct inode *inode, struct file *flip)
{
	pr_info("Release was successful\n");
	return 0;
}

/* File operations of the driver */
struct file_operations pcd_fops=
{
        .open = pcd_open,
        .release = pcd_release,
        .read = pcd_read,
        .write = pcd_write,
        .llseek = pcd_lseek,
        .owner = THIS_MODULE
};

/* Gets called when the device is removed from the system */
int cdev_platform_driver_probe(struct platform_device *pdev)
{
	int ret;
	struct cdev_private_data *dev_data;
	struct cdev_platform_data *pdata;
	pr_info("A device is detected\n");
	/* Get the platform data */
	// pdata = pdev -> dev.platform_data; // linux/device.h I found:
	pdata = (struct cdev_platform_data*)dev_get_platdata(&pdev -> dev);
	if(!pdata)
	{
		pr_info("No platform data available\n");
		return -EINVAL;
	}
	
	/* Dynamically allocate memory for the device private date */
	dev_data = devm_kzalloc(&pdev -> dev,sizeof(struct cdev_private_data),GFP_KERNEL);
	if(!dev_data)
	{
		pr_info("Can not allocate memory \n");
		return -ENOMEM;
	}
	/* Save the device private data pointer in the platform device structure */
	//pdev -> dev.driver_data = dev_data;
	dev_set_drvdata(&pdev -> dev,dev_data);

	dev_data -> pdata.size = pdata -> size;
	dev_data -> pdata.perm = pdata -> perm;
	dev_data -> pdata.serial_number = pdata -> serial_number;

	pr_info("Device serial number = %s\n", dev_data -> pdata.serial_number);
	pr_info("Device size = %d\n", dev_data -> pdata.size);
	pr_info("Device permission = %d\n", dev_data -> pdata.perm);

	pr_info("Config item 1 = %d\n",cdev_config[pdev -> id_entry -> driver_data].config_item1);
	pr_info("Config item 1 = %d\n",cdev_config[pdev -> id_entry -> driver_data].config_item2);
	/* Dynamically allocate memory for the device buffer using size information from the platform data */
	dev_data -> pbuffer = devm_kzalloc(&pdev -> dev,dev_data -> pdata.size,GFP_KERNEL);
        if(!dev_data -> pbuffer)
        {
                pr_info("Can not allocate memory \n");
                return -ENOMEM;
        }
	/* Get the device number */
	dev_data -> dev_num = cdrv_data.dev_num_base + pdev -> id;
	/* Do cdev init and cdev add */
	/* Initialize the cdev structure with fops */
	cdev_init(&dev_data -> cdev,&pcd_fops);
	dev_data -> cdev.owner = THIS_MODULE;
        ret = cdev_add(&dev_data -> cdev,dev_data -> dev_num,1);
                if(ret < 0)
                {
                        pr_err("Cdev add failed\n");
                        return ret;
                }
	pr_info("Device create:");
	/* Create device file for the detected platform device */
	cdrv_data.pdevice = device_create(cdrv_data.pclass,NULL,dev_data -> dev_num,NULL,"cdev_platform-%d",pdev -> id);
		if(IS_ERR(cdrv_data.pdevice)){
			pr_err("Device create failed\n");
			ret = PTR_ERR(cdrv_data.pdevice);
			cdev_del(&dev_data -> cdev);
			return ret;
		}
	pr_info("Probe was successful\n");
	cdrv_data.total_devices++;
	return 0;
}

/* Gets called when matched platform device is foun */
int cdev_platform_driver_remove(struct platform_device *pdev)
{
	struct cdev_private_data *dev_data = dev_get_drvdata(&pdev -> dev);
	/* Remove a device that was created with device_create() */
	device_destroy(cdrv_data.pclass,dev_data -> dev_num);
	/* Remove a cdev entry from the system */
	cdev_del(&dev_data -> cdev);
	
	cdrv_data.total_devices--;

	pr_info("A device is remove\n");
	return 0;
}

struct platform_device_id cdevs_ids[] = {
	[0] = {.name = "cplatform_deviceA1x",.driver_data = CDEVPLATFORMA1X},
	[1] = {.name = "cplatform_deviceB1x",.driver_data = CDEVPLATFORMB1X},
	[2] = {.name = "cplatform_deviceC1x",.driver_data = CDEVPLATFORMC1X},
	{  }	
};

struct platform_driver platform_driver_struct = {
	.probe = cdev_platform_driver_probe,
	.remove = cdev_platform_driver_remove,
	.id_table = cdevs_ids,
	.driver = {
		.name = "char_platform_device" 
	}
};
 
static int __init platform_driver_init(void)
{
	/* Dynamically allocate a device number for NUM_DEVICES */
	int ret;
	pr_info("\n");
	pr_info("MODULE AUTHOR: Nicthe Jimenez\n");
	pr_info("MODULE_DESCRIPTION: A pseudo character platform driver which handles n platform devices with Beaglebone rev C\n");
	pr_info("\n");
	/* Dynamically allocate  device numbers */
	ret = alloc_chrdev_region(&cdrv_data.dev_num_base,0,NUM_DEVICES,"cplatform_devs");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		return 0;;
	}
	/* Create device class under /sys/class/ */
	cdrv_data.pclass = class_create(THIS_MODULE,"cplatform_dev_class");
	if(IS_ERR(cdrv_data.pclass)){
		pr_err("Class creation failed\n");
		unregister_chrdev_region(cdrv_data.dev_num_base, NUM_DEVICES);
		return ret;
	}
	/* Register a platform driver */
	platform_driver_register(&platform_driver_struct);
	pr_info("Char platform driver loaded\n");

	return 0;

}
static void __exit platform_driver_cleanup(void)
{
	/* Unregister the platform driver */
	platform_driver_unregister(&platform_driver_struct);
	/* Class destroy */
	class_destroy(cdrv_data.pclass);
	/* Unregister device number for NUM_DEVICES */
	unregister_chrdev_region(cdrv_data.dev_num_base, NUM_DEVICES);

	pr_info("Char platform driver unloaded\n");
}


module_init(platform_driver_init);
module_exit(platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicthe Jimenez");
MODULE_DESCRIPTION("A pseudo character platform driver which handles n devices with Beaglebone rev C");
