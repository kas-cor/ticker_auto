#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

/*
  Подключение матричного модуля к SPI
  VCC - 5v
  GND - GND
  DIN - 11pin
  CS - 10pin
  CLC - 13pin
*/
#define PIN_CS 10 // Пин CS матрицы
#define NUMBER_OF_HORIZONTAL_DISPLAYS 1 // Модулей по горизонтали
#define NUMBER_OF_VERTICAL_DISPLAYS 8 // Модулей по вертикали
#define WAIT 25 // Задержка в милисекундах (скорость строки)
#define MATRIX_MATRIX_MATRIX_ROTATE 3 // Поворот матрицы 1 - 90,  2 - 180, 3 - 270
#define MATRIX_BRIGHTNESS 15 // Яркость матричного модуля от 0 до 15
Max72xxPanel matrix = Max72xxPanel(PIN_CS, NUMBER_OF_HORIZONTAL_DISPLAYS, NUMBER_OF_VERTICAL_DISPLAYS);

// Пины к которым подключены выводы радиомодуля
#define BUTTON_PIN_1 5 // Кнопка "A"
#define BUTTON_PIN_2 4 // Кнопка "B"
#define BUTTON_PIN_3 3 // Кнопка "C"
#define BUTTON_PIN_4 2 // Кнопка "D"

// Пищалка
#define BUZZ_PIN 6 // Пин к которому подключена пищалка
#define BUZZ_HZ 1000 // Частота пищалки в Hz

// Текст при запуске
#define START_TEXT "Поехали!"

String tape = "";
const int spacer = 1; // Промежуток между символами (кол-во точек)
const int width = 5 + spacer; // Ширина шрифта
int clicks = 0;
int currentBtn = 0;
unsigned long timeBtn = 0;
boolean roll = true;
boolean roll_loop = false;

// Фразы на один, два и три клика
String tapes[][3] = {
  {"Спасибо!", "Не жмись!", "Не дуди!"},
  {"Пропусти", "Обгоняй", "Объезжай"},
  {"Прошу прощения", "Понять и простить", "Сорян, спешу..."},
  {"Я состарюсь в этой пробке", "Спасибо всем кто был со мной всё это время", "АВАРИЯ"}
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
  pinMode(BUTTON_PIN_1, INPUT);
  pinMode(BUTTON_PIN_2, INPUT);
  pinMode(BUTTON_PIN_3, INPUT);
  pinMode(BUTTON_PIN_4, INPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  Serial.begin(9600);
  matrix.setIntensity(MATRIX_BRIGHTNESS);
  matrix.setRotation(matrix.getRotation() + MATRIX_ROTATE);
  tape = utf8rus(START_TEXT);
}

void loop() {
  int btn = 0;
  boolean bloop = true;
  if (roll || roll_loop) {
    if (Serial.available()) {
      tape = Serial_Read();
    }
    if (tape.length() > 8) {
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

        bloop = loopString(bloop);

        delay(WAIT);
      }
    } else {
      for ( int y = matrix.height() ; y >= 0 - matrix.height() ; y-- ) {
        matrix.fillScreen(LOW);
        int x = matrix.width() / 2 - (width * tape.length() - spacer) / 2;
        int letter = 0;
        while ( letter < tape.length() ) {
          matrix.drawChar(x, y, tape[letter], HIGH, LOW, 1);
          letter++;
          x += width;
        }

        matrix.write(); // Посылаем изображение на матрицу

        for ( int i = 0 ; i < tape.length() ; i++ ) {
          bloop = loopString(bloop);
          if (y == 0) {
            delay(500);
          }
        }

        delay(WAIT);
      }
    }
    if (!roll_loop) {
      tone(BUZZ_PIN, BUZZ_HZ, 200);
    } else {
      tone(BUZZ_PIN, BUZZ_HZ, 5);
    }
    roll = false;
  }
  btn = statusBtn();
  if (btn != 0) {
    pressBtn(btn);
  }
  if (timeBtn != 0 && millis() - timeBtn > 500) {
    sendCommand(currentBtn, clicks);
    timeBtn = 0;
    clicks = 0;
    currentBtn = 0;
  }
}

boolean loopString(boolean bloop) {
  if (statusBtn() != 0 && bloop) {
    tone(BUZZ_PIN, BUZZ_HZ, 100);
    roll_loop = !roll_loop;
    bloop = false;
  }
  return bloop;
}

int statusBtn() {
  if (digitalRead(BUTTON_PIN_1) == HIGH) {
    return 1;
  }
  if (digitalRead(BUTTON_PIN_2) == HIGH) {
    return 2;
  }
  if (digitalRead(BUTTON_PIN_3) == HIGH) {
    return 3;
  }
  if (digitalRead(BUTTON_PIN_4) == HIGH) {
    return 4;
  }
  return 0;
}

void pressBtn(int btn) {
  if (btn != currentBtn || millis() - timeBtn > 50) {
    if (btn == currentBtn) {
      clicks++;
    } else {
      clicks = 0;
    }
    tone(BUZZ_PIN, BUZZ_HZ, 100);
    currentBtn = btn;
  }
  timeBtn = millis();
}

void sendCommand(int btn, int clks) {
  delay(50);
  for (int i = 0; i < clks + 1; i++) {
    tone(BUZZ_PIN, BUZZ_HZ, 50);
    delay(250);
  }
  tape = utf8rus(tapes[btn - 1][clks]);
  roll = true;
}
