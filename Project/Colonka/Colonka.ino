#include "Colonka.h"

// Получение значения кнопки
int getPressedButton()
{
  int buttonValue = analogRead(0);
  if (buttonValue < 100) {
    return BUTTON_RIGHT;  
  }
  else if (buttonValue < 200) {
    return BUTTON_UP;
  }
  else if (buttonValue < 400){
    return BUTTON_DOWN;
  }
  else if (buttonValue < 600){
    return BUTTON_LEFT;
  }
  else if (buttonValue < 800){
    return BUTTON_SELECT;
  }
  return BUTTON_NONE;
}

void setup() {
  // поднимаем частоту опроса аналогового порта до 38.4 кГц, по теореме
  // Котельникова (Найквиста) частота дискретизации будет 19 кГц
  // http://yaab-arduino.blogspot.ru/2015/02/fast-sampling-from-analog-input.html
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  // для увеличения точности уменьшаем опорное напряжение,
  // выставив EXTERNAL и подключив Aref к выходу 3.3V на плате через делитель
  // GND ---[2х10 кОм] --- REF --- [10 кОм] --- 3V3

  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Karpochev's");
  lcd.setCursor(0, 1);
  lcd.print("Smart Colonka");
  lcdChars();   // подхватить коды полосочек
  delay(2000);
}

int menu = 0; // 0 - main menu, 1 - music mode, 2 - spectr mode
int updown = 1; // 1 - music highlight, 2 - spectr highlight

void clearLine(int line){
  lcd.setCursor(0, line);
  lcd.print("                ");
}

void printDisplay(String message, int s){
  Serial.println(message);
  lcd.setCursor(0, s);
  lcd.print(message);
}

void loop() {
  
  int button = getPressedButton();

  if (button == BUTTON_DOWN && updown < 2) updown++;
  if (button == BUTTON_UP && updown > 1) updown--;
  
  if (menu == 0) {
      if (updown == 1) {
        clearLine(0);
        printDisplay("MUSIC", 0);
        clearLine(1);
        printDisplay("spectr", 1);
        delay(500);
      }
      else if (updown == 2) {
        clearLine(0);
        printDisplay("music", 0);
        clearLine(1);
        printDisplay("SPECTR", 1);
        delay(500);
      }
      if (button == BUTTON_SELECT) {
        if (updown == 1) menu = 2;
        if (updown == 2) menu = 2; 
      }
  }

  if (menu == 2) {
    pinMode(SPEAKER, OUTPUT);

    
    
    for (int i = 0; i < 39; i++){
      tone(SPEAKER, notes[i], times[i]*2);
      delay(times[i]*2);
      button = getPressedButton();
      if (button == BUTTON_LEFT) {
        menu = 0;
        break;
      }
    }  
    pinMode(SPEAKER, INPUT);
  
  }
  if (menu == 2) {
  
    // если разрешена ручная настройка уровня громкости
    if (GAIN_CONTROL) gain = map(analogRead(POT_PIN), 0, 1023, 0, 150);
  
    analyzeAudio();   // функция FHT, забивает массив fht_log_out[] величинами по спектру
  
    for (int pos = 0; pos < 16; pos++) {   // для окошек дисплея с 0 по 15
      // найти максимум из пачки тонов
      if (fht_log_out[posOffset[pos]] > maxValue) maxValue = fht_log_out[posOffset[pos]];
  
      lcd.setCursor(pos, 0);
  
      // преобразовать значение величины спектра в диапазон 0..15 с учётом настроек
      int posLevel = map(fht_log_out[posOffset[pos]], LOW_PASS, gain, 0, 15);
      posLevel = constrain(posLevel, 0, 15);
  
      if (posLevel > 7) {               // если значение больше 7 (значит нижний квадратик будет полный)
        lcd.printByte(posLevel - 8);    // верхний квадратик залить тем что осталось
        lcd.setCursor(pos, 1);          // перейти на нижний квадратик
        lcd.printByte(7);               // залить его полностью
      } else {                          // если значение меньше 8
        lcd.print(" ");                 // верхний квадратик пустой
        lcd.setCursor(pos, 1);          // нижний квадратик
        lcd.printByte(posLevel);        // залить полосками
      }
    }
  
    if (AUTO_GAIN) {
       maxValue_f = maxValue * k + maxValue_f * (1 - k);
      if (millis() - gainTimer > 1500) {      // каждые 1500 мс
        // если максимальное значение больше порога, взять его как максимум для отображения
        if (maxValue_f > VOL_THR) gain = maxValue_f;
  
        // если нет, то взять порог побольше, чтобы шумы вообще не проходили
        else gain = 100;
        gainTimer = millis();
      }
    }

    if (button == BUTTON_LEFT) menu = 0;
    
  }
  
}

void lcdChars() {
  lcd.createChar(0, v1);
  lcd.createChar(1, v2);
  lcd.createChar(2, v3);
  lcd.createChar(3, v4);
  lcd.createChar(4, v5);
  lcd.createChar(5, v6);
  lcd.createChar(6, v7);
  lcd.createChar(7, v8);
}
void analyzeAudio() {
  for (int i = 0 ; i < FHT_N ; i++) {
    int sample = analogRead(AUDIO_IN);
    fht_input[i] = sample; // put real data into bins
  }
  fht_window();  // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run();     // process the data in the fht
  fht_mag_log(); // take the output of the fht
}