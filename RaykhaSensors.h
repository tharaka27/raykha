#ifndef RaykhaSensors_h
#define RaykhaSensors_h

#define Raykha_EMITTERS_OFF              0
#define Raykha_EMITTERS_ON               1
#define Raykha_EMITTERS_ON_AND_OFF       2
#define Raykha_EMITTERS_ODD_EVEN         3
#define Raykha_EMITTERS_ODD_EVEN_AND_OFF 4
#define Raykha_EMITTERS_MANUAL           5

#define Raykha_NO_EMITTER_PIN  255

#define Raykha_BANK_ODD  1
#define Raykha_BANK_EVEN 2

#define Raykha_MAX_SENSORS 8 // since Raykha has maximum 8 sensors

// Base class: this class cannot be instantiated directly.
// Instead, you should instantiate one of its derived classes.
class RaykhaSensors
{
  public:

    
    virtual void read(unsigned int *sensor_values, unsigned char readMode = Raykha_EMITTERS_ON);
    virtual void emittersOff();
    virtual void emittersOn();

    void calibrate(unsigned char readMode = Raykha_EMITTERS_ON);
	void resetCalibration();
	void readCalibrated(unsigned int *sensor_values, unsigned char readMode = Raykha_EMITTERS_ON);

    int readLine(unsigned int *sensor_values, unsigned char readMode = Raykha_EMITTERS_ON, unsigned char white_line = 0);

    unsigned int *calibratedMinimumOn;
    unsigned int *calibratedMaximumOn;
    unsigned int *calibratedMinimumOff;
    unsigned int *calibratedMaximumOff;

    RaykhaSensors();

  protected:

    RaykhaSensors();

    virtual void init(unsigned char *pins, unsigned char numSensors, unsigned char emitterPin);

    virtual void readPrivate(unsigned int *sensor_values, unsigned char step = 1, unsigned char start = 0) = 0;

    unsigned char *_pins;
    unsigned char _numSensors;
    unsigned char _emitterPin;
    unsigned int _maxValue; 

  private:

    
    void calibrateOnOrOff(unsigned int **calibratedMinimum, unsigned int **calibratedMaximum, unsigned char readMode);

    int _lastValue;
};



class RaykhaSensorsAnalog : virtual public RaykhaSensors
{
  public:

    
    RaykhaSensorsAnalog() {}

    
    RaykhaSensorsAnalog(unsigned char* analogPins, unsigned char numSensors, unsigned char numSamplesPerSensor = 4, unsigned char emitterPin = Raykha_NO_EMITTER_PIN);

    virtual void init(unsigned char* analogPins, unsigned char numSensors, unsigned char numSamplesPerSensor = 4, unsigned char emitterPin = Raykha_NO_EMITTER_PIN);

  protected:

    unsigned char _numSamplesPerSensor;

  private:

    void readPrivate(unsigned int *sensor_values, unsigned char step = 1, unsigned char start = 0) override;
};


#endif