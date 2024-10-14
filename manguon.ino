#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Khai báo các chân
#define RELAY_PIN D2
#define LED_RED D5
#define LED_GREEN D6
#define LED_YELLOW D7
#define ANALOG_PIN A0

// Khai báo các Virtual Pin trên Blynk
#define VPIN_HUMIDITY V1  // Hiển thị giá trị độ ẩm (%)
#define VPIN_HUMIDITY_GAUGE V8 // Hiển thị độ ẩm dạng gauge
#define VPIN_MODE_SELECT V2 // Segmented Switch để chọn chế độ (1: Tự động, 2: Thủ công, 3: Hẹn giờ)
#define VPIN_HUMIDITY_THRESHOLD_HIGH V3 // Ngưỡng độ ẩm cao
#define VPIN_HUMIDITY_THRESHOLD_LOW V4 // Ngưỡng độ ẩm thấp
#define VPIN_RELAY_CONTROL V9 // Button để điều khiển relay ở chế độ thủ công
#define VPIN_LCD_DISPLAY V6  // LCD để hiển thị thông báo độ ẩm
#define VPIN_TIME_INPUT V15 // Time input widget để thiết lập hẹn giờ
#define VPIN_TIME_DISPLAY V13 // Hiển thị giờ:phút:giây
#define VPIN_DATE_DISPLAY V14 // Hiển thị ngày/tháng/năm

#define VPIN_LED_RED V10
#define VPIN_LED_GREEN V11
#define VPIN_LED_YELLOW V12

// Thông tin WiFi và Blynk
char auth[] = "YOUR_BLYNK_AUTH_TOKEN"; // Thay bằng Auth Token của bạn
char ssid[] = "YOUR_WIFI_SSID"; // Thay bằng SSID WiFi của bạn
char pass[] = "YOUR_WIFI_PASSWORD"; // Thay bằng mật khẩu WiFi của bạn
char blynkServer[] = "blynk-server.com"; // Thay bằng địa chỉ server Blynk của bạn (nếu tự dựng)
uint16_t blynkPort = 8080;    // Thay bằng cổng của server Blynk của bạn (nếu tự dựng)

// Biến toàn cục
int humidityPercentage;
int humidityThresholdHigh, humidityThresholdLow;
int relayControlButtonState;
int operatingMode = 1; // 1: Tự động, 2: Thủ công, 3: Hẹn giờ
bool relayState = false;
bool humidityHighAlertSent = false;
bool humidityLowAlertSent = false;
int startHour, startMinute, startSecond, stopHour, stopMinute, stopSecond;
bool selectedDays[8]; // Mảng lưu các ngày đã chọn, 1–7: Thứ hai đến Chủ nhật

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);  // UTC+7

// WidgetRTC
WidgetRTC rtc;

// Blynk Timer
BlynkTimer timer;

WidgetLED appledr(VPIN_LED_RED);
WidgetLED appledg(VPIN_LED_GREEN);
WidgetLED appledy(VPIN_LED_YELLOW);

void setup() 
{
  Serial.begin(9600);

  // Kết nối WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Blynk.begin(auth, ssid, pass, blynkServer, blynkPort);
  while (!Blynk.connected()) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Blynk.email("YOUR_EMAIL@gmail.com", "Email Test", "Kiểm tra gửi email");
  // Thiết lập các chân
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Tắt relay mặc định

  // Đồng bộ giá trị từ Blynk app
  Blynk.syncVirtual(VPIN_HUMIDITY_THRESHOLD_HIGH);
  Blynk.syncVirtual(VPIN_HUMIDITY_THRESHOLD_LOW);
  Blynk.syncVirtual(VPIN_RELAY_CONTROL);  
  Blynk.syncVirtual(VPIN_TIME_INPUT);

  // Khởi tạo NTP Client
  timeClient.begin();

  // Đặt thời gian cập nhật thời gian thực
  setSyncInterval(60);

  // Khởi tạo Timer
  timer.setInterval(1000L, updateDisplay); // Cập nhật thời gian và điều khiển relay theo hẹn giờ
}

void loop() 
{
  Blynk.run();
  timer.run();

  // Đọc giá trị độ ẩm
  int analogValue = analogRead(ANALOG_PIN);
  humidityPercentage = map(analogValue, 0, 1023, 100, 0);

  // Cập nhật giá trị độ ẩm lên Blynk app
  Blynk.virtualWrite(VPIN_HUMIDITY, humidityPercentage);
  Blynk.virtualWrite(VPIN_HUMIDITY_GAUGE, humidityPercentage);

  // Kiểm tra độ ẩm và gửi cảnh báo
  checkHumidityAndSendAlerts();

  // Điều khiển relay dựa trên chế độ hoạt động
  controlRelayByMode();
}

// Hàm cập nhật thời gian và điều khiển relay theo hẹn giờ
void updateDisplay() 
{
  updateTimeDisplay();
  controlRelayBySchedule();
}

// Hàm kiểm tra độ ẩm và gửi cảnh báo
void checkHumidityAndSendAlerts() 
{
  if (humidityPercentage > humidityThresholdHigh && !humidityHighAlertSent) 
  {
    appledr.off();
    appledg.off();
    appledy.on();
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);

    Serial.println("High humidity detected");
    Blynk.notify("Độ ẩm cao!");
    Blynk.virtualWrite(VPIN_LCD_DISPLAY, "Độ ẩm cao!");
    Blynk.email("YOUR_EMAIL@gmail.com", "Cảnh báo độ ẩm", "Độ ẩm cao!");
    humidityHighAlertSent = true;
    humidityLowAlertSent = false;
  } 
  else if (humidityPercentage < humidityThresholdLow && !humidityLowAlertSent) 
  {
    appledr.on();
    appledg.off();
    appledy.off();
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);

    Serial.println("Low humidity detected");
    Blynk.notify("Độ ẩm thấp!");
    Blynk.virtualWrite(VPIN_LCD_DISPLAY, "Độ ẩm thấp!");
    Blynk.email("YOUR_EMAIL@gmail.com", "Cảnh báo độ ẩm", "Độ ẩm thấp!");
    humidityLowAlertSent = true;
    humidityHighAlertSent = false;
  } 
  else if (humidityPercentage >= humidityThresholdLow && humidityPercentage <= humidityThresholdHigh)
  {
    appledr.off();
    appledg.on();
    appledy.off();
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    Serial.println("Normal humidity");
    Blynk.virtualWrite(VPIN_LCD_DISPLAY, "Độ ẩm bình thường");

    humidityHighAlertSent = false;
    humidityLowAlertSent = false;
  }
}

// Hàm điều khiển relay dựa trên chế độ hoạt động
void controlRelayByMode() 
{
  switch (operatingMode) 
  {
    case 1: // Chế độ tự động
      controlRelayAutomatically();
      Serial.println("Đang ở chế độ tự động"); // In ra Serial Monitor để dễ debug
      break;
    case 2: // Chế độ thủ công
      controlRelayManually();
      Serial.println("Đang ở chế độ thủ công"); // In ra Serial Monitor để dễ debug
      break;
    case 3: // Chế độ hẹn giờ
      Serial.println("Đang ở chế độ hẹn giờ");// In ra Serial Monitor để dễ debug
      // Relay sẽ được điều khiển trong hàm controlRelayBySchedule() được gọi bởi timer
      break;
  }
}

// Hàm điều khiển relay ở chế độ tự động
void controlRelayAutomatically() 
{
  if (humidityPercentage > humidityThresholdHigh) 
  {
    digitalWrite(RELAY_PIN, LOW); // Tắt relay
    Serial.println("Relay OFF (Automatic - High humidity)");
  } 
  else if (humidityPercentage < humidityThresholdLow) 
  {
    digitalWrite(RELAY_PIN, HIGH); // Bật relay
    Serial.println("Relay ON (Automatic - Low humidity)");
  } 
  else if (humidityPercentage >= humidityThresholdLow && humidityPercentage <= humidityThresholdHigh)
  {
    digitalWrite(RELAY_PIN, LOW); // Tắt relay
    Serial.println("Relay OFF (Automatic - Normal humidity)");
  }
}

// Hàm điều khiển relay ở chế độ thủ công
void controlRelayManually() 
{
  if (relayControlButtonState == 1) 
  {
    digitalWrite(RELAY_PIN, HIGH); // Bật relay
    Serial.println("Relay ON (Manual)");
  } 
  else 
  {
    digitalWrite(RELAY_PIN, LOW); // Tắt relay
    Serial.println("Relay OFF (Manual)");
  }
}

// Hàm điều khiển relay theo hẹn giờ
void controlRelayBySchedule() 
{
  if (operatingMode != 3) return; // Chỉ thực hiện khi ở chế độ hẹn giờ

  time_t now = timeClient.getEpochTime();
  struct tm *timeInfo = localtime(&now);
  int currentDayOfWeek = timeInfo->tm_wday; // Lấy thứ hiện tại (0: Chủ nhật, ..., 6: Thứ bảy)
  int blynkDayOfWeek = (currentDayOfWeek == 0) ? 7 : currentDayOfWeek; // Chuyển đổi sang định dạng Blynk (1: Thứ hai, ..., 7: Chủ nhật)

  if (!selectedDays[blynkDayOfWeek]) 
  {
    Serial.println("Ngày hiện tại không nằm trong khoảng thời gian hẹn giờ");
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;  // reset relay state khi không tưới 
    return;
  }

  int currentHour = timeInfo->tm_hour;
  int currentMinute = timeInfo->tm_min;
  int currentSecond = timeInfo->tm_sec;

  time_t startTime = startHour * 3600 + startMinute * 60 + startSecond;
  time_t endTime = stopHour * 3600 + stopMinute * 60 + stopSecond;
  time_t currentTime = currentHour * 3600 + currentMinute * 60 + currentSecond;

  if (currentTime >= startTime && currentTime <= endTime) 
  {
    if (!relayState) 
    {
      digitalWrite(RELAY_PIN, HIGH); // Bật relay
      Serial.println("Bật Relay (Hẹn giờ)");
      relayState = true;
    }
  } 
  else 
  {
    if (relayState) 
    {
      digitalWrite(RELAY_PIN, LOW); // Tắt relay
      Serial.println("Tắt Relay (Hẹn giờ)");
      relayState = false;
    }
  }
}

// Hàm cập nhật thời gian lên Blynk app
void updateTimeDisplay() 
{
  timeClient.update();

  String currentTime = timeClient.getFormattedTime();
  Blynk.virtualWrite(VPIN_TIME_DISPLAY, currentTime);

  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo = localtime(&rawTime);

  const char* daysOfWeek[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
  String dayOfWeek = daysOfWeek[timeInfo->tm_wday];
  String date = String(timeInfo->tm_mday) + "/" + String(timeInfo->tm_mon + 1) + "/" + String(timeInfo->tm_year + 1900);

  Blynk.virtualWrite(VPIN_DATE_DISPLAY, dayOfWeek + ", " + date);
}

// Hàm xử lý khi giá trị Segmented Switch thay đổi
BLYNK_WRITE(VPIN_MODE_SELECT) 
{
  operatingMode = param.asInt();
  Serial.print("Chế độ hoạt động: ");
  Serial.println(operatingMode);

 // Reset các cờ cảnh báo khi chuyển chế độ (giữ nguyên)
  humidityHighAlertSent = false;
  humidityLowAlertSent = false;

// Nếu chuyển sang chế độ hẹn giờ và chưa thiết lập thời gian, bật Time Input
  if (operatingMode == 3 && !timerSettingsValid()) 
  { 
    Blynk.setProperty(VPIN_TIME_INPUT, "popup", true); // Hiển thị popup của Time Input để nhắc nhở người dùng thiết lập thời gian
  }
}

// Hàm kiểm tra xem đã thiết lập thời gian cho hẹn giờ hay chưa
bool timerSettingsValid() 
{
  return (startHour != -1 && startMinute != -1 && stopHour != -1 && stopMinute != -1); // Kiểm tra nếu tất cả các giá trị giờ, phút đã được đặt
}

BLYNK_WRITE(VPIN_HUMIDITY_THRESHOLD_HIGH) 
{
  humidityThresholdHigh = param.asInt();
  Serial.print("Ngưỡng độ ẩm cao: ");
  Serial.println(humidityThresholdHigh);
  humidityHighAlertSent = false; // Reset cờ để gửi lại email khi vượt ngưỡng mới
}

BLYNK_WRITE(VPIN_HUMIDITY_THRESHOLD_LOW) 
{
  humidityThresholdLow = param.asInt();
  Serial.print("Ngưỡng độ ẩm thấp: ");
  Serial.println(humidityThresholdLow);
  humidityLowAlertSent = false; // Reset cờ để gửi lại email khi vượt ngưỡng mới
}

BLYNK_WRITE(VPIN_RELAY_CONTROL) 
{
  relayControlButtonState = param.asInt();
  
  Serial.print("Trạng thái nút relay: ");
  Serial.println(relayControlButtonState);
}

// Hàm xử lý khi giá trị Time Input thay đổi
BLYNK_WRITE(VPIN_TIME_INPUT) 
{
  TimeInputParam t = param;

  if (t.hasStartTime()) 
  {
    startHour = t.getStartHour();
    startMinute = t.getStartMinute();
    startSecond = t.getStartSecond();
  }
  if (t.hasStopTime()) 
  {
    stopHour = t.getStopHour();
    stopMinute = t.getStopMinute();
    stopSecond = t.getStopSecond();
  }

  // Nếu Time Input đóng lại mà chưa có thiết lập giờ
  if (!t.hasStartTime() || !t.hasStopTime())
  { 
    operatingMode = 1;// Chuyển về chế độ tự động
    Blynk.virtualWrite(VPIN_MODE_SELECT, operatingMode);
    Blynk.notify("Vui lòng thiết lập thời gian hẹn giờ."); // Thông báo cho người dùng
  }
  // Reset mảng ngày đã chọn
  for (int i = 1; i <= 7; i++) 
  {
    selectedDays[i] = false;
  }

  Serial.print("Các ngày đã chọn: ");
  const char* dayNames[] = {"", "Thứ hai", "Thứ ba", "Thứ tư", "Thứ năm", "Thứ sáu", "Thứ bảy", "Chủ nhật"};
  for (int i = 1; i <= 7; i++) 
  {
    if (t.isWeekdaySelected(i)) 
    {
      selectedDays[i] = true;
      Serial.print(dayNames[i]);
      Serial.print(" ");
    }
  }
  Serial.println();
}