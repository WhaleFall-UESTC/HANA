#include <fs/mountp.h>
#include <mm/mm.h>

struct mountpoint mount_table[NR_MOUNT];
int mount_count = 0;

struct mountpoint* mountpoint_find(const char *path, int start)
{
	int max_len = -1, res = -1;

	for (int i = start; i < mount_count; i++)
	{
		int len = str_match_prefix(path, mount_table[i].mountpoint) - 1;
		if (len > max_len && len + 1 == strlen(mount_table[i].mountpoint))
		{
			max_len = len;
			res = i;
		}
	}

	return res == -1 ? NULL : &mount_table[res];
}

void mountpoint_add(struct mountpoint *mp)
{
	if (mount_count >= NR_MOUNT)
	{
		error("mount table is full");
		return;
	}

	if (mp == NULL || mp->fs == NULL || mp->mountpoint == NULL)
	{
		error("invalid mountpoint");
		return;
	}

	mount_table[mount_count++] = *mp;
	debug("mountpoint %s added, count: %d", mp->mountpoint, mount_count);
}

void mountpoint_remove(const char *mountpoint)
{
	int i;
    /**
     * TODO: Add recycle logic
     */
	for (i = 0; i < mount_count; i++)
	{
		if (!strcmp(mount_table[i].mountpoint, mountpoint))
		{
			debug("removing mountpoint %s", mount_table[i].mountpoint);
			kfree((void*)mount_table[i].mountpoint);
			mount_table[i] = (struct mountpoint){0};
			return;
		}
	}

	error("mountpoint %s not found", mountpoint);
}