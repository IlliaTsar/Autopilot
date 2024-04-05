#include <Wire.h>
#include <VL53L0X.h>
#include <EEPROM.h>

//Servo distance

//4 + 3 + 3 cm


int Gas_State;
int Brake_State;
int Gear_State;

int Brake_Distance_MAX = 180;
int Gas_Distance_MAX = 190;
int G_Revers, G_Parking, G_Drive;

#define Brake_Distance_MIN 50
#define Gas_Distance_MIN 50

//Gear state

#define Drive 172
#define Parking 1809
#define Reverse 992

//Servo control pins

#define Brake_UP 5
#define Brake_Down 6
#define Gas_UP 7
#define Gas_Down 8
#define Gear_UP 9
#define Gear_Down 10

//Sensor reset pin

#define SEN1_PIN 17
#define SEN2_PIN 16
#define SEN3_PIN 15

VL53L0X sensor1;
VL53L0X sensor2;
VL53L0X sensor3;


#define sread readRangeContinuousMillimeters()

// #define sread readRangeSingleMillimeters()

// ------------------------------------------
int l = 0;
int b;
int c;
int d;
int f;
int temp;
int data_status;
int Call_state = 0;
int temp_position;
int move_curr_pos;
int last_mode;

int valueC, valueG, valueS;

String receivedData;

// ------------------------------------------
void(* resetFunc) (void) = 0;


void setID(){
  digitalWrite(SEN1_PIN, LOW);
  digitalWrite(SEN2_PIN, LOW);
  digitalWrite(SEN3_PIN, LOW);
  Serial.println("Start");
  delay(1000);

  Serial.println("Set id");
  digitalWrite(SEN1_PIN, HIGH);//On sensor 1
  delay(150);
  sensor1.init(true);
  Serial.println(F("Sensor 1 on"));
  delay(100);
  sensor1.setAddress(0x30);//Write new addres to sensor 1
  Serial.println(F("s1 addres set"));
  delay(100);
  digitalWrite(SEN2_PIN, HIGH);//On sensor 2
  delay(150);
  sensor2.init(true);
  Serial.println(F("Sensor 2 on"));
  delay(100);
  sensor2.setAddress(0x31);//Write new addres to sensor 2
  Serial.println(F("s3 addres set"));
  delay(100);
  digitalWrite(SEN3_PIN, HIGH);//On sensor 3
  delay(150);
  Serial.println(F("Sensor 3 on"));
  sensor3.init(true);
  delay(100);
  sensor3.setAddress(0x32);//Write new addres to sensor 3
  Serial.println(F("s4 addres set"));
  
  Serial.println(F("addresses set"));

  sensor1.setMeasurementTimingBudget(75000);
  sensor3.setMeasurementTimingBudget(75000);
  sensor2.setMeasurementTimingBudget(75000);


  sensor1.startContinuous(100);
  sensor2.startContinuous(100);
  sensor3.startContinuous(100);
}


void setup(){

  Serial.begin (9600);

  pinMode(SEN1_PIN, OUTPUT);
  pinMode(SEN2_PIN, OUTPUT);
  pinMode(SEN3_PIN, OUTPUT);

  pinMode(Gas_UP, OUTPUT);
  pinMode(Gas_Down , OUTPUT);
  digitalWrite(Gas_UP, LOW);
  digitalWrite(Gas_Down, LOW);

  pinMode(Brake_UP, OUTPUT);
  pinMode(Brake_Down , OUTPUT);
  digitalWrite(Brake_UP, LOW);
  digitalWrite(Brake_Down, LOW);

  pinMode(Gear_UP, OUTPUT);
  pinMode(Gear_Down , OUTPUT);
  digitalWrite(Gear_UP, LOW);
  digitalWrite(Gear_Down, LOW);

  delay(500);
  Wire.begin();

  setID();
  EEPROM.get(0, Brake_Distance_MAX);
  EEPROM.get(2, Gas_Distance_MAX);
  EEPROM.get(4, G_Drive);
  EEPROM.get(6, G_Parking);
  EEPROM.get(8, G_Revers);
  EEPROM.get(10, Gear_State);
  EEPROM.get(12, Call_state);
  // Brake_Distance_MAX = 180;
  // Gas_Distance_MAX = 180;
  // G_Revers = 130;
  // G_Parking = 180;
  // G_Drive = 80;
  Serial.println(Brake_Distance_MAX);
  Serial.println(Gas_Distance_MAX);
  Serial.println(G_Drive);
  Serial.println(G_Parking);
  Serial.println(G_Revers);
  Serial.println(Gear_State);
  
  delay(500);
  Serial.println("Ok");
}


void loop()
{
  SensorCheck();
  DataCollect();
  if (valueC == 0){
    Serial.println("1");
    GearBox(valueS);
    delay(50);
    Move(valueG);
  } else Calibration(valueC, valueG);
  b=sensor1.sread - 100;
  Serial.println("Ok");
  Serial.print("1:");
  Serial.print(b);
  Serial.print("  ");
  c=sensor2.sread;
  Serial.print("2:");
  Serial.print(c);
  Serial.print("  ");
  d=sensor3.sread;
  Serial.print("3:");
  Serial.print(d);
  Serial.print("  ");
  Serial.println("");

}

void DataCollect(){
  if (Serial.available() > 14){
    while (Serial.available() != 0){
    char k = Serial.read();
    }
  } 
  if (Serial.available() < 14) {
    String receivedData = Serial.readStringUntil('\n'); // Чтение строки до символа новой строки

    // Предполагаем, что формат данных "C****G****S****", где **** - числа
    if (receivedData.startsWith("C") && receivedData.indexOf('G') != -1 && receivedData.indexOf('S') != -1) {
      // Разделение строки на три части: C****, G****, S****
      String partC = receivedData.substring(1, receivedData.indexOf('G'));
      String partG = receivedData.substring(receivedData.indexOf('G') + 1, receivedData.indexOf('S'));
      String partS = receivedData.substring(receivedData.indexOf('S') + 1, receivedData.indexOf('\n'));

      // Преобразование строк в числа
      valueC = partC.toInt();
      valueG = partG.toInt();
      valueS = partS.toInt();

      // Теперь у вас есть значения C, G, S для дальнейшей обработки
      Serial.print("Value C: ");
      Serial.println(valueC);
      Serial.print("Value G: ");
      Serial.println(valueG);
      Serial.print("Value S: ");
      Serial.println(valueS);
    }
  }
  
}

void Calibration(int mode, int positi){
  int position = map(positi, 172, 1809, 40, 210);
  SensorCheck();
  long int temp_time = millis();
  if (temp_position != positi or mode != last_mode){
    temp_position = positi;
    last_mode = mode;
    Serial.println("CALL");
    switch (mode){
      case 1://start brake calibration
        if ((sensor1.sread - 100) < position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor1.sread-100)){
            digitalWrite(Brake_UP,HIGH);
          }
          digitalWrite(Brake_UP,LOW);
        } else if ((sensor1.sread - 100) > position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor1.sread-100)){
            digitalWrite(Brake_Down,HIGH);
          }
          digitalWrite(Brake_Down,LOW);
        }
        break;
      case 2://stop break calibration
        if (Call_state == 0){
          Call_state++;
          EEPROM.put(0, sensor1.sread - 100);
          Serial.print("Break callibration complete:");
          while (sensor1.sread - 100 > Brake_Distance_MIN){
            digitalWrite(Brake_Down, HIGH);
          }
          Brake_Distance_MAX = EEPROM.read(0);
          Serial.println(Brake_Distance_MAX);
          digitalWrite(Brake_Down, LOW);
        }
        temp_position = 0;
        break;
      case 3://start gas callibration
        if (sensor2.sread < position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor2.sread)){
            digitalWrite(Gas_UP,HIGH);
          }
          digitalWrite(Gas_UP,LOW);
        } else if (sensor2.sread > position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor2.sread)){
            digitalWrite(Gas_Down,HIGH);
          }
          digitalWrite(Gas_Down,LOW);
        }
        break;
      case 4://stop gas  callibration
        if (Call_state == 1){
          Call_state++;
          EEPROM.put(2, sensor2.sread);
          Gas_Distance_MAX = EEPROM.read(2);
          Serial.print("Gas callibration complete:");
          Serial.println(Gas_Distance_MAX);
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor2.sread)){
            digitalWrite(Gas_Down, HIGH);
          }
          digitalWrite(Gas_Down, LOW);
        }
        temp_position = 0;
        break;
      case 5://start gear1 callibration
        if (sensor3.sread < position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor3.sread)){
              digitalWrite(Gear_UP,HIGH);
            }
            digitalWrite(Gear_UP,LOW);
        } else if (sensor3.sread > position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor3.sread)){
              digitalWrite(Gear_Down,HIGH);
            }
            digitalWrite(Gear_Down,LOW);
        }
        break;
      case 6:
        if (Call_state == 2){
          Call_state++;
          EEPROM.put(4, sensor3.sread);//Drive
          G_Drive = EEPROM.read(4);
          Serial.print("Gear 1 callibration complete:");
          Serial.println(G_Drive);
        }
        break;
      case 7://start gear2 callibration
        if (sensor3.sread < position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor3.sread)){
            digitalWrite(Gear_UP,HIGH);
          }
          digitalWrite(Gear_UP,LOW);
        } else if (sensor3.sread > position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor3.sread)){
            digitalWrite(Gear_Down,HIGH);
          }
          digitalWrite(Gear_Down,LOW);
        }
        break;
      case 8:
        if (Call_state == 3){
          Call_state++;
          EEPROM.put(6, sensor3.sread);//Revers
          G_Revers = EEPROM.read(6);
          Serial.print("Gear 2 callibration complete:");
          Serial.println(G_Revers);
        }
        temp_position = 0;
        break;
      case 9://start gear3 callibration
        if (sensor3.sread < position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor3.sread)){
            digitalWrite(Gear_UP,HIGH);
          }
          digitalWrite(Gear_UP, LOW);
        } else if (sensor3.sread > position){
          temp_time = millis();
          while ((millis() - temp_time) <= time_res(position, sensor3.sread)){
            digitalWrite(Gear_Down,HIGH);
          }
          digitalWrite(Gear_Down, LOW);
        }
        break;
      case 10:
        if (Call_state == 4){
          Call_state = 0;
          EEPROM.put(8, sensor3.sread);//Parking
          G_Parking = EEPROM.read(8);
          Serial.print("Gear 3 callibration complete:");
          Serial.println(G_Parking);
          GearBox(Parking);
        }
        temp_position = 0;
        break;
      case 11:
        Serial.println("Engine_OFF");
        GearBox(Parking);
        delay(50);
        Gas(Gas_Distance_MIN);
        delay(50);
        Brake(Brake_Distance_MIN);
        break;
      default: break;
    }
  } else if (temp_position == positi);
}


void Move(int position){
  if (position >= 1010 and position <= 1809){
      Brake(Brake_Distance_MIN+5);
      delay(100);
      Gas(map(position, 1010, 1809, Gas_Distance_MIN, Gas_Distance_MAX));
    Serial.println("GAS");
  } else if (position <= 970 and position >= 172){
      Gas(Gas_Distance_MIN+5);
      delay(100);
      Brake(map(position, 970, 172, Brake_Distance_MIN, Brake_Distance_MAX));
    Serial.println("BRAKE");
  } else if (position >= 970 and position <=1010){
    Brake(Brake_Distance_MIN+5);
    delay(50);
    Gas(Gas_Distance_MIN+5);
  }
  move_curr_pos = position;
}

void GearBox(int state){
  if (Gear_State != state){

    
    long int temp_time = millis();
    Serial.println("GEAR");
    Gas(Gas_Distance_MIN+5);
    delay(100);
    Brake(Brake_Distance_MAX);
    delay(500);

    switch (state){
      case Drive:
        temp_time = millis();
        while ((millis() - temp_time) <= time_res(G_Drive, sensor3.sread)){
          SensorCheck();
          digitalWrite(Gear_Down, HIGH);
        }
        digitalWrite(Gear_Down, LOW);
        Serial.println(F("Gear D"));
        break;
      case Parking:
        temp_time = millis();
        while ((millis() - temp_time) <= time_res(G_Parking, sensor3.sread)){
          SensorCheck();
          digitalWrite(Gear_UP, HIGH);
        } 
        digitalWrite(Gear_UP, LOW);
        Serial.println(F("Gear P"));
        break;
      case Reverse:
        temp_time = millis();
        if (sensor3.sread  > G_Revers){
          while ((millis() - temp_time) <= time_res(G_Revers, sensor3.sread)){
            SensorCheck();
            digitalWrite(Gear_Down, HIGH);
          }
          digitalWrite(Gear_Down, LOW);
        } else if (sensor3.sread < G_Revers){
          while ((millis() - temp_time) <= time_res(G_Revers, sensor3.sread)){
            SensorCheck();
            digitalWrite(Gear_UP, HIGH);
          }
          digitalWrite(Gear_UP, LOW);
        }
        Serial.println(F("Gear R"));
        break;
        default: break;
    }
    EEPROM.put(10, state);
    Gear_State = state;
    Brake(Brake_Distance_MIN);
  } else if (Gear_State == state);
}

void Gas(int dist){
  if (Gas_State != dist){
    Serial.println("GAS1");
    long int temp_time = millis();
    if (dist > sensor2.sread  and dist < Gas_Distance_MAX){
      temp_time = millis();
      Serial.println("GASUP");
      while ((millis() - temp_time) <= time_res(dist, sensor2.sread)){
        SensorCheck();
        Serial.println(sensor2.sread);
        digitalWrite(Gas_UP, HIGH);
      }
      digitalWrite(Gas_UP, LOW);
    } else if (dist < sensor2.sread and dist > Gas_Distance_MIN){
      temp_time = millis();
      Serial.println("GASDOWN");
      while ((millis() - temp_time) <= time_res(dist, sensor2.sread)){
        SensorCheck();
        Serial.println(sensor2.sread);
        digitalWrite(Gas_Down, HIGH);
      }
      digitalWrite(Gas_Down, LOW);
      Gas_State = dist;
    }
  } else if (Gas_State == dist);
}

void Brake(int dist){
  if (Brake_State != dist) {
    
    long int temp_time = millis();
    if (dist >= (sensor1.sread -100)  and dist <= Brake_Distance_MAX){
      temp_time = millis();
      Serial.println("BRAKEUP");
      Serial.print(dist);
      Serial.print("|");
      Serial.println(time_res(dist, sensor1.sread - 100));
      while ((millis() - temp_time) <= time_res(dist, sensor1.sread - 100)){
        digitalWrite(Brake_UP, HIGH);
        Serial.println(".");
      }
      Serial.println(sensor1.sread - 100);
      digitalWrite(Brake_UP, LOW);
    } else if (dist <= (sensor1.sread -100)  and dist >= Brake_Distance_MIN){
      temp_time = millis();
      Serial.print("BRAKEDOWN, go to:");
      Serial.print(dist);
      Serial.print("|");
      Serial.println(time_res(dist, sensor1.sread - 100));
      while ((millis() - temp_time) <= time_res(dist, sensor1.sread - 100)){
        digitalWrite(Brake_Down, HIGH);
        Serial.println(".");
      }
      Serial.println(sensor1.sread - 100);
      digitalWrite(Brake_Down, LOW);
      Brake_State = dist;
    }
  } else if (Brake_State == dist);
}

int time_res(int s1, int s2){
  float time;
  if (s1 > s2) time = (s1 - s2);
  else if (s1 < s2) time = (s2 - s1);
  time = time * 1000;
  time = time / 60;
  return int(time);
}

void SensorCheck(){
  if ((sensor1.sread == -1 or sensor1.sread == 8191) or (sensor2.sread == -1 or sensor2.sread == 8191) or (sensor3.sread == -1 or sensor3.sread == 8191))
 resetFunc();
}