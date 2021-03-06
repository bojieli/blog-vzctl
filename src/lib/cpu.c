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
#include <unistd.h>
#include <linux/vzcalluser.h>
#include <linux/fairsched.h>
#include <errno.h>

#include "types.h"
#include "cpu.h"
#include "env.h"
#include "vzerror.h"
#include "logger.h"
#include "vzsyscalls.h"

static inline int fairsched_chwt(unsigned int id, unsigned wght)
{
	int ret;

	ret = syscall(__NR_fairsched_chwt, id, wght);
	if (ret && errno == ENOSYS)
		ret = 0;
	return ret;
}

static inline int fairsched_rate(unsigned int id, int op, unsigned rate)
{
	int ret;

	ret = syscall(__NR_fairsched_rate, id, op, rate);
	if (ret && errno == ENOSYS)
		ret = 0;
	return ret;
}

static inline int fairsched_vcpus(unsigned int id, unsigned vcpus)
{
	int ret;

	ret = syscall(__NR_fairsched_vcpus, id, vcpus);
	if (ret && errno == ENOSYS)
		ret = 0;
	return ret;
}

static int set_cpulimit(envid_t veid, unsigned int cpulimit)
{
	unsigned cpulim1024 = (float)cpulimit * 1024 / 100;
	int op = cpulim1024 ? FAIRSCHED_SET_RATE : FAIRSCHED_DROP_RATE;

	logger(0, 0, "Setting CPU limit: %d", cpulimit);
	if (fairsched_rate(veid, op, cpulim1024) < 0) {
		logger(-1, errno, "fairsched_rate");
		return VZ_SETFSHD_ERROR;
	}
	return 0;
}

static int set_cpuweight(envid_t veid, unsigned int cpuweight)
{

	if (fairsched_chwt(veid, cpuweight)) {
		logger(-1, errno, "fairsched_chwt");
		return VZ_SETFSHD_ERROR;
	}
	return 0;
}

static int set_cpuunits(envid_t veid, unsigned int cpuunits)
{
	int cpuweight, ret;

	if (cpuunits < MINCPUUNITS || cpuunits > MAXCPUUNITS) {
		logger(-1, 0, "Invalid value for cpuunits: %d"
			" allowed range is %d-%d",
			cpuunits, MINCPUUNITS, MAXCPUUNITS);
		return VZ_SETFSHD_ERROR;
	}
	cpuweight = MAXCPUUNITS / cpuunits;
	logger(0, 0, "Setting CPU units: %d", cpuunits);
	ret = set_cpuweight(veid, cpuweight);
	return ret;
}

/** Change number of CPUs available in the running CT.
 *
 * @param veid		CT ID
 * @param vcpu		number of CPUs
 */
int env_set_vcpus(envid_t veid, unsigned int vcpus)
{
	int ret;

	logger(0, 0, "Setting CPUs: %d", vcpus);
	ret = fairsched_vcpus(veid, vcpus);
	if (ret)
		logger(-1, errno, "Unable to set cpus");

	return ret;
}


/**  Apply cpu parameters on Host system.
 *
 * @param cpu		cpu parameters.
 * @return		0 on success.
 */
int hn_set_cpu(cpu_param *cpu)
{
	if (cpu->units == NULL)
		return 0;
	return set_cpuunits(2147483647, *cpu->units);
}

/**  Apply cpu parameters on running CT.
 *
 * @param h		CT handler.
 * @param veid		CT ID.
 * @param cpu		cpu parameters.
 * @return		0 on success.
 */
int vps_set_cpu(vps_handler *h, envid_t veid, cpu_param *cpu)
{
	int ret = 0;

	if (cpu->limit == NULL &&
		cpu->units == NULL &&
		cpu->weight == NULL &&
		cpu->vcpus == NULL)
	{
		return 0;
	}
	if (!vps_is_run(h, veid)) {
		logger(-1, 0, "Unable to apply CPU parameters: "
			"container is not running");
		return VZ_VE_NOT_RUNNING;
	}
	if (cpu->limit != NULL) {
		ret = set_cpulimit(veid, *cpu->limit);
	}
	if (cpu->units != NULL) {
		ret = set_cpuunits(veid, *cpu->units);
	} else if (cpu->weight != NULL)
		ret = set_cpuweight(veid, *cpu->weight);
	if (cpu->vcpus != NULL) {
		ret = env_set_vcpus(veid, *cpu->vcpus);
	}

	return ret;
}
