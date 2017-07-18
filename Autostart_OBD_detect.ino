#include <Wire.h>
#include <SeeedOLED.h> // какчать тут https://github.com/Seeed-Studio/OLED_Display_128X64
#include <SoftwareSerial.h>
SoftwareSerial m590(7, 8); // RX, TX

#define STARTER_Pin 10    // на реле стартера, через транзистор с 11-го пина ардуино
#define ON_Pin 9      // на реле зажигания, через транзистор с 12-го пина ардуино
#define ACTIV_Pin 4    // на светодиод через транзистор, c 4-го пина для индикации активности 
#define BAT_Pin A0      // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Feedback_Pin A3 // на прикуриватель или на ДАДМ, для детектирования момента запуска, если не через OBD
#define STOP_Pin A2     // на концевик педали тормоза для отключения режима прогрева или датчик нейтральной передачи
//#define DDM_Pin A1     // на датчик давления масла

int k = 0;
int interval = 30;      // интервал отправки данных на народмон 
String at = "";
String SMS_phone = "+375000000000"; // куда шлем СМС
String call_phone = "375000000001"; // телефон хозяина без плюса
unsigned long Time1 = 0;
int Timer = 0;           // переменная времени прогрева двигателя по умолчанию
bool start = false;      // переменная разовой команды запуска
bool heating = false;    // переменная состояния режим прогрева двигателя
bool SMS_send = false;   // флаг разовой отправки СМС
float Vbat;              // переменная хранящая напряжение бортовой сети
float m = 66.91;         // делитель для перевода АЦП в вольты для резистров 39/11kOm

char obd[20];
char obdIndex=0;
int rpm = -1;
int temp1 = 90;
int temp2 = 20;

void setup() {
  Serial.begin(38400);
  m590.begin(9600); 
// m590.println("AT+IPR=9600");  // настройка скорости M590
  Wire.begin();  
  SeeedOled.init();
  SeeedOled.clearDisplay(); 
  SeeedOled.setNormalDisplay();  
  SeeedOled.setPageMode(); 
  SeeedOled.setTextXY(0,0), SeeedOled.putString("LOADING_18/07/17");
             }

void loop() {
  if (m590.available()) {         // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    
    if (at.indexOf("RING") > -1) {  // если это что-то вызов (RING) то...
        m590.println("AT+CLIP=1");  //включаем АОН командой AT+CLIP=1
      if (at.indexOf(call_phone) > -1) delay(50), m590.println("ATH0"), Timer = 60, start = true;

    } else if (at.indexOf("+PBREADY") > -1) { // если модем стартанул  
           m590.println("ATE0"),              delay(100); // отключаем режим ЭХА 
           m590.println("AT+CMGD=1,4"),       delay(100); // стираем все СМС
           m590.println("AT+CMGF=1"),         delay(100); // 
           m590.println("AT+CSCS=\"gsm\""),   delay(100); // кодировка смс
  
    } else    //  Serial.println(at);    // если пришло что-то другое выводим в серийный порт
      at = "";  // очищаем переменную
}

if (Timer > 0 && start == true) start = false, enginestart(); 
if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
if (heating == true) {
                     if (digitalRead(STOP_Pin) == HIGH) heatingstop();
                     }
}    


void detection(){ // условия проверяемые каждые 10 сек  
    
  Vbat = analogRead(BAT_Pin);  // замеряем напряжение на батарее
  Vbat = Vbat / m ;            // переводим попугаи в вольты
 
  if (heating == true)  {  
     
  Serial.println("ATZ"), delay(2000);
      
  Serial.flush(), Serial.println("0105");
  emlRead(), temp1 = (strtol(&obd[6],0,16)-40);
  SeeedOled.setTextXY(2,0),  SeeedOled.putString("Engine TEMP: "); // курсор в 2,0 
  SeeedOled.setTextXY(2,13), SeeedOled.putNumber(temp1), delay(100);

  Serial.flush(), Serial.println("0146");
  emlRead(), temp2 = (strtol(&obd[6],0,16)-40);
  SeeedOled.setTextXY(3,0),  SeeedOled.putString("Temperatura: "); // курсор в 3,0 
  SeeedOled.setTextXY(3,13), SeeedOled.putNumber(temp2), delay(100);
  
  Serial.flush(), Serial.println("010C");
  emlRead(), rpm = ((strtol(&obd[6],0,16)*256)+strtol(&obd[9],0,16))/4;
  SeeedOled.setTextXY(1,0),  SeeedOled.putString("Engine RPM: "); // курсор в 1,0 
  SeeedOled.setTextXY(1,12), SeeedOled.putNumber(rpm), delay(100);
                            } 
                            
    if (SMS_send == true) {  // если фаг SMS_send равен 1 высылаем отчет по СМС
     delay(3000); 
     m590.println("AT+CMGF=1"),        delay(100); // устанавливаем режим кодировки СМС
     m590.println("AT+CSCS=\"gsm\""),  delay(100);  // кодировка GSM
     m590.println("AT+CMGS=\"" + SMS_phone + "\""),delay(100);
     m590.print("Status M590 + CAN +OLED v 1.2 ");
     m590.print("\n Temp.Dvigatel: "), m590.print(temp1);
     m590.print("\n Temp.Ulica: "),    m590.print(temp2);
     m590.print("\n Oboroty: "),       m590.print(rpm);
     m590.print("\n Vbat: "),          m590.print(Vbat);
     m590.print((char)26), delay(1000), SMS_send = false;
                        }                          
  
  if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
  if (Timer > 0 ) Timer--;                                      // если таймер больше ноля  
  if (heating == true && Timer <1) heatingstop();
  if (heating == true && rpm < 600) heatingstop(); 
 
  
}             

void enginestart() {  // программа запуска двигателя
int count = 2;   // переменная хранящая число оставшихся потыток зауска
int StartTime = 1000;  // переменная хранения времени работы стартера (1 сек. для первой попытки)  
 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && digitalRead(STOP_Pin) == LOW && count > 0) 
    { // если напряжение АКБ больше 10 вольт, зажигание выключено, КПП в нейтрали  и счетчик числа попыток не равен 0 то...
  digitalWrite(ACTIV_Pin, HIGH); // зажжем светодиод для индикации процесса 
  SeeedOled.setTextXY(0,0),  SeeedOled.putString("ACC+    OFF     ");
  digitalWrite(ON_Pin, LOW),    delay (3000);  // выключаем зажигание на 3 сек. на всякий случай  
  digitalWrite(ON_Pin, HIGH),   delay (2000);  // включаем зажигание  и ждем 5 сек.
  SeeedOled.setTextXY(0,0),  SeeedOled.putString("ACC+    ON      ");

  Serial.println("ATZ"), delay(2000);
   
  Serial.flush(), Serial.println("0105");
  emlRead(), temp1 = (strtol(&obd[6],0,16)-40);
  SeeedOled.setTextXY(2,0),  SeeedOled.putString("Engine TEMP: "); // курсор в 2,0 
  SeeedOled.setTextXY(2,13), SeeedOled.putNumber(temp1), delay(100);

  if (temp1 < -10 ) {
   SeeedOled.setTextXY(0,0),  SeeedOled.putString("ACC+    OFF     ");
   digitalWrite(ON_Pin, LOW),    delay (2000);  // выключаем зажигание на 2 сек. 
   digitalWrite(ON_Pin, HIGH),   delay (4000);  // включаем зажигание  и ждем 4 сек.
   SeeedOled.setTextXY(0,0),  SeeedOled.putString("ACC+    ON      ");
   StartTime = 3000;
   }

  Serial.flush(), Serial.println("0146");
  emlRead(), temp2 = (strtol(&obd[6],0,16)-40);
  SeeedOled.setTextXY(3,0),  SeeedOled.putString("Temperatura: "); // курсор в 3,0 
  SeeedOled.setTextXY(3,13), SeeedOled.putNumber(temp2), delay(100);

  SeeedOled.setTextXY(0,0),  SeeedOled.putString("STARTER    ON  ");          
  digitalWrite(STARTER_Pin, HIGH), delay (StartTime); // включаем реле стартера на 1.0 сек. 
  digitalWrite(STARTER_Pin, LOW),  delay (4000); // отключаем реле, выжидаем 4 сек.
  SeeedOled.setTextXY(0,0),  SeeedOled.putString("STARTER    OFF ");
  
  Serial.flush(), Serial.println("010C");
  emlRead(), rpm = ((strtol(&obd[6],0,16)*256)+strtol(&obd[9],0,16))/4;
  SeeedOled.setTextXY(1,0),  SeeedOled.putString("Engine RPM: "); // курсор в 1,0 
  SeeedOled.setTextXY(1,12), SeeedOled.putNumber(rpm), delay(100);
  
  StartTime = StartTime + 100; // увеличиваем время следующего старта на 0.2 сек.
  count--;                         // уменьшаем на еденицу число оставшихся потыток запуска
  
  // if (digitalRead(DDM_Pin) != LOW) // если детектировать по датчику давления масла 
     if (rpm > 700) // если обороты двигателя больше 700
        { 
        heating = true, digitalWrite(ACTIV_Pin, HIGH);
        SeeedOled.setTextXY(0,0),  SeeedOled.putString("STARTING  OK!  ");  
        break; 
        }
        else 
        { // если статра нет вертимся в цикле пока 
        heating = false, digitalWrite(ON_Pin, LOW), digitalWrite(ACTIV_Pin, LOW);
        SeeedOled.setTextXY(0,0),  SeeedOled.putString("STARTING Fail.."), delay(3000); 
        }
      }
  if (digitalRead(Feedback_Pin) == LOW) heating = false, SMS_send = true;  // шлем смс только в случае незапуска
 //  SMS_send = true;  // отчет всегда
 }


void heatingstop() {  // остановка прогрева двигателя
    digitalWrite(ON_Pin, LOW), digitalWrite(ACTIV_Pin, LOW), heating= false; 
    SeeedOled.setTextXY(0,0),  SeeedOled.putString("Heating  stopped"); 
                   }

void emlRead(void){ // обрабатываем ответ типа 41 0С 0С D8
  char obdChar=0;
  while(obdChar != '\r'){
    if(Serial.available() > 0){
      if(Serial.peek() == '\r'){
        obdChar = Serial.read(), obd[obdIndex]='\0',obdIndex=0;
       }else  obdChar = Serial.read(), obd[obdIndex++]=obdChar;
            
                              }
                        }
                  }

/*
Скетч использует 13092 байт (40%) памяти устройства. Всего доступно 32256 байт.
Глобальные переменные используют 946 байт (46%) динамической памяти, оставляя 1102 байт для локальных переменных. Максимум: 2048 байт.*/
