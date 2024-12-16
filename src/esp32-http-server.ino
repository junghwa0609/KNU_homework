#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

WebServer server(80);

const int LED1 = 26;           // LED1 핀
const int LIGHT_SENSOR_PIN = 34; // 조도 센서 핀
const int THRESHOLD = 1000;    // 조도가 불편한 수준을 나타내는 임계값

bool led1State = false;        // LED1 상태 (ON/OFF)
bool userControl = false;      // 사용자가 웹서버에서 제어했는지 여부
int lastLightValue = 0;        // 마지막으로 읽은 조도 센서 값
unsigned long lastUserControlTime = 0; // 사용자가 제어한 마지막 시간
const unsigned long USER_CONTROL_TIMEOUT = 5000; // 사용자 제어 유효 시간 (5초)

// HTML 페이지 전송
void sendHtml() {
  String response = R"rawliteral(
    <!DOCTYPE html><html>
      <head>
        <title>Classroom Environment Control Center</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; }
          body { margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; }
          h1 { margin-bottom: 1em; color: #333; }
          h2 { margin: 0; color: #555; font-size: 1.2em; }
          .control-section { display: flex; flex-direction: column; align-items: center; margin-bottom: 2em; }
          .btn { background-color: #5B5; border: none; color: white; padding: 0.8em 1.5em; font-size: 1.2em; border-radius: 5px; cursor: pointer; }
          .btn.OFF { background-color: #333; }
          .light-value { margin-top: 2em; background-color: #fff; padding: 1em; border-radius: 8px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); width: 80%; max-width: 400px; }
          .light-value h2 { font-size: 1.4em; color: #444; margin-bottom: 0.5em; }
          .light-value p { font-size: 1.6em; color: #222; font-weight: bold; }
        </style>
        <script>
          async function toggleLed() {
            const response = await fetch(`/toggle/1`, { method: 'POST' });
            const status = await response.text();
            const button = document.getElementById(`led1`);
            button.textContent = status;
            button.className = status === 'ON' ? 'btn ON' : 'btn OFF';
          }

          async function updateLightValue() {
            const response = await fetch('/light-value');
            const lightValue = await response.text();
            document.getElementById('lightValue').textContent = lightValue;
          }

          async function updateLedState() {
            const response = await fetch('/led-state');
            const ledState = await response.text();
            const button = document.getElementById(`led1`);
            button.textContent = ledState;
            button.className = ledState === 'ON' ? 'btn ON' : 'btn OFF';
          }

          setInterval(() => {
            updateLightValue();
            updateLedState();
          }, 500); // 0.5초마다 상태 갱신
        </script>
      </head>
      <body>
        <h1>Classroom Environment Control Center</h1>

        <div class="control-section">
          <h2>Light Control</h2>
          <button id="led1" class="btn LED1_TEXT" onclick="toggleLed()">LED1_TEXT</button>
        </div>

        <div class="light-value">
          <h2>Light Sensor Reading</h2>
          <p>The current brightness level is: <span id="lightValue">LIGHT_VALUE</span></p>
        </div>
      </body>
    </html>
  )rawliteral";

  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LIGHT_VALUE", String(lastLightValue)); // 최신 조도 값 반영
  server.send(200, "text/html", response);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // 초기 조도 값 및 LED 상태 설정
  lastLightValue = analogRead(LIGHT_SENSOR_PIN);
  if (lastLightValue < THRESHOLD) {
    led1State = true;
    digitalWrite(LED1, HIGH);
  } else {
    led1State = false;
    digitalWrite(LED1, LOW);
  }

  // HTML 페이지 요청 핸들러
  server.on("/", sendHtml);

  // LED 토글 핸들러
  server.on("/toggle/1", HTTP_POST, []() {
    led1State = !led1State; // 상태 변경
    digitalWrite(LED1, led1State);
    userControl = true; // 사용자 제어 활성화
    lastUserControlTime = millis();
    server.send(200, "text/plain", led1State ? "ON" : "OFF");
  });

  // 조도 센서 값 요청 핸들러
  server.on("/light-value", []() {
    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    lightValue = constrain(lightValue, 0, 4095);
    lastLightValue = lightValue; // 최신 조도 값 저장
    server.send(200, "text/plain", String(lightValue));
  });

  // LED 상태 요청 핸들러
  server.on("/led-state", []() {
    server.send(200, "text/plain", led1State ? "ON" : "OFF");
  });

  server.begin();
  Serial.println("HTTP server started (http://localhost:8180)");
}

void loop() {
  server.handleClient(); // 클라이언트 요청 처리

  // 현재 조도 값 읽기
  int lightValue = analogRead(LIGHT_SENSOR_PIN);
  lightValue = constrain(lightValue, 0, 4095);

  // 사용자 제어 타임아웃 확인
  if (userControl && millis() - lastUserControlTime > USER_CONTROL_TIMEOUT) {
    userControl = false; // 자동 제어 재활성화
  }

  // 사용자 제어가 비활성화된 경우 자동 제어 실행
  if (!userControl && abs(lightValue - lastLightValue) > 10) {
    lastLightValue = lightValue;

    if (lightValue < THRESHOLD && !led1State) {
      led1State = true;
      digitalWrite(LED1, HIGH);
    } else if (lightValue >= THRESHOLD && led1State) {
      led1State = false;
      digitalWrite(LED1, LOW);
    }
  }
}