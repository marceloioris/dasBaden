#include <Arduino.h>
#include <inttypes.h>

class Thread{
protected:
	// Desired interval between runs
	unsigned long interval;

	// Last runned time in Ms
	unsigned long last_run;

	// Scheduled run in Ms (MUST BE CACHED)	
	unsigned long _cached_next_run;

	/*
		IMPORTANT! Run after all calls to run()
		Updates last_run and cache next run.
		NOTE: This MUST be called if extending
		this class and implementing run() method
	*/
	void runned(unsigned long time);

	// Default is to mark it runned "now"
	void runned() { runned(millis()); }

	// Callback for run() if not implemented
	void (*_onRun)(void);		

public:
	// If the current Thread is enabled or not
	bool enabled;

	// ID of the Thread (initialized from memory adr.)
	int ThreadID;

	Thread(void (*callback)(void) = NULL, unsigned long _interval = 0);

	// Set the desired interval for calls, and update _cached_next_run
	virtual void setInterval(unsigned long _interval);

	// Return if the Thread should be runned or not
	virtual bool shouldRun(unsigned long time);

	// Default is to check whether it should run "now"
	bool shouldRun() { return shouldRun(millis()); }

	// Callback set
	void onRun(void (*callback)(void));

	// Runs Thread
	virtual void run();
};

Thread::Thread(void (*callback)(void), unsigned long _interval){
	enabled = true;
	onRun(callback);
	_cached_next_run = 0;
	last_run = millis();

	ThreadID = (int)this;
	#ifdef USE_THREAD_NAMES
		ThreadName = "Thread ";
		ThreadName = ThreadName + ThreadID;
	#endif

	setInterval(_interval);
};

void Thread::runned(unsigned long time){
	// Saves last_run
	last_run = time;

	// Cache next run
	_cached_next_run = last_run + interval;
}

void Thread::setInterval(unsigned long _interval){
	// Save interval
	interval = _interval;

	// Cache the next run based on the last_run
	_cached_next_run = last_run + interval;
}

bool Thread::shouldRun(unsigned long time){
	// If the "sign" bit is set the signed difference would be negative
	bool time_remaining = (time - _cached_next_run) & 0x80000000;

	// Exceeded the time limit, AND is enabled? Then should run...
	return !time_remaining && enabled;
}

void Thread::onRun(void (*callback)(void)){
	_onRun = callback;
}

void Thread::run(){
	if(_onRun != NULL)
		_onRun();

	// Update last_run and _cached_next_run
	runned();
}

/**********************************************************/

void checkWhetherThreadsShouldRun(void);
void send(char *message);
void analyseMessage(void);

Thread threadButtons = Thread(); // Digital Pins Thread
void threadB2C1Callback(void);

Thread threadTemperatureSensors = Thread(); // Temperature Sensor (Bathroom 2 Shower 1) Thread
void threadTemperatureSensorsCallback(void);

Thread threadWarmer = Thread();
void threadWarmerCallback(void);

Thread threadCooling = Thread();
void threadCoolingCallback(void);

const unsigned int coolingPeriod = 10000; // Universal Cooling Period

#define NUMBER_OF_POINTS 5

unsigned int buttons[NUMBER_OF_POINTS] = {2,  // C1T1
                           			      3,  // B1T1
                                          6,  // B1T2
                           			      4,  // B2T1
                                          5}; // B2C1

unsigned int buttonsLED[NUMBER_OF_POINTS] = {8,   // C1T1
                                             9,   // B1T1
                                             12,  // B1T2	
                                             10,  // B2T1
                                             11}; // B2C1

unsigned int tempSensor[NUMBER_OF_POINTS] = {0,
                                             0,
                                             0,
                                             0,
                                             A3}; // B2C1

bool hotPlaces[NUMBER_OF_POINTS] = {false,  // C1T1
                                    false,  // B1T1
                                    false,  // B1T2
                                    false,  // B2T1
                                    false}; // B2C1

bool toBeActivatedPlaces[NUMBER_OF_POINTS] = {false,  // C1T1
                                              false,  // B1T1
                                              false,  // B1T2
                                              false,  // B2T1
                                              false}; // B2C1

unsigned long HeatPeriod[NUMBER_OF_POINTS] = {1000,  // C1T1
                                              3000,  // B1T1
                                              4000,  // B1T2
                                              6000,  // B2T1
                                              10000}; // B2C1

unsigned long ConsumerPeriod[NUMBER_OF_POINTS] = {2000,  // C1T1
                                                  2000,  // B1T1
                                                  2000,  // B1T2
                                                  2000,  // B2T1
                                                  3000}; // B2C1

unsigned long buttonActivationMilli[NUMBER_OF_POINTS] = {0,  // C1T1
                                                         0,  // B1T1
                                                         0,  // B1T2
                                                         0,  // B2T1
                                                         0}; // B2C1

unsigned long placeHeatedMilli[NUMBER_OF_POINTS] = {0,  // C1T1
                                                    0,  // B1T1
                                                    0,  // B1T2
                                                    0,  // B2T1
                                                    0}; // B2C1

unsigned long minShowerTemp = 30;
  
char *msg = "0000";

void setup()
{
  Serial.begin(115200);

  for(int buttonIdx = 0; buttonIdx < NUMBER_OF_POINTS; buttonIdx++)
  {
    pinMode(buttons[buttonIdx], INPUT_PULLUP);
    pinMode(buttonsLED[buttonIdx], OUTPUT);
    pinMode(tempSensor[buttonIdx],INPUT);
    digitalWrite(buttonsLED[buttonIdx], LOW);
  }
  
  threadButtons.onRun(threadButtonsCallback);
  threadButtons.setInterval(200);

  threadTemperatureSensors.onRun(threadTemperatureSensorsCallback);
  threadTemperatureSensors.setInterval(200);
  
  threadWarmer.onRun(threadWarmerCallback);
  threadWarmer.setInterval(200);
  
  threadCooling.onRun(threadCoolingCallback);
  threadCooling.setInterval(200);
}

void loop()
{
  checkWhetherThreadsShouldRun();

  delay(1);
}

void threadButtonsCallback(void)
{
    int buttonIdx;
    for(buttonIdx = 0; buttonIdx < NUMBER_OF_POINTS; buttonIdx++)
    {
	  if(digitalRead(buttons[buttonIdx]))
      {
        Serial.print("Button ");
        Serial.print(buttonIdx, DEC);
        Serial.println(" pressed!");

        if(hotPlaces[buttonIdx] == false)
        {
          for(int i = 0; i <= buttonIdx; i++)
          {
            toBeActivatedPlaces[i] = true;
            
            if(buttonActivationMilli[i] == 0)
            {
              buttonActivationMilli[i] = millis();

              if (i > 0)
              {
              	buttonActivationMilli[i] += HeatPeriod[i - 1];
              }
            }
          }
        }
        else
        {
          unsigned long currentMillis = millis();

          for(int i = 0; i <= buttonIdx; i++)
          {
            toBeActivatedPlaces[i] = true;
            
            placeHeatedMilli[i] = currentMillis;
          }
        }
      }
    }
}

void threadTemperatureSensorsCallback(void)
{
  int buttonIdx;
  for(buttonIdx = 0; buttonIdx < NUMBER_OF_POINTS; buttonIdx++)
  {
    if(tempSensor[buttonIdx] != 0)
    {
      int tmp = analogRead(tempSensor[buttonIdx]);

      float voltage = (tmp * 5.0)/1024;//(5*temp)/1024 is to convert the 10 bit number to a voltage reading.
      float milliVolt = voltage * 1000;//This is multiplied by 1000 to convert it to millivolt.

      float tempSensor =  (milliVolt-500)/10;

      if(tempSensor >= minShowerTemp)
      {
        unsigned long currentMillis = millis();

        for(int i = 0; i <= buttonIdx; i++)
        {
          digitalWrite(buttonsLED[i], HIGH);

          placeHeatedMilli[i] = currentMillis;

          toBeActivatedPlaces[i] = false;
          hotPlaces[i] = true;
        }
      }
      else
      {
        // First, lets turn the LEDS off for all
        // points after the current temp sensor
        for(int i = buttonIdx; i < NUMBER_OF_POINTS; i++)
        {
          digitalWrite(buttonsLED[i], LOW);

          hotPlaces[i] = false;
        }

        // Now we analyse all previous points
        int mostPreviousNotToBeActivatedPlaceIdx = -1;

        for(int i = (buttonIdx - 1); i >= 0; i--)
        {
          if(toBeActivatedPlaces[i] == true)
          {
            break;
          }
          
          mostPreviousNotToBeActivatedPlaceIdx = i;
        }
        
        for(int i = mostPreviousNotToBeActivatedPlaceIdx; i < buttonIdx; i++)
        {
          digitalWrite(buttonsLED[i], LOW);

          hotPlaces[i] = false;
        }
      }
    }
  }
}

void threadWarmerCallback(void)
{
  int idx;
  int isAnywhereHot = false;

  for(idx = (NUMBER_OF_POINTS - 1); idx >=0; idx--)
  {
    if(hotPlaces[idx] == true)
    {
      isAnywhereHot = true;
      break;
    }
  }

  if(!isAnywhereHot)
  {
    idx = -1;
  }
  
  unsigned long currentMillis = millis();

  int buttonIndex;
  for(buttonIndex = 0; buttonIndex < NUMBER_OF_POINTS; buttonIndex++)
  {
    if(toBeActivatedPlaces[buttonIndex] == true)
    {
      long mustElapseTime = 0;
      
      if(buttonIndex == 0)
      {
        mustElapseTime = HeatPeriod[buttonIndex];
      }
      else
      {
        int reducedTime = 0;

        for(int i = 0; i <= buttonIndex; i++)
        {
          int tmp = buttonActivationMilli[i];

          if(buttonActivationMilli[i] == buttonActivationMilli[buttonIndex])
          {
            break;
          }

          reducedTime = tmp;
        }

        mustElapseTime = HeatPeriod[buttonIndex] - (buttonActivationMilli[buttonIndex] - reducedTime);
      }

      if (mustElapseTime <= 0)
      {
       if(idx != -1)
        {
          // If there is already a hot place, we dont need the full nominal period
          mustElapseTime = HeatPeriod[buttonIndex] - HeatPeriod[idx];
        }
        else
        {
          mustElapseTime = HeatPeriod[buttonIndex];
        }
      }

      if(long(currentMillis - buttonActivationMilli[buttonIndex]) >= mustElapseTime)
      {
        digitalWrite(buttonsLED[buttonIndex], HIGH);
		hotPlaces[buttonIndex] = true;
        
        for(int i = 0; i <= buttonIndex; i++)
        {
          placeHeatedMilli[i] = currentMillis + ConsumerPeriod[buttonIndex];

          toBeActivatedPlaces[i] = false;
        }
      }
    }
  }
}

void threadCoolingCallback(void)
{
  bool toBeCooled = true;
  for(int i = 0; i < NUMBER_OF_POINTS; i++)
  {
    if(toBeActivatedPlaces[i] == true)
    {
      toBeCooled = false;
      
      break;
    }
  }
  
  if(toBeCooled)
  {
    unsigned long currentMillis = millis();

    int buttonIdx;
    for(buttonIdx = 0; buttonIdx < NUMBER_OF_POINTS; buttonIdx++)
    {
      if(hotPlaces[buttonIdx] == true)
      {
        if((currentMillis > placeHeatedMilli[buttonIdx]) && (long(currentMillis - placeHeatedMilli[buttonIdx]) >= coolingPeriod))
        {
          digitalWrite(buttonsLED[buttonIdx], LOW);

          hotPlaces[buttonIdx] = false;
          buttonActivationMilli[buttonIdx] = 0;
          placeHeatedMilli[buttonIdx] = 0;
        }
      }
    }
  } 
}

void checkWhetherThreadsShouldRun(void)
{
  if(threadButtons.shouldRun())
  {
    threadButtons.run();
  }
  
  if(threadTemperatureSensors.shouldRun())
  {
    threadTemperatureSensors.run();
  }
  
  if(threadWarmer.shouldRun())
  {
    threadWarmer.run();
  }
  
  if(threadCooling.shouldRun())
  {
    threadCooling.run();
  }
}
