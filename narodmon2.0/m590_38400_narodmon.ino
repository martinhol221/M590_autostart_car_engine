//https://www.drive2.ru/l/474186105906790427/
//https://www.drive2.ru/c/485387655492665696/
#include <SoftwareSerial.h>
SoftwareSerial m590(4, 5);          // RX, TX  для новой платы
#include <DallasTemperature.h>      // https://github.com/milesburton/Arduino-Temperature-Control-Library
#define ONE_WIRE_BUS 12             // https://github.com/PaulStoffregen/OneWire
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
/*  ----------------------------------------- НАЗНАЧАЕМ ВЫВДЫ --------------------------------------------------------------------   */
#define STARTER_Pin 8               // на реле стартера, через транзистор с 8-го пина ардуино
#define ON_Pin 9                    // на реле зажигания, через транзистор с 9-го пина ардуино
#define FIRST_P_Pin 11              // на для дополнительного реле первого положения ключа зажигания
#define ACTIV_Pin 13                // на светодиод c 13-го пина для индикации активности 
#define BAT_Pin A0                  // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Feedback_Pin A3             // на силовой провод замка зажигания
#define STOP_Pin A2                 // на концевик педали тормоза для отключения режима прогрева или датчик нейтральной передачи
#define DDM_Pin A1                  // на датчик давления масла
#define boot_Pin 3                  // на 19-ю ногу модема для его пробуждения
#define ring_Pin 2                  // на 10-ю ногу модема для пробуждения ардуино
/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String call_phone = "375000000000"; // телефон входящего вызова
String SMS_phone = "+375000000000"; // телефон куда отправляем СМС
String MAC = "59-01-AA-00-00-00";   // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 59-01-XX-XX-XX-XX-XX)
String SENS = "GSM-Sensor";         // Название устройства (придумать свое Citroen 566 или др. для отображения в программе и на карте)
String APN = "internet.mts.by";     // точка доступа выхода в интернет вашего сотового оператора
String USER = "mts";               // имя выхода в интернет вашего сотового оператора
String PASS = "mts";               // пароль доступа выхода в интернет вашего сотового оператора
String PIN = "123";                 // код для ограничения доступа через SMS
bool  n_send = true;                // отправка данных на народмон включена (true), отключена (false)
float Vstart = 12.50;               // поорог распознавания момента запуска по напряжению
int SMS_time = 2;                   // время в минутах с момента попытки запуска, черз которое отправляем СМС-отчет
int Timer_time = 15;                // таймер времени прогрева в минутах
/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ --------------------------------------------------------------------   */
String SERVER = "94.142.140.101,8283";  // сервер, порт народмона (на октябрь 2017) 
String at = "";
String buf ;
float TempDS0;                     // переменная хранения температуры с датчика двигателя
float TempDS1;                     // переменная хранения температуры с датчика на улице
float Vbat;                        // переменная хранящая напряжение бортовой сети
float m = 69.80;                   // делитель для перевода АЦП в вольты для резистров 39/11kOm
int k = 0;
int interval = 5;                  // интервал отправки данных на народмон 20 сек после старта
int WarmUpTimer = 0;               // переменная времени прогрева двигателя по умолчанию
bool heating = false;              // переменная состояния режим прогрева двигателя
bool SMS_send = false;             // флаг разовой отправки СМС
bool SMS_report = true;            // флаг СМС отчета
unsigned long Time1 = 0;



void setup() {
  pinMode(ring_Pin,    INPUT);
  pinMode(STARTER_Pin, OUTPUT);
  pinMode(ON_Pin,      OUTPUT);
  pinMode(FIRST_P_Pin, OUTPUT);
  pinMode(boot_Pin,    OUTPUT);
  digitalWrite(boot_Pin, LOW);
  Serial.begin(9600);              // скорость порта для отладки
  m590.begin(38400);               // скорость порта модема, может быть 38400
  delay(2000);

  if (digitalRead(STOP_Pin) == HIGH) SMS_report = true;  // включаем народмон при нажатой педали тормоза при подаче питания 
  Serial.print("Starting M590, MAC: "+MAC+" Sensor name: "+SENS+" 4.11.2017, SMS_report =  "), Serial.println(SMS_report);

/*-смена скорости модема с 9600 на 38400:
установить m590.begin(9600);, раскоментировать m590.println("AT+IPR=38400"), delay (1000);  и m590.begin(38400), прошить
вернуть  вернуть все обратно и прошить. снова.
*/
// m590.println("AT+IPR=38400"), delay (1000); // настройка скорости M590 если не завелся на 9600 но завелся на 38400
 // m590.begin(38400);
              }

void loop() {
  if (m590.available()) { // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    
    if (at.indexOf("RING") > -1) { m590.println("AT+CLIP=1");                                // реакция на любой вызов , включаем АОН                              
        if (at.indexOf("CLIP: \""+call_phone+"\",") > -1 && at.indexOf("+CMGR:") == -1 ) {   // если прилетел номер телефона и он прилетел не из SMS то 
        delay(50), m590.println("ATH0");
            if (heating == false) {
                         enginestart();                                                      // включаем запуск если двигатьль не в прогревве  
                           } else {
                         heatingstop();                                                      // иначе останавливаем прогрев
                                  }
        }

     /*  --------------------------------------------- РЕАКЦИЯ НА ВХОДЯЩЕЕ СМС -------------------------------------------------------------------------------- 
   //  } else if (at.indexOf("+CMTI: \"SM\",") > -1)     { int i = at.substring(at.indexOf("\"SM\",")+5, at.indexOf("\"SM\",")+7).toInt();
   //                                                          m590.print("AT+CMGR="), m590.print(i), m590.println("");
 
    } else if (at.indexOf(""+PIN+"Webasto") > -1 ) { Timer_time = at.substring(at.indexOf("Webasto")+7,at.indexOf("Webasto")+9).toInt(), enginestart();  
                                                   if (Timer_time == 0) Timer_time = 10;  // если вдруг вернулся ноль меняем его на 10                      
    } else if (at.indexOf("Stop") > -1 )           { heatingstop();                                                                // команда остановки прогрева.
*/
        
    /*  --------------------------------------------------- ПРЕДНАСТРОЙКА МОДЕМА M590 ----------------------------------------------------------------------   */
    } else if (at.indexOf("+PBREADY\r\n") > -1)                    {m590.println ("ATE1"),             delay(100);      // Включить режим ЭХО
    } else if (at.indexOf("ATE1\r\r\nOK\r\n") > -1)                {m590.println ("AT+CMGF=1"),        delay(100);      // Включаем Текстовый режим СМС
    } else if (at.indexOf("AT+CMGF=1\r\r\nOK\r\n") > -1)           {m590.println ("AT+CSCS=\"gsm\""),  delay(100);      // Выбираем кодировку СМС
    } else if (at.indexOf("AT+CSCS=\"gsm\"\r\r\nOK\r\n") > -1)     {m590.println ("AT+CMGD=1,4"),      delay(300);      // Удаляем все СМС
    } else if (at.indexOf("AT+CMGD=1,4\r\r\n") > -1)               {m590.println ("AT+CNMI=2,1,0,0,0"),delay(300);      // Разрешаем прием входящих СМС
    /*  ---------------------------------------------------------- ВХОДИМ В ИНТЕРНЕТ ----------------------------------------------------------------------   */
    } else if (at.indexOf("AT+XISP=0\r\r\nOK\r\n") > -1 )                       {delay(30), m590.println ("AT+CGDCONT=1,\"IP\",\""+APN+"\""),        delay( 50); 
    } else if (at.indexOf("AT+CGDCONT=1,\"IP\",\""+APN+"\"\r\r\nOK\r\n") > -1 ) {delay(30), m590.println ("AT+XGAUTH=1,1,\""+USER+"\",\""+PASS+"\""),delay (100); 
    } else if (at.indexOf("AT+XGAUTH=1,1,\""+USER+"\",\""+PASS+"\"") > -1 )     {delay(30), m590.println ("AT+XIIC=1"),                              delay (100);
    /*  --------------------------------------------------- ПОДКЛЮЧАЕМСЯ К СЕРВЕРУ narodmon.ru:8283 -------------------------------------------------------   */
    } else if (at.indexOf("AT+XIIC=1\r\r\nOK\r\n") > -1 )                       {delay(30), m590.println ("AT+TCPSETUP=0," +SERVER+ ""), delay (1200);
    /*  ------------------------------ ПОЛУЧАЕМ ДОБРО ОТ СЕРВЕРА, СОБИРАЕМ ПАКЕТ ДАННЫХ И ОТПРАВЛЯЕМ ДЛИННУ TCP ПАКЕТА В МОДЕМ ----------------------------   */    
    } else if (at.indexOf("+TCPSETUP:0,OK") > -1 )                                       { buf = "";  // в переменную и набиваем пакет данных:
              buf ="#" +MAC+ "#" +SENS+ "\n";                                                    // MAC адресс устройства
          if (TempDS0 > -40 && TempDS0 < 54) buf=buf+ "#Temp1#" +TempDS0+ "\n";                  // температура двигателя по датчику DS18B20 N1
          if (TempDS1 > -40 && TempDS1 < 54) buf=buf+ "#Temp2#" +TempDS1+ "\n";                  // температура сална по датчику DS18B20 N2
              buf=buf+ "#Vbat#" +Vbat+ "\n";                                                     // напряжение аккумулятора
              buf=buf+ "#Uptime#" +millis()/1000+ "\n";                                          // время работы ардуино в секундах
              buf=buf+ "##";                                                                     // закрываем пакет ##
             m590.print("AT+TCPSEND=0,"),    m590.print(buf.length()),  m590.println(""), delay (200);               
   /*  ------------------------------ ПОЛУЧАЕМ ПРИГЛАШЕНИЕ НА ОТПРАВКУ TCP ПАКЕТА И ШВЫРЯЕМ ЕГО В МОДЕМ ---------------------------------------------------   */  
   } else if (at.indexOf("AT+TCPSEND=0,") > -1 && at.indexOf("\r\r\n>") > -1) {m590.print(buf), Serial.println(buf), delay (500), m590.println("AT+TCPCLOSE=0");
   } else if (at.indexOf("+TCPSEND:0,") > -1 )                                {delay (100), m590.println("AT+TCPCLOSE=0");  
   }
     Serial.println(at), at = "";                                           // очищаем переменную
}

if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin) == HIGH) heatingstop();

}   
void detection(){                           // услови проверяемые каждые 10 сек  
    sensors.requestTemperatures();          // читаем температуру с трех датчиков
    TempDS0 = sensors.getTempCByIndex(0);
    TempDS1 = sensors.getTempCByIndex(1);
    
  Vbat = analogRead(BAT_Pin);              // замеряем напряжение на батарее
  Vbat = Vbat / m ;                        // переводим попугаи в вольты
  Serial.print("Vbat= "), Serial.print(Vbat), Serial.print (" V.");  
  Serial.print(" || Temp : "), Serial.print(TempDS0);  
  Serial.print(" || Interval : "), Serial.print(interval);  
  Serial.print(" || WarmUpTimer ="), Serial.println (WarmUpTimer);

     
    if (SMS_send == true && SMS_report == true) { SMS_send = false;  // если фаг SMS_send равен 1 высылаем отчет по СМС
        Serial.print("SMS send start...");
        m590.println("AT+CMGS=\""+SMS_phone+"\""), delay(100);
        m590.print(""+SENS+"  v 28.10.2017 ");
        m590.print("\n Temp.Dvig: "),  m590.print(TempDS0);
        m590.print("\n Temp.Salon: "), m590.print(TempDS1);
        m590.print("\n Vbat: "),       m590.print(Vbat);
        m590.print("\n Webasto time: "), m590.print(Timer_time);
        m590.print((char)26);                   }

    if (WarmUpTimer == (Timer_time * 6 - SMS_time * 6)) WarmUpTimer--, SMS_send = true;                   // отправляем СМС 
    if (WarmUpTimer > 0 )                               WarmUpTimer--;                  // вычитаем из таймера 1 цу каждых 10 сек.
    if (heating == true && WarmUpTimer <1)              Serial.println("End timer"),   heatingstop(); 
    if (heating == true && Vbat < 11.0)                 Serial.println("Low voltage"), heatingstop(); 
    if (heating == false)                               digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);
    if (n_send == true)                                 interval--;
    if (interval <1 )                                   interval = 30, m590.println ("AT+XISP=0"), delay(100); // выходим в интернет
    if (interval == 15 )                                delay (100),  m590.println("AT+CMGD=1,4"), delay(20);
}             
 
void enginestart() {                                                   // программа запуска двигателя
WarmUpTimer = Timer_time * 6;                                          // присваиваем таймеру обратного отсчета значение
int count = 3;                                                         // переменная хранящая число оставшихся потыток зауска
int StarterTime = 1400;                                                // переменная хранения времени работы стартера (1 сек. для первой попытки)  
if (TempDS0 < 15 && TempDS0 != -127)  StarterTime = 1200, count = 2;   // при 15 градусах крутим 1.2 сек 2 попытки 
if (TempDS0 < 5  && TempDS0 != -127)  StarterTime = 1800, count = 2;   // при 5  градусах крутим 1.8 сек 2 попытки 
if (TempDS0 < -5 && TempDS0 != -127)  StarterTime = 2200, count = 3;   // при -5 градусах крутим 2.2 сек 3 попытки 
if (TempDS0 <-10 && TempDS0 != -127)  StarterTime = 3300, count = 4;   //при -10 градусах крутим 3.3 сек 4 попытки 
if (TempDS0 <-15 && TempDS0 != -127)  StarterTime = 6000, count = 5;   //при -15 градусах крутим 6.0 сек 5 попыток 
if (TempDS0 <-20 && TempDS0 != -127)  StarterTime = 1,count = 0, SMS_send = true;   //при -20 отправляем СМС 

Serial.print ("enginestart.. Timer_time:  "), Serial.print (Timer_time);
Serial.print (" min. || StarterTime: "),      Serial.print (StarterTime);   
Serial.print (" milisec. || TempDS0: "),      Serial.print (TempDS0);  
Serial.print (" milisec. || Coun: "),         Serial.println (count);  


 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && count > 0) 
    { // если напряжение АКБ больше 10 вольт, зажигание выключено, счетчик числа попыток не равен 0 то...
   
    Serial.print ("count = "), Serial.println (count), Serial.print (" ||  StarterTime = "), Serial.println (StarterTime);

    digitalWrite(ON_Pin, LOW),       delay (2000);        // выключаем зажигание на 2 сек. на всякий случай   
    digitalWrite(FIRST_P_Pin, HIGH), delay (500);         // включаем реле первого положения замка зажигания 
    digitalWrite(ON_Pin, HIGH),      delay (4000);        // включаем зажигание  и ждем 4 сек.

//  if (TempDS0 < -5 &&  TempDS0 != -127) digitalWrite(ON_Pin, LOW),  delay (2000), digitalWrite(ON_Pin, HIGH), delay (8000);
//  if (TempDS0 < -15 && TempDS0 != -127) digitalWrite(ON_Pin, LOW), delay (10000), digitalWrite(ON_Pin, HIGH), delay (8000);

    if (digitalRead(STOP_Pin) == LOW) digitalWrite(STARTER_Pin, HIGH); // включаем реле стартера
    
    delay (StarterTime);                                  // выдерживаем время StarterTime
    digitalWrite(STARTER_Pin, LOW),   delay (6000);       // отключаем реле, выжидаем 6 сек.
    
    Vbat =        analogRead(BAT_Pin), delay (300);       // замеряем напряжение АКБ 1-й раз
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
 // if (digitalRead(Feedback_Pin) == LOW) SMS_send = true;   // отправляем смс СРАЗУ только в случае незапуска
 }

void heatingstop() {  // программа остановки прогрева двигателя
    digitalWrite(ON_Pin, LOW),      delay (200);
    digitalWrite(ACTIV_Pin, LOW),   delay (200);
    digitalWrite(FIRST_P_Pin, LOW), delay (200);
    Serial.println ("Ignition OFF");
    heating= false,                 delay(3000); 
                   }
