/*
 *  drivers/cpufreq/cpufreq_conservative.c
 *
 *  Copyright (C)  2020 Tong Chen <tongchen126@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/slab.h>
#include "cpufreq_governor.h"

struct ra_policy_dbs_info {
	struct policy_dbs_info policy_dbs;
	unsigned int max_freq, min_freq;
};
static inline struct ra_policy_dbs_info *to_dbs_info(struct policy_dbs_info *policy_dbs)
{
	return container_of(policy_dbs, struct ra_policy_dbs_info, policy_dbs);
}
#define DEF_FREQUENCY_UP_THRESHOLD	(40)
#define DEF_SAMPLING_DOWN_FACTOR	(10)
#define MAX_SAMPLING_DOWN_FACTOR	(1000)
gov_show_one_common(sampling_rate);
gov_show_one_common(sampling_down_factor);
gov_show_one_common(up_threshold);
gov_show_one_common(ignore_nice_load);
gov_attr_ro(sampling_rate);
gov_attr_ro(sampling_down_factor);
gov_attr_ro(up_threshold);
gov_attr_ro(ignore_nice_load);
static struct attribute *ra_attributes[] = {
	&sampling_rate.attr,
	&sampling_down_factor.attr,
	&up_threshold.attr,
	&ignore_nice_load.attr,
	NULL
};
static unsigned int ra_dbs_timer(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct ra_policy_dbs_info *dbs_info = to_dbs_info(policy_dbs);
	struct dbs_data *dbs_data = policy_dbs->dbs_data;
	unsigned int load = dbs_update(policy);
	unsigned int freq;
	//printk("ra_dbs_timer: load %d\n",load);
	if (load < dbs_data->up_threshold)
		freq = dbs_info->min_freq;
	else
		freq = dbs_info->max_freq;
	__cpufreq_driver_target_force(policy, dbs_info->max_freq, CPUFREQ_RELATION_L);
	return dbs_data->sampling_rate;
}
static struct policy_dbs_info *ra_alloc(void)
{
	struct ra_policy_dbs_info *dbs_info;
	dbs_info = kzalloc(sizeof(*dbs_info), GFP_KERNEL);
	return dbs_info ? &(dbs_info->policy_dbs) : NULL;
}
static void ra_free(struct policy_dbs_info *policy_dbs)
{
	kfree(to_dbs_info(policy_dbs));
}
static int ra_init(struct dbs_data *dbs_data)
{
	dbs_data->up_threshold = DEF_FREQUENCY_UP_THRESHOLD;
	dbs_data->sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;
	dbs_data->ignore_nice_load = 0;
	dbs_data->tuners = NULL;
	return 0;
}
static void ra_exit(struct dbs_data *dbs_data)
{
}
static void ra_start(struct cpufreq_policy *policy)
{
	struct ra_policy_dbs_info *dbs_info = to_dbs_info(policy->governor_data);
	dbs_info->max_freq = cpufreq_frequency_table_max(policy);
	dbs_info->min_freq = cpufreq_frequency_table_min(policy);
	printk("ra_start:min freq %u,max freq %u\n",dbs_info->min_freq,dbs_info->max_freq);
}
static struct dbs_governor ra_dbs_gov = {
	.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER("reactive"),
		.kobj_type = { .default_attrs = ra_attributes },
		.gov_dbs_timer = ra_dbs_timer,
		.alloc = ra_alloc,
		.free = ra_free,
		.init = ra_init,
		.exit = ra_exit,
		.start = ra_start,
		.ignore_limits = 1,
};
#define CPU_FREQ_GOV_REACTIVE &(ra_dbs_gov.gov)

static int __init cpufreq_gov_dbs_init(void)
{
	return cpufreq_register_governor(CPU_FREQ_GOV_REACTIVE);
}
static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(CPU_FREQ_GOV_REACTIVE);
}
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_REACTIVE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return CPU_FREQ_GOV_REACTIVE;
}

core_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);

MODULE_AUTHOR("TONG CHEN <tongchen126@gmail.com>");
MODULE_LICENSE("GPL");
