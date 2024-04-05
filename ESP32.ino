#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <HardwareSerial.h>
#include <EEPROM.h>

HardwareSerial AtmegaSerial(1);
HardwareSerial sbusSerial(2);

TimerHandle_t timer1;
String temp;

//Sbus
uint8_t sbusData[25];
volatile int ch[8];


#define Start_Stop 4
#define Step_DIR 15
#define Step_PULL 13 
#define Step_ENA 18 

int tmpa;
volatile int start_temp = 0;
volatile int count;
volatile int qwe;
volatile int tmp;
volatile int cal_state = 0;
volatile int dir;
volatile int position;
volatile int cur_pos = 50000;
volatile int start_s, start_a;
volatile int Left_limit = 0; 
volatile int Right_limit = 100000;
volatile int i = 100000;
int step_temp = 0;

//---------------------------------------------------------------------
void sbus_Collect();

void Step_Callibration(int position, int call){
  
  switch (call){
    case 11:
      runToPosition(map(position, 172, 1809, 0, 100000));
    break;
    case 12:
    if (step_temp == 0){
      step_temp = 1;
      EEPROM.put(2, cur_pos);
      EEPROM.commit();
      Right_limit = cur_pos;
      Serial.print("|");
      Serial.println(cur_pos);
      delay(1000);
    }
      while( cur_pos != 50091){
        runToPosition(50091);
        vTaskDelay(pdMS_TO_TICKS(1)); 
      }
    break;
    case 13:
      runToPosition(map(position, 172, 1809, 0, 100000));
    break;
    case 14:
      if (step_temp == 1){
      step_temp = 0;
      EEPROM.put(6, cur_pos);
      EEPROM.commit();
      Left_limit = cur_pos;
      Serial.print("|");
      Serial.println(cur_pos);
    }
    break;
    default: break;
  }

}

void Start(int mode){
  if (mode == 1809){
    start_temp = 1;
    
  } else if (mode == 992 and start_temp == 1){
    start_s++;
    start_a = 1;
    start_temp = 0;
  }
  if (start_s == 1 and start_a == 1){
    digitalWrite(Start_Stop, HIGH);
    DataTransmission(ch[2], ch[0], ch[3]);
  } else if (start_s == 2 and start_a == 1){
    AtmegaSerial.println("C11G992S1809");
    Serial.println("C11G992S1809");
    digitalWrite(Start_Stop, LOW);
  } else if (start_s == 3)
    start_s = 1;
  start_a == 1;
}

void timerCallback1(TimerHandle_t xTimer) {
  Start(ch[4]);
  
}

void run() {
  cur_pos += dir;
  digitalWrite(Step_PULL, HIGH);
  delayMicroseconds(5);
  digitalWrite(Step_PULL, LOW);  
  delayMicroseconds(5);
  Serial.println(cur_pos);
}

void runToPosition(int pos) {
  if (pos > cur_pos) {
    digitalWrite(Step_DIR, HIGH);
    dir = 1;
  } else if (pos < cur_pos) {
    digitalWrite(Step_DIR, LOW);
    dir = -1;
  }
  if (cur_pos != pos) {
    digitalWrite(Step_ENA, LOW);
    run();
  } else if ( cur_pos == pos)
    digitalWrite(Step_ENA, HIGH);
  
}


//Функция получения даних с передатчика
void sbus_Collect(){

if (sbusSerial.available() >= 25) { 
   if (sbusSerial.read() == 0) tmpa = 1;
    if (sbusSerial.read() == 0x0F and tmpa == 1){
      sbusSerial.readBytes(sbusData, 23);
      tmpa = 0;
    }
    if (sbusData[20] == 0x0F){
    ch[0] = ((sbusData[1] & B0111)<<8) | sbusData[0]; //GAS, BRAKE
    ch[1] = ((sbusData[2] & B111111) << 5) | (sbusData[1] >> 3); //Right|Left
    ch[2] = ((((sbusData[4] & 1) << 8) | sbusData[3]) << 2) | ((sbusData[2] & B11000000) >> 6); //Callibration
    ch[3] = ((sbusData[5] & B1111) << 7) | (sbusData[4] >> 1); //Gear
    ch[4] = ((sbusData[6] & B1111111) << 4) | (sbusData[5] >> 4); // Start|stop
    }
    // for (int i = 0; i < 5; i++){
    //   Serial.print(ch[i]);
    //   Serial.print("|");
    // }
    // Serial.println();
  }
}


void DataTransmission(int call, int gas, int gear){//Transmission data to MCU2
  char buffer[50];

  if (call == 1809){
    tmp = 1;
  } else if (call == 992 and tmp == 1){
    cal_state++;
    Serial.print("cal_state:");
    Serial.println(cal_state);
    tmp = 0;
  }
  if (cal_state == 0) snprintf(buffer, sizeof(buffer), "C%dG%dS%d", cal_state, gas, gear);
  else if (cal_state > 0 and cal_state < 11) { snprintf(buffer, sizeof(buffer), "C%dG%dS", cal_state, gas);}
  else if (cal_state >= 11 and cal_state <= 14);
  else if (cal_state == 15) cal_state = 0;
  // Вывод строки
  temp = buffer;
  
  if (cal_state < 11){
    Serial.println(buffer);
    AtmegaSerial.println(buffer);
  }   

} 


void core0Task(void *parameter) {
  while (1) {
    if (cal_state < 11) {
     runToPosition(map(ch[1], 172, 1809, Left_limit, Right_limit));
    } else if (cal_state >= 11 and cal_state < 15) Step_Callibration(ch[1], cal_state);
    vTaskDelay(pdMS_TO_TICKS(1)); 
 
  }
}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(100);
  AtmegaSerial.begin(9600, SERIAL_8N1, 32, 33);//Определяем UART для работы с MCU2
  sbusSerial.begin(100000, SERIAL_8E1, 16, 17, true);//Определяем UART для работы с передатчиком

  int a;
  EEPROM.get(2, Right_limit);
  int b;
  EEPROM.get(6, Left_limit);

  //Определяем режими работы пинов
  Serial.print("||||");
  Serial.print(Right_limit);
  Serial.print("|");
  Serial.println(Left_limit);
  
  pinMode(Step_ENA, OUTPUT);
  digitalWrite(Step_ENA, HIGH);

  pinMode(Step_DIR, OUTPUT);
  pinMode(Step_PULL, OUTPUT);

  digitalWrite(Step_PULL, LOW);
  pinMode(Start_Stop, OUTPUT);

  xTaskCreatePinnedToCore(
    core0Task,
    "Core 0 Task",
    65536,
    NULL,
    1,
    NULL,
    0
  );

  timer1 = xTimerCreate(
    "timer1",
    pdMS_TO_TICKS(150),
    pdTRUE,
    (void *) 0,
    timerCallback1
  );
  xTimerStart(timer1, 0);
}


void loop() {
 
  sbus_Collect();
  // Serial.println(AtmegaSerial.readStringUntil('\n'));
  // vTaskDelay(pdMS_TO_TICKS(10));   
  
}
