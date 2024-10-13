# Hệ Thống Tưới Tự Động Sử Dụng ESP8266 và Blynk

## 1. Giới thiệu

Hệ thống tưới tự động này sử dụng vi điều khiển ESP8266 kết hợp với app Blynk (phiên bản legacy) và các cảm biến, relay để kiểm soát quá trình tưới nước cho cây trồng. Dự án này cho phép bạn điều khiển quá trình tưới tự động dựa trên độ ẩm của đất hoặc theo thời gian đã thiết lập. Bạn cũng có thể theo dõi và điều khiển hệ thống từ xa qua smartphone.

### Chức năng chính:
- Điều khiển hệ thống tưới tự động dựa trên độ ẩm đất.
- Điều khiển thủ công qua ứng dụng Blynk.
- Hẹn giờ tưới nước dựa trên ngày và thời gian thiết lập.
- Gửi cảnh báo qua email và thông báo đẩy trên điện thoại khi độ ẩm đất vượt quá mức an toàn.
- Hiển thị thời gian thực và trạng thái hệ thống (gồm đồ thị theo thời gian, giá trị dạng số và trạng thái: cao, thấp, bình thường của độ ẩm, led cảnh báo,...) trên ứng dụng Blynk.

---

## 2. Yêu cầu hệ thống

### Phần cứng:
- **ESP8266** (NodeMCU)
- **Relay 5V**: Điều khiển máy bơm nước.
- **Cảm biến độ ẩm đất**: Để đo độ ẩm.
- **Bơm nước 12V**: Để tưới nước.
- **LED RGB**: Để hiển thị trạng thái cảnh báo của hệ thống.
- **Dây kết nối**, **nguồn 5V** cho relay và ESP8266.
- **Nguồn 12V** cho máy bơm

### Phần mềm:
- **Arduino IDE**: Dùng để lập trình ESP8266.
- **Blynk App (Legacy)**: Điều khiển và giám sát hệ thống qua smartphone.
- **Thư viện cần thiết:** ESP8266, Blynk, Time, NTPClient.

---

## 3. Sơ đồ kết nối phần cứng

| ESP8266 Pin | Kết nối với     | Mô tả                            |
|-------------|-----------------|-----------------------------------|
| D2          | Relay        | Điều khiển relay                  |
| D5          | LED đỏ          | Cảnh báo độ ẩm thấp               |
| D6          | LED xanh        | Báo độ ẩm bình thường             |
| D7          | LED vàng        | Báo độ ẩm cao                     |
| A0          | Cảm biến độ ẩm đất | Đọc giá trị độ ẩm đất            |

Sơ đồ chi tiết cho các chân kết nối và linh kiện sẽ được cung cấp trong phần hình ảnh minh họa.

---

## 4. Cài đặt phần mềm

### 4.1. Cài đặt Arduino IDE và board ESP8266

#### Bước 1: Tải và cài đặt Arduino IDE
- Truy cập [trang tải xuống Arduino IDE](https://www.arduino.cc/en/software) và tải về phiên bản mới nhất.
- Cài đặt IDE trên máy tính của bạn.

#### Bước 2: Cài đặt board ESP8266 vào Arduino IDE
- Mở Arduino IDE.
- Vào **File > Preferences**.
- Trong phần **Additional Boards Manager URLs**, thêm đường dẫn sau:

  ```
  http://arduino.esp8266.com/stable/package_esp8266com_index.json
  ```

- Nhấn **OK**.
- Vào **Tools > Board > Boards Manager** và tìm kiếm "ESP8266".
- Chọn **Install** để cài đặt board ESP8266.

### 4.2. Cài đặt thư viện cho Arduino IDE

#### Bước 1: Cài thư viện ESP8266WiFi
- Trong Arduino IDE, vào **Tools > Manage Libraries**.
- Tìm kiếm `ESP8266WiFi` và chọn **Install**.

#### Bước 2: Cài thư viện Blynk
- Tương tự, tìm và cài thư viện `Blynk` từ **Manage Libraries**.

#### Bước 3: Cài thư viện Time và NTPClient
- Tìm kiếm `Time` và `NTPClient`, sau đó chọn **Install**.

---

## 5. Cài đặt Blynk App (Legacy)

### 5.1. Tải và cài đặt Blynk App (Legacy)
- **iOS**: Truy cập [App Store](https://apps.apple.com) và tìm kiếm **Blynk Legacy** để tải về.
- **Android**: Truy cập [Google Play Store](https://play.google.com) và tìm kiếm **Blynk Legacy** để tải về.

### 5.2. Đăng ký tài khoản và đăng nhập
- Mở app Blynk, chọn **Create New Account** và nhập email và mật khẩu để đăng ký tài khoản.
- Bạn có thể sử dụng server Blynk mặc định hoặc nhập thông tin **custom server** nếu bạn tự host server.

### 5.3. Tạo project Blynk
- Sau khi đăng nhập, nhấn dấu `+` để tạo một project mới.
- Chọn **Device** là ESP8266 và chọn **Connection Type** là WiFi.
- Nhấn **Create** để tạo project, bạn sẽ nhận được mã `auth token` qua email. Lưu mã này để dùng trong code.

### 5.4. Thêm các widget vào project
- **Button Widget**: Điều khiển relay thủ công.
- **Gauge Widget**: Hiển thị độ ẩm đất.
- **LCD Widget**: Hiển thị trạng thái và thời gian hệ thống.
- **Time Input Widget**: Cài đặt thời gian hẹn giờ tưới nước.

---

## 6. Cách sử dụng hệ thống

### 6.1. Các chế độ hoạt động
- **Chế độ tự động:** Dựa vào ngưỡng độ ẩm đất, hệ thống sẽ tự động bật/tắt relay để điều khiển bơm nước.
- **Chế độ thủ công:** Người dùng có thể bật/tắt bơm nước trực tiếp qua app Blynk.
- **Hẹn giờ tưới nước:** Người dùng có thể cài đặt lịch tưới qua **Time Input Widget** trên app.

### 6.2. Điều chỉnh ngưỡng độ ẩm
- Ngưỡng độ ẩm cao và thấp có thể được điều chỉnh trực tiếp trong mã nguồn hoặc trong app Blynk bằng cách thiết lập giá trị phù hợp với điều kiện cây trồng.

### 6.3. Hiển thị trạng thái và thời gian
- Thời gian và ngày tháng được đồng bộ với NTP server và hiển thị trên **Value Display Widget**.

---

## 7. Chức năng các đoạn mã


### 7.1. `setup()`:
- Hàm này thực hiện các thiết lập ban đầu cho hệ thống, bao gồm:
    - Kết nối với WiFi và server Blynk.
    - Thiết lập các chân đầu ra cho đèn LED và relay.
    - Đồng bộ các giá trị từ ứng dụng Blynk như ngưỡng độ ẩm, chế độ hoạt động, và thời gian hẹn giờ.
    - Khởi tạo NTP client để đồng bộ thời gian từ Internet.
    - Thiết lập một bộ hẹn giờ (timer) để thực hiện cập nhật thời gian và điều khiển relay theo chu kỳ.

   **Tác dụng đối với hệ thống**: Đảm bảo hệ thống khởi động chính xác, kết nối với WiFi, server Blynk và đồng bộ các giá trị ban đầu từ ứng dụng.

### 7.2. `loop()`:
- Hàm này được gọi liên tục trong suốt thời gian hoạt động của hệ thống, bao gồm:
     - Thực thi các tác vụ của Blynk (`Blynk.run()`), đảm bảo hệ thống giao tiếp với server Blynk.
     - Thực hiện các tác vụ định kỳ do bộ hẹn giờ thiết lập (`timer.run()`), bao gồm cập nhật hiển thị thời gian và điều khiển relay.
     - Đọc giá trị độ ẩm từ cảm biến và gửi giá trị này lên ứng dụng Blynk.
     - Gọi các hàm điều khiển relay và kiểm tra độ ẩm để đưa ra các cảnh báo cần thiết.

   **Tác dụng đối với hệ thống**: Đảm bảo hệ thống hoạt động liên tục, kiểm soát relay và giám sát giá trị độ ẩm.

### 7.3. `updateDisplay()`:
- Hàm này được gọi mỗi giây để cập nhật thời gian và điều khiển relay theo hẹn giờ.
   
   **Tác dụng đối với hệ thống**: Hiển thị thời gian và quản lý điều khiển relay theo lịch hẹn.

### 7.4. `checkHumidityAndSendAlerts()`:
- Hàm này kiểm tra giá trị độ ẩm hiện tại so với các ngưỡng cao và thấp đã đặt. Nếu độ ẩm vượt ngưỡng, hệ thống sẽ gửi cảnh báo qua ứng dụng Blynk và email. Các đèn LED trên thiết bị cũng sẽ thay đổi trạng thái tương ứng với tình trạng độ ẩm:
     - Đèn đỏ sáng nếu độ ẩm thấp.
     - Đèn vàng sáng nếu độ ẩm cao.
     - Đèn xanh lá sáng nếu độ ẩm ở mức bình thường.

   **Tác dụng đối với hệ thống**: Giúp người dùng giám sát độ ẩm một cách trực quan và qua cảnh báo từ xa (qua ứng dụng và email).

### 7.5. `controlRelayByMode()`:
- Hàm này điều khiển relay dựa trên chế độ hoạt động đã được cài đặt:
     - Chế độ 1 (Tự động): Relay được điều khiển dựa trên giá trị độ ẩm.
     - Chế độ 2 (Thủ công): Người dùng có thể bật/tắt relay bằng nút nhấn trên ứng dụng Blynk.
     - Chế độ 3 (Hẹn giờ): Relay được bật/tắt theo khoảng thời gian đã hẹn trước.

   **Tác dụng đối với hệ thống**: Cho phép điều khiển relay một cách linh hoạt, phù hợp với nhu cầu của người dùng.

### 7.6. `controlRelayAutomatically()`:
- Hàm này điều khiển relay trong chế độ tự động. Nếu độ ẩm vượt ngưỡng cao, relay sẽ tắt. Nếu độ ẩm dưới ngưỡng thấp, relay sẽ bật.

   **Tác dụng đối với hệ thống**: Điều khiển hệ thống tưới nước tự động dựa trên độ ẩm của đất.

### 7.7. `controlRelayManually()`:
- Hàm này điều khiển relay trong chế độ thủ công. Relay sẽ bật/tắt theo trạng thái của nút điều khiển trong ứng dụng Blynk.

   **Tác dụng đối với hệ thống**: Cho phép người dùng tự tay điều khiển hệ thống tưới nước mà không phụ thuộc vào độ ẩm hay lịch hẹn giờ.

### 7.8. `controlRelayBySchedule()`:
- Hàm này điều khiển relay trong chế độ hẹn giờ. Hệ thống sẽ bật/tắt relay dựa trên thời gian hẹn và các ngày trong tuần mà người dùng đã chọn.

   **Tác dụng đối với hệ thống**: Cung cấp khả năng tưới nước theo lịch hẹn, cho phép người dùng thiết lập thời gian chính xác cho việc bật/tắt hệ thống tưới.

### 7.9. `updateTimeDisplay()`:
- Hàm này lấy thời gian thực từ server NTP và cập nhật lên ứng dụng Blynk (bao gồm giờ:phút:giây và ngày tháng năm).

   **Tác dụng đối với hệ thống**: Hiển thị thời gian thực trên ứng dụng Blynk để người dùng theo dõi.

### 7.10. `BLYNK_WRITE(VPIN_MODE_SELECT)`:
- Hàm này xử lý sự kiện khi người dùng thay đổi chế độ hoạt động trên ứng dụng Blynk.

    **Tác dụng đối với hệ thống**: Cho phép người dùng chuyển đổi giữa các chế độ tự động, thủ công, và hẹn giờ.

### 7.11. `BLYNK_WRITE(VPIN_HUMIDITY_THRESHOLD_HIGH)` và `BLYNK_WRITE(VPIN_HUMIDITY_THRESHOLD_LOW)`:
- Hai hàm này xử lý sự kiện khi người dùng thay đổi ngưỡng độ ẩm cao và thấp trên ứng dụng Blynk.

    **Tác dụng đối với hệ thống**: Cập nhật ngưỡng độ ẩm theo yêu cầu của người dùng để điều khiển hệ thống tưới nước.

### 7.12. `BLYNK_WRITE(VPIN_RELAY_CONTROL)`:
- Hàm này xử lý sự kiện khi người dùng bật/tắt relay trong chế độ thủ công.

    **Tác dụng đối với hệ thống**: Điều khiển trạng thái bật/tắt của hệ thống tưới nước trong chế độ thủ công.

### 7.13. `BLYNK_WRITE(VPIN_TIME_INPUT)`:
- Hàm này xử lý dữ liệu đầu vào từ widget Time Input (giờ hẹn) trên ứng dụng Blynk và thiết lập thời gian hẹn cho hệ thống.

    **Tác dụng đối với hệ thống**: Cho phép người dùng hẹn giờ bật/tắt hệ thống tưới nước cho các ngày trong tuần.

---

## 8. Xử lý sự cố

- **ESP8266 không kết nối được WiFi**: Kiểm tra tên và mật khẩu WiFi trong mã. Đảm bảo tín hiệu WiFi mạnh và gần.
- **Ứng dụng Blynk không hiển thị đúng thông tin**: Kiểm tra kết nối mạng và đảm bảo mã `auth token` trong mã nguồn trùng khớp với token trên app.
- **Relay không hoạt động**: Đảm bảo relay được cấp nguồn đúng cách và được kết nối đúng chân với ESP8266.

---

## 9. Ghi chú
- Đảm bảo bơm nước và nguồn cung cấp điện được kết nối an toàn và chính xác để tránh các tai nạn điện.

---

## 10. Thông tin liên hệ
- **Tác giả:** Trần Ngọc Tới
- **Email:** tranngoctoi219@gmail.com
- **GitHub:** https://github.com/yourprofile

---

