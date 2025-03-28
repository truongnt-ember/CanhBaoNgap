#include <WiFi.h>
#include <WebServer.h>

// Thông tin WiFi
const char* ssid = "Free";
const char* password = "1234567890";

// Sơ đồ chân
#define TRIG_PIN 12
#define ECHO_PIN 13
#define RELAY_PIN 14
#define BUZZER_PIN 4

WebServer webServer(80);

// Trạng thái máy bơm và còi
bool pumpStatus = false; 
bool buzzerStatus = false;
float currentDistance = 0.0; 
String currentFuzzyState = "";

// Định nghĩa các nốt nhạc
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// Bài nhạc (Twinkle Twinkle Little Star)
int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};

int noteDurations[] = {
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2
};

// Hàm fuzzy phân loại khoảng cách
String fuzzifyDistance(float distance) {
  if (distance > 0 && distance <= 7) {
    return "Nguy hiểm"; // 0-7 cm
  } else if (distance > 7 && distance <= 16) {
    return "Cảnh báo"; // 8-16 cm
  } else if (distance > 16) {
    return "An toàn"; // 17 cm trở lên
  } else {
    return "Ngoài phạm vi"; // Phòng trường hợp đo sai
  }
}

// Hàm phát nhạc
void playMelody() {
  for (int i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
    int noteDuration = 1000 / noteDurations[i];
    tone(BUZZER_PIN, melody[i], noteDuration);
    delay(noteDuration * 1.3);
  }
  noTone(BUZZER_PIN);
}

// Hàm điều khiển máy bơm và còi dựa vào trạng thái fuzzy
void controlSystem(String fuzzyState) {
  if (fuzzyState == "An toàn") {
    digitalWrite(RELAY_PIN, LOW);  // Tắt bơm
    noTone(BUZZER_PIN);           // Tắt còi
    pumpStatus = false;
    buzzerStatus = false;
  } else if (fuzzyState == "Cảnh báo") {
    digitalWrite(RELAY_PIN, LOW);  // Tắt bơm
    tone(BUZZER_PIN, NOTE_A4);     // Phát tần số cảnh báo
    pumpStatus = false;
    buzzerStatus = true;
  } else if (fuzzyState == "Nguy hiểm") {
    digitalWrite(RELAY_PIN, HIGH); // Bật bơm
    playMelody();                  // Phát nhạc báo động
    pumpStatus = true;
    buzzerStatus = true;
  }
}

// Route để thay đổi trạng thái máy bơm
void handleTogglePump() {
  pumpStatus = !pumpStatus;
  digitalWrite(RELAY_PIN, pumpStatus ? HIGH : LOW);
  webServer.send(200, "text/plain", pumpStatus ? "Bật" : "Tắt");
}

// Route để thay đổi trạng thái còi
void handleToggleBuzzer() {
  buzzerStatus = !buzzerStatus;
  if (!buzzerStatus) {
    noTone(BUZZER_PIN);
  }
  webServer.send(200, "text/plain", buzzerStatus ? "Bật" : "Tắt");
}

// Route trả về trạng thái hệ thống dưới dạng JSON
void handleGetState() {
  String json = "{";
  json += "\"pump\":" + String(pumpStatus) + ",";
  json += "\"buzzer\":" + String(buzzerStatus) + ",";
  json += "\"distance\":" + String(currentDistance, 2) + ",";
  json += "\"fuzzyState\":\"" + currentFuzzyState + "\"";
  json += "}";
  webServer.send(200, "application/json", json);
}

// Trang web chính
void handleRoot() {
  String html = "<html>\
                 <head>\
                   <meta charset='UTF-8'>\
                   <title>Hệ thống chống ngập</title>\
                   <style>\
                     body { font-family: Arial, sans-serif; text-align: center; background-color: #f2f2f2; }\
                     h1 { color: #333; margin-top: 20px; }\
                     .status { font-size: 1.2em; margin: 10px; padding: 10px; border-radius: 5px; color: white; display: inline-block; }\
                     .status.on { background-color: #4CAF50; }\
                     .status.off { background-color: #f44336; }\
                     .button { padding: 10px 20px; font-size: 1em; margin: 10px; border: none; border-radius: 5px; cursor: pointer; color: white; }\
                     .button.pump { background-color: #007BFF; }\
                     .button.buzzer { background-color: #FF5722; }\
                   </style>\
                   <script>\
                     function fetchState() {\
                       fetch('/state')\
                         .then(response => response.json())\
                         .then(data => {\
                           document.getElementById('pump-status').innerText = data.pump ? 'Bật' : 'Tắt';\
                           document.getElementById('buzzer-status').innerText = data.buzzer ? 'Bật' : 'Tắt';\
                           document.getElementById('distance').innerText = data.distance + ' cm';\
                           document.getElementById('fuzzy-state').innerText = data.fuzzyState;\
                         });\
                     }\
                     setInterval(fetchState, 1000);\
                   </script>\
                 </head>\
                 <body>\
                   <h1>Hệ thống chống ngập</h1>\
                   <p><strong>Lưu lượng nước:</strong> <span id='distance'>0</span></p>\
                   <p><strong>Trạng thái cảnh báo:</strong> <span id='fuzzy-state'>-</span></p>\
                   <div>\
                     <p><strong>Trạng thái Máy bơm:</strong> <span id='pump-status'>-</span></p>\
                     <button class='button pump' onclick='fetch(\"/togglePump\")'>Chuyển đổi Máy bơm</button>\
                   </div>\
                   <div>\
                     <p><strong>Trạng thái Còi:</strong> <span id='buzzer-status'>-</span></p>\
                     <button class='button buzzer' onclick='fetch(\"/toggleBuzzer\")'>Chuyển đổi Còi</button>\
                   </div>\
                 </body>\
                 </html>";
  webServer.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  // Cấu hình chân I/O
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  noTone(BUZZER_PIN);

  // Kết nối WiFi
  Serial.print("Kết nối tới WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // Định nghĩa route
  webServer.on("/", handleRoot);
  webServer.on("/togglePump", handleTogglePump);
  webServer.on("/toggleBuzzer", handleToggleBuzzer);
  webServer.on("/state", handleGetState);
  webServer.begin();
  Serial.println("Máy chủ web đã sẵn sàng");
}

void loop() {
  // Đo khoảng cách
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = (duration * 0.034) / 2;

  if (distance > 0) {
    currentDistance = distance;
    currentFuzzyState = fuzzifyDistance(distance);
    controlSystem(currentFuzzyState);

    // Hiển thị thông tin trên Serial Monitor
    Serial.print("Khoảng cách đo được: ");
    Serial.print(currentDistance);
    Serial.println(" cm");

    Serial.print("Trạng thái fuzzy: ");
    Serial.println(currentFuzzyState);

    Serial.print("Trạng thái Máy bơm: ");
    Serial.println(pumpStatus ? "BẬT" : "TẮT");

    Serial.print("Trạng thái Còi: ");
    Serial.println(buzzerStatus ? "BẬT" : "TẮT");

    Serial.println("-----------------------------");
    
  }

  // Xử lý máy chủ
  webServer.handleClient();
}
