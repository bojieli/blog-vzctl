#ifndef _CPT_H_
#define _CPT_H_
#include "types.h"
#define PROC_CPT	"/proc/cpt"
#define PROC_RST	"/proc/rst"

#define CMD_CHKPNT		1
#define CMD_SUSPEND		2
#define CMD_DUMP		3
#define CMD_RESTORE		4
#define CMD_UNDUMP		5

#define CMD_KILL		10
#define CMD_RESUME		11

typedef struct {
	char *dumpdir;
	char *dumpfile;
	unsigned int ctx;
	unsigned int cpu_flags;
	int cmd;
	int rst_fd;
} cpt_param;

struct vps_param;
int cpt_cmd(vps_handler *h, envid_t veid, int cmd, cpt_param *param,
	struct vps_param *vps_p);
int vps_chkpnt(vps_handler *h, envid_t veid, struct vps_param *vps_p, int cmd,
	cpt_param *param);
int vps_restore(vps_handler *h, envid_t veid, struct vps_param *vps_p, int cmd,
	cpt_param *param);
void clean_hardlink_dir(const char *mntdir);

#endif
