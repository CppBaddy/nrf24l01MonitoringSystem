#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "PortConfig.h"
#include "Receiver.h"
#include "SensorModel.h"
#include "ws2811.h"
#include "Color24.h"
//#include "Uart.h"


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

inline void RedOn()
{
    PORTB &= ~_BV(PB0);
}
inline void RedOff()
{
    PORTB |= _BV(PB0);
}

inline void GreenOn()
{
    PORTB &= ~_BV(PB1);
}
inline void GreenOff()
{
    PORTB |= _BV(PB1);
}

inline void BlueOn()
{
    PORTB &= ~_BV(PB2);
}
inline void BlueOff()
{
    PORTB |= _BV(PB2);
}

void setup();

volatile bool nrfIRQ; //RF data received flag

volatile bool phoneRing;

#define kSensorsMax     8

volatile SensorModel model;
uint8_t timeout;

volatile uint8_t timerZero;
volatile bool animate;

uint8_t prevPinA;

enum { Normal, Alarm };

Color24 black[] = {
        {0, 0, 0}, { 0, 0, 0}, {  0, 0, 0}, { 0, 0, 0}
};

Color24 color[8] = {
        {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255},
        {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
};

int main()
{
    uint8_t state = Normal;
    uint8_t step = 0;

    setup();

    prevPinA = PINA;

    RF_Init();

    WS2811_Send((uint8_t*)black, 4); //switch off LED strip

    for(;;)
    {
        if(nrfIRQ) //Receive RF sensor state
        {
            nrfIRQ = false;

            /*uint8_t s =*/ RF_ClearIRQ();
            //TODO use s[3:1] as pipe # or index

            RF_Receive((uint8_t*)&model, sizeof(SensorModel));

            if(model.alarmState)
            {
                if(Alarm != state)
                {
                    state = Alarm;
                    animate = true;
                    timeout = 24;

                    Timer0_Enable();
                }
            }

            if(model.batteryVoltage && model.batteryVoltage > LOW_BATTERY_VOLTAGE) //93 - 3V; 140 - 2V
            {
                uint8_t led = model.sensorId % 3;

                switch(led)
                {
                    case 0:
                        RedOn();
                        break;
                    case 1:
                        GreenOn();
                        break;
                    case 2:
                    default:
                        BlueOn();
                        break;
                }
                //TODO start LED flashing
            }
        }

        if(Alarm == state && animate)
        {
            animate = false;

            if(timeout)
            {
                --(timeout);

                if(0 == timeout)
                {
                    model.alarmState = 0;
                    state = Normal;
                    Timer0_Disable();

                    WS2811_Send((uint8_t*)black, 4); //switch off LED strip
                    step = 0;
                }
                else
                {
                    if(step)
                    {
                        WS2811_Send((uint8_t*)black, 4); //switch off LED strip
                        step = 0;
                    }
                    else
                    {
                        WS2811_Send((uint8_t*)color + (model.sensorId % 3), 4);
                        step = 1;
                    }
                }
            }
        }

        set_sleep_mode(SLEEP_MODE_IDLE); //to keep Timer0 & Timer1 running

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
    CLKPR = 0; //change prescaler to 1, effectively set 8 MHz system clock

    //setup port A
    //PA0 - input IRQ from nRF24L01+
    //PA1 - reserve
    //PA2 - ws2811 driver
    //PA3 - CE chip enable for nRF24L01+
    //PA4 - SCK output for nRF24L01+
    //PA5 - MISO input from nRF24L01+
    //PA6 - MOSI output for nRF24L01+
    //PA7 - CSN chip select negative nRF24L01+
    DDRA |= _BV(PA2) | _BV(PA3) | _BV(PA4) | _BV(PA6) | _BV(PA7); /* set PAx as output */
    PORTA |= _BV(PA1) | _BV(PA7); //enable pull-ups and disable CSN

    //setup port B
    //PB0 - output RED LED
    //PB1 - output GREEN LED
    //PB2 - output BLUE LED
    //PB3 - Reset
    DDRB |= _BV(PB0) | _BV(PB1) | _BV(PB2); //enable output
    PORTB |= _BV(PB0) | _BV(PB1) | _BV(PB2); //LEDs off

    //Interrupt inputs
    PCMSK0 = _BV(PCINT0); //enable mask for PA0 input
    GIFR  |= _BV(PCIF0);  //clear pin change interrupt flags
    GIMSK |= _BV(PCIE0);  //enable PortA pin change interrupt

    //Timer0 update timer
    TCCR0A = 0;                      //normal mode
    TCCR0B  = _BV(CS02) | _BV(CS00); //prescaler 1024
    TIMSK0  |= _BV(TOIE0);           //enable TOVF interrupt

    //Timer1 uart baud rate driver
//    TCCR1A = 0;   //CTC mode, TOP is OCR1A
//    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); //prescaler clk/1
//    TCCR1C = 0;
//
//    TCNT1 = 0;
//
//    OCR1A = TIMER1_RELOAD/2;         //transmit out on COMPA
//    OCR1B = TIMER1_RELOAD/2;       //read out in the middle of the bit
//    ICR1 = TIMER1_RELOAD;
//    //TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1A) | _BV(OCIE1A); //enable TOVF interrupt
//
//    Uart_Init();

    PRR |= _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSI) | _BV(PRADC); //disable timers, usi and adc clocks

    sei(); //Enable interrupts
}

/* External input interrupt handler */
//ISR( INT0_vect )
//{}

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
ISR( TIM0_OVF_vect ) // at ~30 Hz
{
    ++timerZero;

    if((timerZero & 0x07) == 0) // at ~8 Hz
    {
        animate = true;
    }
}

// ADC interrupt service routine
//ISR( ADC_vect )
//{}

