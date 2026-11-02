/*++

Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

Module Name:

    ixkdcom.c

Abstract:

    i386 kernel debugger COM port transport.  Implements the full
    16550A UART low-level primitives (CpInitialize, CpSetBaud,
    CpQueryBaud, CpDoesPortExist, CpReadLsr, CpPutByte, CpGetByte)
    and the two public transport entry points that the rest of the
    kernel sees: KdComPortInitialize and KdComPortWriteString.

    All I/O is performed in polling mode.  No interrupts are used.

Author:

    Noah Juopperi <nipfswd@gmail.com>
    Vlad Lymar <ggcc98765432110@gmail.com>

Environment:

    Kernel-mode only.  Must not call malloc, printf, or any libc
    function.

--*/

#include "kdcomp.h"

static const char KdHexDigits[16] = {
    '0','1','2','3','4','5','6','7',
    '8','9','A','B','C','D','E','F'
};

/*
 * KdComPort
 *
 * State record for the kernel debug serial port.  Initialised by
 * KdComPortInitialize.  KdComPort.Address == 0 before initialisation.
 *
 * The Flags field is preset to PORT_DEFAULTRATE to indicate that the
 * baud rate has not yet been explicitly selected (it will be set to
 * BD_115200 during CpInitialize).
 */
CPPORT KdComPort = { 0, 0, PORT_DEFAULTRATE, 0 };

/*
 * KdComDbgPortsPresent
 *
 * Non-zero while the debug port is responding normally.  Set to 0
 * by CpReadLsr after CP_DBG_ACCEPTABLE_ERRORS consecutive reads of
 * SERIAL_LSR_NOT_PRESENT, which indicates that no physical UART is
 * present at the configured I/O base.
 */
int KdComDbgPortsPresent = 1;

/*
 * KdComDbgErrorCount
 *
 * Running count of consecutive SERIAL_LSR_NOT_PRESENT reads.
 * Reset to zero after a successful LSR read.
 */
uint8_t KdComDbgErrorCount = 0;

/*
 * Inline I/O helpers.
 *
 * GCC compiles these to a single IN/OUT instruction with the DX register
 * holding the port address.  We use uint16_t for the port number because
 * the x86 I/O address space is 16 bits wide.
 */

static inline void
CpOutb(
    uint16_t Port,
    uint8_t  Value
    )
{
    __asm__ __volatile__("outb %1, %0" : : "Nd"(Port), "a"(Value));
}

static inline uint8_t
CpInb(
    uint16_t Port
    )
{
    uint8_t Value;
    __asm__ __volatile__("inb %1, %0" : "=a"(Value) : "Nd"(Port));
    return Value;
}

/*++

Routine Description:

    Tests whether a 16550A-compatible UART is present at Address by
    performing a non-destructive internal loopback test.  The modem
    control register is set to loopback mode; a test pattern is written
    to the modem control register and read back from the modem status
    register; the modem control register is then restored.

    On a real 16550A in loopback mode, the lower four MCR output bits
    (DTR, RTS, OUT1, OUT2) are reflected in the upper four MSR bits
    (DSR=DTR, CTS=RTS, RI=OUT1, DCD=OUT2).  We test this reflection.

    Returns non-zero if a port appears to exist.

Arguments:

    Address - I/O base of the candidate UART.

Return Value:

    Non-zero if a 16550A-compatible UART responds, zero otherwise.

--*/

int
CpDoesPortExist(
    uint16_t Address
    )
{
    uint8_t McSave;
    uint8_t McTest;
    uint8_t MsRead;

    /*
     * Save the current modem control register value so we can restore
     * it after the loopback test, leaving the port in an undisturbed state.
     */
    McSave = CpInb((uint16_t)(Address + COM_MCR));

    /*
     * Enable internal loopback.  In this mode the UART's transmit output
     * is internally connected to the receive input, and the MCR output
     * bits are visible in the MSR input bits.
     */
    CpOutb((uint16_t)(Address + COM_MCR), SERIAL_MCR_LOOP);

    /*
     * Write a distinctive test pattern: set DTR, RTS, OUT1 (but NOT OUT2
     * and NOT LOOP itself, so the pattern is 0x07).  In loopback mode these
     * appear as CTS, DSR, and RI in the MSR.  DCD (matching OUT2) should
     * remain clear.
     */
    McTest = MC_DTRRTS | MC_OUT1;
    CpOutb((uint16_t)(Address + COM_MCR), (uint8_t)(SERIAL_MCR_LOOP | McTest));

    MsRead = CpInb((uint16_t)(Address + COM_MSR));

    /*
     * Restore the original MCR so the rest of the boot sequence is not
     * perturbed.
     */
    CpOutb((uint16_t)(Address + COM_MCR), McSave);

    /*
     * Verify that the expected bits are reflected.
     * DTR  -> DSR  (bit 5)
     * RTS  -> CTS  (bit 4)
     * OUT1 -> RI   (bit 6)
     * OUT2 -> DCD  (bit 7) -- must NOT be set since OUT2 was clear.
     */
    if ((MsRead & (SERIAL_MSR_DSR | SERIAL_MSR_CTS | SERIAL_MSR_RI)) !=
                  (SERIAL_MSR_DSR | SERIAL_MSR_CTS | SERIAL_MSR_RI)) {
        return 0;
    }

    if (MsRead & SERIAL_MSR_DCD) {
        return 0;
    }

    return 1;
}

/*++

Routine Description:

    Programs the 16550A baud-rate divisor registers for the requested
    rate.  The DLAB bit in LCR is set to expose the divisor registers,
    the 16-bit divisor is written (LSB then MSB), and DLAB is cleared.
    The line parameters are fixed at 8N1.

Arguments:

    Port - CPPORT record whose Address field identifies the UART.

    Rate - Baud rate in bits per second.  Must not be zero.

Return Value:

    None.

--*/

void
CpSetBaud(
    PCPPORT  Port,
    uint32_t Rate
    )
{
    uint16_t Divisor;

    /*
     * The baud-rate generator divides the internal clock by (16 * divisor).
     *   Divisor = CP_CLOCK_RATE / (16 * Rate)
     *           = CP_CLOCK_RATE / (16 * Rate)
     *
     * For 115200 bps the divisor is 1.
     */
    Divisor = (uint16_t)(CP_CLOCK_RATE / (16UL * Rate));
    if (Divisor == 0) {
        Divisor = 1;
    }

    /* Enable divisor-latch access */
    CpOutb((uint16_t)(Port->Address + COM_LCR), LC_DLAB);

    /* Write divisor LSB then MSB */
    CpOutb((uint16_t)(Port->Address + COM_DAT), (uint8_t)(Divisor & 0xFFu));
    CpOutb((uint16_t)(Port->Address + COM_DLM), (uint8_t)(Divisor >> 8u));

    /* 8N1 -- also clears DLAB */
    CpOutb((uint16_t)(Port->Address + COM_LCR), LC_8DATABITS);

    Port->Baud  = Rate;
    Port->Flags &= (uint16_t)~PORT_DEFAULTRATE;
}

/*++

Routine Description:

    Returns the baud rate that was most recently programmed into Port.

Arguments:

    Port - CPPORT record.

Return Value:

    Baud rate in bits per second, or zero if CpSetBaud has not been
    called on this port.

--*/

uint32_t
CpQueryBaud(
    PCPPORT Port
    )
{
    return Port->Baud;
}

/*++

Routine Description:

    Initialises the CPPORT record at Port and programs the 16550A UART
    at Address for 8N1 operation at Rate bps.  The FIFO is enabled and
    flushed.  Modem control lines (DTR, RTS, OUT2) are asserted.
    Interrupts are disabled at the UART level.

    If the port does not respond to the loopback existence check, the
    function returns without modifying KdComDbgPortsPresent.

Arguments:

    Port    - CPPORT record to initialise.

    Address - I/O base address of the UART (e.g., CP_COM1_PORT).

    Rate    - Initial baud rate in bits per second.

Return Value:

    None.

--*/

void
CpInitialize(
    PCPPORT  Port,
    uint16_t Address,
    uint32_t Rate
    )
{
    if (!CpDoesPortExist(Address)) {
        return;
    }

    /* Disable all UART interrupts */
    CpOutb((uint16_t)(Address + COM_IEN), 0x00u);

    /* Enable and clear the FIFOs */
    CpOutb((uint16_t)(Address + COM_FCR), FC_INIT);

    /* Populate the record so CpSetBaud can use Port->Address */
    Port->Address         = Address;
    Port->Baud            = 0;
    Port->Flags           = PORT_DEFAULTRATE;
    Port->CarrierLostTime = 0;

    /* Program baud rate and 8N1 line control */
    CpSetBaud(Port, Rate);

    /* Assert DTR and RTS; enable OUT2 to route IRQ through PIC */
    CpOutb((uint16_t)(Address + COM_MCR), (uint8_t)(MC_DTRRTS | MC_OUT2));
}

/*++

Routine Description:

    Reads the Line Status Register and performs port-presence accounting.
    If SERIAL_LSR_NOT_PRESENT (0xFF) is seen CP_DBG_ACCEPTABLE_ERRORS
    times consecutively, KdComDbgPortsPresent is cleared.
    If Waiting is non-zero the caller is waiting for THRE (transmitter
    empty) rather than for received data.

Arguments:

    Port    - CPPORT record.

    Waiting - Non-zero means the caller is waiting for COM_OUTRDY.

Return Value:

    The byte read from the LSR, or SERIAL_LSR_NOT_PRESENT if the port
    appears absent.

--*/

uint8_t
CpReadLsr(
    PCPPORT Port,
    uint8_t Waiting
    )
{
    uint8_t Lsr;
    uint8_t Msr;

    Lsr = CpInb((uint16_t)(Port->Address + COM_LSR));

    if (Lsr == SERIAL_LSR_NOT_PRESENT) {
        KdComDbgErrorCount++;
        if (KdComDbgErrorCount >= CP_DBG_ACCEPTABLE_ERRORS) {
            KdComDbgPortsPresent = 0;
        }
        return SERIAL_LSR_NOT_PRESENT;
    }

    /* Successful read -- reset the consecutive-error counter */
    KdComDbgErrorCount = 0;

    /*
     * Modem-control handling.
     *
     * When PORT_MODEMCONTROL is active we must observe carrier-detect
     * transitions.  If carrier has been lost and the 60-second loss
     * window has expired we clear PORT_MODEMCONTROL.
     *
     * This block is never reached in normal debug-console usage because
     * the debug port is not a dial-up modem and PORT_MODEMCONTROL is not
     * set during CpInitialize.
     */
    if (Port->Flags & PORT_MODEMCONTROL) {
        Msr = CpInb((uint16_t)(Port->Address + COM_MSR));

        if (!(Msr & SERIAL_MSR_DCD)) {
            /*
             * Carrier has been lost.  Record the tick count at which it
             * was first lost.  If the port re-acquires carrier before the
             * 60-second timeout, clear the timestamp.
             */
            if (Port->Flags & PORT_NOCDLTIME) {
                /*
                 * First detection of carrier loss: record the time.
                 */
                Port->CarrierLostTime = KernelGetTickCount();
                Port->Flags &= (uint16_t)~PORT_NOCDLTIME;
            } else {
                /*
                 * Carrier still absent: check whether 60 seconds have
                 * elapsed since loss (tick resolution is 10 ms, so 60 s
                 * corresponds to 6000 ticks).
                 */
                if ((KernelGetTickCount() - Port->CarrierLostTime) >= 6000UL) {
                    Port->Flags &= (uint16_t)~PORT_MODEMCONTROL;
                }
            }
        } else {
            /* Carrier restored: clear the loss timestamp */
            Port->Flags |= PORT_NOCDLTIME;
        }
    }

    (void)Waiting;
    return Lsr;
}

/*++

Routine Description:

    Writes a single byte to the 16550A transmitter.  Busy-waits until
    the Transmitter Holding Register Empty (THRE) bit is set in the
    LSR, then writes the byte to the data register.

    When PORT_MODEMCONTROL is active, also waits for DSR, CTS, and DCD
    to be asserted in the MSR before transmitting.

Arguments:

    Port - CPPORT record.

    Byte - The byte to transmit.

Return Value:

    None.

--*/

void
CpPutByte(
    PCPPORT Port,
    uint8_t Byte
    )
{
    uint8_t Lsr;
    uint8_t Msr;

    /*
     * If modem handshaking is active, wait for all modem control signals
     * (DSR, CTS, and DCD) before sending the byte.
     */
    if (Port->Flags & PORT_MODEMCONTROL) {
        do {
            Msr = CpInb((uint16_t)(Port->Address + COM_MSR));
        } while ((Msr & MS_DSRCTSCD) != MS_DSRCTSCD);
    }

    /*
     * Wait for the Transmitter Holding Register to become empty.
     * CpReadLsr updates the port-presence tracking on each read.
     */
    do {
        if (!KdComDbgPortsPresent) {
            return;
        }
        Lsr = CpReadLsr(Port, 1);
    } while (!(Lsr & COM_OUTRDY));

    CpOutb(Port->Address, Byte);
}

/*++

Routine Description:

    Reads a single byte from the 16550A receiver with a configurable
    timeout.  If WaitForByte is zero and no data is available, returns
    CP_GET_NODATA immediately.  If WaitForByte is non-zero, polls until
    data arrives or CP_TIMEOUT_COUNT iterations elapse.

    Returns CP_GET_ERROR if a framing, parity, or overrun error is
    indicated in the LSR.

Arguments:

    Port       - CPPORT record.

    Byte       - On CP_GET_SUCCESS, receives the byte read.

    WaitForByte - Non-zero to wait up to CP_TIMEOUT_COUNT iterations;
                  zero for immediate non-blocking check.

Return Value:

    CP_GET_SUCCESS, CP_GET_NODATA, or CP_GET_ERROR.

--*/

uint16_t
CpGetByte(
    PCPPORT  Port,
    uint8_t *Byte,
    int      WaitForByte
    )
{
    uint32_t Timeout;
    uint8_t  Lsr;

    /*
     * An uninitialised port (Address == 0) can never return data.
     */
    if (Port->Address == 0) {
        return CP_GET_NODATA;
    }

    Timeout = WaitForByte ? CP_TIMEOUT_COUNT : 1UL;

    do {
        Lsr = CpReadLsr(Port, 0);

        if (Lsr == SERIAL_LSR_NOT_PRESENT) {
            return CP_GET_ERROR;
        }

        if (Lsr & (COM_FE | COM_PE | COM_OE)) {
            /*
             * A receive error occurred.  Read and discard the byte from
             * the data register to clear the error condition.
             */
            (void)CpInb(Port->Address);
            return CP_GET_ERROR;
        }

        if (Lsr & COM_DATRDY) {
            *Byte = CpInb(Port->Address);
            return CP_GET_SUCCESS;
        }

        Timeout--;
    } while (Timeout > 0UL);

    return CP_GET_NODATA;
}

/*++

Routine Description:

    Public entry point.  Initialises the primary kernel debug COM port
    (CP_COM1_PORT) at 115200 bps, 8 data bits, no parity, 1 stop bit.

    This function is safe to call more than once; subsequent calls
    reinitialise the UART without visible side effects.

    Should be called as the very first operation in kernelMain, before
    any memory manager initialisation, so that all subsequent bugcheck
    and assertion failures can emit diagnostics even if the video
    subsystem has not yet been set up.

Arguments:

    None.

Return Value:

    None.

--*/

void
KdComPortInitialize(
    void
    )
{
    CpInitialize(&KdComPort, CP_COM1_PORT, BD_115200);
}

/*++

Routine Description:

    Public entry point.  Writes every character in the NUL-terminated
    string String to the debug COM port one byte at a time.

    '\n' is automatically expanded to the CR+LF pair ("\r\n") required
    by most terminal emulators and serial consoles.

    If the port is not present (KdComDbgPortsPresent == 0) or has not
    been initialised (KdComPort.Address == 0), this function is a
    safe no-op.

Arguments:

    String - NUL-terminated ASCII string to write.

Return Value:

    None.

--*/

void
KdComPortWriteString(
    const char *String
    )
{
    if (KdComPort.Address == 0 || !KdComDbgPortsPresent) {
        return;
    }

    while (*String) {
        if (*String == '\n') {
            CpPutByte(&KdComPort, '\r');
        }
        CpPutByte(&KdComPort, (uint8_t)*String);
        String++;
    }
}

void
KdComPortWriteHex32(
    uint32_t Value
    )
{
    int i;

    CpPutByte(&KdComPort, '0');
    CpPutByte(&KdComPort, 'x');
    for (i = 28; i >= 0; i -= 4) {
        CpPutByte(&KdComPort, (uint8_t)KdHexDigits[(Value >> i) & 0xFU]);
    }
}
