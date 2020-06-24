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

Thread        threadC1T1        = Thread(); // Digital Pin 1 (Kitchen 1 Sink 1) Thread
void threadC1T1Callback(void);

Thread        threadB1T1  = Thread(); // Digital Pin 1 (Bathroom 1 Sink 1) Thread
void threadB1T1Callback(void);

Thread        threadB2T1  = Thread(); // Digital Pin 1 (Bathroom 2 Sink 1) Thread
void threadB2T1Callback(void);

Thread        threadB2C1  = Thread(); // Digital Pin 1 (Bathroom 2 Shower 1) Thread
void threadB2C1Callback(void);

Thread        threadTempB2C1  = Thread(); // Temperature Sensor (Bathroom 2 Shower 1) Thread
void threadTempB2C1Callback(void);

Thread threadWarmer = Thread();
void threadWarmerCallback(void);

Thread threadCooling = Thread();
void threadCoolingCallback(void);

const unsigned int coolingPeriod = 10000; // Universal Cooling Period

#define NUMBER_OF_POINTS 4

unsigned int buttons[NUMBER_OF_POINTS] = {2,  // C1T1
                           			      3,  // B1T1
                           			      4,  // B2T1
                                          5}; // B2C1

unsigned int buttonsLED[NUMBER_OF_POINTS] = {8,   // C1T1
                                             9,   // B1T1
                                             10,  // B2T1
                                             11}; // B2C1

unsigned int tempSensor[NUMBER_OF_POINTS] = {0,
                                             0,
                                             0,
                                             A5}; // B2C1

bool hotPlaces[NUMBER_OF_POINTS] = {false,  // C1T1
                                    false,  // B1T1
                                    false,  // B2T1
                                    false}; // B2C1

bool toBeActivatedPlaces[NUMBER_OF_POINTS] = {false,  // C1T1
                                              false,  // B1T1
                                              false,  // B2T1
                                              false}; // B2C1

unsigned long HeatPeriod[NUMBER_OF_POINTS] = {1000,  // C1T1
                                              3000,  // B1T1
                                              6000,  // B2T1
                                              10000}; // B2C1

  unsigned long ConsumerPeriod[NUMBER_OF_POINTS] = {2000,  // C1T1
                                                    2000,  // B1T1
                                                    2000,  // B2T1
                                                    3000}; // B2C1

unsigned long buttonActivationMilli[NUMBER_OF_POINTS] = {0,  // C1T1
                                                         0,  // B1T1
                                                         0,  // B2T1
                                                         0}; // B2C1

unsigned long placeHeatedMilli[NUMBER_OF_POINTS] = {0,  // C1T1
                                                    0,  // B1T1
                                                    0,  // B2T1
                                                    0}; // B2C1

unsigned long minShowerTemp = 30;
  
char *msg = "0000";

void setup()
{
  Serial.begin(115200);

  pinMode(buttons[0], INPUT_PULLUP);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(buttons[3], INPUT_PULLUP);

  pinMode(buttonsLED[0], OUTPUT);
  pinMode(buttonsLED[1], OUTPUT);
  pinMode(buttonsLED[2], OUTPUT);
  pinMode(buttonsLED[3], OUTPUT);

  pinMode(tempSensor[0],INPUT);
  
  threadC1T1.onRun(threadC1T1Callback);
  threadC1T1.setInterval(200);

  threadB1T1.onRun(threadB1T1Callback);
  threadB1T1.setInterval(200);

  threadB2T1.onRun(threadB2T1Callback);
  threadB2T1.setInterval(200);

  threadB2C1.onRun(threadB2C1Callback);
  threadB2C1.setInterval(200);

  threadTempB2C1.onRun(threadTempB2C1Callback);
  threadTempB2C1.setInterval(200);
  
  threadWarmer.onRun(threadWarmerCallback);
  threadWarmer.setInterval(200);
  
  threadCooling.onRun(threadCoolingCallback);
  threadCooling.setInterval(200);
  
  digitalWrite(buttonsLED[0], LOW);
  digitalWrite(buttonsLED[1], LOW);
  digitalWrite(buttonsLED[2], LOW);
  digitalWrite(buttonsLED[3], LOW);
}

void loop()
{
  checkWhetherThreadsShouldRun();

  delay(1);
}

void threadC1T1Callback(void)
{
  if(digitalRead(buttons[0]))
  {
    Serial.println("Button C1T1 pressed!");
    
    if(hotPlaces[0] == false)
    {
      toBeActivatedPlaces[0] = true;
    
      buttonActivationMilli[0] = millis();
    }
    else
    {
      unsigned long currentMillis = millis();

      placeHeatedMilli[0] = currentMillis;
    }
  }
}

void threadB1T1Callback(void)
{
  if(digitalRead(buttons[1]))
  {
    Serial.println("Button B1T1 pressed!");
    
    if(hotPlaces[1] == false)
    {
      toBeActivatedPlaces[0] = true;
      toBeActivatedPlaces[1] = true;
    
      if(buttonActivationMilli[0] == 0)
      {
  	    buttonActivationMilli[0] = millis();
      }
    
      buttonActivationMilli[1] = millis();
    }
    else
    {
      unsigned long currentMillis = millis();

      placeHeatedMilli[0] = currentMillis;
      placeHeatedMilli[1] = currentMillis;
    }
  }
}

void threadB2T1Callback(void)
{
  if(digitalRead(buttons[2]))
  {
    Serial.println("Button B2T1 pressed!");
    
    if(hotPlaces[2] == false)
    {
      toBeActivatedPlaces[0] = true;
      toBeActivatedPlaces[1] = true;
      toBeActivatedPlaces[2] = true;
    
      if(buttonActivationMilli[0] == 0)
      {
        buttonActivationMilli[0] = millis();
      }
    
      if(buttonActivationMilli[1] == 0)
      {
  	    buttonActivationMilli[1] = millis();
      }
    
      buttonActivationMilli[2] = millis();
    }
    else
    {
      unsigned long currentMillis = millis();

      placeHeatedMilli[0] = currentMillis;
      placeHeatedMilli[1] = currentMillis;
      placeHeatedMilli[2] = currentMillis;
    }
  }
}

void threadB2C1Callback(void)
{
  if(digitalRead(buttons[3]))
  {
    Serial.println("Button B2C1 pressed!");
    
    if(hotPlaces[3] == false)
    {
      toBeActivatedPlaces[0] = true;
      toBeActivatedPlaces[1] = true;
      toBeActivatedPlaces[2] = true;
      toBeActivatedPlaces[3] = true;
    
      if(buttonActivationMilli[0] == 0)
      {
  	    buttonActivationMilli[0] = millis();
      }
    
      if(buttonActivationMilli[1] == 0)
      {
  	    buttonActivationMilli[1] = millis();
      }
    
      if(buttonActivationMilli[2] == 0)
      {
  	    buttonActivationMilli[2] = millis();
      }
    
      buttonActivationMilli[3] = millis();
    }
    else
    {
      unsigned long currentMillis = millis();

      placeHeatedMilli[0] = currentMillis;
      placeHeatedMilli[1] = currentMillis;
      placeHeatedMilli[2] = currentMillis;
      placeHeatedMilli[3] = currentMillis;
    }
  }
}

void threadTempB2C1Callback(void)
{
  int tmp = analogRead(tempSensor[3]);

  float voltage = (tmp * 5.0)/1024;//(5*temp)/1024 is to convert the 10 bit number to a voltage reading.
  float milliVolt = voltage * 1000;//This is multiplied by 1000 to convert it to millivolt.
  
  float tempSensor =  (milliVolt-500)/10;

  if(tempSensor > minShowerTemp)
  {
    unsigned long currentMillis = millis();

    for(int i = 0; i < 4; i++)
    {
      digitalWrite(buttonsLED[i], HIGH);
      
      placeHeatedMilli[i] = currentMillis;

      toBeActivatedPlaces[i] = false;
      hotPlaces[i] = true;
    }
  }
  else
  {
  }
}

void threadWarmerCallback(void)
{
  int idx;
  int isAnywhereHot = false;

  for(idx = 3; idx >=0; idx--)
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

  if(toBeActivatedPlaces[0] == true)
  {
    long mustElapseTime = HeatPeriod[0];

    if (mustElapseTime <= 0)
    {
     if(idx != -1)
      {
        // There is already a hot place, we dont need the full nominal period
        mustElapseTime = HeatPeriod[0] - HeatPeriod[idx];
      }
      else
      {
        mustElapseTime = HeatPeriod[0];
      }
    }
    
    if(long(currentMillis - buttonActivationMilli[0]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[0], HIGH);
      
      placeHeatedMilli[0] = currentMillis + ConsumerPeriod[0];
        
      toBeActivatedPlaces[0] = false;
      hotPlaces[0] = true;
    }
  }
  
  if(toBeActivatedPlaces[1] == true)
  {
    long mustElapseTime = HeatPeriod[1] - (buttonActivationMilli[1] - buttonActivationMilli[0]);

    if (mustElapseTime <= 0)
    {
      if(idx != -1)
      {
        // There is already a hot place, we dont need the full nominal period
        mustElapseTime = HeatPeriod[1] - HeatPeriod[idx];
      }
      else
      {
        mustElapseTime = HeatPeriod[1];
      }
    }
    
    if(long(currentMillis - buttonActivationMilli[1]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[1], HIGH);
      
      placeHeatedMilli[0] = currentMillis + ConsumerPeriod[1];
      placeHeatedMilli[1] = currentMillis + ConsumerPeriod[1];
      
      toBeActivatedPlaces[0] = false;
      toBeActivatedPlaces[1] = false;
      hotPlaces[1] = true;
    }
  }
  
  if(toBeActivatedPlaces[2] == true)
  {
    long mustElapseTime = HeatPeriod[2] - (buttonActivationMilli[2] - buttonActivationMilli[1]);

    if (mustElapseTime <= 0)
    {
      if(idx != -1)
      {
        // There is already a hot place, we dont need the full nominal period
        mustElapseTime = HeatPeriod[2] - HeatPeriod[idx];
      }
      else
      {
        mustElapseTime = HeatPeriod[2];
      }
    }
    
    if(long(currentMillis - buttonActivationMilli[2]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[2], HIGH);
      
      placeHeatedMilli[0] = currentMillis + ConsumerPeriod[2];
      placeHeatedMilli[1] = currentMillis + ConsumerPeriod[2];
      placeHeatedMilli[2] = currentMillis + ConsumerPeriod[2];
      
      toBeActivatedPlaces[0] = false;
      toBeActivatedPlaces[1] = false;
      toBeActivatedPlaces[2] = false;
      hotPlaces[2] = true;
    }
  }
  
  if(toBeActivatedPlaces[3] == true)
  {
    long mustElapseTime = HeatPeriod[3] - (buttonActivationMilli[3] - buttonActivationMilli[2]);
	
    if (mustElapseTime <= 0)
    {
      if(idx != -1)
      {
        // There is already a hot place, we dont need the full nominal period
        mustElapseTime = HeatPeriod[3] - HeatPeriod[idx];
      }
      else
      {
        mustElapseTime = HeatPeriod[3];
      }
    }
    
    if(long(currentMillis - buttonActivationMilli[3]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[3], HIGH);
      
      placeHeatedMilli[0] = currentMillis + ConsumerPeriod[3];
      placeHeatedMilli[1] = currentMillis + ConsumerPeriod[3];
      placeHeatedMilli[2] = currentMillis + ConsumerPeriod[3];
      placeHeatedMilli[3] = currentMillis + ConsumerPeriod[3];
      
      toBeActivatedPlaces[0] = false;
      toBeActivatedPlaces[1] = false;
      toBeActivatedPlaces[2] = false;
      toBeActivatedPlaces[3] = false;
      hotPlaces[3] = true;
    }
  }
}

void threadCoolingCallback(void)
{
  if((toBeActivatedPlaces[0] == false)	&&
     (toBeActivatedPlaces[1] == false)	&&
     (toBeActivatedPlaces[2] == false)	&&
     (toBeActivatedPlaces[3] == false))
  {
    unsigned long currentMillis = millis();

    if(hotPlaces[3] == true)
    {
      if((currentMillis > placeHeatedMilli[3]) && (long(currentMillis - placeHeatedMilli[3]) >= coolingPeriod))
      {
        digitalWrite(buttonsLED[3], LOW);

        hotPlaces[3] = false;
        buttonActivationMilli[3] = 0;
        placeHeatedMilli[3] = 0;
      }
    }
    if(hotPlaces[2] == true)
    {
      if((currentMillis > placeHeatedMilli[2]) && (long(currentMillis - placeHeatedMilli[2]) >= coolingPeriod))
      {
        digitalWrite(buttonsLED[2], LOW);

        hotPlaces[2] = false;
        buttonActivationMilli[2] = 0;
        placeHeatedMilli[2] = 0;
      }
    }
    if(hotPlaces[1] == true)
    {
      if((currentMillis > placeHeatedMilli[1]) && (long(currentMillis - placeHeatedMilli[1]) >= coolingPeriod))
      {
        digitalWrite(buttonsLED[1], LOW);

        hotPlaces[1] = false;
        buttonActivationMilli[1] = 0;
        placeHeatedMilli[1] = 0;
      }
    }
    if(hotPlaces[0] == true)
    {
      if((currentMillis > placeHeatedMilli[0]) && (long(currentMillis - placeHeatedMilli[0]) >= coolingPeriod))
      {
        digitalWrite(buttonsLED[0], LOW);

        hotPlaces[0] = false;
        buttonActivationMilli[0] = 0;
        placeHeatedMilli[0] = 0;
      }
    }
  }
}

void checkWhetherThreadsShouldRun(void)
{
  if(threadC1T1.shouldRun())
  {
    threadC1T1.run();
  }

  if(threadB1T1.shouldRun())
  {
    threadB1T1.run();
  }

  if(threadB2T1.shouldRun())
  {
    threadB2T1.run();
  }

  if(threadB2C1.shouldRun())
  {
    threadB2C1.run();
  }
  
  if(threadTempB2C1.shouldRun())
  {
    threadTempB2C1.run();
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
