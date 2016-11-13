#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <init.h>
#include <log_project3.h>
#include <robot.h>
#include <target.h>

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
	struct cylinder_info *info;

	if (!name) {
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
	t->name = malloc(strlen(name) + 1);
	if (!t->name) {
		log_system_err(__FUNCTION__);
		goto alloc_name;
	}
	memset(t->name, 0, strlen(name) + 1);
	memcpy(t->name, name, strlen(name));

	info = get_motion_info(&t->num);
	t->info = malloc(sizeof(struct cylinder_info) * t->num);
	if (!t->info) {
		log_system_err(__FUNCTION__);
		goto alloc_info;
	}
	memcpy(t->info, info, sizeof(struct cylinder_info) * t->num);

	pthread_mutex_lock(&mtx_head);
	t->next = target_head;
	t->prev = 0;
	if (t->next)
		t->next->prev = t;
	target_head = t;
	pthread_mutex_unlock(&mtx_head);

	return 0;
	
	free(t->info);
 alloc_info:	
	free(t->name);
 alloc_name:
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
	free(t->name);
	free(t->info);
	free(t);
	return 0;
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
		free(list->name);
		free(list->info);
		free(list);
		list = list->next;
	}
} 

init_func(target_init);
exit_func(target_exit);
