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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "dist.h"
#include "logger.h"
#include "vzerror.h"

static struct distr_conf {
	char *name;
	int id;
} action2id[] = {
	{"ADD_IP", ADD_IP},
	{"DEL_IP", DEL_IP},
	{"SET_HOSTNAME", SET_HOSTNAME},
	{"SET_DNS", SET_DNS},
	{"SET_USERPASS", SET_USERPASS},
	{"SET_UGID_QUOTA", SET_UGID_QUOTA},
	{"POST_CREATE", POST_CREATE}
};

static int get_action_id(char *name)
{
	unsigned int i;

	for (i = 0; i < sizeof(action2id) / sizeof(*action2id); i++)
		if (!strcmp(name, action2id[i].name))
			return action2id[i].id;
	return -1;
}

static int add_dist_action(dist_actions *d_actions, char *name, char *action,
	char *dir)
{
	char file[256];
	int id;

	if (!action[0])
		return 0;
	if ((id = get_action_id(name)) < 0)
		return 0;
	snprintf(file, sizeof(file), "%s/%s/%s", dir, DIST_SCRIPTS, action);
	if (!stat_file(file)) {
		logger(-1, 0, "Action script %s not found", file);
		return 0;
	}
	switch (id) {
		case ADD_IP:
			if (d_actions->add_ip != NULL)
				break;
			d_actions->add_ip = strdup(file);
			break;
		case DEL_IP:
			if (d_actions->del_ip != NULL)
				break;
			d_actions->del_ip = strdup(file);
			break;
		case SET_HOSTNAME:
			if (d_actions->set_hostname != NULL)
				break;
			d_actions->set_hostname = strdup(file);
			break;
		case SET_DNS:
			if (d_actions->set_dns != NULL)
				break;
			d_actions->set_dns = strdup(file);
			break;
		case SET_USERPASS:
			if (d_actions->set_userpass != NULL)
				break;
			d_actions->set_userpass = strdup(file);
			break;
		case SET_UGID_QUOTA:
			if (d_actions->set_ugid_quota != NULL)
				break;
			d_actions->set_ugid_quota = strdup(file);
			break;
		case POST_CREATE:
			if (d_actions->post_create != NULL)
				break;
			d_actions->post_create = strdup(file);
			break;
	}
	return 0;
}

void free_dist_actions(dist_actions *d_actions)
{
	if (d_actions == NULL)
		return;
	if (d_actions->add_ip != NULL)
		free(d_actions->add_ip);
	if (d_actions->del_ip != NULL)
		free(d_actions->del_ip);
	if (d_actions->set_hostname != NULL)
		free(d_actions->set_hostname);
	if (d_actions->set_dns != NULL)
		free(d_actions->set_dns);
	if (d_actions->set_userpass != NULL)
		free(d_actions->set_userpass);
	if (d_actions->set_ugid_quota != NULL)
		free(d_actions->set_ugid_quota);
	if (d_actions->post_create != NULL)
		free(d_actions->post_create);
}

static int get_dist_conf_name(char *dist_name, char *dir, char *file, int len)
{
	char buf[256];
	char *ep;

	if (dist_name != NULL) {
		snprintf(buf, sizeof(buf), "%s", dist_name);
		ep = buf + strlen(buf);
		do {
			snprintf(file, len, "%s/%s.conf", dir, buf);
			if (stat_file(file))
				return 0;
			while (ep > buf && *ep !=  '-') --ep;
			*ep = 0;
		} while (ep > buf);
		snprintf(file, len, "%s/%s", dir, DIST_CONF_DEF);
		logger(-1, 0, "Warning: configuration file "
			"for distribution %s not found, "
			"using defaults from %s/%s",
			dist_name, dir, DIST_CONF_DEF);
	} else {
		snprintf(file, len, "%s/%s", dir, DIST_CONF_DEF);
		logger(-1, 0, "Warning: distribution not specified "
			"in CT config, "
			"using defaults from %s/%s", dir, DIST_CONF_DEF);
	}
	if (!stat_file(file)) {
		logger(-1, 0, "Distribution configuration file "
			       "%s/%s not found", dir, file);
		return VZ_NO_DISTR_CONF;
	}
	return 0;
}

/* Read distribution specific actions configuration file.
 *
 * @param dist_name	distribution name.
 * @param dir		directory distribution config file will be searched.
 * @param dist		filled dist_actions
 * @return		0 on success
 */
int read_dist_actions(char *dist_name, char *dir, dist_actions *actions)
{
	char buf[256];
	char ltoken[256];
	char file[256];
	char *rtoken;
	FILE *fp;
	int ret = 0;

	memset(actions, 0, sizeof(*actions));
	if ((ret = get_dist_conf_name(dist_name, dir, file, sizeof(file))))
		return ret;
	if ((fp = fopen(file, "r")) == NULL) {
		logger(-1, errno, "unable to open %s", file);
		return VZ_NO_DISTR_CONF;
	}
	while (!feof(fp)) {
		buf[0] = 0;
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		if ((rtoken = parse_line(buf, ltoken, sizeof(ltoken))) == NULL)
			continue;
		if ((ret = add_dist_action(actions, ltoken, rtoken, dir))) {
			free_dist_actions(actions);
			break;
		}
	}
	fclose(fp);
	return ret;
}

/** Get distribution name form tmpl_param structure.
 *
 * @param tmpl		distribution data.
 * @return		malloc'ed name.
 */
char *get_dist_name(tmpl_param *tmpl)
{
	const char *p = NULL;

	if (tmpl->dist != NULL)
		p = tmpl->dist;
	else if (tmpl->ostmpl != NULL)
		p = tmpl->ostmpl;
	if (p != NULL)
		return strdup(p);
	return NULL;
}
