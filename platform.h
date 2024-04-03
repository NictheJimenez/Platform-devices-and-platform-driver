/* Permission codes */
#define DRWR 0x11
#define RDONLY 0x01
#define WDONLY 0x10

struct cdev_platform_data
{
	int size;
	int perm;
	const char *serial_number;
};

