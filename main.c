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
#define FM10K_BAR4_SIZE         0x4000000

/* FM10K NVM recovery version */
#define NVM_PCIE_RECOVERY_VER   0x122

enum {
    FM10K_SOFT_RESET_LOCK_FREE = 0,
    FM10K_SOFT_RESET_LOCK_NVM = 1,
    FM10K_SOFT_RESET_LOCK_API = 2,
};

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
 * Wait for SOFT_RESET lock owner
 */
int
wait_for_soft_reset_lock_owner(fm10k_t *fm10k, int owner, long timeout)
{
    uint32_t m32;
    int lockowner;
    struct timespec ts;
    long atime;
    int cnt;

    /* Get the lock owner */
    m32 = rd32(fm10k->mmio, FM10K_BSM_SCRATCH(2));
    lockowner = m32 & 0x3;

    if ( FM10K_SOFT_RESET_LOCK_API == lockowner ) {
        if ( FM10K_SOFT_RESET_LOCK_API != owner ) {
            fprintf(stderr, "warning...\n");
            return -1;
        }
    }

    atime = 0;
    cnt = 1;
    while ( lockowner != owner ) {
        printf("%x:%x\n", owner, lockowner);
        if ( atime >= timeout ) {
            return -1;
        }
        /* Check timeout */
        ts.tv_sec  = 0;
        ts.tv_nsec = 10000L * cnt;
        nanosleep(&ts, NULL);
        atime += 10000L * cnt;
        cnt++;

        m32 = rd32(fm10k->mmio, FM10K_BSM_SCRATCH(2));
        lockowner = m32 & 0x3;
    }

    return 0;
}

/*
 * Take SOFT_RESET lock
 */
int
take_soft_reset_lock(fm10k_t *fm10k)
{
    uint32_t m32;
    int i;
    int ret;
    struct timespec ts;

    /* Get NVM version */
    m32 = rd32(fm10k->mmio, FM10K_BSM_SCRATCH(401));
    if ( m32 > NVM_PCIE_RECOVERY_VER ) {
        /* Support locking in NVM */
        for ( i = 0; i < 3; i++ ) {
            ret = wait_for_soft_reset_lock_owner(fm10k,
                                                 FM10K_SOFT_RESET_LOCK_FREE,
                                                 1000 * 1000 * 2000);
            if ( ret < 0 ) {
                /* Error */
                return -1;
            }
            /* Take a lock */
            wr32(fm10k->mmio, FM10K_BSM_SCRATCH(2), FM10K_SOFT_RESET_LOCK_API);

            /* Wait 50us */
            ts.tv_sec  = 0;
            ts.tv_nsec = 50000L;
            nanosleep(&ts, NULL);

            /* Check the owner */
            ret = wait_for_soft_reset_lock_owner(fm10k,
                                                 FM10K_SOFT_RESET_LOCK_API, 0);
            if ( 0 == ret ) {
                return 0;
            }
        }
    } else {
        /* No support locking in NVM */
        return -1;
    }

    return -1;
}

/*
 * Drop SOFT_RESET lock
 */
int
drop_soft_reset_lock(fm10k_t *fm10k)
{
    uint32_t m32;
    int i;
    int ret;

    /* Get NVM version */
    m32 = rd32(fm10k->mmio, FM10K_BSM_SCRATCH(401));
    if ( m32 > NVM_PCIE_RECOVERY_VER ) {
        /* Support locking in NVM */
        m32 = rd32(fm10k->mmio, FM10K_BSM_SCRATCH(2));
        switch ( m32 & 3 ) {
        case FM10K_SOFT_RESET_LOCK_API:
            wr32(fm10k->mmio, FM10K_BSM_SCRATCH(2), 0);
            return 0;
            break;
        case FM10K_SOFT_RESET_LOCK_FREE:
            fprintf(stderr, "warning\n");
            return 0;
        default:
            return -1;
        }
    } else {
        /* No support locking in NVM */
        return -1;
    }

    return 0;
}

/*
 * Set frame handler clock (use default value)
 */
int
set_frame_handler_clock(fm10k_t *fm10k)
{
    uint32_t m32;
    int sku;
    int feature;
    int freq;
    int refdiv;
    int outdiv;
    int fbdiv4;
    int fbdiv255;
    int skuclock;
    struct timespec ts;
    int maxfreq;
    int fhclock;
    int freqsel;

    /* Read SKU from FUSE_DATA_0[15:11] */
    m32 = rd32(fm10k->mmio, FM10K_FUSE_DATA_0);
    if ( 0 == m32 ) {
        /* Unknown SKU */
        fprintf(stderr, "Unknown SKU\n");
        sku = 0xff;
    } else {
        sku = (m32 >> 11) & 0x1f;
    }

    /* Get feature code and frequency selection */
    m32 = rd32(fm10k->mmio, FM10K_PLL_FABRIC_LOCK);
    /* FeatureCode:
       0000b = Full -- All frequencies are supported
       0001b = LIMITED0 -- Restricted to 600, 500, 400, 300 MHz
       0010b = LIMITED1 -- Restricted to 500, 400, 300 MHz
       0011b = LIMITED2 -- Restricted to 400, 300 MHz
       0100b = LIMITED3 -- Restricted to 300 MHz */
    feature = m32 & 0xf;
    /* FreqSel:
       0000b = USE_CTRL -- Use PLL_FABRIC_CTRL's values to set frequency
       0001b = F600 -- 600 MHz
       0010b = F500 -- 500 MHz
       0011b = F400 -- 400 MHz
       0100b = F300 -- 300 MHz */
    freq = (m32 >> 4) & 0xf;

    switch ( sku ) {
    case 0: /* FM10840 */
        refdiv = 0x19;
        outdiv = 0x5;
        fbdiv4 = 0x1;
        fbdiv255 = 0xc4;
        skuclock = 980000000;   /* 980 MHz in binary */
        break;
    case 1: /* FM10420 */
    default:
        refdiv = 0x3;
        outdiv = 0x7;
        fbdiv4 = 0x0;
        fbdiv255 = 0x2f;
        skuclock = 699404761;   /* 700 MHz in binary */
        break;
    }

    if ( 0 == feature ) {
        /* Full control over PLL */
        m32 = rd32(fm10k->mmio, FM10K_PLL_FABRIC_CTRL);
        /* RefDiv | FbDiv4 | FbDiv255 | OutDiv */
        m32 = (m32 & ~(0xfffff8UL)) |
            (refdiv << 3) | (fbdiv4 << 4) | (fbdiv255 << 10) | (outdiv << 18);
        wr32(fm10k->mmio, FM10K_PLL_FABRIC_CTRL, m32);

        /* Toggle reset */
        m32 &= ~1UL;
        wr32(fm10k->mmio, FM10K_PLL_FABRIC_CTRL, m32);

        /* Wait 500ns */
        ts.tv_sec  = 0;
        ts.tv_nsec = 500L;
        nanosleep(&ts, NULL);

        m32 |= 1UL;
        wr32(fm10k->mmio, FM10K_PLL_FABRIC_CTRL, m32);

        /* Wait 1ms */
        ts.tv_sec  = 0;
        ts.tv_nsec = 1000000L;
        nanosleep(&ts, NULL);

        m32 = rd32(fm10k->mmio, FM10K_PLL_FABRIC_LOCK);
        m32 = (m32 & ~0xfUL) | (0);
        wr32(fm10k->mmio, FM10K_PLL_FABRIC_LOCK, m32);
    } else {
        switch ( feature ) {
        case 1:                 /* LIMITED0 */
            maxfreq = 612 * 1000000;
            break;
        case 2:                 /* LIMITED1 */
            maxfreq = 510 * 1000000;
            break;
        case 3:                 /* LIMITED2 */
            maxfreq = 408 * 1000000;
            break;
        case 4:                 /* LIMITED3 */
            maxfreq = 306 * 1000000;
            break;
        default:
            maxfreq = 612 * 1000000;
        }
        if ( maxfreq > skuclock ) {
            maxfreq = skuclock;
        }

        if ( fhclock >= 612 * 1000000 ) {
            /* F600 */
            freqsel = 1;
        } else if ( fhclock >= 510 * 1000000 ) {
            /* F500 */
            freqsel = 2;
        } else if ( fhclock >= 408 * 1000000 ) {
            /* F400 */
            freqsel = 3;
        } else {
            /* F300 */
            freqsel = 4;
        }

        m32 = rd32(fm10k->mmio, FM10K_PLL_FABRIC_LOCK);
        m32 = (m32 & ~0xf0UL) | (freqsel << 4);
        wr32(fm10k->mmio, FM10K_PLL_FABRIC_LOCK, m32);
    }

    return 0;
}

/*
 * Reset switch
 */
int
reset_switch(fm10k_t *fm10k)
{
    uint32_t m32;
    int ret;
    struct timespec ts;

    /* Try to take a lock */
    ret = take_soft_reset_lock(fm10k);
    if ( ret < 0 ) {
        fprintf(stderr, "Could not take lock\n");
        return -1;
    }

    /* Set SwitchReady=0 */
    m32 = rd32(fm10k->mmio, FM10K_SOFT_RESET);
    m32 &= ~(1 << 3);
    wr32(fm10k->mmio, FM10K_SOFT_RESET, m32);

    /* Wait 100us */
    ts.tv_sec  = 0;
    ts.tv_nsec = 100000L;
    nanosleep(&ts, NULL);

    /* Reduce EPL frequency to reduce power during reset */
    m32 = rd32(fm10k->mmio, FM10K_PLL_EPL_CTRL);
    m32 |= (63 << 18);          /* OutDiv = 63 */
    wr32(fm10k->mmio, FM10K_PLL_EPL_CTRL, m32);

    /* Apply OutDiv (Bit[4]) */
    m32 = rd32(fm10k->mmio, FM10K_PLL_EPL_STAT);
    m32 |= (1 << 6);
    wr32(fm10k->mmio, FM10K_PLL_EPL_STAT, m32);
    m32 &= ~(1 << 6);
    wr32(fm10k->mmio, FM10K_PLL_EPL_STAT, m32);

    /* Assert Switch/EPL reset */
    m32 = rd32(fm10k->mmio, FM10K_SOFT_RESET);
    m32 |= (1 << 2);            /* SwitchReset */
    m32 |= (1 << 1);            /* EPLReset */
    wr32(fm10k->mmio, FM10K_SOFT_RESET, m32);

    /* Wait 1ms */
    ts.tv_sec  = 0;
    ts.tv_nsec = 1000000L;
    nanosleep(&ts, NULL);

    ret = drop_soft_reset_lock(fm10k);
    if ( ret < 0 ) {
        return -1;
    }

    return 0;
}

/*
 * Release switch
 */
int
release_switch(fm10k_t *fm10k)
{
    uint32_t m32;
    uint64_t m64;
    struct timespec ts;
    int ret;

    /* Clear switch/tunnel/EPL memories */
    m64 = rd64(fm10k->mmio, FM10K_BIST_CTRL);
    m64 |= (1 << 10) | (1 << 11) | (1 << 9);
    wr64(fm10k->mmio, FM10K_BIST_CTRL, m64);

    /* Wait 0.8ms */
    ts.tv_sec  = 0;
    ts.tv_nsec = 800000L;
    nanosleep(&ts, NULL);

    m64 &= ~((1 << 10) | (1 << 11) | (1 << 9) | (1ULL << 42) | (1ULL << 43)
             | (1ULL << 41));
    wr64(fm10k->mmio, FM10K_BIST_CTRL, m64);

    /* Try to take a lock */
    ret = take_soft_reset_lock(fm10k);
    if ( ret < 0 ) {
        fprintf(stderr, "Could not take lock\n");
        return -1;
    }

    m32 = rd32(fm10k->mmio, FM10K_SOFT_RESET);
    m32 &= ~((1 << 2) | (1 << 1));
    wr32(fm10k->mmio, FM10K_SOFT_RESET, m32);

    /* Wait 100ns */
    ts.tv_sec  = 0;
    ts.tv_nsec = 100000L;
    nanosleep(&ts, NULL);

    ret = drop_soft_reset_lock(fm10k);
    if ( ret < 0 ) {
        return -1;
    }

    m32 = rd32(fm10k->mmio, FM10K_PLL_EPL_CTRL);
    m32 = (m32 & ~(0x3f << 18)) | (6 << 18);
    wr32(fm10k->mmio, FM10K_PLL_EPL_CTRL, m32);

    /* Apply OutDiv (toggle PLL_EPL_STAT.MiscCtrl[4]) */
    m32 = rd32(fm10k->mmio, FM10K_PLL_EPL_STAT);
    m32 |= (1 << 6);
    wr32(fm10k->mmio, FM10K_PLL_EPL_STAT, m32);

    m32 &= ~(1 << 6);
    wr32(fm10k->mmio, FM10K_PLL_EPL_STAT, m32);

    return 0;
}

/*
 * Initialize SerDes
 */
int
serdes_init_op_mode(fm10k_t *fm10k)
{
    /* FIXME */
    return 0;
}

/*
 * Boot switch
 */
int
boot_switch(fm10k_t *fm10k)
{
    int ret;

    /* Reset the switch */
    ret = reset_switch(fm10k);
    if ( ret < 0 ) {
        fprintf(stderr, "Failed to reset the switch\n");
        return -1;
    }

    /* Configure clock via FABRIC_PLL */
    ret = set_frame_handler_clock(fm10k);
    if ( ret < 0 ) {
        fprintf(stderr, "Failed to set frame handler clock\n");
        return -1;
    }

    /* Release switch */
    ret = release_switch(fm10k);
    if ( ret < 0 ) {
        fprintf(stderr, "Failed to release switch\n");
        return -1;
    }


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
    //m32 = rd32(fm10k->mmio, FM10K_CM_GLOBAL_CFG);
    //m32 |= (0x1 << 10) | (0x1 << 11) | (0x1 << 12) | (0x4 << 13);
    //wr32(fm10k->mmio, FM10K_CM_GLOBAL_CFG, m32);

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

    /* Boot switch */
    boot_switch(&fm10k);

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
    printf("CM_GLOBAL_CFG: %x\n", rd32(fm10k.mmio, FM10K_CM_GLOBAL_CFG));
    printf("LED_CFG: %x\n", rd32(fm10k.mmio, FM10K_LED_CFG));

#if 1
    uint32_t info;
    ssize_t nr;
    while ( 1 ) {
        /* Enable IRQ */
        info = 1;
        write(fd, &info, sizeof(info));
        /* Read interrupt */
        nr = read(fd, &info, sizeof(info));
        printf("RD: %lu %u\n", nr, info);
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
