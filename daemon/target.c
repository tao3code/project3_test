#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <robot.h>
#include <target.h>
#include <input_cmd.h>

static pthread_mutex_t mtx_head;
static pthread_mutexattr_t mat_head;

static struct target *target_head = 0;
static struct cylinder_info *info;

struct target *find_target(char *name)
{
	struct target *list = target_head;

	if (!name)
		return target_head;

	pthread_mutex_lock(&mtx_head);
	while (list) {
		if (strcmp(list->name, name)) {
			list = list->next;
			continue;
		}
		pthread_mutex_unlock(&mtx_head);
		return list;
	}
	pthread_mutex_unlock(&mtx_head);
	return 0;
}

int alloc_target(char *name)
{
	struct target *t;
	int name_len;

	if (!name) {
		log_err();
		return -1;
	}

	name_len = strlen(name);

	if (name_len > sizeof(t->name)) {
		log_err();
		return -1;
	}

	t = find_target(name);
	if (t) {
		log_info("target:%s already exist!\n", name);
		return -1;
	}

	t = malloc(sizeof(struct target));
	if (!t) {
		log_system_err(__FUNCTION__);
		goto alloc_mem;
	}

	memset(t, 0, sizeof(struct target));
	memcpy(t->name, name, name_len);

	pthread_mutex_lock(&mtx_head);
	t->next = target_head;
	t->prev = 0;
	if (t->next)
		t->next->prev = t;
	target_head = t;
	pthread_mutex_unlock(&mtx_head);

	return 0;

	free(t);
 alloc_mem:
	return -1;
}

static void clean_records(struct transform_record *record)
{
	struct transform_record *last;
	struct transform_record *p = record;

	while (p) {
		last = p->last;
		free(p);
		p = last;
	};
}

static void clean_target(struct target *tag)
{
	int i;

	for (i = 0; i < NUM_CYLINDERS; i++)
		clean_records(tag->trans[i].record);
}

int free_target(char *name)
{
	struct target *t;

	if (!name) {
		log_err();
		return -1;
	}

	t = find_target(name);
	if (!t) {
		log_info("free target:%s, not found!\n", name);
		return -1;
	}

	pthread_mutex_unlock(&mtx_head);
	if (t->next)
		t->next->prev = t->prev;
	if (t->prev)
		t->prev->next = t->next;
	else
		target_head = t->next;
	pthread_mutex_unlock(&mtx_head);
	clean_target(t);
	free(t);
	return 0;
}

static int new_record(struct transform_info *trans)
{
	struct transform_record *record;

	record = malloc(sizeof(*record));
	if (!record) {
		log_system_err(__FUNCTION__);
		return -1;
	}

	memset(record, 0, sizeof(*record));
	if (trans->record)
		record->last = trans->record;
	trans->record = record;

	return 0;
}

#define TRANS_ACCURACE	20

static int get_meg_val(int diff, struct transform_record *record)
{
	return diff / 20;
}

int try_transform_once(struct target *tag)
{
	int i;
	int err = 0;
	int len_diff;
	char cmd[256];
	int done;

	for (i = 0; i < NUM_CYLINDERS; i++) {
		if (!tag->trans[i].cy.id) {
			err = -1;
			log_err();
			goto trans_err;
		}

		if (!tag->trans[i].cy.len) {
			err = -1;
			log_err();
			goto trans_err;
		}
	}

	for (i = 0; i < NUM_CYLINDERS; i++) {
		len_diff = tag->trans[i].cy.len - info[i].enc.len;
		if (abs(len_diff) < TRANS_ACCURACE)
			continue;
		err = new_record(&tag->trans[i]);
		if (err) {
			log_err();
			break;
		}

		tag->trans[i].record->start_len = info[i].enc.len;
		tag->trans[i].record->meg_val =
		    get_meg_val(len_diff, tag->trans[i].record);
		if (!tag->trans[i].record->meg_val)
			continue;

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "motion set,id=%d,meg=%d", i,
			tag->trans[i].record->meg_val);

		err = run_cmd(cmd);
		if (err) {
			log_err();
			break;
		}
	}

	do {
		usleep(50000);
		done = 1;
		for (i = 0; i < NUM_CYLINDERS; i++) {
			if (!tag->trans[i].record)
				continue;

			if (tag->trans[i].record->stop_len)
				continue;

			if (info[i].meg_dir) {
				done = 0;
				continue;
			}

			tag->trans[i].record->stop_len = info[i].enc.len;
		}
	} while (!done);

	return 0;

 trans_err:
	return err;
}

static int target_init(void)
{
	int err;

	err = pthread_mutexattr_init(&mat_head);
	if (err) {
		log_system_err(__FUNCTION__);
		goto init_mattr;
	}

	err = pthread_mutex_init(&mtx_head, &mat_head);
	if (err) {
		log_system_err(__FUNCTION__);
		goto init_mutex;
	}

	info = get_motion_info();

	return 0;

	pthread_mutex_destroy(&mtx_head);
 init_mutex:
	pthread_mutexattr_destroy(&mat_head);
 init_mattr:
	return err;
}

static void target_exit(void)
{
	struct target *list = target_head;

	while (list) {
		free_target(list->name);
		list = list->next;
	}

	pthread_mutex_destroy(&mtx_head);
	pthread_mutexattr_destroy(&mat_head);
}

init_func(target_init);
exit_func(target_exit);
