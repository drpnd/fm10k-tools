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

#ifndef _FM10K_H
#define _FM10K_H

#define FM10K_EPL(a)            ((0x0e0000 + (a)) * 4)
#define FM10K_PARSER(a)         ((0xcf0000 + (a)) * 4)
#define FM10K_FFU(a)            ((0xc00000 + (a)) * 4)
#define FM10K_FFU_MAP(a)        ((0xda0000 + (a)) * 4)
#define FM10K_EACL(a)           ((0xdb0000 + (a)) * 4)
#define FM10K_ARP(a)            ((0xcc0000 + (a)) * 4)
#define FM10K_POLICER_APPLY(a)  ((0xd60000 + (a)) * 4)
#define FM10K_POLICER_USAGE(a)  ((0xe40000 + (a)) * 4)
#define FM10K_L2LOOKUP(a)       ((0xc80000 + (a)) * 4)
#define FM10K_L2LOOKUP_TCN(a)   ((0xe70000 + (a)) * 4)
#define FM10K_HANDLER(a)        ((0xd50000 + (a)) * 4)
#define FM10K_HANDLER_TAIL(a)   ((0xe30000 + (a)) * 4)
#define FM10K_GLORT(a)          ((0xce0000 + (a)) * 4)
#define FM10K_LAG(a)            ((0xd90000 + (a)) * 4)
#define FM10K_TRIG_APPLY(a)     ((0xd70000 + (a)) * 4)
#define FM10K_TRIG_USAGE(a)     ((0xe78000 + (a)) * 4)
#define FM10K_CM_APPLY(a)       ((0xd40000 + (a)) * 4)
#define FM10K_CM_USAGE(a)       ((0xe60000 + (a)) * 4)
#define FM10K_MOD(a)            ((0xe80000 + (a)) * 4)
#define FM10K_RX_STATS(a)       ((0xe00000 + (a)) * 4)
#define FM10K_SCHED(a)          ((0xf00000 + (a)) * 4)
#define FM10K_FIBM(a)           ((0x008000 + (a)) * 4)
#define FM10K_MGMT(a)           ((0x000000 + (a)) * 4)
#define FM10K_PORTS_MGMT(a)     ((0x0e8000 + (a)) * 4)
#define FM10K_PCIE_PF(a)        ((0x100000 + (a)) * 4)
#define FM10K_PCIE_VF(a)        ((0x200000 + (a)) * 4)
#define FM10K_PCIE_CFG(a)       ((0x120000 + (a)) * 4)
#define FM10K_PCIE_CFG_VF(a)    ((0x130000 + (a)) * 4)
#define FM10K_TE(a)             ((0xa00000 + (a)) * 4)


/*
 * FM10K_BIST_CTRL
 * 0..8: PCIE[0..8]
 * 9:    EPL
 * 10:   FABRIC
 * 11:   Tunnel
 * 12:   BSM
 * 13:   CRM
 * 14:   FIBM
 * 15:   SBM
 * (16..32: Reserved)
 */
#define FM10K_BIST_CTRL         FM10K_MGMT(0xc10)
#define FM10K_BIST_CTRL_RUN     FM10K_MGMT(0xc10)
#define FM10K_BIST_CTRL_MODE    FM10K_MGMT(0xc10 + 1)

/*
 * SCHED_RX_SCHEDULE[0..1023]
 * 7:0   PhysPort
 * 13:8  Port
 * 14    Quad
 * 15    Color
 * 16    Idle
 * 31:17 Reserved
 */
#define FM10K_SCHED_RX_SCHEDULE(i)              \
    FM10K_SCHED(0x1 * i + 0x20000)

/*
 * SCHED_TX_SCHEDULE[0..1023]
 * 7:0   PhysPort
 * 13:8  Port
 * 14    Quad
 * 15    Color
 * 16    Idle
 */
#define FM10K_SCHED_TX_SCHEDULE(i)              \
    FM10K_SCHED(0x1 * i + 0x20400)

/*
 * SCHED_SCHEDULE_CTRL
 * 0     RxEnable
 * 1     RxPage
 * 10:2  RxMaxIndex
 * 11    TxEnable
 * 12    TxPage
 * 21:13 TxMaxIndex
 * 31:22 Reserved
 */
#define FM10K_SCHED_SCHEDULE_CTRL       FM10K_SCHED(0x20800)

/*
 * SCHED_RXQ_STORAGE_POINTERS[0..7]
 * Atomicity: 64
 * 9:0   HeadPage
 * 19:10 TailPage
 * 24:20 HeadIdx
 * 29:25 TailIdx
 * 39:30 NextPage
 * 63:40 Reserved
 */
#define FM10K_SCHED_RXQ_STORAGE_POINTERS(i)     \
    FM10K_SCHED(0x2 * i + 0x60400)

/*
 * SCHED_RXQ_FREELIST_INIT
 */
#define FM10K_SCHED_RXQ_FREELIST_INIT   FM10K_SCHED(0x60410)

/*
 * SCHED_TXQ_TAIL0_PERQ[0..383]
 * (Indexed by QID = PORT * 8 + TC)
 */
#define FM10K_SCHED_TXQ_TAIL0_PERQ(i)           \
    FM10K_SCHED(0x1 * i + 0x60600)

/*
 * SCHED_TXQ_TAIL1_PERQ[0..383]
 */
#define FM10K_SCHED_TXQ_TAIL1_PERQ(i)           \
    FM10K_SCHED(0x1 * i + 0x60800)

/*
 * SCHED_TXQ_HEAD_PERQ[0..383]
 */
#define FM10K_SCHED_TXQ_HEAD_PERQ(i)            \
    FM10K_SCHED(0x1 * i + 0x60a00)

/*
 * SCHED_TXQ_FREELIST_INIT
 */
#define FM10K_SCHED_TXQ_FREELIST_INIT   FM10K_SCHED(0x62000)

/*
 * SSCHED_RX_PERPORT[0..47]
 */
#define FM10K_SCHED_SSCHED_RX_PERPORT(i)        \
    FM10K_SCHED(0x1 * i + 0x20840)

/*
 * SCHED_FREELIST_INIT
 */
#define FM10K_SCHED_FREELIST_INIT       FM10K_SCHED(0x30244)

/*
 * PLL_FABRIC_CTRL
 * 0     Nreset
 * 1     Enable
 * 2     Halt
 * 8:3   RefDiv
 * 9     FbDiv4
 * 17:10 FbDiv255
 * 23:18 OutDiv
 * 26:24 OutMuxSel
 * 31:27 Reserved
 */
#define FM10K_PLL_FABRIC_CTRL   FM10K_PORTS_MGMT(0x2)

/*
 * PLL_FABRIC_STAT
 * 0     PllLocked
 * 1     PllFreqChange
 * 9:2   MiscCtrl
 * 31:10 Reserved
 */
#define FM10K_PLL_FABRIC_STAT   FM10K_PORTS_MGMT(0x3)


/*
 * PLL_FABRIC_LOCK
 * 3:0  FeatureCode
 * 7:4  FreqSel
 * 31:8 Reserved
 */
#define FM10K_PLL_FABRIC_LOCK   FM10K_PORTS_MGMT(0x4)

/*
 * SBUS_EPL_CFG
 * 0    SBUS_ControllerReset
 * 1    RomEnable
 * 2    RomBusy
 * 3    BistDonePass
 * 4    BistDoneFail
 * 31:5 Reserved
 */
#define FM10K_SBUS_EPL_CFG      FM10K_PORTS_MGMT(0x5)

/*
 * PLL_PCIE_CTRL
 * 0     Nreset
 * 1     Enable
 * 2     Halt
 * 8:3   RefDiv
 * 9     FbDiv4
 * 17:10 FbDiv255
 * 23:18 OutDiv
 * 26:24 OutMuxSel
 * 31:27 Reserved
 */
#define FM10K_PLL_PCIE_CTRL     FM10K_MGMT(0x2241)

/*
 * PLL_PCIE_STAT
 * 0     PllLocked
 * 1     PllFreqChange
 * 9:2   MiscCtrl
 * 31:10 Reserved
 */
#define FM10K_PLL_PCIE_STAT     FM10K_MGMT(0x2242)

/*
 * PCIE_CLK_CTRL
 * 3:0   Nreset[0..3]
 * 7:4   Enable[0..3]
 * 11:8  Halt[0..3]
 * 19:12 OutMuxSel[0..3] (4 x 2-bit)
 * 31:20 Mode[0..3] (4 x 3-bit)
 */
#define FM10K_PCIE_CLK_CTRL     FM10K_MGMT(0x3001)

/*
 * PCIE_CLK_CTRL2
 * 3:0  XclkTerm[0..3]
 * 7:4  ClkObs
 * 31:8 Reserved
 */
#define FM10K_PCIE_CLK_CTRL2    FM10K_MGMT(0x3002)

/*
 * DEVICE_CFG
 * 3:0   PCIeMode[0..3]
 * 4     Eth100GDisabled
 * 6:5   FeatureCode
 * 15:7  PCIeEnable[0..8]
 * 16    SystemClockSource
 * 31:17 Reserved
 */
#define FM10K_DEVICE_CFG        FM10K_MGMT(0x4)

/*
 * SOFT_RESET
 * 0     ColdReset
 * 1     EPLReset
 * 2     SwitchReset
 * 3     SwitchReady
 * 12:4  PCIeReset
 * 21:13 PCIeActive
 * 31:22 Reserved
 */
#define FM10K_SOFT_RESET        FM10K_MGMT(0x3)

/*
 * SBUS_PCIE_CFG
 * 0    SBUS_ControllerReset
 * 1    RomEnable
 * 2    RomBusy
 * 3    BistDonePass
 * 4    BistDoneFail
 * 31:5 Reserved
 */
#define FM10K_SBUS_PCIE_CFG     FM10K_MGMT(0x2243)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FM10K_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
