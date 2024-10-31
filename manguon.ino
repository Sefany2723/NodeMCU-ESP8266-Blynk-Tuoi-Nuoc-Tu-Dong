// Khai báo thư viện cần thiết cho Blynk, ESP8266 và các chức năng thời gian thực
#define BLYNK_PRINT Serial // Sử dụng Serial để in các thông tin debug từ Blynk
// Khai báo thông tin Blynk server để kết nối
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"
#include <ESP8266WiFi.h> // Thư viện ESP8266 WiFi để kết nối internet
#include <BlynkSimpleEsp8266.h> // Thư viện Blynk cho ESP8266
#include <TimeLib.h> // Thư viện xử lý thời gian
#include <WidgetRTC.h> // Thư viện Blynk cho WidgetRTC, dùng để đồng bộ thời gian
#include <WiFiUdp.h> // Thư viện UDP để dùng với NTP (Network Time Protocol)
#include <NTPClient.h> // Thư viện NTP để lấy thời gian thực từ server
// Định nghĩa các chân (GPIO) cho các thành phần kết nối đến NodeMCU ESP8266
#define RELAY_PIN D2 // Chân điều khiển relay
#define LED_RED D5 // Chân điều khiển LED đỏ
#define LED_GREEN D6 // Chân điều khiển LED xanh
#define LED_YELLOW D7 // Chân điều khiển LED vàng
#define ANALOG_PIN A0 // Chân đọc giá trị từ cảm biến độ ẩm đất (ADC)
// Định nghĩa các Virtual Pin để giao tiếp giữa ứng dụng Blynk và phần cứng
#define VPIN_HUMIDITY V1  // Hiển thị giá trị độ ẩm (%) trên Blynk app
#define VPIN_HUMIDITY_CHART V8 // Hiển thị biểu đồ độ ẩm theo thời gian
#define VPIN_MODE_SELECT V2 // Segmented Switch để chọn chế độ (1: Tự động, 2: Thủ công, 3: Hẹn giờ)
#define VPIN_HUMIDITY_THRESHOLD_HIGH V3 // Ngưỡng độ ẩm cao để cảnh báo
#define VPIN_HUMIDITY_THRESHOLD_LOW V4 // Ngưỡng độ ẩm thấp để cảnh báo
#define VPIN_RELAY_CONTROL V9 // Button để điều khiển relay ở chế độ thủ công
#define VPIN_LCD_DISPLAY V6  // Hiển thị thông báo trên LCD Widget
#define VPIN_TIME_INPUT V15 // Widget hẹn giờ cho chức năng điều khiển tưới theo lịch
#define VPIN_TIME_DISPLAY V13 // Hiển thị thời gian hiện tại (giờ:phút:giây) trên Blynk
#define VPIN_DATE_DISPLAY V14 // Hiển thị thứ ngày/tháng/năm trên Blynk
#define VPIN_LED_RED V10 // Điều khiển LED đỏ trên Blynk app
#define VPIN_LED_GREEN V11 // Điều khiển LED xanh trên Blynk app
#define VPIN_LED_YELLOW V12 // Điều khiển LED vàng trên Blynk app
// Khai báo thông tin WiFi để kết nối
char ssid[] = "Le Lai"; // Thay bằng SSID WiFi của bạn
char pass[] = "00002222"; // Thay bằng mật khẩu WiFi của bạn
// Các biến toàn cục để lưu trữ trạng thái và dữ liệu của hệ thống
int humidityPercentage; // Biến lưu giá trị độ ẩm hiện tại (%)
int humidityThresholdHigh, humidityThresholdLow; // Ngưỡng độ ẩm cao và thấp
int relayControlButtonState; // Trạng thái nút điều khiển relay ở chế độ thủ công
int operatingMode = 0; // Biến lưu chế độ hoạt động hiện tại (0: Tự động, 1: Thủ công, 2: Hẹn giờ)
bool relayState = false; // Trạng thái hiện tại của relay (bật/tắt)
bool humidityHighAlertSent = false; // Biến đánh dấu đã gửi cảnh báo độ ẩm cao
bool humidityLowAlertSent = false; // Biến đánh dấu đã gửi cảnh báo độ ẩm thấp
int startHour, startMinute, startSecond, stopHour, stopMinute, stopSecond; // Lưu trữ thời gian hẹn giờ
bool selectedDays[8]; // Mảng lưu các ngày đã chọn để tưới (1–7: Thứ hai đến Chủ nhật)
// Khai báo NTP Client để lấy thời gian từ server NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);  // UTC+7
// Khai báo WidgetRTC để sử dụng thời gian thực trong Blynk
WidgetRTC rtc;
// Khai báo Timer của Blynk để định kỳ thực hiện các tác vụ
BlynkTimer timer;
// Khai báo các widget LED trong ứng dụng Blynk
WidgetLED appledr(VPIN_LED_RED);
WidgetLED appledg(VPIN_LED_GREEN);
WidgetLED appledy(VPIN_LED_YELLOW);
// Hàm setup() được thực thi một lần khi khởi động hệ thống
void setup() 
{
  Serial.begin(9600); // Khởi tạo Serial với tốc độ 9600 để giao tiếp với máy tính
  // Kết nối WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid); // Hiển thị tên WiFi đang kết nối
  WiFi.begin(ssid, pass); // Kết nối đến WiFi với SSID và mật khẩu
  int wifiRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) 
  { // Chờ kết nối tối đa 20 lần (10 giây)
    delay(500);
    Serial.print(".");
    wifiRetries++;
  }
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("");
    Serial.println("WiFi connected"); // In ra khi kết nối WiFi thành công
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); // Hiển thị IP sau khi kết nối thành công
  } 
  else 
  {
    Serial.println("Failed to connect to WiFi!"); // Báo lỗi nếu kết nối WiFi thất bại
  }
  // Kết nối Blynk
  Serial.print("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // Kết nối Blynk qua WiFi với thông tin server và token
  int blynkRetries = 0;
  while (!Blynk.connected() && blynkRetries < 20) 
  { 
    // Chờ kết nối tối đa 20 lần (10 giây)
    delay(500);
    Serial.print(".");
    Blynk.run(); // Thử kết nối lại Blynk
    blynkRetries++;
  }
  if (Blynk.connected()) 
  {
    Serial.println("Blynk connected!"); // Báo thành công
  } 
  else 
  {
    Serial.println("Failed to connect to Blynk!"); // Báo lỗi nếu không thể kết nối Blynk
  }
  // Cấu hình các chân của NodeMCU
  pinMode(LED_RED, OUTPUT); // Thiết lập chân D5 làm output cho đèn LED đỏ
  pinMode(LED_GREEN, OUTPUT); // Thiết lập chân D6 làm output cho đèn LED xanh
  pinMode(LED_YELLOW, OUTPUT); // Thiết lập chân D7 làm output cho đèn LED vàng
  pinMode(RELAY_PIN, OUTPUT); // Thiết lập chân D2 làm output cho relay điều khiển bơm nước
  digitalWrite(RELAY_PIN, LOW); // Tắt relay mặc định ban đầu (relay trạng thái thấp)
  // Đồng bộ giá trị từ ứng dụng Blynk (nếu có lưu trữ trước đó)
  Blynk.syncVirtual(VPIN_HUMIDITY_THRESHOLD_HIGH); // Đồng bộ ngưỡng độ ẩm cao
  Blynk.syncVirtual(VPIN_HUMIDITY_THRESHOLD_LOW); // Đồng bộ ngưỡng độ ẩm thấp
  Blynk.syncVirtual(VPIN_RELAY_CONTROL); // Đồng bộ trạng thái nút điều khiển relay
  Blynk.syncVirtual(VPIN_TIME_INPUT); // Đồng bộ thời gian hẹn giờ
  Blynk.syncVirtual(VPIN_MODE_SELECT); //Đồng bộ trạng thái nút chọn chế độ
  // Khởi tạo NTP Client để đồng bộ thời gian thực từ server NTP
  timeClient.begin();
  // Cài đặt thời gian cập nhật đồng bộ với NTP mỗi 60 giây
  setSyncInterval(60);
  // Khởi tạo Timer để cập nhật thời gian và điều khiển relay theo chu kỳ 1 giây
  timer.setInterval(60000L, updateDisplay); // Mỗi 1000ms (1 giây) sẽ gọi hàm updateDisplay
  timer.setInterval(5000L, sendHumidityData); // Mỗi 5 giây gửi độ ẩm
}
void loop() 
{
  // Kiểm tra kết nối WiFi và tự động kết nối lại nếu mất kết nối
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("WiFi lost! Attempting reconnection...");
    WiFi.begin(ssid, pass);
    int wifiReconnectRetries = 0;
    while (WiFi.status() != WL_CONNECTED && wifiReconnectRetries < 20) 
    { 
      // Thử lại 20 lần
      delay(500);
      Serial.print(".");
      wifiReconnectRetries++;
    }
    if (WiFi.status() == WL_CONNECTED) 
    {
      Serial.println("Reconnected to WiFi.");
    } 
    else 
    {
      Serial.println("Failed to reconnect to WiFi.");
    }
  }
  // Kiểm tra kết nối Blynk và tự động kết nối lại nếu mất kết nối
  if (!Blynk.connected()) 
  {
    Serial.println("Blynk lost! Attempting reconnection...");
    Blynk.connect(); // Thử kết nối lại với Blynk
    int blynkReconnectRetries = 0;
    while (!Blynk.connected() && blynkReconnectRetries < 20) 
    { 
      // Thử lại 20 lần
      delay(500);
      Serial.print(".");
      Blynk.run(); // Cần gọi Blynk.run() để duy trì kết nối
      blynkReconnectRetries++;
    }
    if (Blynk.connected()) 
    {
      Serial.println("Reconnected to Blynk.");
    } 
    else 
    {
      Serial.println("Failed to reconnect to Blynk.");
    }
  }
  Blynk.run(); // Chạy tiến trình Blynk, xử lý các sự kiện từ ứng dụng Blynk
  timer.run(); // Chạy timer, xử lý các hàm được thiết lập với timer

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
    Blynk.logEvent("high_humidity", "High humidity!!");
    Blynk.virtualWrite(VPIN_LCD_DISPLAY, "High humidity!");
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
    Blynk.logEvent("low_humidity", "Low humidity!!");
    Blynk.virtualWrite(VPIN_LCD_DISPLAY, "Low humidity!");
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
    Blynk.virtualWrite(VPIN_LCD_DISPLAY, "Normal humidity");
    humidityHighAlertSent = false;
    humidityLowAlertSent = false;
  }
}
// Hàm điều khiển relay dựa trên chế độ hoạt động
void controlRelayByMode() 
{
  switch (operatingMode) 
  {
    case 0: // Chế độ tự động
      controlRelayAutomatically();
      Serial.println("Đang ở chế độ tự động"); // In ra Serial Monitor để dễ debug
      break;
    case 1: // Chế độ thủ công
      controlRelayManually();
      Serial.println("Đang ở chế độ thủ công"); // In ra Serial Monitor để dễ debug
      break;
    case 2: // Chế độ hẹn giờ
      Serial.println("Đang ở chế độ hẹn giờ");// In ra Serial Monitor để dễ debug
      // Relay sẽ được điều khiển trong hàm controlRelayBySchedule() được gọi bởi timer
      break;
  }
}
// Hàm điều khiển relay ở chế độ tự động
void controlRelayAutomatically() 
{
  bool newRelayState = relayState; // Biến lưu trạng thái mới của relay
  if (humidityPercentage > humidityThresholdHigh) 
  {
    newRelayState = false; // Tắt relay nếu độ ẩm cao
    Serial.println("Relay OFF (Automatic - High humidity)");
    Blynk.virtualWrite(V0, " ");
  } 
  else if (humidityPercentage < humidityThresholdLow) 
  {
    newRelayState = true;  // Bật relay nếu độ ẩm thấp
    Serial.println("Relay ON (Automatic - Low humidity)");
    Blynk.virtualWrite(V0, "Watering");
  } 
  else 
  {
    newRelayState = false; // Tắt relay nếu độ ẩm trong khoảng cho phép
    Serial.println("Relay OFF (Automatic - Normal humidity)");
    Blynk.virtualWrite(V0, " ");
  }
  // Chỉ thực hiện thay đổi relay nếu trạng thái mới khác với trạng thái hiện tại
  if (newRelayState != relayState) 
  {
    relayState = newRelayState;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW); // Bật hoặc tắt relay
  }
}
// Hàm điều khiển relay ở chế độ thủ công
void controlRelayManually() 
{
  bool newRelayState = relayControlButtonState == 1; // Xác định trạng thái relay từ nút điều khiển
  // Chỉ thay đổi trạng thái relay nếu khác với trạng thái hiện tại
  if (newRelayState != relayState) 
  {
    relayState = newRelayState;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.println(relayState ? "Relay ON (Manual)" : "Relay OFF (Manual)");
    Blynk.virtualWrite(V0, relayState ? "Watering" : " ");
  }
}
// Hàm điều khiển relay theo hẹn giờ
void controlRelayBySchedule() 
{
  if (operatingMode != 2) return; // Chỉ thực hiện khi ở chế độ hẹn giờ
  // Lấy thời gian hiện tại từ NTP
  time_t now = timeClient.getEpochTime();
  struct tm *timeInfo = localtime(&now);
  int currentDayOfWeek = timeInfo->tm_wday; // Lấy thứ hiện tại (0: Chủ nhật, ..., 6: Thứ bảy)
  int blynkDayOfWeek = (currentDayOfWeek == 0) ? 7 : currentDayOfWeek; // Chuyển đổi sang định dạng thứ Blynk (1: Thứ hai, ..., 7: Chủ nhật)
  // Nếu ngày hiện tại không nằm trong ngày tưới, tắt relay và thoát
  if (!selectedDays[blynkDayOfWeek]) 
  {
    Serial.println("Ngày hiện tại không nằm trong khoảng thời gian hẹn giờ");
    if (relayState)
    {
      // Kiểm tra và tắt relay nếu cần
      digitalWrite(RELAY_PIN, LOW);
      relayState = false;  // reset relay state khi không tưới 
      Serial.println("Tắt Relay do không nằm trong ngày hẹn giờ");
      Blynk.virtualWrite(V0, " ");
    }
    return;
  }
  // Lấy giờ, phút và giây hiện tại
  int currentHour = timeInfo->tm_hour;
  int currentMinute = timeInfo->tm_min;
  int currentSecond = timeInfo->tm_sec;
  // Tính toán thời gian theo giây từ đầu ngày
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
      Blynk.virtualWrite(V0, "Watering");
    }
  } 
  else 
  {
    if (relayState) 
    {
      digitalWrite(RELAY_PIN, LOW); // Tắt relay
      Serial.println("Tắt Relay (Hẹn giờ)");
      relayState = false;
      Blynk.virtualWrite(V0, " ");
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
  if (operatingMode == 2 && !timerSettingsValid()) 
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
    operatingMode = 0;// Chuyển về chế độ tự động
    Blynk.virtualWrite(VPIN_MODE_SELECT, operatingMode);
    Blynk.logEvent("schedule", "Vui lòng thiết lập thời gian hẹn giờ."); // Thông báo cho người dùng
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
void sendHumidityData()
{
  // Đọc giá trị từ cảm biến độ ẩm đất
  int analogValue = analogRead(ANALOG_PIN); // Đọc giá trị từ cảm biến
  // Tính toán độ ẩm từ giá trị của cảm biến
  humidityPercentage = map(analogValue, 0, 1023, 100, 0);
  // Cập nhật giá trị độ ẩm và đồ thị lên Blynk app
  Blynk.virtualWrite(VPIN_HUMIDITY, humidityPercentage);
  Blynk.virtualWrite(VPIN_HUMIDITY_CHART, humidityPercentage);
}
