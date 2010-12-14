#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>

#include <linux/hiddev.h>


/*
 * macros
 */
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


/*
 * structures
 */
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


/*
 * globals
 */
static const char devusb_dir_default[] = "/dev/usb";


/*
 * prototypes
 */
char *concat_(char const *s1, char const *s2);
int hiddev_find_device(char const *dirspec, char const **result, struct hiddev_attr const *search_attrs);
int hiddev_check(char const *name, struct hiddev_attr const *search_attrs);
int hiddev_devinfo(char const *name, struct hiddev_attr *attrs);
int hiddev_devinfo_fd(int fd, struct hiddev_attr *attrs);
int hiddev_init_report(int fd);
int hiddev_get_report(int fd, uint8_t *buf, size_t buf_size);
int hiddev_get_feature_report(int fd, int report_id, unsigned char *buffer, size_t length);

/*
 * basic code
 */
void pretty_print_hid_attrs(struct hiddev_attr const *attrs, char const *msg_prefix)
{
	MSG("%sDevice@Bus %02x@%02x VID/PID %04x/%04x %s",
	    msg_prefix ? msg_prefix : "",
	    attrs->device_info.busnum, attrs->device_info.devnum,
	    attrs->device_info.vendor, attrs->device_info.product,
	    attrs->devname);
}

void pretty_print_buffer(uint8_t *buf, size_t size)
{
	size_t ofs = 0;
	while ( ofs < size )
	{
		printf("%04zx | ", ofs);
		for(int count = 0; count!=16; ++count)
			printf("%02x ", buf[ofs++]);
		printf("\n");
	}
}

int do_command(char const *device, char const *command)
{
	int err;
	int fd = -1;
	if ( (fd=open(device, O_RDONLY)) == -1 )
	{
		err = -errno;
		ERR("failed to open device %s: %d", device, err);
		goto exit;
	}

	struct hiddev_attr attrs;
	err = hiddev_devinfo_fd(fd, &attrs);
	if ( err < 0 )
	{
		ERR("hiddev_devinfo failure %d for device %s", err, device);
		goto exit_close;
	}

	/* ok, this is a working HID device */
	MSG("Found a device:");
	pretty_print_hid_attrs(&attrs,"\t");

	/* default command */
	if ( !command )
		command = "getreport";

	if ( ! strcmp(command,"getreport") )
	{
		uint8_t buf[256];

		err = hiddev_init_report(fd);
		if ( err < 0 )
		{
			ERR("hiddev_init_report failure: %d", err);
			goto exit_close;
		}

		//err = hiddev_get_report(fd, buf, sizeof(buf));
		err = hiddev_get_feature_report(fd, 0, buf, sizeof(buf));
		if ( err < 0 )
		{
			ERR("hiddev_get_report failure: %d", err);
			goto exit_close;
		}
		pretty_print_buffer(buf, err);
	}
	else if ( ! strcmp(command,"setreport") )
	{
	}
	
exit_close:
	close(fd);

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

	err = do_command(device, argv[optind]);
	if ( err < 0 )
		exit(EXIT_FAILURE);
	return 0;
}


/*
 *
 * HID device functions
 *
 */


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

int hiddev_init_report(int fd)
{
	int err = 0;

	if ( ioctl(fd, HIDIOCINITREPORT, 0) == -1 )
	{
		err = -errno;
		goto exit;
	}

exit:
	return err;
}

// http://google.com/codesearch/p?hl=ru#NFsuUs6GhVY/src/hiddev.c&q=HID_REPORT_TYPE_FEATURE&sa=N&cd=60&ct=rc
int hiddev_get_report(int fd, uint8_t *buf, size_t buf_size)
{
	int err;
	struct hiddev_event event;
	struct hiddev_report_info rinfo = {
		.report_type = HID_REPORT_TYPE_FEATURE,
		.report_id = HID_REPORT_ID_FIRST,
		.num_fields = 2
	};
	
	if ( ioctl(fd, HIDIOCGREPORT, &rinfo) == -1
	     || (MSG("ioctl ok"),0)
	     || (err = read(fd, &event, sizeof(event))) == -1 )
	{
		err = -errno;
		goto exit;
	}
	
	DBG("got %d bytes of event data", err);

exit:
	return err;
}

int hiddev_get_feature_report(int fd, int report_id, unsigned char *buffer, size_t length)
{
	struct hiddev_report_info rinfo;
	struct hiddev_field_info finfo;
	struct hiddev_usage_ref_multi ref_multi_i;
	int ret, report_length, i;

	/* request info about the feature report (get byte length) */
	finfo.report_type = HID_REPORT_TYPE_FEATURE;
	finfo.report_id = report_id;
	finfo.field_index = 0;
	ret = ioctl(fd, HIDIOCGFIELDINFO, &finfo);
	report_length = finfo.maxusage;
	if (report_length > length)
	{
		ERR("report length %d - buffen length %zd\n", report_length, length);
		/* buffer too small */
		return -ENOMEM;
	}
	DBG("report length %d", report_length);


	/* request feature report */
	rinfo.report_type = HID_REPORT_TYPE_FEATURE;
	rinfo.report_id = report_id;
	rinfo.num_fields = 1;
	ret = ioctl(fd, HIDIOCGREPORT, &rinfo);
	if (ret != 0)
	{
		int err = -errno;
		ERR("HIDIOCGREPORT (%s)\n", strerror(errno));
		return err;
	}

	/* multibyte transfer to local buffer */
	ref_multi_i.uref.report_type = HID_REPORT_TYPE_FEATURE;
	ref_multi_i.uref.report_id = report_id;
	ref_multi_i.uref.field_index = 0;
	ref_multi_i.uref.usage_index = 0; /* byte index??? */
	ref_multi_i.num_values = report_length;
	ret = ioctl(fd, HIDIOCGUSAGES, &ref_multi_i);
	if (ret != 0)
	{
		int err = -errno;
		ERR( "HIDIOCGUSAGES (%s)\n", strerror(errno));
		return err;
	}

	/* copy bytes to output buffer (missing the first byte - report id)
	 * can't use memcpy, because values are signed int32
	 */
	for(i=0; i!=report_length; ++i)
		DBG("report[%d] = %04x", i, ref_multi_i.values[i]);
//	for(i=0; i<report_length-1; i++)
//		buffer[i] = ref_multi_i.values[i];

	return report_length;
}

/*
 *
 * Utility functions
 *
 */
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
