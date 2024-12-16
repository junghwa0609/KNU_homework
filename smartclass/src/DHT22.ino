#include "DHT.h"  // 온습도 센서 라이브러리
#include <LiquidCrystal_I2C.h>  // LCD 디스플레이 라이브러리
#include <Servo.h> // 서보모터 라이브러리

#define DHTPIN 2  // DHT 센서 연결 핀
#define DHTTYPE DHT22  // DHT 센서 유형

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD 디스플레이 객체 생성
Servo servo;  // 서보모터 객체 생성

const int redLED = 7;    // 빨간색 LED 핀
const int greenLED = 6;  // 초록색 LED 핀
const int blueLED = 5;   // 파란색 LED 핀
const int buzzer = 12;   // 부저 핀

bool windowOpen = false;  // 창문 상태를 추적하는 변수

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  servo.attach(9);  // 서보모터 핀 설정
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT 센서 읽기 오류!");
    return;
  }

  // LCD 출력
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");

  // 창문 상태 변경 확인
  if (temperature > 30 || humidity < 40) {
    if (!windowOpen) { // 창문이 닫힌 상태에서 열릴 때만
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      tone(buzzer, 1000);  // 부저 울림
      delay(1000);  // 1초 동안 유지
      noTone(buzzer);
      servo.write(90);  // 창문 열기
      windowOpen = true; // 창문 상태를 열림으로 변경
    }
  } else {
    if (windowOpen) { // 창문이 열린 상태에서 닫힐 때만
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, HIGH);
      tone(buzzer, 1000);  // 부저 울림
      delay(1000);  // 1초 동안 유지
      noTone(buzzer);
      servo.write(0);  // 창문 닫기
      windowOpen = false; // 창문 상태를 닫힘으로 변경
    }
  }
}
