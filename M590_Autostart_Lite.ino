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


float tempds0;             // переменная хранения температуры с датчика двигателя
float tempds1;             // переменная хранения температуры с датчика на улице

int k = 0;
int led = 13;
//int count = 0 ;         // счетчик неудачных попыток запуска  
int interval = 30;      // интервал отправки данных на народмон 
String at = "";
String SMS_phone = "+375000000000"; // куда шлем СМС
String call_phone = "375000000000"; // телефон хозяина без плюса
unsigned long Time1 = 0;
int WarmUpTimer = 0;     // переменная времени прогрева двигателя по умолчанию
int modem =0;            // переменная состояние модема
bool start = false;      // переменная разовой команды запуска
bool heating = false;    // переменная состояния режим прогрева двигателя
bool SMS_send = false;   // флаг разовой отправки СМС
bool SMS_report = false;   // флаг разовой отправки СМС
bool n_send = false;     // флаг отправки данных на народмон 
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
  m590.begin(9600);     // скорость порта модема, может быть 38400
  delay(1000);
  if (digitalRead(STOP_Pin) == HIGH) SMS_report = true;  // включаем народмон при нажатой педали тормоза при подаче питания 
  Serial.print("Starting M590 12.10.2017, SMS_report =  "), Serial.println(SMS_report);
//m590.println("AT+IPR=9600");  // настройка скорости M590 если не завелся на 9600 но завелся на 38400
              }

void loop() {
  if (m590.available()) { // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    
    if (at.indexOf("RING") > -1) {
        m590.println("AT+CLIP=1"), Serial.println(at);  //включаем АОН
        if (at.indexOf(call_phone) > -1) delay(50), m590.println("ATH0"), WarmUpTimer = 60, start = true;
    /*                                                       
    } else if (at.indexOf("\"SM\",") > -1) {Serial.println("in SMS"); // если пришло SMS
           m590.println("AT+CMGF=1"), delay(50); // устанавливаем режим кодировки СМС
           m590.println("AT+CSCS=\"gsm\""), delay(50);  // кодировки GSM
           m590.println("AT+CMGR=1"), delay(20), m590.println("AT+CMGD=1,4"), delay(20);  
    */
    } else if (at.indexOf("+PBREADY") > -1) {             // если модем стартанул  
           Serial.println(at));
           m590.println("ATE0"),              delay(100); // отключаем режим ЭХА 
           m590.println("AT+CMGD=1,4"),       delay(100); // стираем все СМС
        // m590.println("AT+CNMI=2,1,0,0,0"), delay(100); // включем оповещения при поступлении СМС
           m590.println("AT+CMGF=1"),         delay(100); // 
           m590.println("AT+CSCS=\"gsm\""),   delay(100); // кодировка смс

    } else if (at.indexOf("123starting10") > -1 ) {  // команда запуска на 10 мин.
           Serial.println(at), WarmUpTimer = 60, start = true;
           
    } else if (at.indexOf("123starting20") > -1 ) {  // команда запуска на 20 мин.
           Serial.println(at), WarmUpTimer = 120, start = true;

    } else if (at.indexOf("123stop") > -1 ) {  // команда остановки остановки
           Serial.println(at), WarmUpTimer = 0;           

    } else if (at.indexOf("+TCPRECV:0,") > -1 ) {  // если сервер что-то ответил - закрываем соединение
           Serial.println(at), m590.println("AT+TCPCLOSE=0");      

    } else if (at.indexOf("+TCPCLOSE:0,OK") > -1 ) {  // если соеденение закрылось меняем статус модема
           Serial.println(at), modem = 0; 
           
    } else if (at.indexOf("+TCPSETUP:0,OK") > -1 && modem == 2 ) { // если конект к народмону  успешен 
           Serial.println(at);
           m590.println("AT+TCPSEND=0,75"), delay(200), modem = 3; // меняем статус модема

    } else if (at.indexOf(">") > -1 && modem == 3 ) {  // "набиваем" пакет данными и шлем на сервер 
           Serial.println(at);
           
         m590.print("#M5-12-56-XX-XX-XX#M590+Sensor"); // индивидуальный номер для народмона XX-XX-XX заменяем на свое 
         if (tempds0 > -40 && tempds0 < 54) m590.print("\n#Temp1#"), m590.print(tempds0);  // значение первого датчиака для народмона
         if (tempds1 > -40 && tempds1 < 54) m590.print("\n#Temp2#"), m590.print(tempds1);  // значение второго датчиака для народмона
         m590.print("\n#Vbat#"), m590.print(Vbat);  // напряжение АКБ для отображения на народмоне
         m590.println("\n##");      // обязательный параметр окончания пакета данных
         delay (50), m590.println("AT+TCPCLOSE=0"), modem = 4; // закрываем пакет

     } else  Serial.println(at);    // если пришло что-то другое выводим в серийный порт
     at = "";  // очищаем переменную
}

if (millis()> Time1 + 10000) detection(), Time1 = millis(); // выполняем функцию detection () каждые 10 сек 
if (heating == true) {
                     if (digitalRead(STOP_Pin) == HIGH) heatingstop();
                     }
}   
void detection(){                           // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();          // читаем температуру с трех датчиков
    tempds0 = sensors.getTempCByIndex(0);
    tempds1 = sensors.getTempCByIndex(1);
    
  Vbat = analogRead(BAT_Pin);  // замеряем напряжение на батарее
  Vbat = Vbat / m ; // переводим попугаи в вольты
  Serial.print("Vbat= "),Serial.print(Vbat), Serial.print (" V.");  
  Serial.print(" || Temp : "), Serial.print(tempds0);  
  Serial.print(" || Interval : "), Serial.print(interval);  
  Serial.print(" || WarmUpTimer ="), Serial.println (WarmUpTimer);


    if (modem == 1) {                        // отправляем данные на сервер
      m590.println("AT+XISP=0"), delay (50);
      m590.println("AT+CGDCONT=1,\"IP\",\"internet.life.com.by\""), delay (50);
      m590.println("AT+XGAUTH=1,1,\"life\",\"life\""), delay (50);
      m590.println("AT+XIIC=1"), delay (200);
      m590.println("AT+TCPSETUP=0,94.142.140.101,8283"), modem = 2;
                    }

        
    if (modem == 0 && SMS_send == true && SMS_report == true) {  // если фаг SMS_send равен 1 высылаем отчет по СМС
        delay(3000); 
        m590.println("AT+CMGF=1"),delay(100); // устанавливаем режим кодировки СМС
        m590.println("AT+CSCS=\"gsm\""),delay(100);  // кодировки GSM
        Serial.print("SMS send start...");
        m590.println("AT+CMGS=\"" + SMS_phone + "\""),delay(100);
        m590.print("Status m590 v 12.10.2017 ");
        m590.print("\n Temp.Dvig: "), m590.print(tempds0);
        m590.print("\n Temp.Salon: "), m590.print(tempds1);
        m590.print("\n Vbat: "), m590.print(Vbat);
        m590.print((char)26),delay(100);     
        Serial.println(".... SMS send stop"), delay(1000);
        SMS_send = false;
              }
             
    
    if (WarmUpTimer > 0 && start == true) Serial.println("Starting engine..."), start = false, enginestart(); 
    if (WarmUpTimer > 0 ) WarmUpTimer--;  
    if (heating == true && WarmUpTimer <1) Serial.println("End timer"), heatingstop(); 
    if (heating == true && Vbat < 11.3) Serial.println("Low voltage"), heatingstop(); 
    if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
    
    if (n_send == true) interval--;
    if (interval <1 ) interval = 30, modem = 1;
    if (modem != 0 && interval == 28) Serial.println(" modem != 0 && interval == 28 > T C P C L O S E "), m590.println("AT+TCPCLOSE=0"), modem = 0;  
   
}             
 
void enginestart() {  // программа запуска двигателя
Serial.println ("enginestart() > , count = 3 || StarterTime = 1000");
int count = 3;   // переменная хранящая число оставшихся потыток зауска
int StarterTime = 1000;  // переменная хранения времени работы стартера (1 сек. для первой попытки)  
 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && digitalRead(STOP_Pin) == LOW && count > 0) 
    { // если напряжение АКБ больше 10 вольт, зажигание выключено, КПП в нейтрали  и счетчик числа попыток не равен 0 то...
    Serial.print ("count = "), Serial.println (count), Serial.print (" ||  StarterTime = "), Serial.println (StarterTime);
     
    digitalWrite(ACTIV_Pin, HIGH);                        // зажжем светодиод для индикации процесса 
    digitalWrite(FIRST_P_Pin, HIGH);                      // включаем реле первого положения замка зажигания 
    digitalWrite(ON_Pin, LOW),    delay (3000);           // выключаем зажигание на 3 сек. на всякий случай  
    digitalWrite(ON_Pin, HIGH),   delay (5000);           // включаем зажигание  и ждем 5 сек.
    digitalWrite(STARTER_Pin, HIGH), delay (StarterTime); // включаем реле стартера время StarterTime
    digitalWrite(STARTER_Pin, LOW),  delay (6000);        // отключаем реле, выжидаем 6 сек.
    
    Vbat =        analogRead(BAT_Pin), delay (300);       // замеряем напряжение АКБ 1 раз
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  2-й раз 
    Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  3-й раз
    Vbat = Vbat / m / 3 ;                                 // переводим попугаи в вольты и плучаем срееднне 3-х замеров
   
    StarterTime = StarterTime + 200;                      // увеличиваем время следующего старта на 0.2 сек.
    count--;                                              // уменьшаем на еденицу число оставшихся потыток запуска
   
     // if (digitalRead(DDM_Pin) != LOW)                  // если детектировать по датчику давления масла /-А-/
     // if (digitalRead(DDM_Pin) != HIGH)                 // если детектировать по датчику давления масла /-В-/
     if (Vbat > Vstart) {                                 // если детектировать по напряжению зарядки     /-С-/
        Serial.print ("heating = true, break,  Vbat > Vstart = "), Serial.println(Vbat); 
        heating = true, digitalWrite(ACTIV_Pin, HIGH);
        break; // считаем старт успешным, выхдим из цикла запуска двигателя
                        }
                        
        else { // если статра нет вертимся в цикле пока 
        Serial.print ("heating = true, break,  Vbat < Vstart = "), Serial.println(Vbat); 
        digitalWrite(ON_Pin, LOW);
        digitalWrite(ACTIV_Pin, LOW);
        digitalWrite(FIRST_P_Pin, LOW);
        heating = false, delay(3000);
             }
      }
  Serial.println (" out >>>>> enginestart()");
  if (digitalRead(Feedback_Pin) == LOW) SMS_send = true;  // отправляем смс только в случае незапуска
  
 }


void heatingstop() {  // программа остановки прогрева двигателя
    digitalWrite(ON_Pin, LOW);
    digitalWrite(ACTIV_Pin, LOW);
    digitalWrite(FIRST_P_Pin, LOW);
    Serial.println ("Warming stopped"),
    heating= false, delay(1000); 
                   }

