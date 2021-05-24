#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "PortConfig.h"
#include "Uart.h"
#include "Transmitter.h"


inline void Timer0_Enable()
{
    PRR &= ~_BV(PRTIM0);
    TCNT0 = 0;
}
inline void Timer0_Disable()
{
    PRR |= _BV(PRTIM0);
}

inline void ADC_Enable()
{
    PRR &= ~_BV(PRADC); //power on ADC
    ADCSRA |= _BV(ADEN) | _BV(ADIE) | _BV(ADPS2); //prescaler 16, enable interrupt
    ADCSRB |= _BV(ADLAR);
}
inline void ADC_Disable()
{
    PRR |= _BV(PRADC); //power off ADC
}
inline void AdcIn_VBat()
{
    //input internal bandgap 1.1V voltage reference
    //vbat as reference
    ADMUX = _BV(MUX5) | _BV(MUX0);
}
inline void ADC_Start()
{
    ADCSRA |= _BV(ADSC); //start conversion
}

void SendAlarmState();
void setup();

volatile bool nrfIRQ; //RF data sent flag
volatile bool phoneRing;
volatile bool stateRequest;
volatile bool stateReady;

volatile SensorModel model;

volatile uint8_t timerZero;

uint8_t prevPinA;

enum { Normal, Alarm };


int main()
{
    setup();

    prevPinA = PINA;

    RF_Init();

    model.sensorId = SENSOR_ID;
    model.alarmState = Normal;

    SendAlarmState();

    for(;;)
    {
        if(Normal == model.alarmState)
        {
            if(phoneRing)
            {
                phoneRing = false;

                model.inputs |= PhoneRing;
                model.alarmState = Alarm;

                Timer0_Enable();

                ADC_Enable();
                AdcIn_VBat();

                ADC_Start();

                SendAlarmState();
            }
        }
        else if(Alarm == model.alarmState)
        {
            if(stateRequest)
            {
                stateRequest = false;

                ADC_Start();

                if(false == phoneRing)
                {
                    model.alarmState = Normal;
                    model.inputs &= ~PhoneRing;

                    Timer0_Disable();
                }

                phoneRing = false;

                SendAlarmState();
            }
        }

        if(nrfIRQ)
        {
            nrfIRQ = false;

            RF_ClearIRQ();

            RF_PowerDown();
        }

        if(stateReady)
        {
            SendAlarmState();
        }

        if(Alarm == model.alarmState /*|| Uart_TxBusy()*/)
        {
            set_sleep_mode(SLEEP_MODE_IDLE); //to keep Timer0 & Timer1 running
        }
        else if(Normal == model.alarmState)
        {
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        }

        sleep_enable();

            sleep_cpu();

        sleep_disable();
    }

    return 0;
}

void setup()
{
    cli();

    //switch to 8 MHz clock
    CLKPR = _BV(CLKPCE); //enable Clock Prescale Register write
    CLKPR = _BV(CLKPS0); //change prescaler to 2, effectively set 4 MHz system clock

    //setup port A
    //PA0 - input IRQ from nRF24L01+
    //PA1 - input from Phone Ring
    //PA2 - input/output (reserve)
    //PA3 - CE chip enable for nRF24L01+
    //PA4 - SCK output for nRF24L01+
    //PA5 - MISO input from nRF24L01+
    //PA6 - MOSI output for nRF24L01+
    //PA7 - CSN chip select negative nRF24L01+
    DDRA |= _BV(PA3) | _BV(PA4) | _BV(PA6) | _BV(PA7); /* set PAx as output */
    PORTA |= _BV(PA1) | _BV(PA2) | _BV(PA7); //enable pull-ups and disable CSN

    //setup port B
    //PB0 - input/output (reserve)
    //PB1 - input/output (reserve)
    //PB2 - input/output (reserve)
    //PB3 - Reset
    PORTB |= _BV(PB0) | _BV(PB1) | _BV(PB2); //enable pull-ups

    //Interrupt inputs
    PCMSK0 = _BV(PCINT0) | _BV(PCINT1); //enable mask for PA0 & PA1 inputs
    GIFR  |= _BV(PCIF0) | _BV(PCIF1);    //clear pin change interrupt flags
    GIMSK |= _BV(PCIE0);     //enable PortA pin change interrupt

    //Timer0 update timer
    //GTCCR  |= _BV(TSM) | _BV(PSR0); //stop timer0 and clear prescaler
    TCCR0A = 0;                      //normal mode
    TCCR0B  = _BV(CS02) | _BV(CS00); //prescaler 1024
    TIMSK0  |= _BV(TOIE0);           //enable TOVF interrupt

    //Timer1 uart baud rate driver
    TCCR1A = 0;   //CTC mode, TOP is OCR1A
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); //prescaler clk/1
    TCCR1C = 0;

    TCNT1 = 0;

    OCR1A = TIMER1_RELOAD/2;         //transmit out on COMPA
    OCR1B = TIMER1_RELOAD/2;       //read out in the middle of the bit
    ICR1 = TIMER1_RELOAD;
    //TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1A) | _BV(OCIE1A); //enable TOVF interrupt

    //Uart_Init();

    PRR |= /*_BV(PRTIM1) |*/ _BV(PRTIM0) | _BV(PRUSI) | _BV(PRADC); //disable timers, usi and adc clocks

    sei(); //Enable interrupts
}

void SendAlarmState()
{
    RF_Transmit((uint8_t*)&model, sizeof(model));

//    if(Normal == model.alarmState)
//    {
//        Uart_PutStr("\n\rBack to normal");
//    }
//    else
//    {
//        Uart_PutStr("\n\rAlarm!");
//    }
}


/* External input interrupt handler */
//ISR( INT0_vect )
//{
//}

/* Pin change interrupt handler */
ISR( PCINT0_vect )
{
    uint8_t pinA = PINA;
    uint8_t changed = pinA ^ prevPinA;

    prevPinA = pinA;

    if((changed & _BV(NRF_IRQ)) & PCMSK0) //enabled IRQ from nRF
    {
        if((pinA & _BV(NRF_IRQ)) == 0) //active low
        {
            nrfIRQ = true;
        }
    }

    if((changed & _BV(PA1)) & PCMSK0) //enabled Phone Ringing
    {
        if((pinA & _BV(PA1)) == 0) //active low
        {
            phoneRing = true;
        }
    }
}

/* Pin change interrupt handler */
//ISR( PCINT1_vect )
//implemented in Uart.c

/* Timer1 interrupt handler */
//ISR( TIMER1_COMPA_vect )
//implemented in Uart.c

/* Timer1 interrupt handler */
//ISR( TIMER1_OVF_vect ) // each 67sec = 1MHz / 1024 / 0xffff
//{}

/* Timer0 interrupt handler */
ISR( TIM0_OVF_vect ) // at ~15 Hz
{
    ++timerZero;

    if((timerZero & 3) == 0) // at ~4 Hz
    {
        stateRequest = true;

        ADC_Enable();
        AdcIn_VBat();
    }
}

// ADC interrupt service routine
ISR( ADC_vect )
{
    model.batteryVoltage = ADCH;

    ADC_Disable();
}

