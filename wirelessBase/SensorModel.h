/*
 * SensorModel.h
 *
 *  Created on: 2020-09-01
 *      Author: yulay
 */

#ifndef SENSORMODEL_H_
#define SENSORMODEL_H_



typedef struct SensorData
{
    uint8_t sensorId;
    uint8_t alarmState;
    uint8_t inputs;
    uint8_t batteryVoltage;
} SensorModel;


enum inputFlags
{
    NoInput     = 0,
    PhoneRing   = 1,
    Sensor1     = 2,
    Sensor2     = 4,
    Sensor3     = 8,
    Sensor4     = 16,
};



#endif /* SENSORMODEL_H_ */
