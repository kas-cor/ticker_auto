#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

/*
 * Настройки матричного модуля
 */
 
/*
  Подключение матричного модуля
  VCC - 5v
  GND - GND
  DIN - 11pin
  CS - 10pin
  CLC - 13pin
*/
int pinCS = 10;
int numberOfHorizontalDisplays = 1; // Модулей по горизонтали
int numberOfVerticalDisplays = 8; // Модулей по вертикали
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
String tape = "";
int wait = 25; // Задержка в милисекундах (скорость строки)
int spacer = 1; // Промежуток между символами (кол-во точек)
int width = 5 + spacer; // Ширина шрифта

/*
 * Настройки радиомодуля
 */

// Пины к которым подключены выводы радиомодуля
const int buttonPin1 = 5; // Кнопка "A"
const int buttonPin2 = 4; // Кнопка "B"
const int buttonPin3 = 3; // Кнопка "C"
const int buttonPin4 = 2; // Кнопка "D"

// Пищалка
const int buzzPin = 6; // Пин к которому подключена пищалка
conct int buzzHz = 1000; // Частота пищалки в Hz

int clicks = 0;
int currentBtn = 0;
unsigned long timeBtn = 0;
boolean roll = false;

// Фразы на один и два клика
String tapes[][2] = {
  {"Спасибо!", "Объезжай"},
  {"Хорошо стоим", "Не дуди"},
  {"Не жмись", "Эх, дороги..."},
  {"Понять и простить", "Сорян, спешу..."}
};

/* Recode russian fonts from UTF-8 to Windows-1251 */
String utf8rus(String source) {
  int i, k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };
  k = source.length(); i = 0;
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
            n = source[i]; i++;
            if (n == 0x81) {
              n = 0xA8;
              break;
            }
            if (n >= 0x90 && n <= 0xBF) n = n + 0x2F;
            break;
          }
        case 0xD1: {
            n = source[i]; i++;
            if (n == 0x91) {
              n = 0xB7;
              break;
            }
            if (n >= 0x80 && n <= 0x8F) n = n + 0x6F;
            break;
          }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}

String Serial_Read() {
  unsigned char c; // Переменная для чтения сериал порта
  String Serial_string = ""; // Формируемая из символов строка
  while (Serial.available() > 0) { // Если в сериал порту есть символы
    c = Serial.read(); // Читаем символ
    //Serial.print(c,HEX); Serial.print(" "); Serial.print(c);
    if (c == '\n') {  // Если это конец строки
      return Serial_string; // Возвращаем строку
    }
    if (c == 0xB8) c = c - 0x01;  // Коррекция кодов символа под таблицу ???? так как русские символы в таблице сдвинуты относительно стандартной кодировки utf на 1 символ
    if (c >= 0xBF && c <= 0xFF) c = c - 0x01;
    Serial_string = Serial_string + String(char(c)); // Добавить символ в строку
  }
  return Serial_string;
}

void setup() {
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  pinMode(buttonPin4, INPUT);
  pinMode(buzzPin, OUTPUT);
  Serial.begin(9600);
  matrix.setIntensity(15); // Яркость матричного модуля от 0 до 15
  matrix.setRotation(matrix.getRotation() + 1); // Поворот 1 - 90,  2 - 180, 3 - 270
}

void loop() {
  if (roll) {
    if (Serial.available()) {
      tape = Serial_Read();
    }
    for ( int i = 0 ; i < width * tape.length() + matrix.width() - spacer; i++ ) {
      matrix.fillScreen(LOW);
      int letter = i / width; // номер символа выводимого на матрицу
      int x = (matrix.width() - 1) - i % width;
      int y = (matrix.height() - 8) / 2; // Центр текста по вертикали

      while ( x + width - spacer >= 0 && letter >= 0 ) {
        if ( letter < tape.length() ) {
          matrix.drawChar(x, y, tape[letter], HIGH, LOW, 1);
        }
        letter--;
        x -= width;
      }
      matrix.write(); // Посылаем изображение на матрицу
      delay(wait);
    }
    tone(buzzPin, buzzHz, 200);
    roll = false;
  }

  int btn1 = digitalRead(buttonPin1);
  int btn2 = digitalRead(buttonPin2);
  int btn3 = digitalRead(buttonPin3);
  int btn4 = digitalRead(buttonPin4);
  if (btn1 == HIGH) {
    pressBtn(1);
  }
  if (btn2 == HIGH) {
    pressBtn(2);
  }
  if (btn3 == HIGH) {
    pressBtn(3);
  }
  if (btn4 == HIGH) {
    pressBtn(4);
  }
  if (timeBtn != 0 && millis() - timeBtn > 500) {
    sendCommand(currentBtn, clicks);
    timeBtn = 0;
    clicks = 0;
    currentBtn = 0;
  }
}

void pressBtn(int btn) {
  if (btn != currentBtn || millis() - timeBtn > 50) {
    if (btn == currentBtn) {
      clicks++;
    } else {
      clicks = 0;
    }
    tone(buzzPin, buzzHz, 100);
    currentBtn = btn;
  }
  timeBtn = millis();
}

void sendCommand(int btn, int clks) {
  delay(50);
  for (int i = 0; i < clks + 1; i++) {
    tone(buzzPin, buzzHz, 50);
    delay(250);
  }
  tape = utf8rus(tapes[btn - 1][clks]);
  roll = true;
}

