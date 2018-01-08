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
 * Port mapping
 */
typedef struct _fm10k_portmap {
    int logical;
    int physical;
    int quad;
} fm10k_portmap_t;

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
    int j;

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
    fm10k_portmap_t portmap[5] = {
        { .logical = 0, .physical = 0x3f, .quad = 0 },
        /* Physical ports 0..35: Ethernet; Set Quad to 1 for 100 GbE */
        { .logical = 1, .physical = 0x04, .quad = 1 },
        { .logical = 2, .physical = 0x18, .quad = 1 },
        { .logical = 3, .physical = 0x24, .quad = 1 },
        { .logical = 4, .physical = 0x28, .quad = 1 },
    };
    for ( i = 0; i < 5; i++ ) {
        /* PhysPort | Port | Quad */
        m32 = portmap[i].physical | (portmap[i].logical << 8)
            | (portmap[i].quad << 14);
        wr32(fm10k->mmio, FM10K_SCHED_RX_SCHEDULE(i), m32);
        wr32(fm10k->mmio, FM10K_SCHED_TX_SCHEDULE(i), m32);
    }
    /* Start scheduler */
    m32 = 1 | (5 << 2) | (1 << 11) | (5 << 13);
    wr32(fm10k->mmio, FM10K_SCHED_SCHEDULE_CTRL, m32);

    return 0;
}

/*
 * Initialize switch manager control
 */
int
init_switch_manager(fm10k_t *fm10k)
{
    uint32_t m32;
    uint64_t m64;
    int i;

    /* De-assert SWITCH_RESET */
    m32 = rd32(fm10k->mmio, FM10K_SOFT_RESET);
    m32 &= ~(1UL << 2);
    wr32(fm10k->mmio, FM10K_SOFT_RESET, m32);

    /* Disable switch scan */
    m32 = (1UL << 27) | (1UL << 30);
    wr32(fm10k->mmio, FM10K_SCAN_DATA_IN, m32);

    /* Disable switch loopbacks */
    m32 = rd32(fm10k->mmio, FM10K_PCIE_CTRL_EXT);
    m32 &= ~(1UL << 2);
    wr32(fm10k->mmio, FM10K_PCIE_CTRL_EXT, m32);
    /* Set EPL_CFG_A.Active = 0xf */
    for ( i = 0; i <= 8; i++ ) {
        m32 = rd32(fm10k->mmio, FM10K_EPL_CFG_A(i));
        m32 |= (0xf << 7);
        wr32(fm10k->mmio, FM10K_EPL_CFG_A(i), m32);
    }
    /* Set TE_CFG.SwitchLoopbackDisable = 1 */
    for ( i = 0; i < 2; i++ ) {
        m64 = rd64(fm10k->mmio, FM10K_TE_CFG(i));
        m64 |= (1 << 25);
        wr64(fm10k->mmio, FM10K_TE_CFG(i), m64);
    }

    /* Initialize the switch functions */

    /* Initialize scheduler */
    init_scheduler(fm10k);

    /* Start LED cntroller */
    m32 = rd32(fm10k->mmio, FM10K_LED_CFG);
    m32 |= (1 << 24);
    wr32(fm10k->mmio, FM10K_LED_CFG, m32);

    /* Initialize IEEE 1588 system time */

    /* Assert SWITCH_READY */
    m32 = rd32(fm10k->mmio, FM10K_SOFT_RESET);
    m32 |= (1UL << 3);
    wr32(fm10k->mmio, FM10K_SOFT_RESET, m32);

    /* Enable auto-negotiation */
    wr32(fm10k->mmio, FM10K_AN_73_CFG(1, 0), 1);
    wr32(fm10k->mmio, FM10K_AN_73_CFG(6, 0), 1);

    /* Enable LTSSM */
    m32 = rd32(fm10k->mmio, FM10K_PCIE_CTRL);
    m32 |= 1;
    wr32(fm10k->mmio, FM10K_PCIE_CTRL, m32);


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
    int i;

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
    printf("Initializing switch manager control\n");
    //init_switch_manager(&fm10k);

    /* Testing */
    printf("SOFT_RESET: %x\n", rd32(fm10k.mmio, FM10K_SOFT_RESET));
    for ( i = 0; i < 9; i++ ) {
        printf("PORT_STATUS[%d][%d]: %06x, ", i, 0,
               rd32(fm10k.mmio, FM10K_PORT_STATUS(i, 0)));
        printf("LED[%d] %05x, ", i, rd32(fm10k.mmio, FM10K_EPL_LED_STATUS(i)));
        printf("CFG_A[%d] %05x, ", i, rd32(fm10k.mmio, FM10K_EPL_CFG_A(i)));
        printf("CFG_B[%d] %05x\n", i, rd32(fm10k.mmio, FM10K_EPL_CFG_B(i)));
    }
    printf("PCIE_PORTLOGIC: %x\n", rd32(fm10k.mmio, FM10K_PCIE_PORTLOGIC));
    printf("DEVICE_CFG: %x\n", rd32(fm10k.mmio, FM10K_DEVICE_CFG));
    printf("AN_37_CFG: %x\n", rd32(fm10k.mmio, FM10K_AN_37_CFG(1, 0)));
    printf("AN_73_CFG: %x\n", rd32(fm10k.mmio, FM10K_AN_73_CFG(1, 0)));
    printf("PCIE_IP: %x\n", rd32(fm10k.mmio, FM10K_PCIE_IP));
    printf("PCIE_IM: %x\n", rd32(fm10k.mmio, FM10K_PCIE_IM));
    printf("GLOBAL_INTERRUPT_DETECT: %x\n",
           rd32(fm10k.mmio, FM10K_GLOBAL_INTERRUPT_DETECT));
    printf("CORE_INTERRUPT_DETECT: %x\n",
           rd32(fm10k.mmio, FM10K_CORE_INTERRUPT_DETECT));
    printf("CORE_INTERRUPT_MASK: %x\n",
           rd32(fm10k.mmio, FM10K_CORE_INTERRUPT_MASK));

#if 1
    unsigned int irq_on;
    unsigned int data;
    ssize_t nr;
    while ( 1 ) {
        /* Enable IRQ */
        irq_on = 1;
        write(fd, &irq_on, sizeof(irq_on));
        /* Read interrupt */
        nr = read(fd, &data, sizeof(data));
        printf("RD: %lu %u\n", nr, data);
    }
#endif

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
