#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>

#include <linux/hiddev.h>

#define msg__(stream, fmt, ...) fprintf(stream, fmt "\n", ##__VA_ARGS__)
#ifndef NODEBUG
#define DBG(fmt, ...) msg__(stderr, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif
#define MSG(fmt, ...) msg__(stdout, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) msg__(stderr, fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) msg__(stderr, fmt, ##__VA_ARGS__)

#define DEFAULT_VID 0x16C0
#define DEFAULT_PID 0x05DF

static const char devusb_dir_default[] = "/dev/usb";

char *concat_(char const *s1, char const *s2)
{
	void *ptr = 0;
	int len = 0;

	for(;;)
	{
		int r = snprintf(ptr, len, "%s/%s", s1, s2);
		if ( r < len ) break;
		ptr = realloc(ptr, r+1);
		len = r+1;
	}
	return ptr;
}

enum hiddev_attr_fields
{
	HID_DEVNAME_VALID=1<<0,
	HID_BUSNUM_VALID=1<<1,
	HID_DEVNUM_VALID=1<<2,
	HID_VID_VALID=1<<3,
	HID_PID_VALID=1<<4,
};

struct hiddev_attr
{
	unsigned int validity_mask;
	char devname[HID_STRING_SIZE];
	struct hiddev_devinfo device_info;
};

void hiddev_pretty_print(struct hiddev_attr const *attrs)
{
	MSG("Device@Bus %02x@%02x VID/PID %04x/%04x %s",
	    attrs->device_info.busnum, attrs->device_info.devnum,
	    attrs->device_info.vendor, attrs->device_info.product,
	    attrs->devname);
}

int hiddev_devinfo_fd(int fd, struct hiddev_attr *attrs)
{
	attrs->validity_mask = 0;

	if ( -1 == ioctl(fd, HIDIOCGNAME(sizeof(attrs->devname)), &attrs->devname) )
		return -errno;

	attrs->validity_mask |= HID_DEVNAME_VALID;

	if ( -1 == ioctl(fd, HIDIOCGDEVINFO, &attrs->device_info) )
		return -errno;

	attrs->validity_mask |= ( HID_BUSNUM_VALID | HID_DEVNUM_VALID | HID_VID_VALID | HID_PID_VALID );

	return 0;
}

int hiddev_devinfo(char const *name, struct hiddev_attr *attrs)
{
	int fd;
	if ( (fd=open(name, O_RDONLY)) == -1 )
		return -errno;

	int r = hiddev_devinfo_fd(fd, attrs);

	close(fd);
	return r;
}

/*
 * \return <0 if error
 *         =0 no match
 *         >0 match
 */

int hiddev_check(char const *name, struct hiddev_attr const *search_attrs)
{
	struct hiddev_attr device_attrs;
	int err = hiddev_devinfo(name, &device_attrs);
	if ( err < 0 ) goto exit;

	if ( !search_attrs ) /* instant match */
		return 1;

	err = 1;
	if ( search_attrs->validity_mask & HID_DEVNAME_VALID )
	{
		/* TODO */
	}

	if ( search_attrs->validity_mask & HID_BUSNUM_VALID )
		if ( search_attrs->device_info.busnum != device_attrs.device_info.busnum )
		{
			err = 0;
			goto exit;
		}

	if ( search_attrs->validity_mask & HID_DEVNUM_VALID )
		if ( search_attrs->device_info.devnum != device_attrs.device_info.devnum )
		{
			err = 0;
			goto exit;
		}

	if ( search_attrs->validity_mask & HID_VID_VALID )
		if ( search_attrs->device_info.vendor != device_attrs.device_info.vendor )
		{
			err = 0;
			goto exit;
		}

	if ( search_attrs->validity_mask & HID_PID_VALID )
		if ( search_attrs->device_info.product != device_attrs.device_info.product )
		{
			err = 0;
			goto exit;
		}

exit:
	return err;
}

int hiddev_find_device(char const *dirspec, char const **result, struct hiddev_attr const *search_attrs)
{
	int err = -ENOENT;
	struct dirent *entry = 0;

	DIR *handle = opendir(dirspec);
	if ( !handle )
	{
		err = -errno;
		goto exit;
	}

	errno = 0;
	while ( (entry=readdir(handle)) != 0 )
	{
		unsigned char isblk = 0, ischr = 0;
#if defined(_DIRENT_HAVE_D_TYPE) && defined(_BSD_SOURCE)
		if ( entry->d_type != DT_UNKNOWN )
		{
			isblk = !!(entry->d_type == DT_BLK);
			ischr = !!(entry->d_type == DT_CHR);
		}
		else
#endif
		{
			err = -ENOTSUP;
			ERR("Can't determine dirent type");
			goto exit_close;
		}
		DBG("entry '%s' %c%c", entry->d_name, (isblk ? 'B' : ' '), (ischr ? 'C' : ' ') );
		if ( !isblk && !ischr ) continue;

		/* construct a device name and try to initialize */
		char *name_buf = concat_(dirspec, entry->d_name);
		if ( ! name_buf ) continue;

		err = hiddev_check(name_buf, search_attrs);

		DBG("hiddev_check returned %d for %s", err, name_buf);
		if ( err > 0 ) /* match is found */
		{
			*result = name_buf;
			name_buf = 0;
			break;
		}
		if ( err < 0 ) /* error */
		{
			ERR("device check failure %s: %d", name_buf, err);
		}
		/* r==0, go to the next entry */
		free(name_buf);
	}

exit_close:
	closedir(handle);

exit:
	return err;
}

int main(int argc, char **argv)
{
	int err;
	int ch;
	char const *directory = devusb_dir_default;
	char const *device = 0;

	while( (ch=getopt(argc, argv, "d:D:v")) != -1 )
	{
		switch ( ch )
		{
		case 'd':
			device = optarg;
			break;
		case 'D':
			directory = optarg;
			break;
		default:
			break;
		}
	}

	if ( !device )
	{
		if ( !directory || directory[0] == '\0' )
		{
			ERR("invalid directory specification");
			exit(EXIT_FAILURE);
		}

		struct hiddev_attr search_attr;
		search_attr.validity_mask = HID_VID_VALID | HID_PID_VALID;
		search_attr.device_info.vendor = DEFAULT_VID;
		search_attr.device_info.product = DEFAULT_PID;
		err = hiddev_find_device(directory, &device, &search_attr);
		if ( err < 0 )
		{
			ERR("find device failed %d",err);
			exit(EXIT_FAILURE);
		}
		if ( err == 0 )
		{
			ERR("no suitable devices found");
			exit(EXIT_FAILURE);
		}
	}

	/* still no device */
	if ( !device )
	{
		ERR("could not obtain device name (weird)");
		exit(EXIT_FAILURE);
	}

	DBG("probing device %s", device);

	struct hiddev_attr attrs;
	err = hiddev_devinfo(device, &attrs);
	if ( err < 0 )
	{
		ERR("hiddev_devinfo failure %d for device %s", err, device);
		exit(EXIT_FAILURE);
	}
	hiddev_pretty_print(&attrs);

	return 0;
}
