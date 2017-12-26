int StTime;
int z;
int Timer;


void setup() {
Serial.begin(9600);


for (float TempDS = 85; TempDS > -30; TempDS = TempDS - 3) 

    {
    
    StTime  = map(TempDS, 20, -15, 1000, 5000);         // при -15 крутим стартером 5 сек, при +20 крутим 1 сек
    StTime  = constrain(StTime, 1000, 6000);               // ограничиваем диапазон работы стартера от 1 до 6 сек
    Serial.print("Cтартер "), Serial.print(StTime);
    
    z = map(TempDS, 0, -25, 0, 5);                     // задаем количество раз прогрева свечей пропорциолально температуре
    z = constrain(z, 0, 5);                                // огрничиваем попытки от 0 до 5 попыток
    Serial.print(" | Доп. свечи "), Serial.print(z);
    
    Timer   =  map(TempDS, 30, -25, 30, 150);           // при -25 крутим прогреваем 30 мин, при +50 - 5 мин
    Timer   =  constrain(Timer, 30, 180);                  // ограничиваем таймер в зачениях от 5 до 30 минут
    Serial.print(" |  Таймер "), Serial.print(Timer / 6);
    
    Serial.print(" мин. |  при температуре "), Serial.println(TempDS);}


}

void loop() {


}

    
