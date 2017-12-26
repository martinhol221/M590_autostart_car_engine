//https://www.drive2.ru/l/474186105906790427/
//https://www.drive2.ru/c/485387655492665696/
#include <SoftwareSerial.h>
SoftwareSerial m590(4, 5);          // RX, TX  для новой платы
#include <DallasTemperature.h>      // https://github.com/milesburton/Arduino-Temperature-Control-Library
#define ONE_WIRE_BUS 12             // https://github.com/PaulStoffregen/OneWire
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
/*  ----------------------------------------- НАЗНАЧАЕМ ВЫВОДЫ --------------------------------------------------------------------   */
#define FIRST_P_Pin 11              // на для дополнительного реле первого положения ключа зажигания
#define SECOND_P 9                  // на реле зажигания, через транзистор с 9-го пина ардуино
#define STARTER_Pin 8               // на реле стартера, через транзистор с 8-го пина ардуино
#define WEBASTO_Pin 10              // на реле подогрева сидений
#define LED_Pin 13                  // на светодиод c 13-го пина для индикации активности 
#define BAT_Pin A0                  // на батарею, через делитель напряжения 39кОм / 11 кОм
#define DDM_Pin A1                  // на датчик давления масла
#define STOP_Pin A2                 // на концевик педали тормоза для отключения режима прогрева или датчик нейтральной передачи
#define Feedback_Pin A3             // на силовой провод замка зажигания


/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String call_phone = "375290000000"; // телефон входящего вызова
String MAC = "59-01-AA-00-00-00";   // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 59-01-XX-XX-XX-XX-XX)
String SENS = "GSM-Sensor";         // Название устройства (придумать свое Citroen 566 или др. для отображения в программе и на карте)
String APN = "internet.mts.by";     // тчка доступа выхода в интернет вашего сотового оператора
String USER = "mts";                // имя выхода в интернет вашего сотового оператора
String PASS = "mts";                // пароль доступа выхода в интернет вашего сотового оператора
String SERVER = "94.142.140.101,8283";  // сервер, порт народмона (на октябрь 2017) 
bool  n_send = true;                // отправка данных на народмон включена (true), отключена (false)
bool SMS_report = true;             // флаг СМС отчета
float Vstart = 12.50;               // поорог распознавания момента запуска по напряжению
float m = 67.41;                   // делитель для перевода АЦП в вольты для резистров 39/11kOm
/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ --------------------------------------------------------------------   */
int inDS;                           // индекс датчиков температуры
int count;                          // счетчик произведенных попыток запуска
String at = "";
String buf ;
float TempDS[10];                  // переменная хранения температуры с датчика двигателя
float Vbat;                        // переменная хранящая напряжение бортовой сети
float V_min;                       // минимальное напряжение в момент кручения сартера
int k = 0;
int interval = 5;                  // интервал отправки данных на народмон 50 сек после старта
int Timer = 0;                     // переменная времени прогрева двигателя по умолчанию
bool heating = false;              // переменная состояния режим прогрева двигателя
bool SMS_send = false;             // флаг разовой отправки СМС
unsigned long Time1, StarterTimeON = 0;


void setup() {
  pinMode(FIRST_P_Pin, OUTPUT);  
  pinMode(SECOND_P,    OUTPUT);
  pinMode(STARTER_Pin, OUTPUT);
  pinMode(WEBASTO_Pin, OUTPUT);
  pinMode(LED_Pin,     OUTPUT);
  
  Serial.begin(9600);              // скорость порта для отладки 9600
  m590.begin(38400);               // скорость порта модема, может быть 38400
  delay(1000);

  if (digitalRead(STOP_Pin) == HIGH) n_send = false;   // включаем народмон при нажатой педали тормоза при подаче питания 
  Serial.println("MAC: "+MAC+" Sensor name: "+SENS+"V1.7/26.12.2017");

/*-смена скорости модема с 9600 на 38400:
установить m590.begin(9600);, раскоментировать m590.println("AT+IPR=38400"), delay (1000);  и m590.begin(38400), прошить
вернуть  вернуть все обратно и прошить. снова.
*/
        }

void loop() {
  if (m590.available()) {                                  // если что-то пришло от модема 
    while (m590.available()) k = m590.read(), at += char(k),delay(1);
    
    if (at.indexOf("CLIP: \""+call_phone+"\",") > -1 ) {   // ищем номер телефона
        delay(50), m590.println("ATH0");                   // если нашли, сбрасываем вызов
            if (heating == false) { enginestart(4);        // включаем запуск если двигатель не в прогревве  
                           } else   heatingstop();         // иначе останавливаем прогрев
                    
    /*  --------------------------------------------------- ПРЕДНАСТРОЙКА МОДЕМА M590 ----------------------------------------------------------------------   */
    } else if (at.indexOf("+PBREADY\r\n") > -1)                                 {m590.println("ATE1;+CMGF=1;+CSCS=\"gsm\";+CLIP=1"); // дважды  ATE1 для модемов версии ниже 1.3
    /*  ----------------------------------------------- ЕСЛИ НЕТ СОЕДИНЕНИЯ с ИНТЕРНЕТ ТО УСТАНОАВЛИВАЕМ ЕГО -----------------------------------------------   */
    } else if (at.indexOf("+XIIC:    0,") > -1 )                                { m590.println("AT+TCPCLOSE=0"),delay(200),m590.println("AT+XISP=0"),delay(200),interval=5;
    } else if (at.indexOf("AT+XISP=0\r\r\nOK\r\n") > -1 )                       {delay(30), m590.println ("AT+CGDCONT=1,\"IP\",\""+APN+"\""),        delay(50); 
    } else if (at.indexOf("AT+CGDCONT=1,\"IP\",\""+APN+"\"\r\r\nOK\r\n") > -1 ) {delay(30), m590.println ("AT+XGAUTH=1,1,\""+USER+"\",\""+PASS+"\""),delay(100); 
    } else if (at.indexOf("AT+XGAUTH=1,1,\""+USER+"\",\""+PASS+"\"") > -1 )     {delay(30), m590.println ("AT+XIIC=1"),                              delay(100);
   /*  ------------------------------ ЕСЛИ ПОДКЛЮЧЕНИЕ ЕСТЬ, ТО КОННЕКТИКСЯ К СЕРВЕРУ И ШВЫРЯЕМ ПАКЕТ ДАНЫХ НА СЕРВЕР---------------------------------------   */    
    } else if (at.indexOf("+XIIC:    1,") > -1 )                                { delay(50), m590.println ("AT+TCPSETUP=0," +SERVER+ "");
    } else if (at.indexOf("+TCPSETUP:0,OK") > -1 )                              { buf = ""; // в переменную и набиваем пакет данных:
              buf ="#" +MAC+ "#" +SENS+ "\n";                                               // MAC адресс устройства
         for (int i=0; i < inDS; i++) buf = buf+ "#Temp" +i+ "#" +TempDS[i]+ "\n";          // выводим массив температур
              buf=buf+ "#Vbat#" +Vbat+ "\n";                                                // напряжение аккумулятора
              buf=buf+ "#Uptime#" +millis()/1000+ "\n";                                     // время работы ардуино в секундах
              buf=buf+ "##";                                                                // закрываем пакет ##
               m590.print("AT+TCPSEND=0,"),    m590.print(buf.length()),  m590.println(""), delay (200);               
    } else if (at.indexOf("AT+TCPSEND=0,") > -1 && at.indexOf("\r\r\n>") > -1) {m590.print(buf), Serial.println(buf), delay (500), m590.println("AT+TCPCLOSE=0");
    } else if (at.indexOf("+TCPRECV:0,7,#estop") > -1 )                        {heatingstop();
    } else if (at.indexOf("+TCPRECV:0,8,#estart") > -1 )                       {enginestart(3);
   }
     Serial.println(at), at = ""; }          // печатаем ответ и очищаем переменную

if (millis()> Time1 + 10000) detection(), Time1 = millis();           // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin) == HIGH) heatingstop();  // экстренный останов прогрева
  
   /*  для отладки*/
     if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    m590.println(at), at = ""; }  //очищаем
  
}   
void detection(){                          // услови проверяемые каждые 10 сек  
  
    Vbat = VoltRead();                     // замеряем напряжение на батарее
    Serial.print("Сетчк: "), Serial.println(interval);
    Serial.print("Таймер:"), Serial.println (Timer);
    
    inDS = 0;                              // индекс датчиков
    sensors.requestTemperatures();         // читаем температуру с трех датчиков
    while (inDS < 10){
          TempDS[inDS] = sensors.getTempCByIndex(inDS);            // читаем температуру
      if (TempDS[inDS] == -127.00){ 
                                  TempDS[inDS]= 80;  // подменяем -127 на 80 в случае не подключенной 1-Wire шины
                                  break; }                               // пока не доберемся до неподключенного датчика
              inDS++;} 
    for (int i=0; i < inDS; i++) Serial.print("Temp"), Serial.print(i), Serial.print("= "), Serial.println(TempDS[i]); 
    Serial.println ("");
        
    if (SMS_send == true && SMS_report == true) { SMS_send = false;  // если фаг SMS_send равен 1 высылаем отчет по СМС
        m590.println("AT+CMGS=\"+"+call_phone+"\""), delay(100);
        m590.print("Privet "+SENS+"!");
        m590.print("\n Voltage BAT Now: "), m590.print(Vbat);
        m590.print("\n Voltage BAT Min: "), m590.print(V_min);   
    for (int i=0; i < inDS; i++)  m590.print("\n Temp"), m590.print(i), m590.print(": "), m590.print(TempDS[i]);
        m590.print("\n Attempts: "), m590.print(count);        
        m590.print("\n Uptime: "),m590.print(millis()/3600000),         m590.print("H.");
        m590.print((char)26);  }
             
    if (Timer == 12) SMS_send = true;             // отправляем СМС за 2 минуты до окончания работы таймера 
    if (Timer > 0 )  Timer--;                     // вычитаем из таймера 1 каждых 10 сек.
    if (heating == true && Timer <1)              Serial.println("Отановка по таймеру"),   heatingstop(); 
    if (heating == true && Vbat < 10.0)           Serial.println("Остановка по напряжению"), heatingstop(); 
    if (heating == false)                         digitalWrite(LED_Pin, HIGH), delay (50), digitalWrite(LED_Pin, LOW);
    if (n_send == true)                           interval--;
    if (interval <1 ) interval = 30, m590.println ("AT+XIIC?");       // запрашиваем статус соединения           
                            
}             
 
void enginestart(int Attempts ) {                                      // программа запуска двигателя
 /*  ----------------------------------------- ПРЕДНАСТРОЙКА ПЕРЕД ЗАПУСКОМ ---------------------------------------------------------*/
Serial.println("Предпусковая настройка");
//  detachInterrupt(1);                                    // отключаем аппаратное прерывание, что бы не мешало запуску

int StTime  = map(TempDS[0], 20, -15, 1000, 5000);         // при -15 крутим стартером 5 сек, при +20 крутим 1 сек
    StTime  = constrain(StTime, 1000, 6000);               // ограничиваем диапазон работы стартера от 1 до 6 сек

int z = map(TempDS[0], 0, -25, 0, 5);                     // задаем количество раз прогрева свечей пропорциолально температуре
    z = constrain(z, 0, 5);                                // огрничиваем попытки от 0 до 5 попыток

    Timer   =  map(TempDS[0], 30, -25, 30, 150);           // при -25 крутим прогреваем 30 мин, при +50 - 5 мин
    Timer   =  constrain(Timer, 30, 180);                  // ограничиваем таймер в зачениях от 5 до 30 минут

    count = 0;                                             // переменная хранящая число совершенных попыток запуска
    V_min = 14;                                            // переменная хранящая минимальные напряжения в ммент старта

// если напряжение АКБ больше 10 вольт, зажигание выключено, температуры выше -25 град, счетчик числа попыток  меньше 5
 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && TempDS[0] > -30 && count < Attempts){ 
 
 count++, Serial.print ("Попытка №"), Serial.println(count); 
    
 digitalWrite(SECOND_P,     LOW),   delay (2000);        // выключаем зажигание на 2 сек. на всякий случай   
 digitalWrite(FIRST_P_Pin, HIGH),   delay (1000);        // включаем реле первого положения замка зажигания 
 digitalWrite(SECOND_P,    HIGH),   delay (4000);        // включаем зажигание, и выжидаем 4 сек.

// прогреваем свечи несколько раз пропорционально понижению температуры, греем по 8 сек. с паузой 2 сек.
while (z > 0) digitalWrite(SECOND_P, LOW), delay(2000), digitalWrite(SECOND_P, HIGH), delay(8000);
 
// если не нажата педаль тормоза или КПП в нейтрали то включаем реле стартера на время StTime
 if (digitalRead(STOP_Pin) == LOW) {         // увеличиваем на еденицу число оставшихся потыток запуска
                                   Serial.println("Стартер включил");
                                   StarterTimeON = millis();
                                   digitalWrite(STARTER_Pin, HIGH);  // включаем реле стартера
                                   } else {
                                   heatingstop();
                                   count = -1;  
                                   break; 
                                   } 
 delay (100);
// V_stON = VoltRead();                              // временно так
 while ((millis() < (StarterTimeON + StTime)) /* && ((VoltRead() + V_stOFF) < V_stON)*/)VoltRead(), delay (50);
 Serial.println("Стартер выключил, ожидаем 6 сек.");
 digitalWrite(STARTER_Pin, LOW), delay (6000);       

// if (digitalRead(DDM_Pin) != LOW) {                // если детектировать по датчику давления масла 

 if (VoltRead() > Vstart) {                          // если детектировать по напряжению зарядки     
                          Serial.println ("Есть запуск!"); 
                          heating = true;
                          digitalWrite(LED_Pin, HIGH);
                          break; }                   // считаем старт успешным, выхдим из цикла запуска двигателя
                          
                
  StTime = StTime + 200;                             // увеличиваем время следующего старта на 0.2 сек.
  heatingstop();   }                                 // уменьшаем на еденицу число оставшихся потыток запуска
                  
Serial.println ("Выход из запуска");
 if (count != 1) SMS_send = true;                   // отправляем смс СРАЗУ только в случае незапуска c первой попытки
 if (heating == false){ Timer = 0;                  // обнуляем таймер если запуска не произошло
           } else digitalWrite(WEBASTO_Pin, HIGH);  // включаем подогрев седений если все ОК
           
 // SIM800.println("ATH0");          // вешаем трубку (для SIM800L) 
 // attachInterrupt(1, callback, FALLING);          // включаем прерывание на обратный звонок
 }


float VoltRead()    {                               // замеряем напряжение на батарее и переводим значения в вольты
              float ADCC = analogRead(BAT_Pin);
                    ADCC = ADCC / m ;
                    Serial.print("Напряжение: "), Serial.print(ADCC), Serial.println(" Вольт");    
                    if (ADCC < V_min) V_min = ADCC;                   
                    return(ADCC); }                  // переводим попугаи в вольты
                    


void heatingstop() {                                // программа остановки прогрева двигателя
    digitalWrite(SECOND_P,    LOW), delay (100);
    digitalWrite(LED_Pin,     LOW), delay (100);
    digitalWrite(FIRST_P_Pin, LOW), delay (100);
    digitalWrite(WEBASTO_Pin, LOW), delay (100);
    heating= false,                 delay(2000); 
    Serial.println ("Выключить все"); }
