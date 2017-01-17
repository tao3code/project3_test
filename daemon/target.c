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

static struct target  *target_head = 0;

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
	free(t);
	return 0;
}


static int target_check(struct target *tag)
{
	int i;

	if (!tag)
		return -1;
	for (i = 0; i < NUM_CYLINDERS; i++) {
		if (!tag->cy[i].id)
			return -1;
		if (!tag->cy[i].len)
			return -1;
	}

	return 0;
}

static unsigned try_transform_once(struct target *tag)
{
	return 0;
}

#define TARGET_DIFF_OK	500

int transform(struct target *tag, unsigned long expire)
{
	unsigned long timeout_ms;
	unsigned diff;

	if (target_check(tag)) {
		log_err();
		return -1;
	}

	timeout_ms = sys_ms + expire;

	while (timeout_ms > sys_ms) {
		diff = try_transform_once(tag);
		if (diff < TARGET_DIFF_OK)
			return 0;
		usleep(50000);
	}

	log_info("%s time out, diff:%u\n", __FUNCTION__, diff);

	return -1;
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

        pthread_mutex_destroy(&mtx_head);
        pthread_mutexattr_destroy(&mat_head);

	while(list) {
		free(list);
		list = list->next;
	}
} 

init_func(target_init);
exit_func(target_exit);
