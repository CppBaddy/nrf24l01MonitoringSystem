/*
 * Uart.c
 *
 *  Created on: 2020-08-28
 *      Author: yulay
 */
//
//#include <avr/io.h>
//#include <avr/interrupt.h>
//
//#include "Uart.h"
//#include "PortConfig.h"
//
//void Uart_RxReset();
//
//void Uart_TxStart();
//void Uart_TxStop();
//void Uart_SetTxPort(uint8_t b);
//
//volatile struct Uart uart;
//
//
//void Uart_Init()
//{
//    uart.rxState = Idle;
//    uart.rxFifo.head = 0;
//    uart.rxFifo.tail = 0;
//
//    uart.txState = Idle;
//    uart.txFifo.head = 0;
//    uart.txFifo.tail = 0;
//
//    UART_PORT |= _BV(RxPAD); //enable pull-up on Rx pin
//    UART_DDR  |= _BV(TxPAD); //set Tx to output mode
//
//    Uart_RxReset();
//    Uart_SetTxPort(1); //SPACE
//}
//
//void Uart_RxStart()
//{
//    //TCNT0  = 0;                    //clear timer0
//    //GTCCR &= ~_BV(TSM);            //start timer0 and clear prescaler
//#if  TIMER1_RELOAD > (0xffff - (TIMER1_RELOAD/2))
//#error  This math does not work for timer reload value > 168
//#endif
//    OCR1B  = (TCNT1 + TIMER1_RELOAD/2) % TIMER1_RELOAD; //capture exact middle of the bit
//
//    TIMSK1 |= _BV(OCIE1B);          //enable CompareMatch1B interrupt
//
//    uart.rxState = StartFrame;
//
//    PCMSK1 &= ~_BV(RxPAD);       //disable pc interrupt for Rx
//}
//
//void Uart_RxReset()
//{
//    //GTCCR |= _BV(TSM) | _BV(PSR1); //stop timer1
//    TIMSK1 &= ~_BV(OCIE1B);         //disable CompareMatch1B interrupt
//    PCMSK1 |= _BV(RxPAD);          //enable pc interrupt for Rx
//    uart.rxState = Idle;
//}
//
//bool Uart_RxReady()
//{
//    return uart.rxFifo.tail != uart.rxFifo.head;
//}
//
//uint8_t Uart_GetByte()
//{
//    uint8_t b = uart.rxFifo.buf[uart.rxFifo.tail];
//    uart.rxFifo.tail = (uart.rxFifo.tail + 1) % MAX_FIFO_BUF_SIZE;
//    return b;
//}
//
//
//void Uart_PutByte(uint8_t b)
//{
//    uint8_t head = (uart.txFifo.head + 1) % MAX_FIFO_BUF_SIZE;
//
//    while(head == uart.txFifo.tail) //busy wait for free space
//    {}
//
//    uart.txFifo.buf[uart.txFifo.head] = b;
//
//    uart.txFifo.head = head;
//
//    Uart_TxStart();
//}
//
//void Uart_PutStr(const char* s)
//{
//    for( ;*s; ++s)
//    {
//        Uart_PutByte(*s);
//    }
//}
//
//bool Uart_TxBusy()
//{
//    return uart.rxFifo.tail != uart.rxFifo.head;
//}
//
//inline bool Uart_TxDone()
//{
//    return uart.rxFifo.tail == uart.rxFifo.head;
//}
//
//inline void Uart_TxStart()
//{
//    TIMSK1 |= _BV(OCIE1A);          //enable interrupt for CompareMatch1A
//}
//
//inline void Uart_TxStop()
//{
//    TIMSK1 &= ~_BV(OCIE1A);          //disable interrupt for CompareMatch1A
//}
//
//void Uart_SetTxPort(uint8_t b)
//{
//    if(b) //logic polarity control
//    {
//        UART_PORT |= _BV(TxPAD);
//    }
//    else
//    {
//        UART_PORT &= ~_BV(TxPAD);
//    }
//}
//
//uint8_t calcParityBit(uint8_t b)
//{
//    b ^= (b >> 4);
//    b ^= (b >> 2);
//    b ^= (b >> 1);
//
//    return b & 1;
//}
//
//
///* Pin change interrupt handler */
//ISR( PCINT1_vect )
//{
//    if(PCMSK1 & _BV(RxPAD)) //UART Rx
//    {
//        if((UART_PORT & _BV(RxPAD)) == 0)
//        {
//            Uart_RxStart();
//        }
//    }
//}
//
///* Timer1 compare match A interrupt handler */
////UART transmit driver
//ISR( TIM1_COMPA_vect )
//{
//    if(uart.txState == Idle)
//    {
//        if(uart.txFifo.tail != uart.txFifo.head)
//        {
//            uart.txBuf = uart.txFifo.buf[uart.txFifo.tail];
//            uart.txState = StartFrame;
//            uart.txFifo.tail = (uart.txFifo.tail + 1) % MAX_FIFO_BUF_SIZE;
//
////            uart.txParity = calcParityBit(uart.txBuf);
//        }
//    }
//    else
//    {
//        ++(uart.txState); //next state
//
//        switch(uart.txState)
//        {
//        case StartBit:
//            Uart_SetTxPort(0);
//            break;
//        case Bit0:
//        case Bit1:
//        case Bit2:
//        case Bit3:
//        case Bit4:
//        case Bit5:
//        case Bit6:
//        case Bit7:
//            Uart_SetTxPort(uart.txBuf & 1);
//            uart.txBuf >>= 1;
//            break;
//        case ParityBit:
////            Uart_SetTxPort(uart.txParity);
//            Uart_SetTxPort(1);
//            break;
//        case StopBit2:
//        {
//            Uart_SetTxPort(1);
//
//            volatile struct Fifo* fifo = &uart.txFifo;
//
//            if(fifo->tail == fifo->head) //no more data to send
//            {
//                Uart_TxStop();
//            }
//
//            uart.txState = Idle;
//            break;
//        }
//        default:
//            break;
//        }
//    }
//}
//
///* Timer0 compare match B interrupt handler */
////UART receive driver
//ISR( TIM1_COMPB_vect )
//{
//    if(uart.rxState != Idle)
//    {
//        ++(uart.rxState); //next state
//
//        switch(uart.rxState)
//        {
//        case StartBit:
//            if(UART_PIN & _BV(RxPAD)) //false start?
//            {
//                Uart_RxReset();
//            }
//            break;
//        case Bit0:
//        case Bit1:
//        case Bit2:
//        case Bit3:
//        case Bit4:
//        case Bit5:
//        case Bit6:
//        case Bit7:
//            uart.rxBuf >>= 1;
//            if(UART_PIN & _BV(RxPAD))
//            {
//                uart.rxBuf |= 0x80;
//            }
//            break;
//        case ParityBit:
////            if(calcParityBit(uart.rxBuf) != ((PINB & _BV(UART_RxPORT)) >> UART_RxPORT)) //parity error
////            {
////                Uart_RxReset();
////            }
////            break;
//        case StopBit2:
//            if(UART_PIN & _BV(RxPAD))
//            {
//                volatile struct Fifo* fifo = &uart.rxFifo;
//
//                //NOTE:
//                // No check for available space!
//                // Main loop must read out faster then new data come
//                fifo->buf[fifo->head] = uart.rxBuf;
//                fifo->head = (fifo->head + 1) % MAX_FIFO_BUF_SIZE;
//            }
//
//            Uart_RxReset();
//            break;
//        default:
//            break;
//        }
//    }
//}
