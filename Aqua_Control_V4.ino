// Aqua Control V3
// by Kalin Hristov
// This sketch was created by integrating open source sketches from various authors.
#include <Wire.h> 
#include <Time.h>
#include <DS1307RTC.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define I2C_ADDR    0x27  //Дефиниране на I2C адрес на LCD с чип PCF8574T 
//---(Адресиране на крачетата на чип PCF8574 )----
// This are different than earlier/different I2C LCD displays

#define Rs_pin  0
#define Rw_pin  1
#define En_pin  2
#define BACKLIGHT_PIN  3
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
//Флагове за включена и изключена подсветка 
#define  LED_OFF  1
#define  LED_ON  0

#define greenPin  6 //Зелен диод на пин 6.
#define yellowPin 7// Жълт диод на пин 7.
#define redPin 8//Червен диод на пин 8.
#define echoPin 12//Ултрасоник ехо на пин 12.
#define trigPin 13// Утрасоник тригър на пин 13.
//Използвам Relay Board с 4 релета закачени съответно на 2,3,4 и 5 пин
int RelayPin3 = 4;// Управление на вентилатор през реле 3
int RelayPin4 = 5;//Управление на Помпа през реле 4

//int distance;//Дефиниране на променлива разстояние.
int CritTemp = 27; //температура при която се включва охлаждащият вентилатор
//Понеже библиотеката която използвам е за RTC1307 който не поддържа температура
//се налага дефиниране на адресите на термометъра
#define DS3231_I2C_ADDR             0x68
#define DS3231_TEMPERATURE_MSB      0x11// температурна стойносвт превди десетична запетая
#define DS3231_TEMPERATURE_LSB      0x12// температурна свтойновст следв десетична запетая
//Дефиниране на пин за устройство за измерване на температура и влажност
#define DHTPIN A0      
#define DHTTYPE DHT22   
DHT dht(DHTPIN, DHTTYPE);
//Дефиниране на променливи за измерване температурата на часовника
byte temp_msb;
byte temp_lsb;
//Дефениране на променливи и пиновете на които са закачени 3 LED канала
int LED1 = 9;
int LED2 =10;
int LED3 =11;
//Дефиниране на променливи са силата на осветлението.
int CH1;
int CH2;
int CH3;
int map1;
int map2;
int map3;
int map4;
//Дефиниране на променлива за отброяване циклите на лупа
//Използвам се като таймер за визуализиране на текст на дисплея.
int i;
/*-----(Обявяване на обекти на LCD )-----*/  
LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
// Създаване на символи термометър, капка,външна/вътрешна температура, LED канал и разстояние до обект
//Използвам иконките за да пестя място на дисплея, 32 /16х2/ се оказаха прекалено малко за моите нужди :)
byte termometer[8] = //икона термометър
{
    B00100,
    B01010,
    B01010,
    B01110,
    B01110,
    B11111,
    B11111,
    B01110
};

byte droplet[8] = //икона водна капка
{
    B00100,
    B00100,
    B01010,
    B01010,
    B10001,
    B10001,
    B10001,
    B01110,
};

byte internalTemp[8] = {//вътрешна температура, датчик на RTC модула
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b11111,
  0b10001,
  0b11111
};

byte outTemp[8] = { //външна температура, датчик DHT22
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00000,
  0b11111,
  0b10001,
  0b11111
};

byte LED[8] = { //иконка за LED канал
  0b00100,
  0b10101,
  0b01110,
  0b11111,
  0b01110,
  0b10101,
  0b00100,
  0b00000
};

byte Distance[8] = {//иконка за разстояние до обект
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b11111
};

void setup(){
 Serial.begin(9600);//стартиране на конзолата най-вече с цел дебъгинг 
 Wire.begin();
  dht.begin();// инициализиране на сензор DHT22 
 lcd.begin (16,2);  // инициализиране на LCD 
// Включване на подсветката
 lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
 lcd.setBacklight(LED_ON);
//символи...
  lcd.createChar(1,termometer);
  lcd.createChar(2,droplet);
  lcd.createChar(3,internalTemp);
  lcd.createChar(4,outTemp);
  lcd.createChar(5,LED);
  lcd.createChar(6,Distance);
 
pinMode(greenPin, OUTPUT);//Стартиране на 5 пин в режим на предаване.
pinMode(yellowPin, OUTPUT);//Стартиране на 6 пин в режим на предаване. 
pinMode(redPin, OUTPUT);//Стартиране на 7 пин в режим на предаване.
pinMode(echoPin, INPUT);//Стартиране на 8 пин в режим на приемане. 
pinMode(trigPin, OUTPUT);//Стартиране на 9 пин в режим на предаване.
pinMode(RelayPin4, OUTPUT);//Стартиране на 10 пин в режим на предаване.
pinMode(RelayPin3, OUTPUT);// на 3 реле е закачен вентилатора  
pinMode(LED1, OUTPUT);
pinMode(LED2, OUTPUT);
pinMode(LED3, OUTPUT);


}

void loop(){
//Настройки на дисплея
   tmElements_t tm;
   lcd.backlight();  //Backlight ON if under program control
   RTC.read(tm);
// Отчитане на температурата и влажноста. Отнема около 250 милисекунди!
// Дефиниране на променливи за температура и влажност.
//Използвам променливи за да пестя място на дисплея. По-надолу съм дефинирал флоут за конзолата.
  int h = dht.readHumidity();
//Температурата се отчита в градуси по Целзий по подразбиране.
  int t = dht.readTemperature();
//Настройки за утразвуковият датчик и управление на помпата
 int duration; 
 int distance;
digitalWrite(trigPin, LOW);//Изпращане на импус за измерване на разстояние.
delayMicroseconds(2);//изчакване на импулса.
digitalWrite(trigPin, HIGH);//Приемане на импулса.
delayMicroseconds(10);//Изчакване. 
duration = pulseIn(echoPin,HIGH);//пресмятане продължителноста на пътуване на импулса.
distance = (duration/2)/29.1; //Пресмятане на разстоянието в см. спрямо времето на пътуване на импулса.
 //Отброяване на циклите на лупа. 1 цикъл е равен на 1 секунда заради закъснението от 990 милисекунди.
 if (i++ > 10) i = 0; 
 // Поради ограничения размер на дисплея съм разделил информацията която искам да се вижда на две групи 
 //всеки пакет инфо се се вижда приблизтелно по 5 секунди.
  if(i<=5){
    lcd.clear();
    lcd.setCursor(0,0); //Стариране на писането на  позиция 0 ред 0
    print2digits(tm.Hour);
    lcd.print(':');
    print2digits(tm.Minute);
    lcd.print(':');
    print2digits(tm.Second);
    lcd.setCursor(9,0);
    lcd.write(3);
    lcd.write(1);
    lcd.print(" ");
    lcd.print(temp_msb);
    lcd.print(char(223));
    lcd.print("C");
    
    lcd.setCursor(1,1); //Стариране на писането на  позиция 0 ред 1
    //lcd.print(tm.Day);
    //lcd.print('/');
    //lcd.print(tm.Month);
   // lcd.print('/');
    //lcd.print(tmYearToCalendar(tm.Year));
    lcd.write(2);
    lcd.print(" ");
    lcd.print(h);
    lcd.print(" %");
   
    lcd.setCursor(9,1);
    lcd.write(4);
    lcd.write(1);
    lcd.print(" ");
    lcd.print(t);
    lcd.print(char(223));
    lcd.print("C");
    }

 else{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(5);
  lcd.print("1 ");
  print3digits(CH1);
  lcd.print("% ");
  lcd.setCursor(9,0);
  lcd.write(5);
  lcd.print("3 ");
  print3digits(CH3);
  lcd.print("%");
  lcd.setCursor(0,1);
  lcd.write(5);
  lcd.print("2 ");
  print3digits(CH2);
  lcd.print("% ");
  lcd.setCursor(9,1);
  lcd.write(6);
  lcd.print(" ");
  print3digits(distance);
  lcd.print("cm"); 
 }
   delay(990);
 
   
   //PWM настройки на трите канала управляващи ЛЕД осветление
   //Използвам мап функция като зависимоста е между минути и аналогово писане на пина
   //Тъй като времето не може да е повече от 60 мин. съм разделил писането на 2 равни интервала
   //за да постигна двучасово плавно светване и изгасване на LED-a.
 if(tm.Hour ==12) analogWrite(LED1, map(tm.Minute, 0,59,0,125));
 if(tm.Hour ==13) analogWrite(LED1, map(tm.Minute, 0,59,126,255));
   if(tm.Hour ==13) analogWrite(LED2, map(tm.Minute, 0,59,0,125));
   if(tm.Hour ==14) analogWrite(LED2, map(tm.Minute, 0,59,126,255));
 if(tm.Hour ==12) analogWrite(LED3, map(tm.Minute, 0,59,0,125));
 if(tm.Hour ==13) analogWrite(LED3, map(tm.Minute, 0,59,126,255));
 //Тъй като мап функцията функционира само в периода за който е дефинирана за да няма проблеми/например при рестарт на ардуино/ 
 //в помеждутъка между изгрева и залеза дефинирам максимално напрежение на трите канала.  
   if(tm.Hour>13&&tm.Hour<19) digitalWrite(LED1, HIGH);
   if(tm.Hour>=15&&tm.Hour<19) digitalWrite(LED2, HIGH);
   if(tm.Hour>13&&tm.Hour<20) digitalWrite(LED3, HIGH);
   //От тук започва залеза.
 if(tm.Hour ==19) analogWrite(LED1, map(tm.Minute, 0,59,255,126));
 if(tm.Hour ==20) analogWrite(LED1, map(tm.Minute, 0,59,125,0));
   if(tm.Hour ==19) analogWrite(LED2, map(tm.Minute, 0,59,255,126));
   if(tm.Hour ==20) analogWrite(LED2, map(tm.Minute, 0,59,125,0));            
 if(tm.Hour ==20) analogWrite(LED3, map(tm.Minute, 0,59,255,126));
 if(tm.Hour ==21) analogWrite(LED3, map(tm.Minute, 0,59,125,0));

//Сила на осветлението в %
// Тъй като не съм дефинирал променливи за начален час на изгрев и залез
//визуализирането на силата на осветление и неговата стойност не са пряко свързани
//просто съм използвал една и съща зависимост, т.е. за да са еднакви стойностите
//то часовете трябва да са еднакви и при двете функции
map1 = map(tm.Minute, 0, 59, 0 ,50);
map2 = map(tm.Minute, 0, 59, 51, 100);
map3 = map(tm.Minute, 0, 59, 100, 51);
map4 = map(tm.Minute, 0, 59, 50, 0);

if(tm.Hour>=1 && tm.Hour<12){(CH1=0);}
if(tm.Hour>=21 && tm.Hour<=24) {(CH1=0);}

if(tm.Hour>=1 && tm.Hour<13){(CH2=0);}
if(tm.Hour>=21 && tm.Hour<=24) {(CH2=0);}

if(tm.Hour>=1 && tm.Hour<12){(CH3=0);}
if(tm.Hour>21 && tm.Hour<=24) {(CH3=0);}

if(tm.Hour==12){(CH1=map1);}
if(tm.Hour==13){(CH1=map2);}

if(tm.Hour==13){(CH2=map1);}
if(tm.Hour==14){(CH2=map2);}

if(tm.Hour==12){(CH3=map1);}
if(tm.Hour==13){(CH3=map2);}

if(tm.Hour==19){(CH1=map3);}
if(tm.Hour==20){(CH1=map4);}

if(tm.Hour==19){(CH2=map3);}
if(tm.Hour==20){(CH2=map4);}

if(tm.Hour==20){(CH3=map3);}
if(tm.Hour==21){(CH3=map4);}

if(tm.Hour>=14 && tm.Hour<19){(CH1=100);}
if(tm.Hour>=15 && tm.Hour<19){(CH2=100);}
if(tm.Hour>=14 && tm.Hour<20){(CH3=100);}
//визуализиране на силата на свттлината в проценти
Serial.print("LED#1-PWM =");
Serial.print(CH1);
Serial.print("%  "); 
Serial.print("LED#2-PWM =");
Serial.print(CH2);
Serial.print("%  "); 
Serial.print("LED#3-PWM =");
Serial.print(CH3);
Serial.println("%  "); 




if (distance>=20&&distance>=15){digitalWrite(greenPin, HIGH); } //Включване на зеления ЛЕД при определено разстояние в см.
 else{digitalWrite(greenPin, LOW);}//Ако разстоянието не отговаря на горното условие зеленият ЛЕД е изключен.

if (distance<15&&distance>=4){ digitalWrite(yellowPin, HIGH);} //Включване на жълтия ЛЕД при определено разстояние в см.
 else{digitalWrite(yellowPin, LOW);}//Ако разстоянието не отговаря на горното условие жълтият ЛЕД е изключен
  
if (distance<4&&distance>=1){digitalWrite(redPin, HIGH);}//Включване на червения ЛЕД при определено разстояние в см.
 else{digitalWrite(redPin, LOW);}//Ако разстоянието не отговаря на горното условие червеният ЛЕД е изключен.

if (distance<=3){digitalWrite(RelayPin4, LOW);}//При разтояние по-малко или равно на....см. помпата се изключва.
 else{digitalWrite(RelayPin4, HIGH);}//Ако горното условие не е изпълнено помпата стои включена. 

if (distance>120||distance<=0){Serial.println("Out of Range");}// Дефиниране на диапазона "Извън обхват" който се показва чрез сериния порт.
  
if (distance<=2){Serial.println("Critical Level");}//Дефиниране на "Критично ниво".
 else{
  Serial.print("Distance to object = ");
  Serial.print(distance);
  Serial.println("cm");}//Ако горните условия не са изпълнени на сериният порт ще се показват стойностите за разстояние в см.


//Управление на охлаждащият ветилатор чрез температурата RTC модула (msb е температурата преди десетичната запетая)
if(temp_msb>CritTemp){digitalWrite(RelayPin3, LOW);}
else {digitalWrite(RelayPin3, HIGH);}


//Визуализиране на температурата на RTC модула
  temp_msb = DS3231_get_MSB();
  temp_lsb = DS3231_get_LSB();
  Serial.print("Internal Clock temperature = ");
  Serial.print(temp_msb);

 switch(temp_lsb){
 case 0:
  Serial.print(".00");
 break;
 case 1 :
  Serial.print(".25");
 break;
 case 2:
  Serial.print(".50");
 break;
 case 3:
  Serial.print(".75");
 break;
 }
  Serial.print(char(176));
  Serial.println("C ");
//Дефиниране на флоут за температура и влажност които ще се показват в конзолата  
float T = dht.readTemperature();
float H = dht.readHumidity();
  Serial.print("Humidity: ");
  Serial.print(H);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(T);
  Serial.print(char(176));
  Serial.println("C ");
  
}
//Отчитане на температурата на часовника.
byte DS3231_get_MSB(){
  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_TEMPERATURE_MSB);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDR, 1);
  temp_msb = Wire.read();

}

byte DS3231_get_LSB(){

  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_TEMPERATURE_LSB);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDR, 1);
  temp_lsb = Wire.read() >> 6;


}
//Визуализиране на стойности между 0 и като двуцифрени т.е. 01,02 и т.н.
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.print('0');
  }
  lcd.print(number);
}
//идеята е същата както при горното правило но в случая правилото се отнася за
// трицифрени числа, и двете правила имат чисто естетически характер 
//използват че за по-прегледно изобразяване на инфото на дисплея 
void print3digits (int number) {
  if (number >=0 && number< 10){
    lcd.print("00");}
  if (number >9 && number <100){
    lcd.print("0");}
    lcd.print(number);
}  


