Control the car engine start using the sms request, and send the data to narodmon.ru Проект для дешевого GSM модема M590 и Arduino Pro Mini для удаленного запуска двигателя автомобиля с управлением по СМС и отправкой данных на сервис сбора данных narodmon.ru https://www.drive2.ru/c/476276827267007358/ DC-DC преобразователь накрученный на выходное напряжение в 4,1 в. для питания модема

На данный момент прошивка поддерживает следующие функции:

Запуск двигателя при звонке с заданного номера и остановка при повторном. Вызов будет отклонен и запустится алгоритм запуска двигателя, или остановки если последний уже запущен, через 2 минуты устройство вернет SMS отчет.
Запуск и остановка по входящему СМС с текстом startXX, и stop для остановки, где xx- это заданное время в минутах (не на всех скетчах, только ...SMS.ino.ino).
Отправка данных о температуре и напряжении АКБ автомобиля и времени непрерывной работы устройства на сервер народмон каждые 5 минут.
Остановка прогрева путем нажатия на педаль тормоза.
![](https://github.com/martinhol221/M590_autostart_car_engine/blob/master/other/IMG_3714.JPG)
![](https://github.com/martinhol221/M590_autostart_car_engine/blob/master/other/IMG_3712-001.JPG)
![](https://github.com/martinhol221/M590_autostart_car_engine/blob/master/other/IMG_3711-002.JPG)
![](https://github.com/martinhol221/M590_autostart_car_engine/blob/master/other/Shema.jpg)
