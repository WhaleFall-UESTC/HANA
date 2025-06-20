#include <init.h>
#include <io/blk.h>
#ifdef ARCH_RISCV
#include <drivers/virtio-mmio.h>
#else
#include <drivers/virtio-pci.h>
#endif
#include <common.h>
#include <debug.h>
#include <klib.h>
#include <arch.h>
#include <mm/mm.h>

#define TEST_CYCLES 10
#define BLOCK_SIZE 4096
#define BLOCKS_PER_TEST 10
#define TEST_DATA_SIZE (BLOCK_SIZE * BLOCKS_PER_TEST)

static uint32 rand_state = 1;

/* Random number generator implementation */
void srand(uint32 seed)
{
    rand_state = seed ? seed : 1;
}

uint32 krand(void)
{
    rand_state = (uint32)((uint64)rand_state * 279470273UL) % 4294967291UL;
    return rand_state;
}

void init_random()
{
    uint32 seed;
#ifdef ARCH_RISCV
    __asm__ __volatile__("csrr %0, cycle" : "=r"(seed));
#else
    uint32 tmp = 0;
    __asm__ __volatile__("rdtimel.w %0, %1" : "=r"(seed) : "r"(tmp));
#endif
    srand(seed);
}

void test_virtio()
{
    char *write_buf = NULL;
    char *read_buf = NULL;
    struct blkdev *blkdev = NULL;
    uint64 nr_sectors;
    uint64 sectors[BLOCKS_PER_TEST];

    init_random();

    log("Starting enhanced virtio block device test...");

    if (!(write_buf = kalloc(TEST_DATA_SIZE)) || !(read_buf = kalloc(TEST_DATA_SIZE)))
    {
        error("Memory allocation failed");
        goto cleanup;
    }

    if (!(blkdev = blkdev_get_default_dev()))
    {
        error("Device not found");
        goto cleanup;
    }

    log("using blkdev: %s", blkdev->dev.name);

    nr_sectors = blkdev->size / BLOCK_SIZE;

    for (int cycle = 0; cycle < TEST_CYCLES; cycle++)
    {
        // Generate 10 unique random sectors
        for (int i = 0; i < BLOCKS_PER_TEST; i++)
        {
            sectors[i] = krand() % nr_sectors;
            log("Cycle %d: Sector %d - %lu", cycle, i, sectors[i]);
        }

        // Generate unique random data for each sector
        for (int i = 0; i < TEST_DATA_SIZE; i++)
        {
            write_buf[i] = (char)(krand() & 0xFF);
        }

        /*---------- Random Write Phase ----------*/
        for (int i = 0; i < BLOCKS_PER_TEST; i++)
        {
            struct blkreq *req = blkreq_alloc(blkdev, sectors[i],
                                              write_buf + i * BLOCK_SIZE, BLOCK_SIZE, 1);

            if (!req)
            {
                error("Write req failed: cycle %d sector %lu", cycle, sectors[i]);
                goto cleanup;
            }
            blkdev_submit_req(blkdev, req);
        }

        if (blkdev_wait_all(blkdev) > 0)
        {
            error("Write errors in cycle %d", cycle);
            blkdev_free_all(blkdev);
            goto cleanup;
        }
        blkdev_free_all(blkdev);

        /*---------- Random Read & Verify Phase ----------*/
        memset(read_buf, 0, TEST_DATA_SIZE);

        for (int i = 0; i < BLOCKS_PER_TEST; i++)
        {
            struct blkreq *req = blkreq_alloc(blkdev, sectors[i],
                                              read_buf + i * BLOCK_SIZE, BLOCK_SIZE, 0);

            if (!req)
            {
                error("Read req failed: cycle %d sector %lu", cycle, sectors[i]);
                goto cleanup;
            }
            blkdev_submit_req(blkdev, req);
        }

        if (blkdev_wait_all(blkdev) > 0)
        {
            error("Read errors in cycle %d", cycle);
            blkdev_free_all(blkdev);
            goto cleanup;
        }
        blkdev_free_all(blkdev);

        // Verify all sectors
        if (memcmp(write_buf, read_buf, TEST_DATA_SIZE) != 0)
        {
            error("Data mismatch in cycle %d", cycle);
            goto cleanup;
        }
    }

    PASS("Completed %d cycles with random sector access", TEST_CYCLES);

cleanup:
    if (write_buf)
        kfree(write_buf);
    if (read_buf)
        kfree(read_buf);
}
