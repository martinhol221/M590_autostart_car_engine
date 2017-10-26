//https://www.drive2.ru/l/474186105906790427/
//https://www.drive2.ru/c/476276827267007358/
#include <SoftwareSerial.h>
SoftwareSerial m590(4, 5); // RX, TX  для новой платы
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


float TempDS0;             // переменная хранения температуры с датчика двигателя
float TempDS1;             // переменная хранения температуры с датчика на улице

int k = 0;
int led = 13;
int interval = 2;        // интервал отправки данных на народмон 20 сек после старта
String at = "";
String SMS_phone = "+375000000000"; // куда шлем СМС
String call_phone = "375000000000"; // телефон хозяина без плюса
unsigned long Time1 = 0;
int WarmUpTimer = 0;     // переменная времени прогрева двигателя по умолчанию
bool start = false;      // переменная разовой команды запуска
bool heating = false;    // переменная состояния режим прогрева двигателя
bool SMS_send = false;   // флаг разовой отправки СМС
bool SMS_report = false;   // флаг разовой отправки СМС
bool n_send = false;     // флаг отправки данных на народмон 
float Vbat;              // переменная хранящая напряжение бортовой сети
float Vstart = 12.50;    // поорог распознавания момента запуска по напряжению
float m = 69.80;         // делитель для перевода АЦП в вольты для резистров 39/11kOm

void setup() {
  pinMode(ring_Pin, INPUT);
  pinMode(STARTER_Pin, OUTPUT);
  pinMode(ON_Pin, OUTPUT);
  pinMode(FIRST_P_Pin, OUTPUT);
  pinMode(boot_Pin, OUTPUT);
  digitalWrite(boot_Pin, LOW);

  Serial.begin(9600);     // скорость порта для отладки
  m590.begin(9600);     // скорость порта модема, может быть 38400
  delay(1000);
  if (digitalRead(STOP_Pin) == HIGH) SMS_report = true;  // включаем народмон при нажатой педали тормоза при подаче питания 
  Serial.print("Starting M590 12.10.2017, SMS_report =  "), Serial.println(SMS_report);
//m590.println("AT+IPR=9600");  // настройка скорости M590 если не завелся на 9600 но завелся на 38400
              }

void loop() {
  if (m590.available()) { // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    Serial.println(at);  //  вернем пакет в монитор порта
    
    if (at.indexOf("RING") > -1) {
        m590.println("AT+CLIP=1");                                                           //включаем АОН
        if (at.indexOf(call_phone) > -1) delay(50), m590.println("ATH0"), WarmUpTimer = 60, start = true;
   
    } else if (at.indexOf("+PBREADY\r\n") > -1)                                              {m590.println ("ATE1"),                                       delay(100); 
    } else if (at.indexOf("AT+CMGF=1\r\r\nOK\r\n") > -1)                                     {m590.println ("AT+CSCS=\"gsm\""),                            delay(100);
    } else if (at.indexOf("AT+CSCS=\"gsm\"\r\r\nOK\r\n") > -1)                               {m590.println ("AT+CMGD=1,4"),                                delay(300);
    } else if (at.indexOf("AT+CMGD=1,4\r\r\n") > -1)                                         {m590.println ("AT+CNMI=2,1,0,0,0"),                          delay(300);   
    } else if (at.indexOf("AT+CNMI=2,1,0,0,0\r\r\nOK\r\n") > -1)                             {m590.println ("AT+CMGR=1"),                                  delay(50);  
   
    } else if (at.indexOf("AT+XISP=0\r\r\nOK\r\n") > -1 )                                    {m590.println ("AT+CGDCONT=1,\"IP\",\"internet.life.com.by\""),delay(50); 
    } else if (at.indexOf("AT+CGDCONT=1,\"IP\",\"internet.life.com.by\"\r\r\nOK\r\n") > -1 ) {m590.println ("AT+XGAUTH=1,1,\"life\",\"life\""),            delay (50);
    } else if (at.indexOf("AT+XGAUTH=1,1,\"life\",\"life\"\r\r\nOK\r\n") > -1 )              {m590.println ("AT+XIIC=1"),                                  delay (200);
    } else if (at.indexOf("AT+XIIC=1\r\r\nOK\r\n") > -1 )                                    {m590.println ("AT+TCPSETUP=0,94.142.140.101,8283"),          delay (50);
    } else if (at.indexOf("+TCPSETUP:0,OK") > -1 )                                           {m590.println ("AT+TCPSEND=0,75"),                            delay(300); 
    } else if (at.indexOf("AT+TCPSEND=0,75\r\r\n>") > -1)                                    {// по приглашению "набиваем" пакет данными и шлем на сервер 
           m590.print("#M5-12-56-78-99-66#M590+DS18b20");                                    // индивидуальный номер для народмона 78-99-66 заменяем на свое !!!!
           if (TempDS0 > -40 && TempDS0 < 54) m590.print("\n#Temp1#"), m590.print(TempDS0);  // значение первого датчиака для народмона
           if (TempDS1 > -40 && TempDS1 < 54) m590.print("\n#Temp2#"), m590.print(TempDS1);  // значение второго датчиака для народмона
           m590.print("\n#Vbat#"), m590.print(Vbat);                                         // напряжение АКБ для отображения на народмоне
           m590.print("\n#Uptime#"), m590.print(millis()/3600000);                           // время непрерывной работы устройства
        // delay (50), m590.println("AT+TCPCLOSE=0");                                        // закрываем пакет
            Serial.print("Send Narodmon");
    } else if (at.indexOf("+TCPRECV:0,") > -1 )                                              {delay (5000), m590.println("AT+TCPCLOSE=0");  

  //  } else if (at.indexOf("AT+CMGS=\"" + SMS_phone + "\"\r\r\n>") > -1)                                    {// по приглашению "набираем" СМС-ку


    
  //} else if (at.indexOf("\"SM\",") > -1) {Serial.println("in SMS"); // если пришло SMS 
 //   } else if (at.indexOf("123starting10") > -1 ) { WarmUpTimer = 60,  start = true; // команда запуска на 10 мин.
 //   } else if (at.indexOf("123starting20") > -1 ) { WarmUpTimer = 120, start = true; // команда запуска на 20 мин.
    } else if (at.indexOf("123stop") > -1 )       { WarmUpTimer = 0;  // команда остановки остановки
    
    }
     at = "";                                                                        // очищаем переменную
}

if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
if (heating == true) {
                     if (digitalRead(STOP_Pin) == HIGH) heatingstop();
                     }
}   
void detection(){                           // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();          // читаем температуру с трех датчиков
    TempDS0 = sensors.getTempCByIndex(0);
    TempDS1 = sensors.getTempCByIndex(1);
    
  Vbat = analogRead(BAT_Pin);              // замеряем напряжение на батарее
  Vbat = Vbat / m ;                        // переводим попугаи в вольты
 /* Serial.print("Vbat= "), Serial.print(Vbat), Serial.print (" V.");  
  Serial.print(" || Temp : "), Serial.print(TempDS0);  
  Serial.print(" || Interval : "), Serial.print(interval);  
  Serial.print(" || WarmUpTimer ="), Serial.println (WarmUpTimer);

*/
        
    if (SMS_send == true && SMS_report == true) {  // если фаг SMS_send равен 1 высылаем отчет по СМС
        delay(3000); 
        m590.println("AT+CMGF=1"),delay(100); // устанавливаем режим кодировки СМС
        m590.println("AT+CSCS=\"gsm\""),delay(100);  // кодировки GSM
        Serial.print("SMS send start...");
        m590.println("AT+CMGS=\"" + SMS_phone + "\""), delay(100);
        m590.print("Status m590 v 12.10.2017 ");
        m590.print("\n Temp.Dvig: "), m590.print(TempDS0);
        m590.print("\n Temp.Salon: "), m590.print(TempDS1);
        m590.print("\n Vbat: "), m590.print(Vbat);
        m590.print((char)26),delay(100), SMS_send = false;    
        Serial.println(".... SMS send stop"), delay(1000);
                                               }
             
     
    if (WarmUpTimer > 0 && start == true) Serial.println("Starting engine..."), start = false, enginestart(); 
    if (WarmUpTimer == 48) WarmUpTimer--, SMS_send = true; // отправляем СМС спустя 2 минуты после успешного запуска
    if (WarmUpTimer > 0 )  WarmUpTimer--;                  // вычитаем из таймера 1 цу каждых 10 сек.
    if (heating == true && WarmUpTimer <1) Serial.println("End timer"),   heatingstop(); 
    if (heating == true && Vbat < 11.0)    Serial.println("Low voltage"), heatingstop(); 
    if (heating == false)  digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
    if (n_send == true)    interval--;
    if (interval <1 )      interval = 30, m590.println ("AT+XISP=0"), delay(50);                    // выходим в интернет
   
}             
 
void enginestart() {                                      // программа запуска двигателя
// Serial.println ("enginestart() > , count = 3 || StarterTime = 1000");
int count = 3;                                            // переменная хранящая число оставшихся потыток зауска
int StarterTime = 1400;                                   // переменная хранения времени работы стартера (1 сек. для первой попытки)  
if (TempDS0 < 15 && TempDS0 != -127)  StarterTime = 1200, count = 2;   // при 15 градусах крутим 1.2 сек 2 попытки 
if (TempDS0 < 5  && TempDS0 != -127)  StarterTime = 1800, count = 2;   // при 5  градусах крутим 1.8 сек 2 попытки 
if (TempDS0 < -5 && TempDS0 != -127)  StarterTime = 2200, count = 3;   // при -5 градусах крутим 2.2 сек 3 попытки 
if (TempDS0 <-10 && TempDS0 != -127)  StarterTime = 3300, count = 4;   //при -10 градусах крутим 3.3 сек 4 попытки 
if (TempDS0 <-15 && TempDS0 != -127)  StarterTime = 6000, count = 5;   //при -15 градусах крутим 6.0 сек 5 попыток 
if (TempDS0 <-20 && TempDS0 != -127)  StarterTime = 1,count = 0, SMS_send = true;   //при -20 отправляем СМС 

 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && count > 0) 
    { // если напряжение АКБ больше 10 вольт, зажигание выключено, счетчик числа попыток не равен 0 то...
   
    Serial.print ("count = "), Serial.println (count), Serial.print (" ||  StarterTime = "), Serial.println (StarterTime);

    digitalWrite(ON_Pin, LOW),       delay (2000);        // выключаем зажигание на 2 сек. на всякий случай   
    digitalWrite(FIRST_P_Pin, HIGH), delay (500);         // включаем реле первого положения замка зажигания 
    digitalWrite(ON_Pin, HIGH),      delay (4000);        // включаем зажигание  и ждем 4 сек.

    if (TempDS0 < -5 &&  TempDS0 != -127) digitalWrite(ON_Pin, LOW),  delay (2000), digitalWrite(ON_Pin, HIGH), delay (8000);
    if (TempDS0 < -15 && TempDS0 != -127) digitalWrite(ON_Pin, LOW), delay (10000), digitalWrite(ON_Pin, HIGH), delay (8000);

    if (digitalRead(STOP_Pin) == LOW) digitalWrite(STARTER_Pin, HIGH); // включаем реле стартера
    
    delay (StarterTime);                                  // выдерживаем время StarterTime
    digitalWrite(STARTER_Pin, LOW),  delay (6000);        // отключаем реле, выжидаем 6 сек.
    
    Vbat =        analogRead(BAT_Pin), delay (300);       // замеряем напряжение АКБ 1 раз
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  2-й раз 
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  3-й раз
    Vbat = Vbat / m / 3 ;                                 // переводим попугаи в вольты и плучаем срееднне 3-х замеров
   
     // if (digitalRead(DDM_Pin) != LOW)                  // если детектировать по датчику давления масла /-А-/
     // if (digitalRead(DDM_Pin) != HIGH)                 // если детектировать по датчику давления масла /-В-/
     
     if (Vbat > Vstart) {                                 // если детектировать по напряжению зарядки     /-С-/
        Serial.print ("heating = true, break,  Vbat > Vstart = "), Serial.println(Vbat); 
        heating = true, digitalWrite(ACTIV_Pin, HIGH);
        break; // считаем старт успешным, выхдим из цикла запуска двигателя
                        }
                        
        else { // если статра нет вертимся в цикле пока 
        Serial.print ("heating = true, break,  Vbat < Vstart = "), Serial.println(Vbat); 
        StarterTime = StarterTime + 200;                      // увеличиваем время следующего старта на 0.2 сек.
        count--;                                              // уменьшаем на еденицу число оставшихся потыток запуска
        heatingstop();
             }
      }
  Serial.println (" out >>>>> enginestart()");
 // if (digitalRead(Feedback_Pin) == LOW) SMS_send = true;  // отправляем смс только в случае незапуска
  
 }


void heatingstop() {  // программа остановки прогрева двигателя
    digitalWrite(ON_Pin, LOW),      delay (200);
    digitalWrite(ACTIV_Pin, LOW),   delay (200);
    digitalWrite(FIRST_P_Pin, LOW), delay (200);
    Serial.println ("Ignition OFF"),
    heating= false,                 delay(3000); 
                   }
