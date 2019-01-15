#include <stdlib.h>
#include "ReykhaSensors.h"
#include <Arduino.h>


// Base class constructor
ReykhaSensors::ReykhaSensors() :
    calibratedMinimumOn(nullptr),
    calibratedMaximumOn(nullptr),
    calibratedMinimumOff(nullptr),
    calibratedMaximumOff(nullptr),
    _pins(nullptr)
{
    // empty
}


// Base class data member initialization (called by derived class init())
void ReykhaSensors::init(unsigned char *pins, unsigned char numSensors, unsigned char emitterPin)
{
    calibratedMinimumOn = nullptr;
    calibratedMaximumOn = nullptr;
    calibratedMinimumOff = nullptr;
    calibratedMaximumOff = nullptr;

    _lastValue = 0; // assume initially that the line is left.

    if (numSensors > Reykha_MAX_SENSORS)
        _numSensors = Reykha_MAX_SENSORS;
    else
        _numSensors = numSensors;

    if (_pins == nullptr)
    {
        _pins = (unsigned char*)malloc(sizeof(unsigned char)*_numSensors);
        if (_pins == nullptr)
            return;
    }

    unsigned char i;
    for (i = 0; i < _numSensors; i++)
    {
        _pins[i] = pins[i];
    }

    _emitterPin = emitterPin;
}



void ReykhaSensors::read(unsigned int *sensor_values, unsigned char readMode)
{
    unsigned char i;

    if (readMode == Reykha_EMITTERS_ON || readMode == Reykha_EMITTERS_ON_AND_OFF)
        emittersOn();
    else if (readMode == Reykha_EMITTERS_OFF)
        emittersOff();

    readPrivate(sensor_values);

    if (readMode != Reykha_EMITTERS_MANUAL)
        emittersOff();

    if (readMode == Reykha_EMITTERS_ON_AND_OFF)
    {
        unsigned int off_values[Reykha_MAX_SENSORS];
        readPrivate(off_values);

        for(i=0;i<_numSensors;i++)
        {
            sensor_values[i] += _maxValue - off_values[i];
        }
    }
}



void ReykhaSensors::emittersOff()
{
    if (_emitterPin == Reykha_NO_EMITTER_PIN)
        return;
    pinMode(_emitterPin, OUTPUT);
    digitalWrite(_emitterPin, LOW);
    delayMicroseconds(200);
}

void ReykhaSensors::emittersOn()
{
    if (_emitterPin == Reykha_NO_EMITTER_PIN)
        return;
    pinMode(_emitterPin, OUTPUT);
    digitalWrite(_emitterPin, HIGH);
    delayMicroseconds(200);
}


void ReykhaSensors::resetCalibration()
{
    unsigned char i;
    for(i=0;i<_numSensors;i++)
    {
        if(calibratedMinimumOn)
            calibratedMinimumOn[i] = _maxValue;
        if(calibratedMinimumOff)
            calibratedMinimumOff[i] = _maxValue;
        if(calibratedMaximumOn)
            calibratedMaximumOn[i] = 0;
        if(calibratedMaximumOff)
            calibratedMaximumOff[i] = 0;
    }
}


void ReykhaSensors::calibrate(unsigned char readMode)
{
    if(readMode == Reykha_EMITTERS_ON_AND_OFF || readMode == Reykha_EMITTERS_ON)
    {
        calibrateOnOrOff(&calibratedMinimumOn, &calibratedMaximumOn, Reykha_EMITTERS_ON);
    }


    if(readMode == Reykha_EMITTERS_ON_AND_OFF || readMode == Reykha_EMITTERS_OFF)
    {
        calibrateOnOrOff(&calibratedMinimumOff, &calibratedMaximumOff, Reykha_EMITTERS_OFF);
    }
}

void ReykhaSensors::calibrateOnOrOff(unsigned int **calibratedMinimum, unsigned int **calibratedMaximum, unsigned char readMode)
{
    int i;
    unsigned int sensor_values[16];
    unsigned int max_sensor_values[16];
    unsigned int min_sensor_values[16];

    // Allocate the arrays if necessary.
    if(*calibratedMaximum == 0)
    {
        *calibratedMaximum = (unsigned int*)malloc(sizeof(unsigned int)*_numSensors);

        // If the malloc failed, don't continue.
        if(*calibratedMaximum == 0)
            return;

        // Initialize the max and min calibrated values to values that
        // will cause the first reading to update them.

        for(i=0;i<_numSensors;i++)
            (*calibratedMaximum)[i] = 0;
    }
    if(*calibratedMinimum == 0)
    {
        *calibratedMinimum = (unsigned int*)malloc(sizeof(unsigned int)*_numSensors);

        // If the malloc failed, don't continue.
        if(*calibratedMinimum == 0)
            return;

        for(i=0;i<_numSensors;i++)
            (*calibratedMinimum)[i] = _maxValue;
    }

    int j;
    for(j=0; j<10; j++)
    {
        read(sensor_values, readMode);
        for(i=0;i<_numSensors;i++)
        {
            // set the max we found THIS time
            if(j == 0 || max_sensor_values[i] < sensor_values[i])
                max_sensor_values[i] = sensor_values[i];

            // set the min we found THIS time
            if(j == 0 || min_sensor_values[i] > sensor_values[i])
                min_sensor_values[i] = sensor_values[i];
        }
    }

    // record the min and max calibration values
    for(i=0;i<_numSensors;i++)
    {
        if(min_sensor_values[i] > (*calibratedMaximum)[i])
            (*calibratedMaximum)[i] = min_sensor_values[i];
        if(max_sensor_values[i] < (*calibratedMinimum)[i])
            (*calibratedMinimum)[i] = max_sensor_values[i];
    }
}



void ReykhaSensors::readCalibrated(unsigned int *sensor_values, unsigned char readMode)
{
    int i;

    // if not calibrated, do nothing
    if(readMode == Reykha_EMITTERS_ON_AND_OFF || readMode == Reykha_EMITTERS_OFF)
        if(!calibratedMinimumOff || !calibratedMaximumOff)
            return;
    if(readMode == Reykha_EMITTERS_ON_AND_OFF || readMode == Reykha_EMITTERS_ON)
        if(!calibratedMinimumOn || !calibratedMaximumOn)
            return;

    // read the needed values
    read(sensor_values,readMode);

    for(i=0;i<_numSensors;i++)
    {
        unsigned int calmin,calmax;
        unsigned int denominator;

        // find the correct calibration
        if(readMode == Reykha_EMITTERS_ON)
        {
            calmax = calibratedMaximumOn[i];
            calmin = calibratedMinimumOn[i];
        }
        else if(readMode == Reykha_EMITTERS_OFF)
        {
            calmax = calibratedMaximumOff[i];
            calmin = calibratedMinimumOff[i];
        }
        else // Reykha_EMITTERS_ON_AND_OFF
        {

            if(calibratedMinimumOff[i] < calibratedMinimumOn[i]) // no meaningful signal
                calmin = _maxValue;
            else
                calmin = calibratedMinimumOn[i] + _maxValue - calibratedMinimumOff[i]; // this won't go past _maxValue

            if(calibratedMaximumOff[i] < calibratedMaximumOn[i]) // no meaningful signal
                calmax = _maxValue;
            else
                calmax = calibratedMaximumOn[i] + _maxValue - calibratedMaximumOff[i]; // this won't go past _maxValue
        }

        denominator = calmax - calmin;

        signed int x = 0;
        if(denominator != 0)
            x = (((signed long)sensor_values[i]) - calmin) * 1000 / denominator;
        if(x < 0)
            x = 0;
        else if(x > 1000)
            x = 1000;
        sensor_values[i] = x;
    }

}



int ReykhaSensors::readLine(unsigned int *sensor_values, unsigned char readMode, unsigned char white_line)
{
    unsigned char i, on_line = 0;
    unsigned long avg; // this is for the weighted total, which is long before division
    unsigned int sum; // this is for the denominator which is <= 64000

    readCalibrated(sensor_values, readMode);

    avg = 0;
    sum = 0;

    for(i=0; i< _numSensors; i++) {
        int value = sensor_values[i];
        if(white_line)
            value = 1000- value;

        // keep track of whether we see the line at all
        if(value > 200) {
            on_line = 1;
        }

        // only average in values that are above a noise threshold
        if(value > 50) {
            avg += (long)(value) * (i * 1000);
            sum += value;
        }
    }

    if(!on_line)
    {
        // If it last read to the left of center, return 0.
        if(_lastValue < (_numSensors-1)*1000/2)
            return 0;

        // If it last read to the right of center, return the max.
        else
            return (_numSensors-1)*1000;

    }

    _lastValue = avg/sum;

    return _lastValue;
}

















// Derived Analog class constructor
ReykhaSensorsAnalog::ReykhaSensorsAnalog(unsigned char* analogPins, unsigned char numSensors, unsigned char numSamplesPerSensor, unsigned char emitterPin)
{
    init(analogPins, numSensors, numSamplesPerSensor, emitterPin);
}


void ReykhaSensorsAnalog::init(unsigned char* analogPins, unsigned char numSensors, unsigned char numSamplesPerSensor, unsigned char emitterPin)
{
    ReykhaSensors::init(analogPins, numSensors, emitterPin);

    _numSamplesPerSensor = numSamplesPerSensor;
    _maxValue = 1023; // this is the maximum returned by the A/D conversion
}



void ReykhaSensorsAnalog::readPrivate(unsigned int *sensor_values, unsigned char step, unsigned char start)
{
    unsigned char i, j;

    if (_pins == 0)
        return;

    // reset the values
    for(i = start; i < _numSensors; i += step)
        sensor_values[i] = 0;

    for (j = 0; j < _numSamplesPerSensor; j++)
    {
        for (i = start; i < _numSensors; i += step)
        {
            sensor_values[i] += analogRead(_pins[i]);   // add the conversion result
        }
    }

    // get the rounded average of the readings for each sensor
    for (i = start; i < _numSensors; i += step)
        sensor_values[i] = (sensor_values[i] + (_numSamplesPerSensor >> 1)) / _numSamplesPerSensor;
}


// the destructor frees up allocated memory
ReykhaSensors::~ReykhaSensors()
{
    if (_pins)
        free(_pins);
    if(calibratedMaximumOn)
        free(calibratedMaximumOn);
    if(calibratedMaximumOff)
        free(calibratedMaximumOff);
    if(calibratedMinimumOn)
        free(calibratedMinimumOn);
    if(calibratedMinimumOff)
        free(calibratedMinimumOff);
}
