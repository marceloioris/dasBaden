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

Thread threadWarmer = Thread();
void threadWarmerCallback(void);

Thread threadCooling = Thread();
void threadCoolingCallback(void);

const unsigned int coolingPeriod[4] = {3000,   // C1T1
                                       5000,   // B1T1
                                   7000,   // B2T1
                                       10000}; // B2C1

unsigned int buttons[4] = {2,  // C1T1
                           3,  // B1T1
                           4,  // B2T1
                           5}; // B2C1

unsigned int buttonsLED[4] = {8,   // C1T1
                              9,   // B1T1
                              10,  // B2T1
                              11}; // B2C1

bool hotPlaces[4] = {false,  // C1T1
                     false,  // B1T1
                     false,  // B2T1
                     false}; // B2C1

bool toBeActivatedPlaces[4] = {false,  // C1T1
                               false,  // B1T1
                               false,  // B2T1
                               false}; // B2C1

unsigned long buttonActivationMilli[4] = {0,  // C1T1
                                          0,  // B1T1
                                          0,  // B2T1
                                          0}; // B2C1

unsigned long botao_act = 0;
unsigned long botao_anterior = 0;
bool valvula = false;
bool enviar = false;

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

  threadC1T1.onRun(threadC1T1Callback);
  threadC1T1.setInterval(200);

  threadB1T1.onRun(threadB1T1Callback);
  threadB1T1.setInterval(200);

  threadB2T1.onRun(threadB2T1Callback);
  threadB2T1.setInterval(200);

  threadB2C1.onRun(threadB2C1Callback);
  threadB2C1.setInterval(200);

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

  /*
  if(enviar)
  {
    send(msg);

    enviar = false;
  }
  */

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
  }
}

void threadWarmerCallback(void)
{
  unsigned long currentMillis = millis();

  if(toBeActivatedPlaces[0] == true)
  {
    unsigned long mustElapseTime = 1000;

    if(long(currentMillis - buttonActivationMilli[0]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[0], HIGH);
      
      toBeActivatedPlaces[0] = false;
      hotPlaces[0] = true;
    }
  }
  
  if(toBeActivatedPlaces[1] == true)
  {
    unsigned long mustElapseTime = 3000 - (buttonActivationMilli[1] - buttonActivationMilli[0]);

    if(long(currentMillis - buttonActivationMilli[1]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[1], HIGH);
      
      toBeActivatedPlaces[1] = false;
      hotPlaces[1] = true;
    }
  }
  
  if(toBeActivatedPlaces[2] == true)
  {
    unsigned long mustElapseTime = 6000 - (buttonActivationMilli[2] - buttonActivationMilli[1]);

    if(long(currentMillis - buttonActivationMilli[2]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[2], HIGH);
      
      toBeActivatedPlaces[2] = false;
      hotPlaces[2] = true;
    }
  }
  
  if(toBeActivatedPlaces[3] == true)
  {
    unsigned long mustElapseTime = 10000 - (buttonActivationMilli[3] - buttonActivationMilli[2]);

    if(long(currentMillis - buttonActivationMilli[3]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[3], HIGH);
      
      toBeActivatedPlaces[3] = false;
      hotPlaces[3] = true;
    }
  }
}

void threadCoolingCallback(void)
{
  unsigned long currentMillis = millis();
  
  if(hotPlaces[3] == true)
  {
    unsigned long mustElapseTime = coolingPeriod[3];
    
    if(long(currentMillis - buttonActivationMilli[3]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[3], LOW);
      
      hotPlaces[3] = false;
      buttonActivationMilli[3] = 0;
    }
  }
  else if(hotPlaces[2] == true)
  {
    unsigned long mustElapseTime = coolingPeriod[2];
    
    if(long(currentMillis - buttonActivationMilli[2]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[2], LOW);
      
      hotPlaces[2] = false;
      buttonActivationMilli[2] = 0;
    }
  }
  else if(hotPlaces[1] == true)
  {
    unsigned long mustElapseTime = coolingPeriod[1];
    
    if(long(currentMillis - buttonActivationMilli[1]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[1], LOW);
      
      hotPlaces[1] = false;
      buttonActivationMilli[1] = 0;
    }
  }
  else if(hotPlaces[0] == true)
  {
    unsigned long mustElapseTime = coolingPeriod[0];
    
    if(long(currentMillis - buttonActivationMilli[0]) >= mustElapseTime)
    {
      digitalWrite(buttonsLED[0], LOW);
      
      hotPlaces[0] = false;
      buttonActivationMilli[0] = 0;
    }
  }
}

void send (char *message)
{
  /*
  vw_send((uint8_t *)message, strlen(message));
  
  Serial.println("Transferindo");
  
  vw_wait_tx(); // Aguarda o envio de dados

  Serial.println("transferiu");
  */
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
  
  if(threadWarmer.shouldRun())
  {
    threadWarmer.run();
  }
  
  if(threadCooling.shouldRun())
  {
    threadCooling.run();
  }
}

void analyseMessage(void)
{
  /*
  uint8_t buf[VW_MAX_MESSAGE_LEN]; //VariÃ¡vel para o armazenamento do buffer dos dados
  uint8_t buflen = VW_MAX_MESSAGE_LEN; //VariÃ¡vel para o armazenamento do tamanho do buffer
  
  if(vw_get_message(buf, &buflen)) //Se no buffer tiver algum dado (O ou 1)
  {
    String str = (char*)buf;
    Serial.println(str);
  }
  */
}
