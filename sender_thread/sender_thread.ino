#include <VirtualWire.h>
#include <Thread.h>

#define TEMPO_RESFRIAMENTO 30000 // 3s

void checkWhetherThreadsShouldRun(void);
void send (char *message);

void analyseMessage(void);

unsigned long lastActivatedTime = 0;
unsigned int  buttonC1T1        = 2;
Thread        threadC1T1        = Thread(); // Digital Pin 1 (Kitchen 1 Sink 1) Thread
void threadC1T1Callback(void);

unsigned int  buttonB1T1  = 3;
Thread        threadB1T1  = Thread(); // Digital Pin 1 (Bathroom 1 Sink 1) Thread
void threadB1T1Callback(void);

unsigned int  buttonB2T1  = 5;
Thread        threadB2T1  = Thread(); // Digital Pin 1 (Bathroom 2 Sink 1) Thread
void threadB2T1Callback(void);

unsigned int  buttonB2C1  = 6;
Thread        threadB2C1  = Thread(); // Digital Pin 1 (Bathroom 2 Shower 1) Thread
void threadB2C1Callback(void);

unsigned int ledButtonC1T1  = 8;
unsigned int ledButtonB1T1  = 9;
unsigned int ledButtonB2T1  = 10;
unsigned int ledButtonB2C1  = 11;

unsigned long botao_act = 0;
unsigned long botao_anterior = 0;
bool valvula = false;
bool enviar = false;

char *msg = "0000";

void setup()
{
  Serial.begin(9600); 
  
  //++++++++++++++Inicializa o módulo transmissor+++++++++++++++++++
  vw_set_ptt_inverted(true); 
  vw_setup(2000); //Bits per sec
  vw_set_tx_pin(4); //Configura o pino D4 para a leitura dos dados
  //================================================================

  pinMode(buttonC1T1, INPUT_PULLUP);
  pinMode(buttonB1T1, INPUT_PULLUP);
  pinMode(buttonB2T1, INPUT_PULLUP);
  pinMode(buttonB2C1, INPUT_PULLUP);

  pinMode(ledButtonC1T1, OUTPUT);
  pinMode(ledButtonB1T1, OUTPUT);
  pinMode(ledButtonB2T1, OUTPUT);
  pinMode(ledButtonB2C1, OUTPUT);

  threadC1T1.onRun(threadC1T1Callback);
  threadC1T1.setInterval(200);

  threadB1T1.onRun(threadB1T1Callback);
  threadB1T1.setInterval(200);

  threadB2T1.onRun(threadB2T1Callback);
  threadB2T1.setInterval(200);

  threadB2C1.onRun(threadB2C1Callback);
  threadB2C1.setInterval(200);

  digitalWrite(ledButtonC1T1, HIGH);
  digitalWrite(ledButtonB1T1, HIGH);
  digitalWrite(ledButtonB2T1, HIGH);
  digitalWrite(ledButtonB2C1, HIGH);
}

void loop()
{
  checkWhetherThreadsShouldRun();

  if(enviar)
  {
    send(msg);

    enviar = false;
  }

  delay(1);
}

void threadC1T1Callback(void)
{
  if(!digitalRead(buttonC1T1))
  {
    Serial.println("Button C1T1 pressed!");

    botao_act = millis();

    if((digitalRead(ledButtonC1T1)) && (botao_act > botao_anterior + 1000))
    {
      valvula = !valvula;

      enviar = true;

      botao_anterior = botao_act;

      msg = "C1T1";

      digitalWrite(ledButtonC1T1, LOW);

      lastActivatedTime = millis();
    }
  }

  if((!digitalRead(ledButtonC1T1)) && (millis() - lastActivatedTime >= TEMPO_RESFRIAMENTO))
  {
    digitalWrite(ledButtonC1T1, HIGH);
  }
}

void threadB1T1Callback(void)
{
  if(!digitalRead(buttonB1T1))
  {
    Serial.println("Button B1T1 pressed!");

    botao_act = millis();

    if (botao_act > botao_anterior + 1000)
    {
      valvula = !valvula;
    
      enviar = true;
      
      botao_anterior = botao_act;
      
      msg = "B1T1";

      digitalWrite(ledButtonB1T1, LOW);
    }
  }
}

void threadB2T1Callback(void)
{
  if(!digitalRead(buttonB2T1))
  {
    Serial.println("Button B2T1 pressed!");

    botao_act = millis();
    
    if (botao_act > botao_anterior + 1000)
    {
      valvula = !valvula;
    
      enviar = true;
      
      botao_anterior = botao_act;
      
      msg = "B2T1";

      digitalWrite(ledButtonB2T1, LOW);
    }
  }
}

void threadB2C1Callback(void)
{
  if(!digitalRead(buttonB2C1))
  {
    Serial.println("Button B2C1 pressed!");

    botao_act = millis();
    
    if (botao_act > botao_anterior + 1000)
    {
      valvula = !valvula;
    
      enviar = true;
      
      botao_anterior = botao_act;
      
      msg = "B2C1";

      digitalWrite(ledButtonB2C1, LOW);
    }
  }
}

void send (char *message)
{
  vw_send((uint8_t *)message, strlen(message));
  
  Serial.println("Transferindo");
  
  vw_wait_tx(); // Aguarda o envio de dados

  Serial.println("transferiu");
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
}

void analyseMessage(void)
{
  uint8_t buf[VW_MAX_MESSAGE_LEN]; //VariÃ¡vel para o armazenamento do buffer dos dados
  uint8_t buflen = VW_MAX_MESSAGE_LEN; //VariÃ¡vel para o armazenamento do tamanho do buffer
  
  if(vw_get_message(buf, &buflen)) //Se no buffer tiver algum dado (O ou 1)
  {
    String str = (char*)buf;
    Serial.println(str);
  }
}

