#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <linux/hiddev.h>


/*
 * macros
 */
#define MSG_ERR 0
#define MSG_WARN 1
#define MSG_INFO 2
#define MSG_DEBUG 3

#define msg__(level, stream, fmt, ...) do { if ( (level) <= g_msglevel ) fprintf(stream, fmt "\n", ##__VA_ARGS__); } while (0)
#ifndef NODEBUG
#define DBG(fmt, ...) msg__(MSG_DEBUG, stderr, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif
#define MSG(fmt, ...) msg__(MSG_INFO, stderr, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) msg__(MSG_WARN, stderr, fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) msg__(MSG_ERR, stderr, fmt, ##__VA_ARGS__)

#define DEFAULT_VID 0x16C0
#define DEFAULT_PID 0x05DF
#define DEFAULT_SAMPLE_PERIOD 1

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
static int ARGC_=0;
static char **ARGV_=0;
static int g_msglevel = MSG_INFO;

/*
 * prototypes
 */
char *shift_argv(void);
char *shift_argv_n(unsigned n);
char *concat_(char const *s1, char const *s2);
int hiddev_find_device(char const *dirspec, char const **result, struct hiddev_attr const *search_attrs);
int hiddev_check(char const *name, struct hiddev_attr const *search_attrs);
int hiddev_devinfo(char const *name, struct hiddev_attr *attrs);
int hiddev_devinfo_fd(int fd, struct hiddev_attr *attrs);
int hiddev_init_report(int fd);
int hiddev_get_report(int fd, uint8_t *buf, size_t buf_size);
int hiddev_get_feature_report(int fd, int report_id, unsigned char *buffer, size_t length);
int hiddev_set_feature_report(int fd, int report_id, const unsigned char *buffer, size_t length);

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

void pretty_print_buffer(const uint8_t *buf, size_t size)
{
	size_t ofs = 0;
	while ( ofs < size )
	{
		char str[20];
		char *pstr = str;

		printf("%04zx | ", ofs);
		for(int count = 0; count!=16 && ofs<size; ++count, ++ofs)
		{
			if ( isprint(buf[ofs]) ) *pstr++ = buf[ofs]; else *pstr++ = '.';
			if ( count == 8 ) *pstr++ = ' ';
			printf("%02x ", buf[ofs]);
		}
		*pstr++='\0';

		printf(" | %s\n", str);
	}
}

int do_report_fields(int fd, struct hiddev_report_info const *rinfo)
{
	int err = 0;
	struct hiddev_field_info finfo = {
		.report_type = rinfo->report_type,
		.report_id = rinfo->report_id,
	};
	for(unsigned int i=0; i!=rinfo->num_fields; ++i)
	{
		finfo.field_index = i;
		if ( ioctl(fd, HIDIOCGFIELDINFO, &finfo) !=0 )
		{
			err = -errno;
			ERR("HIDIOCGFIELDINFO failure %d", err);
			goto exit;
		}
		MSG(
"    [%d] maxusage=%x flags=%0x phys=%0x log=%0x app=%0x\n"
"         logmin=%x logmax=%x physmin=%x physmax=%x\n"
"         unit_exp=%x unit=%x",
i, finfo.maxusage, finfo.flags, finfo.physical, finfo.logical, finfo.application,
finfo.logical_minimum, finfo.logical_maximum, finfo.physical_minimum, finfo.physical_maximum,
finfo.unit_exponent, finfo.unit);
	}

exit:
	return err;
}

int do_report_info(int fd, int type)
{
	struct hiddev_report_info info = {
		.report_type = type,
		.report_id = HID_REPORT_ID_FIRST,
	};
	int err = 0;

	MSG("Report %s", type==HID_REPORT_TYPE_INPUT ? "INPUT"
	    : type==HID_REPORT_TYPE_OUTPUT ? "OUTPUT"
	    : type==HID_REPORT_TYPE_FEATURE ? "FEATURE"
	    : "???");

	for(;;)
	{
		if ( ioctl(fd, HIDIOCGREPORTINFO, &info)!=0 && ((err=-errno)!=-EINVAL) )
		{
			ERR("HIDIOCGREPORTINFO failure %d", err);
			goto exit;
		}
		if ( -EINVAL == err )
			break;
		MSG("  id=%d: %d fields", info.report_id, info.num_fields);
		do_report_fields(fd, &info);
		info.report_id |= HID_REPORT_ID_NEXT;
	}
exit:
	return err;
}

int do_command_info(int fd)
{
	do_report_info(fd, HID_REPORT_TYPE_INPUT);
	do_report_info(fd, HID_REPORT_TYPE_OUTPUT);
	do_report_info(fd, HID_REPORT_TYPE_FEATURE);
	return 0;
}

int do_command_getreport(int fd)
{
	uint8_t buf[256];

	int err = hiddev_init_report(fd);
	if ( err < 0 )
	{
		ERR("hiddev_init_report failure: %d", err);
		goto exit;
	}

	err = hiddev_get_report(fd, buf, sizeof(buf));
	if ( err < 0 )
	{
		ERR("hiddev_get_report failure: %d", err);
		goto exit;
	}
	pretty_print_buffer(buf, err);

exit:
	return err;
}

int do_command_monitor(int fd)
{
	int err;
	unsigned int timeout = DEFAULT_SAMPLE_PERIOD;

	int ch;
	while ( (ch=getopt(ARGC_, ARGV_, "t:")) != -1 )
		switch ( ch )
		{
		case 't':
			timeout = strtoul(optarg, 0, 0);
			if ( !timeout ) return -EINVAL;
			break;
		default: return -EINVAL;
		}
	shift_argv_n(optind);

	const int xcmd = !!ARGC_;
	if ( xcmd )
		signal(SIGCHLD, SIG_IGN);

	unsigned int average = 0;
	unsigned int avg_count = 0;
	time_t t_st = 0;
	char *s_output_value = 0;
	size_t z_output_value = 0;
	for(;;)
	{
		/* get sample */
		int32_t value;
		err = hiddev_get_report(fd, (uint8_t*)&value, sizeof(value));
		if ( err < 0 )
		{
			ERR("hiddev_get_report failure: %d", err);
			goto exit;
		}
		average += value;
		avg_count++;

		/* check time */
		const time_t now = time(0);
		if ( now - t_st < timeout ) continue;
		t_st = now;

		/* compute average and produce output string */
		average /= avg_count;
		int n;
	print_again:
		n = snprintf(s_output_value, z_output_value, "_HID_VALUE=%d", average);
		if ( n > 0 && n+1 > z_output_value )
		{
			z_output_value = n + 1;
			s_output_value = realloc(s_output_value, z_output_value);
			goto print_again;
		}

		/* format output value */
		if ( xcmd )
		{
			err = putenv(s_output_value);
			if ( err != 0 )
			{
				err = -errno;
				WARN("putenv error %d", err);
				goto next_cycle;
			}

			pid_t pid = fork();
			if ( pid == -1 )
			{
				err = -errno;
				WARN("fork error %d", err);
				goto next_cycle;
			}

			if ( pid == 0 )
			{
				/* prepare for exec */
				char *args[ARGC_ + 4];
				args[0] = "/bin/sh";
				args[1] = "-c";
				int i;
				for(i=0; i!=ARGC_; ++i) args[2+i] = ARGV_[i];
				args[2+i] = '\0';
				err = execv(args[0], args);
				_exit(1);
			}
		}
		else
			fprintf(stdout, "Light level: %d\n", average);

	next_cycle:
		/* reset for next measurement cycle */
		average = 0;
		avg_count = 0;
	}

exit:
	return err;
}

int do_command_sample(int fd)
{
	int32_t value;
	int err = hiddev_get_report(fd, (uint8_t *)&value, sizeof(value));
	if ( err < 0 )
	{
		ERR("hiddev_get_report failure: %d", err);
		goto exit;
	}
	fprintf(stdout, "%d\n", value);
exit:
	return err;
}

int do_command_feature_get(int fd)
{
	int report_id = 0;
	uint8_t buf[256];

	int ch;
	while ( (ch=getopt(ARGC_, ARGV_, "i:")) != -1 )
	{
		switch ( ch )
		{
		case 'i': report_id = strtoul(optarg, 0, 0); break;
		default: return -EINVAL;
		}
	}

	int err = hiddev_get_feature_report(fd, report_id, buf, sizeof(buf));
	if ( err < 0 )
	{
		ERR("hiddev_get_feature_report failure %d", err);
		return err;
	}

	pretty_print_buffer(buf, err);
	return err;
}

int do_command_feature_set(int fd)
{
	int err = 0;
	int report_id = 0;
	uint8_t buf[256];

	int ch;
	while ( (ch=getopt(ARGC_, ARGV_, "i:")) != -1 )
	{
		switch ( ch )
		{
		case 'i': report_id = strtoul(optarg, 0, 0); break;
		default: return -EINVAL;
		}
	}

	shift_argv_n(optind);
	int num_extra_args;
	for(num_extra_args=0; num_extra_args!=256 && num_extra_args!=ARGC_; ++num_extra_args)
		buf[num_extra_args] = strtoul(ARGV_[num_extra_args],0,0);
	pretty_print_buffer(buf, num_extra_args);

	err = hiddev_set_feature_report(fd, report_id, buf, num_extra_args);
	if ( err < 0 )
	{
		ERR("hiddev_set_feature_report failure %d", err);
		return err;
	}
	return err;
}

int do_command(char const *device)
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
	char *command = ARGV_[0];
	if ( !command )
		command = "getreport";

	if ( ! strcmp(command,"getreport") )
	{
		err = do_command_getreport(fd);
	}
	else if ( ! strcmp(command, "getfeature") )
	{
		err = do_command_feature_get(fd);
	}
	else if ( ! strcmp(command, "setfeature") )
	{
		err = do_command_feature_set(fd);
	}
	else if ( !strcmp(command,"info") )
	{
		err = do_command_info(fd);
	}
	else if ( !strcmp(command, "monitor") )
	{
		err = do_command_monitor(fd);
	}
	else if ( !strcmp(command, "sample") )
	{
		err = do_command_sample(fd);
	}
	else
	{
		ERR("Bad command '%s'", command);
		goto exit_close;
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

	ARGC_ = argc;
	ARGV_ = argv;

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

	char *cmd;
	cmd = shift_argv_n(optind);
	DBG("Running command %s", cmd);
	err = do_command(device);
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
		.report_type = HID_REPORT_TYPE_INPUT,
		.report_id = HID_REPORT_ID_FIRST,
		.num_fields = 2
	};
	
	fd_set rfd;
	struct timeval timeout;

	FD_ZERO(&rfd);
	FD_SET(fd, &rfd);
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000L;
	err = select(fd+1, &rfd, 0, 0, &timeout);
	if ( err < 0 )
	{
		err = -errno;
		goto exit;
	}
	if ( err > 0 && FD_ISSET(fd, &rfd) )
		goto readnow;

	/* timeout, ask the kernel to receive a report */
	if ( ioctl(fd, HIDIOCGREPORT, &rinfo) == -1 )
	{
		err = -errno;
		goto exit;
	}

readnow:
	if ( (err = read(fd, &event, sizeof(event))) == -1 )
	{
		err = -errno;
		goto exit;
	}
#if 0
	DBG("got %d bytes of event data", err);
	DBG("event.hid=%x", event.hid);
	DBG("event.value=%x", event.value);
#endif

#ifndef min
#define min(a,b) ( ((a)<(b))?(a):(b) )
#endif
	err = min(buf_size, sizeof(event.value));
	memcpy(buf, &event.value, err);
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
	for(i=0; i!=report_length; i++)
		buffer[i] = ref_multi_i.values[i];

	return report_length;
}

int hiddev_set_feature_report(int fd, int report_id, const unsigned char *buffer, size_t length)
{
	int err = -1;
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
	DBG("field_index %d", finfo.field_index);
	DBG("report length %d", report_length);
	if ( length > report_length )
	{
		ERR("report length %d - buffer length %zd\n", report_length, length);
		/* buffer too small */
		return -ENOMEM;
	}

	/* multibyte transfer from local buffer */
	for(i=0; i<report_length; i++)
		ref_multi_i.values[i] = buffer[i];
	ref_multi_i.uref.report_type = HID_REPORT_TYPE_FEATURE;
	ref_multi_i.uref.report_id = report_id;
	ref_multi_i.uref.field_index = /*finfo.field_index*/0;
	ref_multi_i.uref.usage_index = 0; /* byte index??? */
//	ref_multi_i.uref.usage_code = 0xff000003;
	ref_multi_i.num_values = length;
	ret = ioctl(fd, HIDIOCSUSAGES, &ref_multi_i);
	if (ret != 0)
	{
		int err = -errno;
		ERR( "HIDIOCSUSAGES (%s)\n", strerror(errno));
		return err;
	}

	/* send feature report */
	rinfo.report_type = HID_REPORT_TYPE_FEATURE;
	rinfo.report_id = report_id;
	rinfo.num_fields = 1;
	ret = ioctl(fd, HIDIOCSREPORT, &rinfo);
	if (ret != 0)
	{
		int err = -errno;
		ERR("HIDIOCSREPORT (%s)\n", strerror(errno));
		return err;
	}
	DBG("report ok");
	err = 0;
	return err;
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

char *shift_argv_n(unsigned n)
{
	if ( ARGC_ < n )
		return 0;
	ARGC_ -= n;
	ARGV_ += n;
	return *ARGV_;
}

char *shift_argv(void)
{
	return shift_argv_n(1);
}
