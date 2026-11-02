/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    kdcomp.h

Abstract:

    Private definitions for the kernel debugger COM port transport.
    Defines the CPPORT state structure, all 16550A register offsets and
    bit masks, baud-rate constants, CpGetByte return codes, and PORT_*
    flag bits.  This header is included only by ixkdcom.c; it must not
    be included by modules outside base/boot/kdcom.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only

--*/

#ifndef _KDCOMP_H_
#define _KDCOMP_H_

#include "ke.h"

/*
 * I/O base addresses for the standard AT COM ports.
 * CP_COM1_PORT is the primary kernel debug port.
 */
#define CP_COM1_PORT    0x3F8u
#define CP_COM2_PORT    0x2F8u

/*
 * 16550A UART register offsets from the port I/O base.
 *
 * When DLAB=0:
 *   +0  Receive buffer (read) / Transmit holding register (write)
 *   +1  Interrupt enable register
 * When DLAB=1:
 *   +0  Baud-rate divisor latch LSB
 *   +1  Baud-rate divisor latch MSB
 * DLAB-independent:
 *   +2  FIFO control register (write-only)
 *   +3  Line control register
 *   +4  Modem control register
 *   +5  Line status register (read-only)
 *   +6  Modem status register (read-only)
 *   +7  Scratch register
 */
#define COM_DAT     0u
#define COM_IEN     1u
#define COM_DLM     1u
#define COM_FCR     2u
#define COM_LCR     3u
#define COM_MCR     4u
#define COM_LSR     5u
#define COM_MSR     6u
#define COM_SCR     7u

/*
 * Line control register (COM_LCR) bit masks.
 */
#define LC_8DATABITS    0x03u   /* 8 data bits */
#define LC_NOPARITY     0x00u   /* No parity */
#define LC_1STOPBIT     0x00u   /* 1 stop bit */
#define LC_DLAB         0x80u   /* Divisor latch access bit */

/*
 * FIFO control register (COM_FCR) initialisation value.
 * Enable FIFOs, clear both direction FIFOs, set 14-byte receive threshold.
 */
#define FC_ENABLE_FIFOS 0x01u
#define FC_CLEAR_RX     0x02u
#define FC_CLEAR_TX     0x04u
#define FC_RX_THRESH14  0xC0u
#define FC_INIT         (FC_ENABLE_FIFOS | FC_CLEAR_RX | FC_CLEAR_TX | FC_RX_THRESH14)

/*
 * Modem control register (COM_MCR) bit masks.
 *
 * MC_OUT2 must be set on AT-class hardware to route the UART IRQ through
 * the interrupt controller.  We set it unconditionally even though we drive
 * the port in polling mode, because some chipsets prevent the transmitter
 * from advancing when OUT2 is clear.
 */
#define MC_DTR          0x01u
#define MC_RTS          0x02u
#define MC_DTRRTS       (MC_DTR | MC_RTS)
#define MC_OUT1         0x04u
#define MC_OUT2         0x08u
#define SERIAL_MCR_LOOP 0x10u   /* Internal loopback enable (for port test) */
#define SERIAL_MCR_OUT1 MC_OUT1

/*
 * Line status register (COM_LSR) bit masks.
 *
 * SERIAL_LSR_NOT_PRESENT (0xFF) is the value returned on a non-existent
 * port because the bus holds all lines high when nothing drives them.
 */
#define COM_DATRDY              0x01u   /* Received data ready */
#define COM_OE                  0x02u   /* Overrun error */
#define COM_PE                  0x04u   /* Parity error */
#define COM_FE                  0x08u   /* Framing error */
#define COM_BI                  0x10u   /* Break interrupt */
#define COM_OUTRDY              0x20u   /* Transmitter holding register empty */
#define COM_TEMT                0x40u   /* Transmitter shift register empty */
#define SERIAL_LSR_NOT_PRESENT  0xFFu

/*
 * Modem status register (COM_MSR) bit masks.
 *
 * MS_DSRCTSCD is the combined mask for DSR, CTS, and DCD.  All three must
 * be asserted before CpPutByte transmits when PORT_MODEMCONTROL is active.
 */
#define SERIAL_MSR_CTS  0x10u
#define SERIAL_MSR_DSR  0x20u
#define SERIAL_MSR_RI   0x40u
#define SERIAL_MSR_DCD  0x80u
#define MS_CD           SERIAL_MSR_DCD
#define MS_DSRCTSCD     (SERIAL_MSR_CTS | SERIAL_MSR_DSR | SERIAL_MSR_DCD)

/*
 * CpGetByte return codes.
 */
#define CP_GET_SUCCESS  0u   /* Byte received successfully */
#define CP_GET_NODATA   1u   /* No data available within timeout */
#define CP_GET_ERROR    2u   /* Line error (framing, parity, overrun) */

/*
 * CPPORT flags.  Stored in CPPORT::Flags.
 *
 * PORT_DEFAULTRATE   - Baud rate has not been explicitly set; the port
 *                      is running at the boot-time default.
 * PORT_MODEMCONTROL  - Modem handshaking (DSR/CTS/DCD) is enabled.
 *                      Set when ring-detect toggling is observed.
 * PORT_SAVED         - The HAL has saved port state (used by power-
 *                      management suspend/resume paths).
 * PORT_NOCDLTIME     - No carrier-detect loss timestamp recorded yet.
 * PORT_MDM_CD        - Carrier detect was active at some point since
 *                      PORT_MODEMCONTROL was set.
 * PORT_SENDINGSTRING - CpSendModemString is currently running.
 */
#define PORT_DEFAULTRATE    0x0001u
#define PORT_MODEMCONTROL   0x0002u
#define PORT_SAVED          0x0004u
#define PORT_NOCDLTIME      0x0008u
#define PORT_MDM_CD         0x0010u
#define PORT_SENDINGSTRING  0x0020u

/*
 * 16550A internal oscillator frequency and baud-rate divisor formula.
 *
 * The baud-rate generator divides the internal clock by (16 * divisor).
 * For 115200 bps: divisor = 1843200 / (16 * 115200) = 1.
 */
#define CP_CLOCK_RATE   1843200UL

/*
 * Supported baud rates.
 */
#define BD_9600         9600UL
#define BD_19200        19200UL
#define BD_57600        57600UL
#define BD_115200       115200UL

/*
 * CpGetByte timeout: approximate number of busy-wait iterations that
 * correspond to ~1 second at typical bus speeds.
 */
#define CP_TIMEOUT_COUNT    (1024UL * 200UL)

/*
 * Number of consecutive SERIAL_LSR_NOT_PRESENT reads that must occur
 * before CpReadLsr marks the port as absent.
 */
#define CP_DBG_ACCEPTABLE_ERRORS    25u

/*
 * CPPORT
 *
 * State record for one physical 16550A-compatible UART.
 *
 * Address - I/O base port number (e.g., CP_COM1_PORT).
 *           Zero before CpInitialize is called, which serves as the
 *           "not yet initialized" sentinel checked by CpGetByte.
 *
 * Baud    - Most recently programmed baud rate; zero before the first
 *           call to CpSetBaud.
 *
 * Flags   - Combination of PORT_* flag bits defined above.
 *
 * CarrierLostTime - Kernel tick count at the moment carrier detect was
 *                   lost.  Used to implement the 60-second modem-control
 *                   timeout in CpReadLsr.
 */
typedef struct _CPPORT {
    uint16_t    Address;
    uint32_t    Baud;
    uint16_t    Flags;
    uint32_t    CarrierLostTime;
} CPPORT, *PCPPORT;

/* Low-level 16550A primitives -- defined in ixkdcom.c */
void     CpInitialize(PCPPORT Port, uint16_t Address, uint32_t Rate);
void     CpSetBaud(PCPPORT Port, uint32_t Rate);
uint32_t CpQueryBaud(PCPPORT Port);
int      CpDoesPortExist(uint16_t Address);
uint8_t  CpReadLsr(PCPPORT Port, uint8_t Waiting);
void     CpPutByte(PCPPORT Port, uint8_t Byte);
uint16_t CpGetByte(PCPPORT Port, uint8_t *Byte, int WaitForByte);
void     KdComPortWriteString(const char *String);
void     KdComPortWriteHex32(uint32_t Value);

/* Public COM transport interface -- defined in ixkdcom.c, declared in ke.h */
extern CPPORT KdComPort;
extern int    KdComDbgPortsPresent;
extern uint8_t KdComDbgErrorCount;

#endif /* _KDCOMP_H_ */
