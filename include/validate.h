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
#ifndef _VALIDATE_H_
#define _VALIDATE_H_

#define ACT_NONE	0
#define ACT_WARN	1
#define ACT_ERROR	2
#define ACT_FIX		3

#define UTILIZATION	0
#define COMMITMENT	1

#define PROCUBC		"/proc/user_beancounters"

struct CRusage {
	double low_mem;
	double total_ram;
	double mem_swap;
	double alloc_mem;
	double alloc_mem_lim;
	double alloc_mem_max_lim;
	double cpu;
};

struct ovrc {
	int action;
	float *level_low_mem;
	float *level_total_ram;
	float *level_mem_swap;
	float *level_alloc_mem;
	float *level_alloc_mem_lim;
	float *level_alloc_mem_max_lim;
};

struct mem_struct {
	unsigned long long ram;
	unsigned long long swap;
	unsigned long long lowmem;
};

struct ub_struct;
struct vps_res;

int validate(struct vps_res *param, int recover, int ask);
int calc_ve_utilization(struct ub_struct *param, struct CRusage *rusage,
	struct mem_struct *mem, int numerator);
int calc_ve_commitment(struct ub_struct *param, struct CRusage *rusage,
	struct mem_struct *mem, int numerator);
void add_ubs_limit(struct ub_struct *ubs, int res_id, unsigned long *limit);
void free_ubs_limit(struct ub_struct *ubs);
void shift_ubs_param(struct ub_struct *param);
void inc_rusage(struct CRusage *rusagetotal, struct CRusage *rusage);
#endif
