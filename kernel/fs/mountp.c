#include <fs/mountp.h>
#include <mm/mm.h>

DECLARE_LIST_HEAD(mp_listhead);

struct mountpoint* mountpoint_find(const char *path)
{
	int max_len = -1;
	struct mountpoint *mp, *res = NULL;

	vfs_for_each_mp(mp) {
		int len = str_match_prefix(path, mp->mountpoint) - 1;
		if(len + 1 == strlen(mp->mountpoint) && len > max_len) {
			max_len = len;
			res = mp;
		}
	}

	return res;
}

void mountpoint_add(struct mountpoint *mp)
{
	struct mountpoint *i;

	if (mp == NULL || mp->fs == NULL || mp->mountpoint == NULL)
	{
		error("invalid mountpoint");
		return;
	}

	vfs_for_each_mp(i) {
		if(!strcmp(mp->mountpoint, i->mountpoint)) {
			error("Mountpoint already exist");
			return;
		}
	}

	list_insert(&mp_listhead, &mp->mp_entry);
	debug("mountpoint %s added", mp->mountpoint);
}

void mountpoint_remove(const char *mountpoint)
{
	struct mountpoint *mp, *next_mp;
    /**
     * TODO: Add recycle logic
     */
	vfs_for_each_mp_safe(mp, next_mp) {
		if(strcmp(mp->mountpoint, mountpoint))
			continue;
		list_remove(&mp->mp_entry);
		kfree((void*)mp->mountpoint);
		kfree(mp);
		return;
	}

	error("mountpoint %s not found", mountpoint);
}