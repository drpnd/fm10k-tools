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
    FM10K_SCHED(0x1 * (i) + 0x20000)

/*
 * SCHED_TX_SCHEDULE[0..1023]
 * 7:0   PhysPort
 * 13:8  Port
 * 14    Quad
 * 15    Color
 * 16    Idle
 */
#define FM10K_SCHED_TX_SCHEDULE(i)              \
    FM10K_SCHED(0x1 * (i) + 0x20400)

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
    FM10K_SCHED(0x2 * (i) + 0x60400)

/*
 * SCHED_RXQ_FREELIST_INIT
 */
#define FM10K_SCHED_RXQ_FREELIST_INIT   FM10K_SCHED(0x60410)

/*
 * SCHED_TXQ_TAIL0_PERQ[0..383]
 * (Indexed by QID = PORT * 8 + TC)
 */
#define FM10K_SCHED_TXQ_TAIL0_PERQ(i)           \
    FM10K_SCHED(0x1 * (i) + 0x60600)

/*
 * SCHED_TXQ_TAIL1_PERQ[0..383]
 */
#define FM10K_SCHED_TXQ_TAIL1_PERQ(i)           \
    FM10K_SCHED(0x1 * (i) + 0x60800)

/*
 * SCHED_TXQ_HEAD_PERQ[0..383]
 */
#define FM10K_SCHED_TXQ_HEAD_PERQ(i)            \
    FM10K_SCHED(0x1 * (i) + 0x60a00)

/*
 * SCHED_TXQ_FREELIST_INIT
 */
#define FM10K_SCHED_TXQ_FREELIST_INIT   FM10K_SCHED(0x62000)

/*
 * SSCHED_RX_PERPORT[0..47]
 */
#define FM10K_SCHED_SSCHED_RX_PERPORT(i)        \
    FM10K_SCHED(0x1 * (i) + 0x20840)

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

/*
 * REI_CTRL
 * 0    Reset
 * 1    Mode
 * 2    Run
 * 3    AutoLoadEnable
 * 31:4 Reserved
 */
#define FM10K_REI_CTRL          FM10K_MGMT(0xc12)

/*
 * REI_STAT
 * 0    ReiDonePass
 * 1    ReiDoneFail
 * 31:2 Reserved
 */
#define FM10K_REI_STAT          FM10K_MGMT(0xc13)

/*
 * SCAN_DATA_IN
 * 24:0 ScanData
 * 25   ShiftIn
 * 26   ShiftOut
 * 27   UpdateNodes
 * 28   Inject
 * 29   Drain
 * 30   Passthru
 * 31   Single
 */
#define FM10K_SCAN_DATA_IN      FM10K_MGMT(0xc2d)

/*
 * PCIE_SERDES_CTRL[0..7]
 * 0     Reserved
 * 1     Interrupt
 * 2     InProgress
 * 15:3  Reserved
 * 31:16 InterruptCode
 * 47:32 DataWrite
 * 63:48 DataRead
 */
#define FM10K_PCIE_SERDES_CTRL(i)               \
    FM10K_PCIE_PF(0x2 * (i) + 0x19010)


/*
 * PCIE_CTRL
 * 0    LTSSM_ENABLE
 * 1    REQ_RETRY_EN
 * 2    BAR4_Allowed
 * 3    Reserved
 * 4    RxLaneflipEn
 * 5    TxLaneflipEn
 * 31:6 Reserved
 */
#define FM10K_PCIE_CTRL         FM10K_PCIE_PF(0x0)

/*
 * PCIE_CTRL_EXT
 * 0    NS_DIS
 * 1    RO_DIS
 * 2    SwitchLoopback
 * 31:3 Reserved
 */
#define FM10K_PCIE_CTRL_EXT     FM10K_PCIE_PF(0x1)

/*
 * EPL_CFG_A[0..8]
 * 0     SpeedUp
 * 6:1   TimeOut
 * 10:7  Active[0..3]
 * 16:11 SkewTolerance
 * 31:17 Reserved
 */
#define FM10K_EPL_CFG_A(i)      FM10K_EPL(0x400 * (i) + 0x304)

/*
 * TE_CFG[0..1]
 * Atomicity: 64
 * 7:0   OuterTTL
 * 15:8  OuterTOS
 * 16    DeriveOuterTOS
 * 18:17 NotIP
 * 20:19 IPnotTCPnotUDP
 * 22:21 IPisTCPorUDP
 * 23    VerifyDecapCSUM
 * 24    UpdateOldHeaderInPlaceCSUM
 * 25    SwitchLoopbackDisable
 * 31:26 Reserved
 */
#define FM10K_TE_CFG(i)         FM10K_TE(0x100000 * (i) + 0x55a02)

/*
 * CM_GLOBAL_WM
 * 14:0  watermark
 * 31:15 Reserved
 */
#define FM10K_CM_GLOBAL_WM      FM10K_CM_USAGE(0x852)

/*
 * CM_GLOBAL_CFG
 * 7:0   ifgPenalty
 * 8     forcePauseOn
 * 9     forcePauseOff
 * 10    WmSweeperEnable
 * 11    PauseGenSweeperEnable
 * 12    PauseRecSweeperEnable
 * 18:13 NumSweeperPorts
 * 31:19 Reserved
 */
#define FM10K_CM_GLOBAL_CFG     FM10K_CM_USAGE(0x853)

/*
 * MA_TCN_IM
 * 0    PendingEvents
 * 1    TCN_Overflow
 * 31:2 Reserved
 */
#define FM10K_MA_TCN_IM         FM10K_L2LOOKUP_TCN(0x8c1)

/*
 * FH_TAIL_IM
 * 1:0   SafSramErr
 * 3:2   EgressPauseSramErr
 * 5:4   RxStatsSramErr
 * 7:6   PolicerUsageSramErr
 * 9:8   TcnSramErr
 * 10    TCN
 * 31:11 Reserved
 */
#define FM10K_FH_TAIL_IM        FM10K_HANDLER_TAIL(0x8e)

/*
 * INTERRUPT_MASK_INT
 * 8:0   PCIE_BSM[0..8]
 * 17:9  PCIE[0..8]
 * 26:18 EPL[0..8]
 * 28:27 TUNNEL[0..1]
 * 29    CORE
 * 30    SOFTWARE
 * 31    GPIO
 * 32    I2C
 * 33    MDIO
 * 34    CRM
 * 35    FH_TAIL
 * 36    FG_HEAD
 * 37    SBUS_EPL
 * 38    SBUS_PCIE
 * 39    PINS
 * 40    FIBM
 * 41    BSM
 * 42    XCLK
 * 63:43 Reserved
 */
#define FM10K_INTERRUPT_MASK_INT    FM10K_MGMT(0x402)

/*
 * INTERRUPT_MASK_PCIE
 * 8:0   PCIE_BSM[0..8]
 * 17:9  PCIE[0..8]
 * 26:18 EPL[0..8]
 * 28:27 TUNNEL[0..1]
 * 29    CORE
 * 30    SOFTWARE
 * 31    GPIO
 * 32    I2C
 * 33    MDIO
 * 34    CRM
 * 35    FH_TAIL
 * 36    FG_HEAD
 * 37    SBUS_EPL
 * 38    SBUS_PCIE
 * 39    PINS
 * 40    FIBM
 * 41    BSM
 * 42    XCLK
 * 63:43 Reserved
 */
#define FM10K_INTERRUPT_MASK_PCIE   FM10K_MGMT(0x420)

/*
 * INTERRUPT_MASK_FIBM
 * 8:0   PCIE_BSM[0..8]
 * 17:9  PCIE[0..8]
 * 26:18 EPL[0..8]
 * 28:27 TUNNEL[0..1]
 * 29    CORE
 * 30    SOFTWARE
 * 31    GPIO
 * 32    I2C
 * 33    MDIO
 * 34    CRM
 * 35    FH_TAIL
 * 36    FG_HEAD
 * 37    SBUS_EPL
 * 38    SBUS_PCIE
 * 39    PINS
 * 40    FIBM
 * 41    BSM
 * 42    XCLK
 * 63:43 Reserved
 */
#define FM10K_INTERRUPT_MASK_FIBM   FM10K_MGMT(0x440)

/*
 * INTERRUPT_MASK_BSM
 * 8:0   PCIE_BSM[0..8]
 * 17:9  PCIE[0..8]
 * 26:18 EPL[0..8]
 * 28:27 TUNNEL[0..1]
 * 29    CORE
 * 30    SOFTWARE
 * 31    GPIO
 * 32    I2C
 * 33    MDIO
 * 34    CRM
 * 35    FH_TAIL
 * 36    FG_HEAD
 * 37    SBUS_EPL
 * 38    SBUS_PCIE
 * 39    PINS
 * 40    FIBM
 * 41    BSM
 * 42    XCLK
 * 63:43 Reserved
 */
#define FM10K_INTERRUPT_MASK_BSM    FM10K_MGMT(0x442)

/*
 * LED_CFG
 * 23:0  LEDFreq
 * 24    Enable
 * 31:25 Reserved
 */
#define FM10K_LED_CFG           FM10K_MGMT(0xc2b)

/*
 * EPL_LED_STATUS[0..8]
 */
#define EPL_LED_STATUS(i)       FM10K_EPL(0x400 * (i) + 0x306)

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
