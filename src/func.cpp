#include "func.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

// Định nghĩa các biến toàn cục
int btnSetDebounceMill = 20; // Thời gian chống nhiễu cho nút bấm, tránh ghi nhận nhiều lần nhấn do nhiễu
int btnSetPressMill = 1000;  // Thời gian tối thiểu để nhận diện một lần nhấn giữ nút
int pIndex = 1;              // Chỉ số hiện tại của mục cài đặt, dùng để định vị mục trong danh sách
int menuIndex = 1;           // Chỉ số menu hiện tại, dùng để xác định menu đang được chọn
int totalChildren;           // Tổng số phần tử con trong menu hiện tại
int maxValue = 0;            // Giá trị lớn nhất có thể, dùng trong các mục cài đặt
int minValue = 0;            // Giá trị nhỏ nhất có thể, dùng trong các mục cài đặt
int maxLength = 0;           // Độ dài tối đa của chuỗi giá trị, dùng để giới hạn đầu vào
int columnIndex = 0;         // Chỉ số cột hiện tại để chỉnh sửa giá trị, thường dùng khi nhập liệu số
int currentValue;            // Giá trị hiện tại của mục cài đặt, lưu trữ giá trị đang được chỉnh sửa
volatile uint64_t soXungDaChay = 0;
int divisorValue = 0; // Biến lưu hệ số chia để hiển thị thập phân

byte trangThaiHoatDong = 0;  // Trạng thái hoạt động của chương trình, dùng để điều hướng giữa các trạng thái khác nhau
byte mainStep = 0;           // Bước chính trong quy trình hoạt động của chương trình
byte testModeStep = 0;       // Bước kiểm tra trong chế độ kiểm tra
byte maxTestModeStep = 0;    // Bước kiểm tra tối đa trong chế độ kiểm tra

byte testOutputStep = 0;     // Bước xuất kiểm tra
byte maxTestOutputStep = 0;  // Bước xuất kiểm tra tối đa

bool explanationMode = false;   // Chế độ giải thích, cho phép hiển thị thông tin chi tiết về mục cài đặt
bool editAllowed = false;       // Cho phép chỉnh sửa, xác định liệu người dùng có thể chỉnh sửa mục cài đặt hay không
bool hienThiTestOutput = false; // Hiển thị kết quả kiểm tra, dùng để điều hướng hiển thị
bool daoTinHieuOutput = false;  // Đảo tín hiệu đầu ra, dùng khi cần thay đổi trạng thái tín hiệu đầu ra
bool chayTestMode = false;      // Chạy chế độ kiểm tra, xác định liệu chế độ kiểm tra có đang hoạt động

const char* menu1;  // Chuỗi chứa nội dung menu 1, lấy từ tài liệu JSON
const char* menu2;  // Chuỗi chứa nội dung menu 2, lấy từ tài liệu JSON
const char* menu3;  // Chuỗi chứa nội dung menu 3, lấy từ tài liệu JSON
const char* configFile = "/config.json"; // Đường dẫn đến tệp cấu hình, nơi lưu trữ thông tin cấu hình của chương trình

String displayScreen = "index"; // Màn hình hiển thị hiện tại, xác định màn hình nào đang được hiển thị
String setupCodeStr;            // Chuỗi mã cài đặt, dùng để lưu trữ mã cài đặt của mục hiện tại
String valueStr;                // Chuỗi giá trị, lưu trữ giá trị của mục hiện tại dưới dạng chuỗi
String textExplanationMode;     // Chuỗi mô tả chế độ giải thích, chứa nội dung giải thích của mục cài đặt
String textStr;                 // Chuỗi mô tả, chứa thông tin văn bản hiển thị trên màn hình
String keyStr;                  // Chuỗi khóa, dùng để lưu trữ khóa của mục cài đặt trong tài liệu JSON
String ListExp[10];             // Mảng chứa các phần chức năng giải thích thông số, chứa các mục giải thích cho mục cài đặt

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Hàm kiểm tra một chuỗi có phải là số hay không
bool isNumeric(const char* str) {
  if (str == nullptr || str[0] == '\0') return false; // Chuỗi rỗng không được coi là số
  int startIndex = 0; // Bắt đầu từ ký tự đầu tiên
  if (str[0] == '-') { // Nếu ký tự đầu tiên là dấu trừ
    if (str[1] == '\0') return false; // Chỉ có dấu trừ không phải là số
    startIndex = 1; // Bỏ qua dấu trừ
  }
  for (int i = startIndex; str[i] != '\0'; i++) { // Duyệt qua từng ký tự trong chuỗi
    if (!isDigit(str[i])) { // Nếu ký tự không phải là số
      return false;
    }
  }
  return true; // Tất cả các ký tự là số
}

// Hàm tách chuỗi theo dấu phẩy
void splitString(const String& input, String* output, int maxParts) {
  int partCount = 0; // Đếm số phần đã tách
  int startIndex = 0; // Vị trí bắt đầu
  while (partCount < maxParts) {
    int commaIndex = input.indexOf(',', startIndex); // Tìm vị trí dấu phẩy
    if (commaIndex == -1) { // Nếu không tìm thấy dấu phẩy
      output[partCount++] = input.substring(startIndex); // Lấy phần còn lại của chuỗi
      break;
    }
    output[partCount++] = input.substring(startIndex, commaIndex); // Lấy phần từ vị trí bắt đầu đến dấu phẩy
    startIndex = commaIndex + 1; // Cập nhật vị trí bắt đầu
  }
}

// Hàm chờ trong khoảng thời gian bằng milliseconds
bool WaitMillis(unsigned long thoiDiemCuoi, unsigned long waitTime) {
  return (millis() - thoiDiemCuoi > waitTime); // Kiểm tra nếu thời gian chờ đã qua
}

// Hàm chờ trong khoảng thời gian bằng microseconds
bool WaitMicros(unsigned long thoiDiemCuoi, unsigned long waitTime) {
  return (micros() - thoiDiemCuoi > waitTime); // Kiểm tra nếu thời gian chờ đã qua
}

// Hàm vẽ văn bản căn giữa theo chiều ngang
void drawCenteredText(const char* text, int y) {
  int screenWidth = u8g2.getDisplayWidth(); // Lấy độ rộng màn hình
  int textWidth = u8g2.getStrWidth(text); // Lấy độ rộng của chuỗi
  int x = (screenWidth - textWidth) / 2; // Tính vị trí x để căn giữa
  u8g2.drawStr(x, y, text); // Vẽ chuỗi căn giữa
}

// Hàm hiển thị văn bản xuống dòng nếu dài quá
void wrapText(const char* text, int16_t x, int16_t y, int16_t lineHeight, int16_t maxWidth) {
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1); // Thiết lập font chữ thường (không đậm)
  int16_t cursorX = x; // Vị trí x bắt đầu in
  int16_t cursorY = y; // Vị trí y bắt đầu in
  const char* wordStart = text; // Vị trí bắt đầu của từ trong chuỗi
  const char* currentChar = text; // Ký tự hiện tại đang xử lý
  while (*currentChar) { // Vòng lặp qua từng ký tự trong chuỗi
    if (*currentChar == ' ' || *(currentChar + 1) == '\0') { // Nếu gặp khoảng trắng hoặc kết thúc chuỗi
      char word[64]; // Tạo chuỗi tạm để chứa từ hiện tại
      int len = currentChar - wordStart + 1; // Độ dài từ
      strncpy(word, wordStart, len); // Sao chép từ vào chuỗi tạm
      word[len] = '\0'; // Kết thúc chuỗi
      int16_t textWidth = u8g2.getStrWidth(word); // Độ rộng của từ
      if (cursorX + textWidth > maxWidth) { // Nếu từ không vừa chiều rộng màn hình
        cursorX = x; // Xuống dòng
        cursorY += lineHeight; // Tăng vị trí y
      }

      u8g2.setCursor(cursorX, cursorY); // Đặt vị trí con trỏ 
      u8g2.print(word); // Hiển thị từ lên màn hình

      //u8g2.drawStr(cursorX, cursorY, word); // Vẽ từ lên màn hình
      cursorX += textWidth; // Cập nhật vị trí x
      if (*currentChar == ' ') {
        // Chỉ thêm khoảng trắng khi ký tự hiện tại là khoảng trắng
        wordStart = currentChar + 1; // Di chuyển đến từ tiếp theo
        //cursorX += u8g2.getStrWidth(" "); // Thêm khoảng trắng
      }
      //wordStart = currentChar + 1; // Di chuyển đến từ tiếp theo
    }
    currentChar++; // Chuyển ký tự hiện tại sang ký tự tiếp theo
  }
}

// Hàm ghi log ra Serial
void log(String mes) {
  Serial.println(mes); // In thông điệp ra Serial
}

// Hàm ghi tệp JSON vào hệ thống tệp
void writeFile(JsonDocument& doc, const char* path) {
  File file = LittleFS.open(path, "w"); // Mở tệp ở chế độ ghi
   if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  serializeJson(doc, file); // Ghi dữ liệu JSON vào tệp
  file.close(); // Đóng tệp sau khi ghi
  Serial.println("File written successfully!");
}

// Hàm hiển thị danh sách menu
void showList(int indexNum) {
  menu1 = jsonDoc["main"]["main1"]["text"]; // Lấy nội dung của menu 1 từ JSON
  menu2 = jsonDoc["main"]["main2"]["text"]; // Lấy nội dung của menu 2 từ JSON
  menu3 = jsonDoc["main"]["main3"]["text"]; // Lấy nội dung của menu 3 từ JSON

  u8g2.clearBuffer(); // Xóa bộ nhớ đệm của màn hình để vẽ mới
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1); // Thiết lập font chữ thường (không đậm)

  u8g2.setCursor(12, 16); // Đặt vị trí con trỏ
  u8g2.print(menu1); // Hiển thị danh mục 1
  u8g2.setCursor(12, 32); // Đặt vị trí con trỏ
  u8g2.print(menu2); // Hiển thị danh mục 2
  u8g2.setCursor(12, 48); // Đặt vị trí con trỏ
  u8g2.print(menu3); // Hiển thị danh mục 3
  u8g2.setCursor(0, indexNum * 16); // Đặt vị trí con trỏ cho dấu nhắc 
  u8g2.print(">"); // Hiển thị dấu nhắc tại vị trí mục đã chọn

  /*u8g2.drawStr(12, 16, menu1); // Hiển thị danh mục 1
  u8g2.drawStr(12, 32, menu2); // Hiển thị danh mục 2
  u8g2.drawStr(12, 48, menu3); // Hiển thị danh mục 3
  u8g2.drawStr(0, indexNum * 16, ">"); // Hiển thị dấu nhắc tại vị trí mục đã chọn*/

  u8g2.sendBuffer(); // Gửi nội dung đệm ra màn hình
}

// Hàm hiển thị văn bản với tiêu đề và nội dung
void showText(const char* title, const char* messenger) {
  u8g2.clearBuffer(); // Xóa bộ nhớ đệm của màn hình để vẽ mới
  u8g2.setFont(u8g2_font_crox3hb_tf); // Thiết lập font chữ đậm

  drawCenteredText(title, 18); // Vẽ tiêu đề căn giữa

  u8g2.setFont(u8g2_font_unifont_t_vietnamese1); // Thiết lập font chữ thường (không đậm)
  wrapText(messenger, 0, 42, 18, 128); // Hiển thị nội dung văn bản xuống dòng nếu dài quá
  u8g2.sendBuffer(); // Gửi nội dung đệm ra màn hình
}

// Hàm hiển thị tiến trình
void showProgress(int parameter1, int parameter2, int parameter3) {
  u8g2.clearBuffer(); // Xóa bộ nhớ đệm của màn hình để vẽ mới
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1); // Thiết lập font chữ thường (không đậm)

  u8g2.setCursor(0, 18); // Đặt vị trí con trỏ
  u8g2.print("M.Tiêu "); // Hiển thị tiêu đề tổng số
  u8g2.setCursor(64, 18); // Đặt vị trí con trỏ cho giá trị
  u8g2.print(parameter1); // Hiển thị giá trị tổng số

  u8g2.setCursor(0, 36); // Đặt vị trí con trỏ
  u8g2.print("H.Tại: "); // Hiển thị tiêu đề Re.Stick
  u8g2.setCursor(64, 36); // Đặt vị trí con trỏ cho giá trị
  u8g2.print(parameter2); // Hiển thị giá trị Re.Stick

  u8g2.setCursor(0, 54); // Đặt vị trí con trỏ
  u8g2.print("Lost: "); // Hiển thị tiêu đề Count output
  u8g2.setCursor(64, 54); // Đặt vị trí con trỏ cho giá trị
  u8g2.print(parameter3); // Hiển thị giá trị Count output

  u8g2.sendBuffer(); // Gửi dữ liệu từ bộ đệm lên màn hình
}

// Hàm tính số chữ số thập phân dựa trên divisor một cách an toàn
// 10 -> 1, 100 -> 2, 1000 -> 3; nếu không phải 10^n, dùng số chữ số của divisor
int calcDecimalDigits(int divisor) {
  if (divisor < 10) return 0;
  int d = divisor;
  int zeros = 0;
  while (d >= 10 && (d % 10 == 0)) { // đếm số 0 ở cuối nếu là 10^n
    zeros++;
    d /= 10;
  }
  if (zeros > 0) return zeros;

  // fallback: số chữ số của divisor (ví dụ 25 -> 2)
  int len = 0;
  while (divisor > 0) { len++; divisor /= 10; }
  return len;
}

// Hàm hiển thị cài đặt
void showSetup(const char* setUpCode, const char* value, const char* text) {
  u8g2.clearBuffer(); // Xóa bộ nhớ đệm của màn hình để vẽ mới
  u8g2.setFont(u8g2_font_crox3hb_tf); // Thiết lập font chữ đậm

  char tempSetUpCode[64]; // Tạo chuỗi tạm chứa mã cài đặt và dấu ":"
  snprintf(tempSetUpCode, sizeof(tempSetUpCode), "%s:", setUpCode); // Nối mã cài đặt với dấu ":"
  u8g2.drawStr(0, 18, tempSetUpCode); // Hiển thị mã cài đặt

  u8g2.setFont(u8g2_font_crox3h_tf); // Thiết lập font chữ thường (không đậm)

  char maxValueStr[16]; // Chuỗi chứa giá trị maxValue sau khi chuyển đổi
  snprintf(maxValueStr, sizeof(maxValueStr), "%d", maxValue); // Chuyển maxValue thành chuỗi

  int valueLength = strlen(value); // Độ dài của chuỗi giá trị
  if (!isNumeric(value)) { // Nếu giá trị không phải là số
    maxLength = 5;
  } else {
    maxLength = strlen(maxValueStr); // Độ dài tối đa của giá trị
    if (valueLength > maxLength) {
      valueLength = maxLength;
    }
  }

  int startX = 128 - 10; // Bắt đầu từ vị trí rìa phải

  // Tính vị trí dấu chấm: vẽ từ phải sang trái, nên cần số chữ số thập phân rồi quy đổi sang chỉ số i
  int dotPosition = -1;
  int dotDigits = 0;
  bool hasDivisor = (divisorValue > 0);

  if (isNumeric(value) && hasDivisor) {
    dotDigits = calcDecimalDigits(divisorValue); // số chữ số thập phân cần hiển thị
    if (dotDigits > 0) {
      // Với vẽ ngược (i=0 là ký tự bên phải nhất), dấu "." nằm sau dotDigits ký tự từ phải sang
      // Nghĩa là vẽ dấu "." khi i == dotDigits - 1
      dotPosition = dotDigits - 1;
    }
  }

  // Xác định số ký tự thực tế sẽ hiển thị (bao gồm chữ số 0 bên trái nếu cần)
  int displayLength = valueLength;
  if (dotDigits > 0) {
    int minLen = dotDigits + 1; // đảm bảo có ít nhất một chữ số nguyên bên trái dấu thập phân
    if (displayLength < minLen) displayLength = minLen;
  }

  // không vượt quá maxLength
  if (displayLength > maxLength) displayLength = maxLength;

  for (int i = 0; i < displayLength; i++) {
    char ch = ' '; // mặc định khoảng trống
    if (i < valueLength) {
      char tmp = value[valueLength - 1 - i];
      ch = tmp;
    } else {
      if (isNumeric(value) && dotDigits > 0) {
        // padding '0' khi cần hiển thị chữ số nguyên 0 trước phần thập phân
        ch = '0';
      } else {
        ch = ' ';
      }
    }

    char temp[2] = { ch, '\0' }; // Lấy ký tự theo thứ tự ngược lại
    int x = startX - (i * 10);
    u8g2.drawStr(x, 18, temp); // Vẽ ký tự lùi về bên trái

    // Vẽ dấu "." phân cách thập phân đúng vị trí khi i đạt dotPosition
    if (dotPosition >= 0 && i == dotPosition) {
      // Vẽ dấu "." nằm giữa ký tự hiện tại và ký tự kế tiếp bên trái
      u8g2.drawStr(x - 3, 18, "."); // Vẽ dấu "." phân cách thập phân
    }
  }

  u8g2.drawLine(0, 23, 128, 23); // Vẽ một đường ngang trên màn hình
  wrapText(text, 0, 42, 18, 128); // Hiển thị nội dung văn bản xuống dòng nếu dài quá
  u8g2.sendBuffer(); // Gửi nội dung đệm ra màn hình
}

// Hàm hiển thị chế độ chỉnh sửa
void showEdit(int columnIndex) {
  u8g2.clearBuffer(); // Xóa bộ nhớ đệm của màn hình để vẽ mới
  u8g2.setFont(u8g2_font_crox3hb_tf); // Thiết lập font chữ đậm

  char tempSetUpCode[64]; // Tạo chuỗi tạm chứa mã cài đặt và dấu ":"
  snprintf(tempSetUpCode, sizeof(tempSetUpCode), "%s:", setupCodeStr.c_str()); // Nối mã cài đặt với dấu ":"
  u8g2.drawStr(0, 18, tempSetUpCode); // Hiển thị mã cài đặt

  u8g2.setFont(u8g2_font_crox3h_tf); // Thiết lập font chữ thường (không đậm)

  char maxValueStr[16]; // Chuỗi chứa giá trị maxValue sau khi chuyển đổi
  snprintf(maxValueStr, sizeof(maxValueStr), "%d", maxValue); // Chuyển maxValue thành chuỗi

  const char* valueChr = valueStr.c_str(); // Lấy giá trị hiện tại
  int valueLength = strlen(valueChr); // Độ dài của giá trị hiện tại
  int maxLength = strlen(maxValueStr); // Độ dài tối đa của giá trị

  if (valueLength > maxLength) {
    valueLength = maxLength;
  }

  int startX = 128 - 10; // Bắt đầu từ vị trí rìa phải

  // Tính vị trí dấu chấm tương tự showSetup
  int dotPosition = -1;
  int dotDigits = 0;
  bool hasDivisor = (divisorValue > 0);

  if (isNumeric(valueChr) && hasDivisor) {
    dotDigits = calcDecimalDigits(divisorValue);
    if (dotDigits > 0) {
      dotPosition = dotDigits - 1;
    }
  }

  // Xác định số ký tự thực tế sẽ hiển thị (bao gồm chữ số 0 bên trái nếu cần)
  int displayLength = valueLength;
  if (dotDigits > 0) {
    int minLen = dotDigits + 1; // đảm bảo có ít nhất một chữ số nguyên bên trái dấu thập phân
    if (displayLength < minLen) displayLength = minLen;
  }

  // không vượt quá maxLength
  if (displayLength > maxLength) displayLength = maxLength;

  for (int i = 0; i < maxLength; i++) {
    char tempCh = ' '; // Khởi tạo chuỗi tạm với giá trị mặc định

    if (i < displayLength) {
      if (i < valueLength) {
        tempCh = valueChr[valueLength - 1 - i]; // Lấy ký tự theo thứ tự ngược lại
      } else {
        if (isNumeric(valueChr) && dotDigits > 0) {
          tempCh = '0'; // padding '0' khi cần
        } else {
          tempCh = ' ';
        }
      }
    } else {
      tempCh = ' ';
    }

    char temp[2] = {tempCh, '\0'};

    int x = startX - (i * 10); // Tính vị trí vẽ ký tự

    if (i == columnIndex) {
      u8g2.setDrawColor(1); // Đặt màu nền
      u8g2.drawBox(x, 5, 9, 15); // Vẽ ô vuông làm nền
      u8g2.setDrawColor(0); // Đặt màu chữ
      u8g2.drawStr(x, 18, temp); // Vẽ ký tự
      u8g2.setDrawColor(1); // Khôi phục màu nền
    } else {
      u8g2.drawStr(x, 18, temp); // Vẽ ký tự
    }

    // Vẽ dấu "." nếu cần, và cũng tô nền nếu columnIndex trùng vị trí dấu "."
    if (dotPosition >= 0 && i == dotPosition) {
      int dotX = x - 3; // Vị trí vẽ dấu "." giữa hai ký tự

      if (i == columnIndex) {
        // Tô nền cho dấu "." như yêu cầu
        u8g2.setDrawColor(1);
        u8g2.drawBox(dotX + 1, 15, 2, 5); // tô nền cho dấu chấm
        u8g2.setDrawColor(0);
        u8g2.drawStr(dotX, 18, "."); 
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawStr(dotX, 18, "."); // Vẽ dấu "." phân cách thập phân
      }
    }
  }

  u8g2.drawLine(0, 23, 128, 23); // Vẽ một đường ngang trên màn hình

  wrapText(textStr.c_str(), 0, 42, 18, 128); // Hiển thị nội dung văn bản xuống dòng nếu dài quá
  u8g2.sendBuffer(); // Gửi nội dung đệm ra màn hình
}


// Hàm tải cấu hình từ JSON
void loadJsonSettings() {
  try {
    const char* code = jsonDoc["main"]["main" + String(menuIndex)]["key"]; // Lấy key của menu hiện tại từ JSON
    totalChildren = jsonDoc["main"]["main" + String(menuIndex)]["totalChildren"]; // Lấy tổng số phần tử con
    setupCodeStr = String(code) + String(pIndex); // Tạo setupCode dựa trên key và pIndex

    // Kiểm tra xem configuredValue có phải là số không
    if (jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["configuredValue"].is<int>()) {
      int valueInt = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["configuredValue"];
      valueStr = String(valueInt); // Chuyển đổi value từ int thành String
      currentValue = valueStr.toInt(); // Cập nhật currentValue
    } else if (jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["configuredValue"].is<const char*>()) {
      valueStr = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["configuredValue"].as<const char*>();
      currentValue = -1; // Nếu không phải là số thì đặt currentValue là -1
    } else {
      valueStr = ""; // Gán giá trị mặc định nếu không phải số hoặc chuỗi
    }

    maxValue = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["maxValue"];
    minValue = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["minValue"];
    explanationMode = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["explanationMode"];
    editAllowed = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["editAllowed"];

    // Đọc divisor từ JSON
    if (jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["divisor"].is<int>()) {
      divisorValue = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["divisor"];
    } else {
      divisorValue = 0; // Mặc định nếu không có hoặc không hợp lệ
    }

    // Kiểm tra chế độ giải thích
    if (explanationMode) {
      String listStr = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["explanationDetails"];
      splitString(listStr, ListExp, 10);
      textExplanationMode = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["text"].as<const char*>();
      textStr = textExplanationMode + ": " + ListExp[currentValue - 1]; // Hiển thị text từ list
    } else {
      textStr = jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["text"].as<const char*>();
    }

    keyStr = String(code);
    log("Truy cập thẻ " + keyStr + "/" + setupCodeStr);
    showSetup(setupCodeStr.c_str(), valueStr.c_str(), textStr.c_str());
  } catch (const std::exception& e) {
    Serial.println("Error reading JSON data: "); // In thông báo lỗi
    Serial.println(e.what()); // In thông tin chi tiết từ e.what()
  } catch (...) {
    Serial.println("An unknown error occurred while reading JSON data."); // In thông báo lỗi chung
  }
}

// Hàm chỉnh sửa giá trị
void editValue(const char* Calculations) {
  int newValue;
  int factor = pow(10, columnIndex); // Tính hàng (đơn vị, chục, trăm, v.v.)

  if (strcmp(Calculations, "addition") == 0) {
    newValue = currentValue + factor; // Tăng giá trị tại hàng đang chọn
  } else if (strcmp(Calculations, "subtraction") == 0) {
    newValue = currentValue - factor; // Giảm giá trị tại hàng đang chọn
  }

  if (newValue >= minValue && newValue <= maxValue) {
    currentValue = newValue; // Cập nhật currentValue nếu newValue hợp lệ
  }

  valueStr = String(currentValue); // Chuyển giá trị thành chuỗi để hiển thị

  if (explanationMode) {
    textStr = textExplanationMode + ": " + ListExp[currentValue - 1];
  }

  showEdit(columnIndex); // Cập nhật màn hình
}

// Hàm đọc tệp cấu hình
void readConfigFile() {
  File config = LittleFS.open(configFile, "r"); // Mở tệp ở chế độ đọc
  if (!config) {
    Serial.println("Failed to open config file"); // In thông báo lỗi
    return;
  }

  DeserializationError error = deserializeJson(jsonDoc, config);
  if (error) {
    Serial.println("Failed to read config file"); // In thông báo lỗi
    return;
  }
  config.close(); // Đóng tệp sau khi đọc
}

// Hàm đặt lại giá trị cấu hình mặc định
void reSet() {
  int totalPrmReSet = jsonDoc["main"]["main1"]["totalChildren"];
  for (size_t i = 0; i < totalPrmReSet; i++) {
    jsonDoc["main"]["main1"]["children"]["CD" + String(i + 1)]["configuredValue"] = jsonDoc["main"]["main1"]["children"]["CD" + String(i + 1)]["defaultValue"];
  }
  writeFile(jsonDoc, "/config.json"); // Ghi lại tệp cấu hình với các giá trị mặc định
}

// Đưa timers ra ngoài để chia sẻ giữa các hàm Wait & reset Wait
static std::map<void*, unsigned long> timers;

bool Wait(unsigned long waitTime) {
  void* callerID = __builtin_return_address(0);  // tạo ID theo vị trí gọi

  if (timers.find(callerID) == timers.end())
    timers[callerID] = millis();

  if (millis() - timers[callerID] > waitTime) {
    timers.erase(callerID); // reset để có thể lặp lại
    return true;
  }
  return false;
}
void resetWait() {
  void* callerID = __builtin_return_address(0);
  timers.erase(callerID);
}
void xuatXungPWMold(unsigned long thoiGianDao,int pinPWM) {
  static bool trangThaiPWM = false;
  static unsigned long thoiDiemCuoiPWM = 0;
  if (WaitMicros(thoiDiemCuoiPWM, thoiGianDao)) {
    trangThaiPWM = !trangThaiPWM;
    digitalWrite(pinPWM, trangThaiPWM);
    thoiDiemCuoiPWM = micros();

    if (trangThaiPWM) {
      soXungDaChay++; // chỉ đếm khi vừa lên HIGH
    }
  }
}

void xuatXungPWM(unsigned long thoiGianDaoMicros, int pinPWM) {
  static bool trangThaiPWM = false;
  static unsigned long thoiDiemCuoiPWM = 0;

  unsigned long now = micros();
  if (WaitMicros(thoiDiemCuoiPWM, thoiGianDaoMicros)) {
    thoiDiemCuoiPWM = now;
    trangThaiPWM = !trangThaiPWM;
    digitalWrite(pinPWM, trangThaiPWM ? HIGH : LOW);

    if (trangThaiPWM) {
      // Gọi đúng: truyền &mux
      portENTER_CRITICAL(&mux);
      soXungDaChay++;
      portEXIT_CRITICAL(&mux);
    }
  }
}

// Hàm đọc số chu kỳ CPU hiện tại (dùng để đo thời gian chính xác đến từng chu kỳ)
static inline uint32_t read_ccount() {
  uint32_t c;
  asm volatile ("rsr.ccount %0" : "=a" (c));  // Lệnh assembly đọc thanh ghi ccount
  return c;                                   // Trả về số chu kỳ CPU hiện tại
}


// Hàm xuất 1 xung PWM với mỗi pha HIGH và LOW kéo dài half_cycles chu kỳ CPU
void xuatXungPWM_cycles(uint32_t half_cycles, int pin) {
  if (half_cycles == 0) return;               // Nếu thời gian bằng 0 thì không làm gì

  uint32_t mask = (1UL << pin);              // Tạo mặt nạ bit để thao tác trực tiếp với chân GPIO

  GPIO.out_w1ts = mask;                      // Đặt chân GPIO lên mức HIGH
  uint32_t start = read_ccount();            // Ghi lại thời điểm bắt đầu pha HIGH
  while ((read_ccount() - start) < half_cycles) {
    asm volatile("nop");                     // Chờ đến khi đủ half_cycles
  }

  GPIO.out_w1tc = mask;                      // Đặt chân GPIO xuống mức LOW
  start = read_ccount();                     // Ghi lại thời điểm bắt đầu pha LOW
  while ((read_ccount() - start) < half_cycles) {
    asm volatile("nop");                     // Chờ đến khi đủ half_cycles
  }

  soXungDaChay++;                             // Tăng biến đếm số xung đã phát
}


// Hàm xuất 1 xung PWM với tổng thời gian duration_us (micro giây)
void xuatXungPWM_us(uint32_t duration_us, int pinPWM) {
  if (duration_us == 0) return;              // Nếu thời gian bằng 0 thì không làm gì

  uint32_t cpu_freq_mhz = getCpuFrequencyMhz();               // Lấy tần số CPU hiện tại (ví dụ: 240 MHz)
  uint32_t half_cycles = (duration_us * cpu_freq_mhz) / 2;    // Tính số chu kỳ cho mỗi pha HIGH/LOW

  xuatXungPWM_cycles(half_cycles, pinPWM);                       // Gọi hàm xuất xung theo chu kỳ đã tính
}

// Đọc mức logic chân nhanh bằng thanh ghi GPIO.in
static inline int gpio_level_fast(int pin) {
  return (GPIO.in >> pin) & 1;
}


// Đợi chân pin toggle (thay đổi mức so với trạng thái ban đầu).
// Trả về true nếu thấy thay đổi trong khoảng timeout_us (micro giây).
// Trả về false nếu timeout.
// Lưu ý: timeout_us = 0 => kiểm tra ngay, nếu đã toggle trước đó sẽ trả về false (không đợi).
bool docPWM_waitToggle(int pin, uint32_t timeout_us) {
  const uint32_t cpu_mhz = getCpuFrequencyMhz();                  // MHz
  const uint32_t timeout_cycles = timeout_us * cpu_mhz;          // cycles (timeout_us * MHz)
  uint32_t start = read_ccount();

  int initial = gpio_level_fast(pin);                            // trạng thái ban đầu

  // Nếu timeout_us == 0 thì chỉ kiểm tra ngay và trả về theo kết quả
  if (timeout_us == 0) {
    return (gpio_level_fast(pin) != initial);
  }

  // Vòng busy-wait: chờ khi level thay đổi so với initial
  while (true) {
    if (gpio_level_fast(pin) != initial) {
      return true; // có thay đổi
    }
    // kiểm tra timeout
    if ((read_ccount() - start) > timeout_cycles) {
      return false; // timeout
    }
    asm volatile("nop"); // giữ vòng ngắn gọn, optional
  }
}

// Đo chu kỳ (HIGH+LOW) bằng cách bắt 3 cạnh: rising, falling, next rising
// Trả về period tính bằng micro giây; trả 0 nếu timeout (ms_timeout tính bằng ms)
uint32_t docPWM_us_fast(int pin, uint32_t ms_timeout = 1000) {
  const uint32_t cpu_mhz = getCpuFrequencyMhz();
  const uint32_t timeout_cycles = ms_timeout * 1000UL * cpu_mhz; // ms -> µs -> cycles
  uint32_t start_cycle = read_ccount();
  uint32_t now;

  // 1) chờ mức LOW (nếu đang HIGH) để sync trước khi bắt rising
  now = read_ccount();
  while (gpio_level_fast(pin) == 1) {
    if ((read_ccount() - start_cycle) > timeout_cycles) return 0;
  }

  // 2) chờ rising edge (LOW -> HIGH)
  start_cycle = read_ccount();
  while (gpio_level_fast(pin) == 0) {
    if ((read_ccount() - start_cycle) > timeout_cycles) return 0;
  }
  uint32_t t_rising = read_ccount(); // thời điểm rising

  // 3) chờ falling edge (HIGH -> LOW)
  while (gpio_level_fast(pin) == 1) {
    if ((read_ccount() - t_rising) > timeout_cycles) return 0;
  }
  uint32_t t_falling = read_ccount(); // thời điểm falling

  // 4) chờ next rising edge (LOW -> HIGH)
  while (gpio_level_fast(pin) == 0) {
    if ((read_ccount() - t_falling) > timeout_cycles) return 0;
  }
  uint32_t t_next_rising = read_ccount();

  // Tính chu kỳ bằng cycles
  uint32_t high_cycles = t_falling - t_rising;
  uint32_t low_cycles  = t_next_rising - t_falling;
  uint32_t total_cycles = high_cycles + low_cycles;

  // Chuyển sang micro giây: cycles / cpu_mhz
  uint32_t period_us = total_cycles / cpu_mhz;
  return period_us;
}


/*static inline uint32_t read_ccount() {
  uint32_t c;
  asm volatile ("rsr.ccount %0" : "=a" (c));
  return c;
}
void xuatXungPWM_cycles(uint32_t half_cycles, uint8_t pin) {
  if (half_cycles == 0) return;
  uint32_t mask = (1UL << pin);
  // set high (direct register = rất nhanh)
  GPIO.out_w1ts = mask;
  uint32_t start = read_ccount();
  while ((read_ccount() - start) < half_cycles) { asm volatile("nop"); }
  // set low
  GPIO.out_w1tc = mask;
  start = read_ccount();
  while ((read_ccount() - start) < half_cycles) { asm volatile("nop"); }
  soXungDaChay++;
}
void xuatXungPWM_ms(uint32_t duration_ms, int pinPWM) {
  if (duration_ms == 0) return;
  uint32_t cpu_freq_mhz = getCpuFrequencyMhz(); // ví dụ: 240
  uint32_t half_cycles = (duration_ms * cpu_freq_mhz * 1000) / 2;
  xuatXungPWM_cycles(half_cycles, pin);
}*/

