/*_
 * Copyright (c) 2018 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "fm10k.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

/* 64 MiB */
#define FM10K_BAR4_SIZE 0x4000000

/*
 * FM10K management structure
 */
typedef struct _fm10k {
    /* /dev/uioX */
    int fd;
    /* Memory mapped to /dev/uioX (BAR4) */
    void *mmio;
} fm10k_t;

/*
 * Read/Write
 */
static __inline__ uint32_t
rd32(void *mmio, long offset)
{
    return *((uint32_t *)(mmio + offset));
}
static __inline__ uint64_t
rd64(void *mmio, long offset)
{
    return *((uint64_t *)(mmio + offset));
}
static __inline__ void
wr32(void *mmio, long offset, uint32_t val)
{
    *((uint32_t *)(mmio + offset)) = val;
}
static __inline__ void
wr64(void *mmio, long offset, uint64_t val)
{
    *((uint64_t *)(mmio + offset)) = val;
}


/*
 * Usage
 */
void
usage(const char *prog)
{
    fprintf(stderr, "Usage: %s /dev/<uioX>\n", prog);
    exit(EXIT_FAILURE);
}

/*
 * Initialize FM10K scheduler
 */
int
init_scheduler(fm10k_t *fm10k)
{
    struct timespec ts;
    uint32_t mode0;
    uint32_t mode1;
    uint32_t run0;
    uint32_t run1;
    uint64_t m64;
    uint32_t m32;
    int i;

    /* Clear BIST-accessible switch memories */
    mode0 = rd32(fm10k->mmio, FM10K_BIST_CTRL_MODE);
    mode1 = mode0 | (1ULL << 10);
    wr64(fm10k->mmio, FM10K_BIST_CTRL_MODE, mode1);
    run0 = rd32(fm10k->mmio, FM10K_BIST_CTRL_RUN);
    run1 = run0 | (1ULL << 10);
    wr64(fm10k->mmio, FM10K_BIST_CTRL_MODE, run1);

    /* Wait 0.8 ms */
    ts.tv_sec  = 0;
    ts.tv_nsec = 800000L;
    nanosleep(&ts, NULL);

    /* FABRIC=0 */
    wr64(fm10k->mmio, FM10K_BIST_CTRL_RUN, run0);
    wr64(fm10k->mmio, FM10K_BIST_CTRL_MODE, mode0);

    /* Initialization of RXQ_MCAST list */
    for ( i = 0; i < 8; i++ ) {
        m64 = i | (i << 10);
        wr64(fm10k->mmio, FM10K_SCHED_RXQ_STORAGE_POINTERS(i), m64);
    }
    for ( i = 0; i < (1024 - 8); i++ ){
        wr32(fm10k->mmio, FM10K_SCHED_RXQ_FREELIST_INIT, i + 8);
    }

    /* Initialization of TXQ list */
    for ( i = 0; i < 384; i++ ) {
        wr32(fm10k->mmio, FM10K_SCHED_TXQ_HEAD_PERQ(i), i);
        wr32(fm10k->mmio, FM10K_SCHED_TXQ_TAIL0_PERQ(i), i);
        wr32(fm10k->mmio, FM10K_SCHED_TXQ_TAIL1_PERQ(i), i);
    }
    for ( i = 0; i < 24576 - 384; i++ ) {
        wr32(fm10k->mmio, FM10K_SCHED_RXQ_FREELIST_INIT, i + 384);
    }

    /* Initialization of FREE segment list */
    for ( i = 0; i < 48; i++ ) {
        wr32(fm10k->mmio, FM10K_SCHED_SSCHED_RX_PERPORT(i), i);
    }
    for ( i = 0; i < 24576 - 48; i++ ) {
        wr32(fm10k->mmio, FM10K_SCHED_FREELIST_INIT, 48 + i);
    }

    /* Initialization of scheduler polling schedule */
    /* Logical ports 0..35: Ethernet */
    for ( i = 0; i < 36; i++ ) {
        m32 = i | (i << 8);
        wr32(fm10k->mmio, FM10K_SCHED_RX_SCHEDULE(i), m32);
        wr32(fm10k->mmio, FM10K_SCHED_TX_SCHEDULE(i), m32);
    }
    /* Logical ports 36..41,46,47: 5xPEPs, 2xTE, 1xFIBM (bifurcation on) */
    int logical_port[8] = { 36, 37, 38, 39, 40, 41, 46, 47 };
    int physical_port[8] = { 36, 40, 44, 48, 52, 56, 60, 63 };
    int quad[8] = { 1, 1, 1, 1, 1, 1, 0, 0 };
    for ( i = 0; i < 8; i++ ) {
        m32 = physical_port[i] | (logical_port[i] << 8) | (quad[i] << 14);
        wr32(fm10k->mmio, FM10K_SCHED_RX_SCHEDULE(i + 36), m32);
        wr32(fm10k->mmio, FM10K_SCHED_TX_SCHEDULE(i + 36), m32);
    }
    /* Start scheduler */
    m32 = 1 | (43 << 2) | (1 << 11) | (43 << 13);
    wr32(fm10k->mmio, FM10K_SCHED_SCHEDULE_CTRL, m32);

    return 0;
}


/*
 * Main routine
 */
int
main(int argc, const char *const argv[])
{
    int fd;
    const char *prog;
    const char *uiodev;
    long pagesize;
    void *ptr;
    size_t size;
    fm10k_t fm10k;

    prog = argv[0];
    if ( argc < 2 ) {
        usage(prog);
    }
    uiodev = argv[1];

    /* Open and memory map uio device */
    fd = open (uiodev, O_RDWR);
    if ( fd < 0 ) {
        perror(uiodev);
        return EXIT_FAILURE;
    }
    pagesize = sysconf(_SC_PAGESIZE);
    size = (FM10K_BAR4_SIZE + pagesize - 1) / pagesize * pagesize;
    ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    /* Set them to the FM10K management structure */
    fm10k.fd = fd;
    fm10k.mmio = ptr;

    /* Initialize scheduler */
    init_scheduler(&fm10k);

    /* Testing */
    printf("%x\n", *((uint32_t *)(ptr + 0x10)));

    /* Unmap and close */
    (void)munmap(ptr, size);
    (void)close(fd);

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
