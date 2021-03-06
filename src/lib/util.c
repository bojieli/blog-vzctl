/*
 *  Copyright (C) 2000-2009, Parallels, Inc. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <limits.h>
#include <dirent.h>

#include "util.h"
#include "logger.h"
#include "fs.h"

#ifndef NR_OPEN
#define NR_OPEN 1024
#endif

char *unescapestr(char *src)
{
	char *p1, *p2;
	int fl;

	if (src == NULL)
		return NULL;
	p2 = src;
	p1 = p2;
	fl = 0;
	while (*p2) {
		if (*p2 == '\\' && !fl)	{
			fl = 1;
			p2++;
		} else {
			*p1 = *p2;
			p1++; p2++;
			fl = 0;
		}
	}
	*p1 = 0;

	return src;
}

char *parse_line(char *str, char *ltoken, int lsz)
{
	char *sp = str;
	char *ep, *p;
	int len;

	unescapestr(str);
	while (*sp && isspace(*sp)) sp++;
	if (!*sp || *sp == '#')
		return NULL;
	ep = sp + strlen(sp) - 1;
	while (isspace(*ep) && ep >= sp) *ep-- = '\0';
	if (*ep == '"')
		*ep = 0;
	if (!(p = strchr(sp, '=')))
		return NULL;
	len = p - sp;
	if (len >= lsz)
		return NULL;
	strncpy(ltoken, sp, len);
	ltoken[len] = 0;
	if (*(++p) == '"')
		p++;

	return p;
}

/*
	1 - exist
	0 - does't exist
	-1 - error
*/
int stat_file(const char *file)
{
	struct stat st;

	if (stat(file, &st)) {
		if (errno != ENOENT)
			return -1;
		return 0;
	}
	return 1;
}

int make_dir(char *path, int full)
{
	char buf[4096];
	char *ps, *p;
	int len;

	if (path == NULL)
		return 0;

	ps = path + 1;
	while ((p = strchr(ps, '/'))) {
		len = p - path + 1;
		snprintf(buf, len, "%s", path);
		ps = p + 1;
		if (!stat_file(buf)) {
			if (mkdir(buf, 0755)) {
				logger(-1, errno, "Can't create directory %s",
					buf);
				return 1;
			}
		}
	}
	if (!full)
		return 0;
	if (!stat_file(path)) {
		if (mkdir(path, 0755)) {
			logger(-1, errno, "Can't create directory %s", path);
			return 1;
		}
	}
	return 0;
}

int parse_int(const char *str, int *val)
{
	char *tail;

	errno = 0;
	*val = (int)strtol(str, (char **)&tail, 10);
	if (*tail != '\0' || errno == ERANGE)
		return 1;
	return 0;
}

int parse_ul(const char *str, unsigned long *val)
{
	char *tail;

	if (!strcmp(str, "unlimited")) {
		*val = LONG_MAX;
		return 0;
	}

	errno = 0;
	*val = (int)strtoul(str, (char **)&tail, 10);
	if (*tail != '\0' || errno == ERANGE)
		return ERR_INVAL;
	return 0;
}

void str_tolower(const char *from, char *to)
{
	if (from == NULL || to == NULL)
		return;
	while ((*to++ = tolower(*from++)));
}

void str_toupper(const char *from, char *to)
{
	if (from == NULL || to == NULL)
		return;
	while ((*to++ = toupper(*from++)));
}

int check_var(const void *val, const char *message)
{
	if (val != NULL)
		return 0;
	logger(-1, 0, "%s", message);

	return 1;
}

int cp_file(char *dst, char *src)
{
	int fd_src, fd_dst, len, ret = 0;
	struct stat st;
	char buf[4096];

	if (stat(src, &st) < 0) {
		logger(-1, errno, "Unable to stat %s", src);
		return -1;
	}
	len = st.st_size;
	if ((fd_src = open(src, O_RDONLY)) < 0) {
		logger(-1, errno, "Unable to open %s", src);
		return -1;
	}
	if ((fd_dst = open(dst, O_CREAT|O_RDWR, st.st_mode)) < 0) {
		logger(-1, errno, "Unable to open %s", dst);
		close(fd_src);
		return -1;
	}
	while(1) {
		ret = read(fd_src, buf, sizeof(buf));
		if (!ret)
			break;
		else if (ret < 0) {
			logger(-1, errno, "Unable to read from %s", src);
			ret = -1;
			break;
		}
		if (write(fd_dst, buf, ret) < 0) {
			logger(-1, errno, "Unable to write to %s", dst);
			ret = -1;
			break;
		}
	}
	close(fd_src);
	close(fd_dst);

	return ret;
}

void get_vps_conf_path(envid_t veid, char *buf, int len)
{
	snprintf(buf, len, VPS_CONF_DIR "%d.conf", veid);
}

char *arg2str(char **arg)
{
	char **p;
	char *str, *sp;
	int len = 0;

	if (arg == NULL)
		return NULL;
	p = arg;
	while (*p)
		len += strlen(*p++) + 1;
	if ((str = (char *)malloc(len + 1)) == NULL)
		return NULL;
	p = arg;
	sp = str;
	while (*p)
		sp += sprintf(sp, "%s ", *p++);

	return str;
}

void free_arg(char **arg)
{
	while (*arg) free(*arg++);
}

inline double max(double val1, double val2)
{
	return (val1 > val2) ? val1 : val2;
}

inline unsigned long max_ul(unsigned long val1, unsigned long val2)
{
	return (val1 > val2) ? val1 : val2;
}

inline unsigned long min_ul(unsigned long val1, unsigned long val2)
{
	return (val1 < val2) ? val1 : val2;
}

int yesno2id(const char *str)
{
	if (!strcmp(str, "yes"))
		return YES;
	else if (!strcmp(str, "no"))
		 return NO;
	return -1;
}

int get_netaddr(const char *ip_str, unsigned int *ip)
{
	if (strchr(ip_str, ':')) {
		if (inet_pton(AF_INET6, ip_str, ip) <= 0)
			return -1;
		return AF_INET6;
	}
	if (inet_pton(AF_INET, ip_str, ip) <= 0)
		return -1;
	return AF_INET;
}

const char *get_netname(unsigned int *ip, int family)
{
	static char buf[64];

	return inet_ntop(family, ip, buf, sizeof(buf) - 1);
}

char *subst_VEID(envid_t veid, char *src)
{
	char *srcp;
	char str[STR_SIZE];
	char *sp, *se;
	int r;
	unsigned int len, veidlen;

	if (src == NULL)
		return NULL;
	/* Skip end '/' */
	se = src + strlen(src) - 1;
	while (se != str && *se == '/') {
		*se = 0;
		se--;
	}
	if ((srcp = strstr(src, "$VEID")))
		veidlen = sizeof("$VEID") - 1;
	else if ((srcp = strstr(src, "${VEID}")))
		veidlen = sizeof("${VEID}") - 1;
	else
		return strdup(src);

	sp = str;
	se = str + sizeof(str);
	len = srcp - src; /* Length of src before $VEID */
	if (len > sizeof(str))
		return NULL;
	memcpy(str, src, len);
	sp += len;
	r = snprintf(sp, se - sp, "%d", veid);
	sp += r;
	if ((r < 0) || (sp >= se))
		return NULL;
	if (*srcp) {
		r = snprintf(sp, se - sp, "%s", srcp + veidlen);
		sp += r;
		if ((r < 0) || (sp >= se))
			return NULL;
	}
	return strdup(str);
}

int get_pagesize()
{
	long pagesize;

	if ((pagesize = sysconf(_SC_PAGESIZE)) == -1) {
		logger(-1, errno, "Unable to get page size");
		return -1;
	}
	return pagesize;
}

int get_mem(unsigned long long *mem)
{
	long pages, pagesize;
	if ((pages = sysconf(_SC_PHYS_PAGES)) == -1) {
		logger(-1, errno, "Unable to get total phys pages");
		return -1;
	}
	if ((pagesize = get_pagesize()) < 0)
		return -1;
	*mem = pages * pagesize;
	return 0;
}

int get_thrmax(int *thrmax)
{
	FILE *fd;
	char str[128];

	if (thrmax == NULL)
		return 1;
	if ((fd = fopen(PROCTHR, "r")) == NULL) {
		logger(-1, errno, "Unable to open " PROCTHR);
		return 1;
	}
	if (fgets(str, sizeof(str), fd) == NULL) {
		fclose(fd);
		return 1;
	}
	fclose(fd);
	if (sscanf(str, "%du", thrmax) != 1)
		return 1;
	return 0;
}

int get_swap(unsigned long long *swap)
{
	FILE *fd;
	char str[128];

	if ((fd = fopen(PROCMEM, "r")) == NULL)	{
		logger(-1, errno, "Cannot open " PROCMEM);
		return -1;
	}
	while (fgets(str, sizeof(str), fd)) {
		if (sscanf(str, "SwapTotal: %llu", swap) == 1) {
			*swap *= 1024;
			fclose(fd);
			return 0;
		}
	}
	logger(-1, errno, "Swap: is not found in " PROCMEM );
	fclose(fd);

	return -1;
}

int get_num_cpu()
{
	FILE *fd;
	char str[128];
	int ncpu = 0;

	if ((fd = fopen("/proc/cpuinfo", "r")) == NULL)	{
		logger(-1, errno, "Cannot open /proc/cpuinfo");
		return 1;
	}
	while (fgets(str, sizeof(str), fd)) {
		if (!strncmp(str, "processor", 9))
			ncpu++;
	}
	fclose(fd);
	return !ncpu ? 1 : ncpu;
}

int get_lowmem(unsigned long long *mem)
{
	FILE *fd;
	char str[128];

	if ((fd = fopen(PROCMEM, "r")) == NULL)	{
		logger(-1, errno, "Cannot open " PROCMEM);
		return -1;
	}
	*mem = 0;
	while (fgets(str, sizeof(str), fd)) {
		if (sscanf(str, "LowTotal: %llu", mem) == 1)
			break;
		/* Use MemTotal in case LowTotal not found */
		sscanf(str, "MemTotal: %llu", mem);
	}
	fclose(fd);
	if (*mem == 0) {
		fprintf(stderr, "Neither LowTotal nor MemTotal found in the "
				PROCMEM "\n");
		return -1;
	}
	*mem *= 1024;
	return 0;
}

char *get_file_name(char *str)
{
	char *p;
	int len;

	len = strlen(str) - sizeof(".conf") + 1;
	if (len <= 0)
	return NULL;
	if (strcmp(str + len, ".conf"))
		return NULL;
	if ((p = malloc(len + 1)) == NULL)
		return NULL;
	strncpy(p, str, len);
	p[len] = 0;

	return p;
}

const char *get_vps_state_str(int vps_state)
{
	const char *p = NULL;

	switch (vps_state) {
	case STATE_RUNNING:
		p = "running";
		break;
	case STATE_STARTING:
		p = "starting";
		break;
	case STATE_STOPPED:
		p = "stopped";
		break;
	case STATE_STOPPING:
		p = "stopping";
		break;
	case STATE_RESTORING:
		p = "restoring";
		break;
	case STATE_CHECKPOINTING:
		p = "checkpointing";
		break;
	}
	return p;
}

int get_dump_file(unsigned veid, const char *dumpdir, char *buf, int size)
{
	return snprintf(buf, size, "%s/" DEF_DUMPFILE,
			dumpdir != NULL ? dumpdir : DEF_DUMPDIR, veid);
}

int set_not_blk(int fd)
{
	int oldfl, ret;

	if ((oldfl = fcntl(fd, F_GETFL)) == -1)
		return -1;
	oldfl |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, oldfl);

	return ret;
}

/** Close all fd.
 * @param close_std	flag for closing the [0-2] fds
 * @param ...		list of fds are skiped, (-1 is the end mark)
*/
void close_fds(int close_std, ...)
{
	int fd, max;
	unsigned int i;
	va_list ap;
	int skip_fds[255];

	max = sysconf(_SC_OPEN_MAX);
	if (max < NR_OPEN)
		max = NR_OPEN;
	if (close_std) {
		fd = open("/dev/null", O_RDWR);
		if (fd != -1) {
			dup2(fd, 0); dup2(fd, 1); dup2(fd, 2);
		} else {
			close(0); close(1); close(2);
		}
	}
	/* build aray of skiped fds */
	va_start(ap, close_std);
	skip_fds[0] = -1;
	for (i = 0; i < (sizeof(skip_fds)/sizeof(skip_fds[0])); i++) {
		fd = va_arg(ap, int);
		skip_fds[i] = fd;
		if (fd == -1)
			break;
	}
	va_end(ap);
	for (fd = 3; fd < max; fd++) {
		for (i = 0; skip_fds[i] != fd && skip_fds[i] != -1; i++);
		if (skip_fds[i] == fd)
			continue;
		close(fd);
	}
}

/* Renames (to "*.destroyed" if action == BACKUP) or removes config,
 * (if action == DESTR)
 * Also, appropriate mount/umount scripts are linked.
 */
int move_config(int veid, int action)
{
	char conf[PATH_LEN];
	char newconf[PATH_LEN];

	snprintf(conf, sizeof(conf), VPS_CONF_DIR "%d.conf", veid);
	snprintf(newconf, sizeof(newconf), "%s." DESTR_PREFIX, conf);
	action == BACKUP ? rename(conf, newconf) : unlink(newconf);

	snprintf(conf, sizeof(conf), VPS_CONF_DIR "%d." MOUNT_PREFIX, veid);
	snprintf(newconf, sizeof(newconf), "%s." DESTR_PREFIX, conf);
	action == BACKUP ? rename(conf, newconf) : unlink(newconf);

	snprintf(conf, sizeof(conf), VPS_CONF_DIR "%d." UMOUNT_PREFIX, veid);
	snprintf(newconf, sizeof(newconf), "%s." DESTR_PREFIX, conf);
	action == BACKUP ? rename(conf, newconf) : unlink(newconf);

	snprintf(conf, sizeof(conf), VPS_CONF_DIR "%d." START_PREFIX, veid);
	snprintf(newconf, sizeof(newconf), "%s." DESTR_PREFIX, conf);
	action == BACKUP ? rename(conf, newconf) : unlink(newconf);

	snprintf(conf, sizeof(conf), VPS_CONF_DIR "%d." STOP_PREFIX, veid);
	snprintf(newconf, sizeof(newconf), "%s." DESTR_PREFIX, conf);
	action == BACKUP ? rename(conf, newconf) : unlink(newconf);

	return 0;
}

void remove_names(envid_t veid)
{
	char buf[STR_SIZE];
	char content[STR_SIZE];
	struct stat st;
	struct dirent *ep;
	DIR *dp;
	char *p;
	int r;
	envid_t id;

	if (!(dp = opendir(VENAME_DIR)))
		return;
	while ((ep = readdir(dp))) {
		snprintf(buf, sizeof(buf), VENAME_DIR "/%s", ep->d_name);
		if (lstat(buf, &st))
			continue;
		if (!S_ISLNK(st.st_mode))
			continue;
		r = readlink(buf, content, sizeof(content) - 1);
		if (r < 0)
			continue;
		content[r] = 0;
		if ((p = strrchr(content, '/')) != NULL)
			p++;
		if (sscanf(p, "%d.conf", &id) == 1 && veid == id)
			unlink(buf);
	}
	closedir(dp);
}

size_t vz_strlcat(char *dst, const char *src, size_t count)
{
	size_t dsize = strlen(dst);
	size_t len = strlen(src);
	size_t res = dsize + len;

	if (dsize >= count)
		return dsize;

	dst += dsize;
	count -= dsize;
	if (len >= count)
		len = count - 1;

	memcpy(dst, src, len);
	dst[len] = 0;

	return res;
}
