//https://www.drive2.ru/l/474186105906790427/
//https://www.drive2.ru/c/476276827267007358/
#include <SoftwareSerial.h>
SoftwareSerial m590(5, 4); // RX, TX  для новой платы
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#define ONE_WIRE_BUS 11 // https://github.com/PaulStoffregen/OneWire
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

#define STARTER_Pin 8      // на реле стартера, через транзистор с 8-го пина ардуино
#define ON_Pin 9           // на реле зажигания, через транзистор с 9-го пина ардуино
#define FIRST_P_Pin 11     // на для дополнительного реле первого положения ключа зажигания
#define ACTIV_Pin 13       // на светодиод c 13-го пина для индикации активности 
#define BAT_Pin A0         // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Feedback_Pin A3    // на силовой провод замка зажигания
#define STOP_Pin A2        // на концевик педали тормоза для отключения режима прогрева или датчик нейтральной передачи
#define DDM_Pin A1         // на датчик давления масла
#define boot_Pin 3         // на 19-ю ногу модема для его пробуждения
#define ring_Pin 2         // на 10-ю ногу модема для пробуждения ардуино


float tempds0;             // переменная хранения температуры с датчика двигателя
float tempds1;             // переменная хранения температуры с датчика на улице
int Timer2 = 1080;       // интервал опроса датчиков на низкую температуру (3 часа)
int k = 0;
String at = "";
String SMS_phone = "+375000000000"; // куда шлем СМС
String call_phone = "375000000000"; // телефон хозяина без плюса
unsigned long Time1 = 0;
unsigned long PMM = 0;   // счетчик импульсов от датчика распредвала
int Timer = 0;           // переменная времени прогрева двигателя по умолчанию
bool heating = false;    // переменная состояния режим прогрева двигателя
bool SMS_send = true;    // флаг разрешений отправки СМС
float Vbat;              // переменная хранящая напряжение бортовой сети
float Vstart = 12.50;    // поорог распознавания момента запуска по напряжению
float m = 66.80;         // делитель для перевода АЦП в вольты для резистров 39/11kOm

void setup() {
  pinMode(ring_Pin, INPUT);
  pinMode(STARTER_Pin, OUTPUT);
  pinMode(ON_Pin, OUTPUT);
  pinMode(FIRST_P_Pin, OUTPUT);
  pinMode(boot_Pin, OUTPUT);
  digitalWrite(boot_Pin, LOW);

  Serial.begin(9600);     // скорость порта для отладки
  m590.begin(9600);       // скорость порта модема, может быть 38400
  delay(1000);
  if (digitalRead(STOP_Pin) == LOW) SMS_send = false;  // выключаем СМС оповещения
  Serial.print("Starting M590 LITE 16.10.2017, n_send =  "), Serial.println(SMS_send);
//m590.println("AT+IPR=9600");  // настройка скорости M590 если не завелся на 9600 но завелся на 38400
              }

void loop() {
  if (m590.available()) { // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    
    if (at.indexOf("RING") > -1) {
        m590.println("AT+CLIP=1"), Serial.print(" R I N G > ");  //включаем АОН
        if (at.indexOf(call_phone) > -1) delay(50), m590.println("ATH0"), Timer = 60, enginestart();
    
    } else if (at.indexOf("123starting10") > -1 ) {                     // команда запуска на 10 мин.
           Serial.println(at);
           Serial.println("123 s t a r t i n g 10");
           Timer = 60;
           enginestart();
           
    } else if (at.indexOf("123starting20") > -1 ) {                   // команда запуска на 20 мин.
           Serial.println(at);
           Serial.println("123 s t a r t i n g 20");
           Timer = 120;
           enginestart();

    } else if (at.indexOf("123stop") > -1 ) {                         // команда остановки остановки
           Serial.println("123 s t o p");
           Timer = 0;      
     /*                                                       
    } else if (at.indexOf("\"SM\",") > -1) {Serial.println("in SMS"); // если пришло SMS
           m590.println("AT+CMGF=1"), delay(50); // устанавливаем режим кодировки СМС
           m590.println("AT+CSCS=\"gsm\""), delay(50);  // кодировки GSM
           m590.println("AT+CMGR=1"), delay(20), m590.println("AT+CMGD=1,4"), delay(20);  
    */
    
    } else if (at.indexOf("+PBREADY") > -1) {             // если модем стартанул  
           Serial.println(" P B R E A D Y > ATE0 / AT+CMGD=1,4 / AT+CNMI / AT+CMGF/ AT+CSCS");
           m590.println("ATE0"),              delay(100);        // отключаем режим ЭХА модема
           m590.println("AT+CMGD=1,4"),       delay(100);        // стираем все СМС
        // m590.println("AT+CNMI=2,1,0,0,0"), delay(100);        // включем оповещения при поступлении СМС
           m590.println("AT+CMGF=1"),         delay(100);        // 
           m590.println("AT+CSCS=\"gsm\""),   delay(100);        // кодировка смс           

     } else  Serial.println(at);                                 // если пришло что-то другое выводим в серийный порт
     at = "";                                                    // очищаем переменную
           }

if (millis()> Time1 + 10000) detection(), Time1 = millis();      // выполняем функцию detection () каждые 10 сек 
if (heating == true) {                                           // если нажали на педаль СТОП в прогреве
                     if (digitalRead(STOP_Pin) == HIGH) heatingstop();
                     }

/*

if (Serial.available())  {                                       // если что-то пришло из сериала
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
     if (at.indexOf("enginestart") > -1 )  Timer = 60, enginestart();  
         at = "";       }
*/
                     
}   
void detection(){                                                // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();                               // читаем температуру с трех датчиков
    tempds0 = sensors.getTempCByIndex(0);
    tempds1 = sensors.getTempCByIndex(1);
    
    Vbat = analogRead(BAT_Pin);                                    // замеряем напряжение на батарее
    Vbat = Vbat / m ;                                              // переводим попугаи в вольты
    Serial.print("Vbat= "),Serial.print(Vbat), Serial.print (" V.");  
    Serial.print(" || Temp0 : "), Serial.print(tempds0);           // дополнительный датчик на двигателе
    Serial.print(" || Temp1 : "), Serial.print(tempds1);           // дополнительный датчик в салоне
    Serial.print(" || Timer2 : "), Serial.print(Timer2);       // таймер 3-х часового отсчета
    Serial.print(" || Timer ="), Serial.println (Timer); // таймер прогрева
   
    if (Timer > 0 ) Timer--;  
    if (heating == true && Timer <1) Serial.println("End timer"), heatingstop(); 
    if (heating == true && Vbat < 11.3) Serial.println("Low voltage"), heatingstop(); 
    if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом

    // if (Timer2 == 2 && tempds0 < -18 && tempds0 != -127) Timer2 = 1080, Timer = 120, enginestart();  
    // if (Timer2 == 1 && tempds0 < -25 && tempds0 != -127) Timer2 = 360, SMS_Send();    
    Timer2--;
    if (Timer2 <1 ) Timer2 = 1080;
   
   
}             

void SMS_Send() {                                                // функция отправки СМС отчета
        Serial.println("SMS send start...");
        m590.println("AT+CMGF=1"), delay(100);                   // устанавливаем режим кодировки СМС
        m590.println("AT+CSCS=\"gsm\""),delay(100);              // кодировки GSM
        m590.println("AT+CMGS=\"" + SMS_phone + "\""),delay(100);
        m590.print("Status m590 v 16.10.2017 ");
        m590.print("\n Temperatura dvigatelja: "), m590.print(tempds0),  m590.print("grad.");
        m590.print("\n Temperatura salona: "),     m590.print(tempds1), m590.print("grad.");
        m590.print("\n Vbat: "), m590.print(Vbat), m590.print(" Volt");
        m590.print("\n PMM: "), m590.print(PMM*2), m590.print("ob./7 sec. ");
        m590.print("\n Uptime: "), m590.print(millis()/3600000), m590.print(" H.");
        m590.print((char)26),delay(500);
        Serial.print("Temperatura dvigatelja: "), Serial.print(tempds0),  Serial.println("grad.");     
        Serial.print("Temperatura salona: "),     Serial.print(tempds1),  Serial.println("grad.");
        Serial.print("Vbat: "), Serial.print(Vbat), Serial.println(" Volt");
        Serial.print("PMM: "), Serial.print(PMM*2), Serial.println("ob./7 sec. ");
        Serial.print("Uptime: "), Serial.print(millis()/60000), Serial.println(" Min.");
                   }
             

 
void enginestart() {                                            // вызов цикла запуска
    Serial.println ("enginestart() > , count = 3 || StarterTime = 1000");
    int count = 3;                                                  // переменная хранящая число оставшихся потыток зауска
    int StarterTime = 1000;                                        //  времени работы стартера (1 сек. для первой попытки)  
    if (tempds0 < 15  && tempds0 != -127)  StarterTime = 1200, count = 2;   // при 15 - 2 папытки по 1.2 / 1.4 сек
    if (tempds0 < 5   && tempds0 != -127)  StarterTime = 1500, count = 2;   // при  5 - 2 папытки по 1.5 / 1.7 сек
    if (tempds0 < -5  && tempds0 != -127)  StarterTime = 2000, count = 3;   // при -5 - 3 папытки по 2.0 / 2.2 / 2.4 сек
    if (tempds0 < -10 && tempds0 != -127)  StarterTime = 3000, count = 3;  // при -10 - 3 папытки по 3.0 / 3.2 / 3.4 сек
    if (tempds0 < -15 && tempds0 != -127)  StarterTime = 5000, count = 4;  // при -15 - 4 папытки по 5.0 / 5.2 / 5.4 / 5.6 сек
    if (tempds0 < -30 && tempds0 != -127)  StarterTime = 0,    count = 0;    // даже не пытаемся заводиться
    
    while (Vbat > 10.00  && digitalRead(Feedback_Pin) == LOW && digitalRead(STOP_Pin) == LOW && count  > 0) { // цикл запуска
     // если напряжение АКБ больше 10 вольт, зажигание выключено, КПП в нейтрали  и счетчик числа попыток не равен 0 то...
    Serial.print ("count = "), Serial.print (count), Serial.print (" ||  StarterTime = "), Serial.println (StarterTime);
     
    digitalWrite(ACTIV_Pin, HIGH);                        // зажжем светодиод для индикации процесса 
    digitalWrite(FIRST_P_Pin, HIGH);                      // включаем реле первого положения замка зажигания 

    digitalWrite(ON_Pin, LOW),    delay (3000);           // выключаем зажигание на 3 сек. на всякий случай  
    digitalWrite(ON_Pin, HIGH),   delay (5000);           // включаем зажигание  и ждем 5 сек.

    // дополнительный прогрев свечей при температурах ниже  -5 и - 15 градусов
    if (tempds0 < -5 && tempds0 != -127)  digitalWrite(ON_Pin, LOW), delay (2000), digitalWrite(ON_Pin, HIGH), delay (6000);
    if (tempds0 < -15 && tempds0 != -127) digitalWrite(ON_Pin, LOW), delay (10000), digitalWrite(ON_Pin, HIGH), delay (8000);
    
    digitalWrite(STARTER_Pin, HIGH);                     // включаем реле стартера
    delay (StarterTime);                                 // держим время StarterTime
    digitalWrite(STARTER_Pin, LOW);                      // включаем реле стартера
    PMM = 0;                                             // обнуляем счетчик импульсов
    attachInterrupt(1, PMM_count, CHANGE);              // внешним прерыванием добавляем еденицу каждых 2 оборота двигателя
    delay (7000);                                        // ожидаем 7 секунд
    detachInterrupt(1);                                  // отключаем прерывание
    Serial.print (" PMM = "), Serial.println (PMM);
    
    Vbat =        analogRead(BAT_Pin), delay (300);       // замеряем напряжение АКБ 1 раз
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  2-й раз 
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  3-й раз
    Vbat = Vbat / m / 3 ;                                 // переводим попугаи в вольты и плучаем срееднне 3-х замеров

    StarterTime = StarterTime + 200;                      // увеличиваем время следующего старта на 0.2 сек.
    count--;                                              // уменьшаем на еденицу число оставшихся потыток запуска
    
     // if (digitalRead(DDM_Pin) != LOW)                  // если детектировать по датчику давления масла /-А-/
     // if (digitalRead(DDM_Pin) != HIGH)                 // если детектировать по датчику давления масла /-В-/
        if (Vbat > Vstart)                                   // если детектировать по напряжению зарядки     /-С-/
    //  if (PMM > 10)                                    // если детектировать по выбегу маховика        /-D-/
     {
        Serial.print ("heating = true, break,  Vbat > Vstart = "), Serial.println(Vbat); 
        heating = true, digitalWrite(ACTIV_Pin, HIGH);
        SMS_Send();
        break; // считаем старт успешным, выхдим из цикла запуска двигателя
                        }
                        
        else { // если статра нет вертимся в цикле пока 
        Serial.print ("heating = false,  Vbat < Vstart = "), Serial.println(Vbat); 
        digitalWrite(ON_Pin, LOW);
        digitalWrite(ACTIV_Pin, LOW);
        digitalWrite(FIRST_P_Pin, LOW);
        heating = false, delay(3000);
             }
      }
  Serial.println ("Out enginestart().....  ");          // выходим из цикла вызова запуска
 // if (digitalRead(Feedback_Pin) == LOW) SMS_Send();  // отправляем смс только в случае незапуска
  
 }


 void PMM_count() {
                PMM++;        
                 }


void heatingstop() {  // программа остановки прогрева двигателя
    digitalWrite(ON_Pin, LOW);
    digitalWrite(ACTIV_Pin, LOW);
    digitalWrite(FIRST_P_Pin, LOW);
    Serial.println ("Warming stopped"),
    heating = false, delay(1000); 
                   }

