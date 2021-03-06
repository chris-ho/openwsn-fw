/**
\brief z1-specific definition of the "uart" bsp module.

\author Xavier Vilajosana <xvilajosana@eecs.berkeley.edu>, May 2013.
*/

#include "msp430x26x.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "uart.h"

//=========================== defines =========================================

//=========================== variables =======================================

typedef struct {
    uart_tx_cbt txCb;
    uart_rx_cbt rxCb;
    bool        fXonXoffEscaping;
    uint8_t     xonXoffEscapedByte;
} uart_vars_t;

uart_vars_t uart_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void uart_init(void) {
    // reset local variables
    memset(&uart_vars,0,sizeof(uart_vars_t));
    
    //initialize UART openserial_vars.mode
    P3SEL    |=  0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
//   UCA0CTL0  = 0;
//   UCA0CTL1  = 0;
    UCA0CTL1 |=  UCSSEL_2;                         // CLK = SMCL
    UCA0BR0   =  0x45;                             // 115200 baud if SMCLK@8MHz (
    UCA0BR1   =  0x00;
    UCA0MCTL  = UCBRS1 + UCBRS0;                    // Modulation UCBRSx = 7
    UCA0CTL1 &= ~UCSWRST;                          // Initialize USCI state machine
    //UC1IFG   &= ~(UCA1TXIFG | UCA1RXIFG);          // clear possible pending interrupts
    //UC1IE    |=  (UCA1RXIE  | UCA1TXIE);           // Enable USCI_A1 TX & RX interrupt  
}

void    uart_setCallbacks(uart_tx_cbt txCb, uart_rx_cbt rxCb) {
    uart_vars.txCb = txCb;
    uart_vars.rxCb = rxCb;
}

void    uart_enableInterrupts(void) {
    UC0IE    |=  (UCA0RXIE  | UCA0TXIE);  
}

void    uart_disableInterrupts(void) {
    UC0IE &= ~(UCA0RXIE | UCA0TXIE);
}

void    uart_clearRxInterrupts(void) {
    UC0IFG   &= ~(UCA0RXIFG);
}

void    uart_clearTxInterrupts(void) {
    UC0IFG   &= ~(UCA0TXIFG);
}

void    uart_setCTS(bool state) {
    if (state==0x01) {
        UCA0TXBUF = XON;
    } else {
        UCA0TXBUF = XOFF;
    }
}

void    uart_writeByte(uint8_t byteToWrite){
    if (byteToWrite==XON || byteToWrite==XOFF || byteToWrite==XONXOFF_ESCAPE) {
        uart_vars.fXonXoffEscaping     = 0x01;
        uart_vars.xonXoffEscapedByte   = byteToWrite;
        UCA0TXBUF = XONXOFF_ESCAPE;
    } else {
        UCA0TXBUF = byteToWrite;
    }
}

uint8_t uart_readByte(void) {
    return UCA0RXBUF;
}

//=========================== interrupt handlers ==============================

kick_scheduler_t uart_tx_isr(void) {
    uart_clearTxInterrupts(); // TODO: do not clear, but disable when done
    uart_vars.txCb();
    if (uart_vars.fXonXoffEscaping==0x01) {
        uart_vars.fXonXoffEscaping = 0x00;
        UCA0TXBUF = uart_vars.xonXoffEscapedByte^XONXOFF_MASK;
    } else {
        uart_vars.txCb();
    }
    return DO_NOT_KICK_SCHEDULER;
}

kick_scheduler_t uart_rx_isr(void) {
    uart_clearRxInterrupts(); // TODO: do not clear, but disable when done
    uart_vars.rxCb();
    return DO_NOT_KICK_SCHEDULER;
}