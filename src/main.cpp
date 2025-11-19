#include <Arduino.h>
#include <OneButton.h>
#include "ota.h"
#include "func.h"  // Bao gồm file header func.h để sử dụng các hàm từ func.cpp
#include <AccelStepper.h>



//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Khởi tạo đối tượng màn hình OLED U8G2
//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Khởi tạo đối tượng màn hình OLED U8G2

// Thông tin mạng WiFi và OTA



StaticJsonDocument<200> jsonDoc;



const char* jsonString = R"()";
bool isChanged = false;
void tinhToanCaiDat();
void loadSetup();
void veGoc();

OneButton btnMenu(0, true,false);
OneButton btnSet(2, false,false);
OneButton btnUp(12, false,false);
OneButton btnDown(15, false,false);
OneButton btnRun(23,false,false);
OneButton btnEstop(13,false,false);


void btnMenuClick() {
  //Serial.println("Button Clicked (nhấn nhả)");
  if (displayScreen == "ScreenCD") {
    if (keyStr == "CD" && isChanged) {
      isChanged = false;
      writeFile(jsonDoc,"/config.json");
    }
    showList(menuIndex);  // Hiển thị danh sách menu hiện tại
    displayScreen = "MENU";
  } else if (displayScreen == "ScreenEdit") {
    loadJsonSettings();
    displayScreen = "ScreenCD";
  } else if (displayScreen == "index" && mainStep == 0) {
    trangThaiHoatDong = 0;
    showList(menuIndex);  // Hiển thị danh sách menu hiện tại
    displayScreen = "MENU";
  } else if (displayScreen == "MENU" && mainStep == 0){
    displayScreen= "index";
    trangThaiHoatDong = 1;
    veGoc();
    //showText("HELLO", "ESP32-OPTION");
  } else if (displayScreen == "testIO"){
    loadJsonSettings();
    displayScreen = "ScreenCD";
    trangThaiHoatDong = 0;
  } else if (displayScreen == "testOutput"){
    loadJsonSettings();
    displayScreen = "ScreenCD";
    trangThaiHoatDong = 0;
  } else if (displayScreen == "screenTestMode" && testModeStep == 0){
    loadJsonSettings();
    displayScreen = "ScreenCD";
    trangThaiHoatDong = 0;
  } else if (displayScreen == "OTA"){
    loadJsonSettings();
    displayScreen = "ScreenCD";
    trangThaiHoatDong = 0;
  }
}

// Hàm callback khi bắt đầu nhấn giữ nút
void btnMenuLongPressStart() {
  if (displayScreen == "OTA") {
  }
}
// Hàm callback khi nút đang được giữ
void btnMenuDuringLongPress() {
  //Serial.println("Button is being Long Pressed (BtnMenu)");
}

void btnSetClick() {
  if (displayScreen == "MENU") {
    pIndex = 1;
    loadJsonSettings(); // Hiển thị giá trị thiết lập
    displayScreen = "ScreenCD"; // Chuyển màn hình sau khi xử lý dữ liệu thành công
  } else if (displayScreen == "ScreenCD" && editAllowed){
    if (keyStr == "CD"){
      columnIndex = maxLength-1;
      showEdit(columnIndex);
      displayScreen = "ScreenEdit";
    } else if (keyStr == "CN") {
      if (setupCodeStr == "CN1"){
        trangThaiHoatDong = 201;   //Trạng thái hoạt động 201 là trạng thái TestMode
        testModeStep = 0;
        chayTestMode = true;
        showText("TEST MODE", String("Step " + String(testModeStep)).c_str());
        displayScreen = "screenTestMode";
      } else if (setupCodeStr == "CN2"){
        trangThaiHoatDong = 202;   //Trạng thái hoạt động 202 là trạng thái TEST IO INPUT
        showText("TEST I/O", "TEST I/O INPUT");
        displayScreen = "testIO";
      } else if ((setupCodeStr == "CN3")){
        trangThaiHoatDong = 203;  //Trạng thái hoạt động 203 là trạng thái TEST IO OUTPUT
        testOutputStep = 0;
        displayScreen = "testOutput";
        hienThiTestOutput = true;
      } else if ((setupCodeStr == "CN5")){
        setupOTA();
        displayScreen = "OTA";
        trangThaiHoatDong = 204;  //Trạng thái hoạt động 204 là trạng thái OTA UPDATE+0
      } else {
        columnIndex = maxLength - 1;
        showEdit(columnIndex);
        displayScreen = "ScreenEdit";
      }
    }
  } else if (displayScreen == "ScreenEdit" && editAllowed)  {
    if (keyStr == "CD"){
      if (columnIndex - 1 < 0){
        columnIndex = maxLength-1;
      } else {
        columnIndex --;
      }
      showEdit(columnIndex);
    }
  } else if (displayScreen == "testOutput"){
    daoTinHieuOutput = true;
  }
}

// Hàm callback khi bắt đầu nhấn giữ nút
void btnSetLongPressStart() {
  if (displayScreen == "ScreenEdit"){
    if (keyStr == "CD"){
      jsonDoc["main"]["main" + String(menuIndex)]["children"][setupCodeStr]["configuredValue"] = currentValue;
      isChanged = true;
      log("Đã lưu giá trị:" + String(currentValue) + " vào thẻ " + keyStr + "/" + setupCodeStr);
      loadJsonSettings();
      loadSetup();
      tinhToanCaiDat();
      displayScreen = "ScreenCD";
    } else if (keyStr == "CN"){
      if (setupCodeStr == "CN4" && currentValue == 1){
        reSet();
        showText("RESET","Tắt máy khởi động lại!");
        trangThaiHoatDong = 200;  //Trạng thái hoạt động 200 là reset, không cho phép thao tác nào
        displayScreen = "RESET";
      }
    }
  }
}

// Hàm callback khi nút đang được giữ
void btnSetDuringLongPress() {
  //showSetup("Setup", "OFF", "Dang giu nut");
}

void btnUpClick() {
  if (displayScreen == "MENU") {
    if (menuIndex + 1 > 3) {
      menuIndex = 1;  // Khi chỉ số vượt quá giới hạn, quay lại đầu danh sách
    } else {
      menuIndex++;    // Tăng menuIndex lên 1
    }
    showList(menuIndex);  // Hiển thị danh sách với chỉ số mới
  } else if (displayScreen == "ScreenCD") {
    if (pIndex + 1 > totalChildren) {
      pIndex = 1;
    } else {
      pIndex ++;
    }
    loadJsonSettings(); // Hiển thị giá trị thiết lập
  } else if (displayScreen == "ScreenEdit") {
    if (keyStr == "CD"){
      editValue("addition");
      log("Value:" + valueStr);
    } else if (keyStr == "CN") {
      editValue("addition");
      log("Value:" + valueStr);
    }
  } else if (displayScreen == "testOutput"){
    if (testOutputStep == maxTestOutputStep){
      testOutputStep = 0;
      hienThiTestOutput = true;
    } else {
      testOutputStep ++;
      hienThiTestOutput = true;
    }
  } else if (displayScreen == "screenTestMode"){
    if (testModeStep < maxTestModeStep){
      testModeStep ++;
    } else {
      testModeStep = 0;
    }
    chayTestMode = true;
    showText("TEST MODE", String("Step " + String(testModeStep)).c_str());
  }
}

// Hàm callback khi bắt đầu nhấn giữ nút
void btnUpLongPressStart() { 
  //Serial.println("Button Long Press Started (btnUp)");
}

// Hàm callback khi nút đang được giữ
void btnUpDuringLongPress() {
  //Serial.println("Button is being Long Pressed (btnUp)");
}

void btnDownClick() {
  if (displayScreen == "MENU") {
    if (menuIndex - 1 < 1) {
      menuIndex = 3;  // Khi chỉ số nhỏ hơn giới hạn, quay lại cuối danh sách
    } else {
      menuIndex--;    // Giảm menuIndex đi 1
    }
    showList(menuIndex);  // Hiển thị danh sách với chỉ số mới
  } else if (displayScreen == "ScreenCD"){
    if (pIndex - 1 < 1) {
      pIndex = totalChildren;
    } else {
      pIndex --;
    }
    loadJsonSettings(); // Hiển thị giá trị thiết lập
  } else if (displayScreen == "ScreenEdit"){
    if (keyStr == "CD"){
      editValue("subtraction");
      log("Value:" + valueStr);
    } else if (keyStr == "CN"){
      editValue("subtraction");
      log("Value:" + valueStr);
    }
  } else if (displayScreen == "testOutput"){
    if (testOutputStep == 0){
      testOutputStep = maxTestOutputStep;
      hienThiTestOutput = true;
    } else {
      testOutputStep --;
      hienThiTestOutput = true;
    }
  } else if (displayScreen == "screenTestMode"){
    if (testModeStep > 0){
      /*testModeStep --;
      chayTestMode = true;
      showText("TEST MODE", String("Step " + String(testModeStep)).c_str());*/
    }
  }
}

// Hàm callback khi bắt đầu nhấn giữ nút
void btnDownLongPressStart() {
  //Serial.println("Button Long Press Started (btnDown)");
  veGoc();
}

// Hàm callback khi nút đang được giữ
void btnDownDuringLongPress() {
  //Serial.println("Button is being Long Pressed (btnDown)");
}

//KHAI BÁO CHÂN IO Ở ĐÂY

const int sensorPWM = 36;
const int sensorFoot = 39;
const int SensorIn3 = 34;
const int SensorIn4 = 35;
const int SensorIn5 = 32;
const int SensorIn6 = 33;

const int SensorIn7 = 25;
const int SensorIn8 = 26;

const int SensorIn9 = 23;
const int SensorIn10 = 13;


const int OutCylinferFoot = 4;
const int OutCylinferRelay = 16;
const int Out3 = 17;
const int Out4 = 5;
const int Out5 = 18;
const int Out6 = 19;

const int CHAN_STEP = 27;
const int CHAN_DIR = 14;


//KHAI BÁO THÔNG SỐ TRƯƠNG TRÌNH
/* Ví dụ: int thoiGianNhaDao = 200;
          int soDuMuiDauVao = 10;*/

int SO_BUOC_MOT_VONG_MOTOR = 200; // bước đầy đủ / vòng motor (ví dụ 200)
int MICROSTEP = 16;                   // microstep trên driver (ví dụ 8,16,32)
float DUONG_KINH_TRUC_B_MM = 40.0;    // đường kính trục/puli gắn motor B (mm)

float QUANG_DUONG_MOI_XUNG_CAM_MM = 2.5; // trục A di chuyển 1 xung = 2.5 mm

// Giới hạn tốc độ và gia tốc
// ------------------------
int TOC_DO_MAX_RPM_B = 340; // RPM tối đa cho motor B (giới hạn vật lý)
int TOC_DO_MAX_BUOC_MOI_GIAY = (TOC_DO_MAX_RPM_B / 60.0) * SO_BUOC_MOT_VONG_MOTOR * MICROSTEP;

// Gia tốc (tùy chỉnh để tránh mất bước). Giảm nếu mất bước, tăng nếu cần phản ứng nhanh
float GIA_TOC_BUOC_MOI_GIAY2 = 20000.0; // steps / s^2

// ------------------------
// Debounce cảm biến (microseconds)
// ------------------------
unsigned long DEBOUNCE_CAM_US = 2000; // 2000 µs ~ 2 ms. Tùy chỉnh nếu cần

// ------------------------
// Biến trạng thái (volatile dùng cho ISR)
// ------------------------

bool CHIEU_QUAY_DONG_CO = 0;
int DO_TRE_NGAT_UI = 200;

volatile uint32_t pulseCount = 0;               // chỉ đếm xung, nhỏ gọn trong ISR
volatile unsigned long lastPulseUs = 0;

long mucTieuCu;
volatile int lost;
long getStepperPosition;


//TRƯƠNG TRÌNH NGƯỜI DÙNG LẬP TRÌNH
TaskHandle_t progressTaskHandle = NULL;
AccelStepper stepper(AccelStepper::DRIVER, CHAN_STEP, CHAN_DIR);



void testMode() {
  switch (testModeStep){
  case 0:
    if(chayTestMode){
      maxTestModeStep = 2;
      chayTestMode = false;
    }
    break;

  case 1:
    if (chayTestMode){
      /* code */
    }
    break;
  case 2:
    /* code */
    break;
  default:
    /* code */
    break;
  }
}

void testInput(){
  static bool trangthaiCuoiIO1;
  if (digitalRead(sensorPWM)!= trangthaiCuoiIO1){
    trangthaiCuoiIO1 = digitalRead(sensorPWM);
    showText("IO 36" , String(trangthaiCuoiIO1).c_str());
  }
  static bool trangthaiCuoiIO2;
  if (digitalRead(sensorFoot)!= trangthaiCuoiIO2){
    trangthaiCuoiIO2 = digitalRead(sensorFoot);
    showText("IO 39" , String(trangthaiCuoiIO2).c_str());
  }
  static bool trangthaiCuoiIO3;
  if (digitalRead(SensorIn3)!= trangthaiCuoiIO3){
    trangthaiCuoiIO3 = digitalRead(SensorIn3);
    showText("IO 34" , String(trangthaiCuoiIO3).c_str());
  }
  static bool trangthaiCuoiIO4;
  if (digitalRead(SensorIn4)!= trangthaiCuoiIO4){
    trangthaiCuoiIO4 = digitalRead(SensorIn4);
    showText("IO 35" , String(trangthaiCuoiIO4).c_str());
  }
  static bool trangthaiCuoiIO5;
  if (digitalRead(SensorIn5)!= trangthaiCuoiIO5){
    trangthaiCuoiIO5 = digitalRead(SensorIn5);
    showText("IO 32" , String(trangthaiCuoiIO5).c_str());
  }
  static bool trangthaiCuoiIO6;
  if (digitalRead(SensorIn6)!= trangthaiCuoiIO6){
    trangthaiCuoiIO6 = digitalRead(SensorIn6);
    showText("IO 33" , String(trangthaiCuoiIO6).c_str());
  }
  static bool trangthaiCuoiIO7;
  if (digitalRead(SensorIn7)!= trangthaiCuoiIO7){
    trangthaiCuoiIO7 = digitalRead(SensorIn7);
    showText("IO 25" , String(trangthaiCuoiIO7).c_str());
  }
  static bool trangthaiCuoiIO8;
  if (digitalRead(SensorIn8)!= trangthaiCuoiIO8){
    trangthaiCuoiIO8 = digitalRead(SensorIn8);
    showText("IO 26" , String(trangthaiCuoiIO8).c_str());
  }
  static bool trangthaiCuoiIO9 = true;
  if (digitalRead(SensorIn9)!= trangthaiCuoiIO9){
    trangthaiCuoiIO9 = digitalRead(SensorIn9);
    showText("IO 23" , String(trangthaiCuoiIO9).c_str());
  }
  static bool trangthaiCuoiIO10 = true;
  if (digitalRead(SensorIn10)!= trangthaiCuoiIO10){
    trangthaiCuoiIO10 = digitalRead(SensorIn10);
    showText("IO 13" , String(trangthaiCuoiIO10).c_str());
  }
}

void testOutput(){
  switch (testOutputStep){
    case 0:
      if (hienThiTestOutput){
        maxTestOutputStep = 7;
        bool tinHieuHienTai = digitalRead(OutCylinferFoot);
        showText("IO 4", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(OutCylinferFoot);
        digitalWrite(OutCylinferFoot,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
    case 1:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(OutCylinferRelay);
        showText("IO 16", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(OutCylinferRelay);
        digitalWrite(OutCylinferRelay,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
    case 2:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(Out3);
        showText("IO 17", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(Out3);
        digitalWrite(Out3,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
    case 3:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(Out4);
        showText("IO 5", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(Out4);
        digitalWrite(Out4,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
    case 4:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(Out5);
        showText("IO 18", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(Out5);
        digitalWrite(Out5,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
      case 5:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(Out6);
        showText("IO 19", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(Out6);
        digitalWrite(Out6,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
      case 6:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(CHAN_STEP);
        showText("IO 27", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(CHAN_STEP);
        digitalWrite(CHAN_STEP,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;
      case 7:
      if (hienThiTestOutput){
        bool tinHieuHienTai = digitalRead(CHAN_DIR);
        showText("IO 14", String(tinHieuHienTai).c_str());
        hienThiTestOutput = false;
      } else if (daoTinHieuOutput){
        bool tinHieuHienTai = digitalRead(CHAN_DIR);
        digitalWrite(CHAN_DIR,!tinHieuHienTai);
        hienThiTestOutput = true;
        daoTinHieuOutput = false;
      }
      break;

    default:
      break;
  }
}


void tinhToanCaiDat(){
  /* Ví dụ:
  soXungCanChay = soXungMotor * soVongCuon;
  thoiGianDaoPWM = (1000000*30)/(tocDoQuay*soXungMotor);
  digitalWrite(pinDir,chieuQuayDongCo);
  */
  TOC_DO_MAX_BUOC_MOI_GIAY = (TOC_DO_MAX_RPM_B / 60.0) * SO_BUOC_MOT_VONG_MOTOR * MICROSTEP;
  QUANG_DUONG_MOI_XUNG_CAM_MM = (jsonDoc["main"]["main1"]["children"]["CD1"]["divisor"].as<float>() != 0.0f) ? (jsonDoc["main"]["main1"]["children"]["CD1"]["configuredValue"].as<float>() / jsonDoc["main"]["main1"]["children"]["CD1"]["divisor"].as<float>()) : 0.0f;
  DUONG_KINH_TRUC_B_MM     = (jsonDoc["main"]["main1"]["children"]["CD2"]["divisor"].as<float>() != 0.0f) ? (jsonDoc["main"]["main1"]["children"]["CD2"]["configuredValue"].as<float>() / jsonDoc["main"]["main1"]["children"]["CD2"]["divisor"].as<float>()) : 0.0f;
  digitalWrite(CHAN_DIR,CHIEU_QUAY_DONG_CO);
}

void loadSetup(){
  /* Ví dụ:
  cheDoHoatDong = jsonDoc["main"]["main1"]["children"]["CD1"]["configuredValue"];
  thoiGianNhaDao = jsonDoc["main"]["main1"]["children"]["CD2"]["configuredValue"];
  soDuMuiDauVao = jsonDoc["main"]["main1"]["children"]["CD3"]["configuredValue"];
  soDuMuiDauRa = jsonDoc["main"]["main1"]["children"]["CD4"]["configuredValue"];
  */
  QUANG_DUONG_MOI_XUNG_CAM_MM = (jsonDoc["main"]["main1"]["children"]["CD1"]["divisor"].as<float>() != 0.0f) ? (jsonDoc["main"]["main1"]["children"]["CD1"]["configuredValue"].as<float>() / jsonDoc["main"]["main1"]["children"]["CD1"]["divisor"].as<float>()) : 0.0f;
  DUONG_KINH_TRUC_B_MM     = (jsonDoc["main"]["main1"]["children"]["CD2"]["divisor"].as<float>() != 0.0f) ? (jsonDoc["main"]["main1"]["children"]["CD2"]["configuredValue"].as<float>() / jsonDoc["main"]["main1"]["children"]["CD2"]["divisor"].as<float>()) : 0.0f;
  SO_BUOC_MOT_VONG_MOTOR = jsonDoc["main"]["main1"]["children"]["CD3"]["configuredValue"];
  CHIEU_QUAY_DONG_CO = jsonDoc["main"]["main1"]["children"]["CD4"]["configuredValue"];
  TOC_DO_MAX_RPM_B = jsonDoc["main"]["main1"]["children"]["CD5"]["configuredValue"];
  DO_TRE_NGAT_UI = jsonDoc["main"]["main1"]["children"]["CD6"]["configuredValue"];
  MICROSTEP = jsonDoc["main"]["main1"]["children"]["CD7"]["configuredValue"];
  
  
  // Gia tốc (tùy chỉnh để tránh mất bước). Giảm nếu mất bước, tăng nếu cần phản ứng nhanh
  GIA_TOC_BUOC_MOI_GIAY2 = jsonDoc["main"]["main1"]["children"]["CD8"]["configuredValue"];

  // ------------------------
  // Debounce cảm biến (microseconds)
  // ------------------------
  DEBOUNCE_CAM_US = jsonDoc["main"]["main1"]["children"]["CD9"]["configuredValue"];
  
}

void veGoc(){
  // trangThaiHoatDong = 198 , 199;
  showText("ORIGIN", "Đang về gốc");
  trangThaiHoatDong = 199;
}

void khoiDong(){
  delay(200);
  displayScreen = "index";
  showText("HELLO","Xin Chào");
  mainStep = 0;
  trangThaiHoatDong = 0;
  loadSetup();
  delay(200);
  tinhToanCaiDat();
  delay(100);
  veGoc();
}

float tinhBuocMoiXung() {
  const float PI_f = 3.14159265358979323846;
  float chuViB = PI_f * DUONG_KINH_TRUC_B_MM;
  float vongCan = QUANG_DUONG_MOI_XUNG_CAM_MM / chuViB;
  float buoc = vongCan * SO_BUOC_MOT_VONG_MOTOR * MICROSTEP;
  return buoc;
}

void IRAM_ATTR camISR() {
  unsigned long now = micros();
  if (now - lastPulseUs < DEBOUNCE_CAM_US) return;
  lastPulseUs = now;
  pulseCount++; // đơn giản, nhanh, atomic trên 32-bit
}

void progressTask(void* pvParameters) {
  unsigned long lastPrint = 0;
  const TickType_t idleDelay = 50 / portTICK_PERIOD_MS; // kiểm tra ~20Hz

  for (;;) {
    if (trangThaiHoatDong == 1) {
      unsigned long now = millis();
      if (now - lastPrint > 1000) {
        lastPrint = now;

        // đọc giá trị an toàn: copy vào biến cục bộ
        long pos = getStepperPosition;
        long target = mucTieuCu;
        int lostLocal = lost;

        showProgress(target, pos, lostLocal);
      }  
    }
    vTaskDelay(idleDelay);
  }
  // vTaskDelete(NULL); // không tới đây
}



void mainRun(){
  switch (mainStep){
  case 0:
    /* code */
    break;
  
  case 1:

    break;
  case 2:
    /* code */
    break;
  default:
    break;
  }
}



void setup() {

  Serial.begin(115200);     // Khởi tạo Serial và màn hình

  u8g2.begin();  // Khởi tạo màn hình OLED
  u8g2.enableUTF8Print(); // Kích hoạt hỗ trợ UTF-8

  btnMenu.attachClick(btnMenuClick);
  btnMenu.attachLongPressStart(btnMenuLongPressStart);
  btnMenu.attachDuringLongPress(btnMenuDuringLongPress);

  btnSet.attachClick(btnSetClick);
  btnSet.attachLongPressStart(btnSetLongPressStart);
  btnSet.attachDuringLongPress(btnSetDuringLongPress);

  btnUp.attachClick(btnUpClick);
  btnUp.attachLongPressStart(btnUpLongPressStart);
  btnUp.attachDuringLongPress(btnUpDuringLongPress);

  btnDown.attachClick(btnDownClick);  
  btnDown.attachLongPressStart(btnDownLongPressStart);
  btnDown.attachDuringLongPress(btnDownDuringLongPress);

  btnMenu.setDebounceMs(btnSetDebounceMill);
  btnSet.setDebounceMs(btnSetDebounceMill);
  btnUp.setDebounceMs(btnSetDebounceMill);
  btnDown.setDebounceMs(btnSetDebounceMill);

  btnMenu.setPressMs(btnSetPressMill);
  btnSet.setPressMs(btnSetPressMill);
  btnUp.setPressMs(btnSetPressMill);
  btnDown.setPressMs(btnSetPressMill);

  pinMode(sensorPWM,INPUT);
  pinMode(sensorFoot,INPUT);
  pinMode(SensorIn3,INPUT);
  pinMode(SensorIn4,INPUT);
  pinMode(SensorIn5,INPUT);
  pinMode(SensorIn6,INPUT);

  pinMode(SensorIn7,INPUT);
  pinMode(SensorIn8,INPUT);
  pinMode(SensorIn9,INPUT);
  pinMode(SensorIn10,INPUT);

  pinMode(OutCylinferFoot,OUTPUT);
  pinMode(OutCylinferRelay,OUTPUT);
  pinMode(Out3,OUTPUT);
  pinMode(Out4,OUTPUT);
  pinMode(Out5,OUTPUT);
  pinMode(Out6,OUTPUT);

  pinMode(CHAN_STEP,OUTPUT);
  pinMode(CHAN_DIR,OUTPUT);


  if (!LittleFS.begin()) {
    showSetup("Error", "E003", "LittleFS Mount Failed");
    Serial.println("LittleFS Mount Failed");
    return;
  }

  

  // Kiểm tra xem file config.json có tồn tại không
  if (!LittleFS.exists(configFile)) {
    DeserializationError error = deserializeJson(jsonDoc, jsonString); // Phân tích chuỗi JSON
    if (error) {
        showSetup("Error", "E005", "JsonString Error");
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    showSetup("Error", "E007", "JsonString Load Mode");
    Serial.println("Read Data From JsonString");
    loadSetup();
    Serial.println("File config.json does not exist.");
    return;
  }

  attachInterrupt(digitalPinToInterrupt(sensorPWM), camISR, RISING);
  // cấu hình stepper
  stepper.setMaxSpeed(TOC_DO_MAX_BUOC_MOI_GIAY);
  stepper.setAcceleration(GIA_TOC_BUOC_MOI_GIAY2);
  //stepper.enableOutputs();
// In thông tin ban đầu để kiểm tra
  Serial.println("Khởi động Sync trục A -> Motor B");
  Serial.print("Số bước trên 1 xung cảm biến (approx): ");
  Serial.println(tinhBuocMoiXung(), 6);
  Serial.print("Tốc độ tối đa (steps/s): ");
  Serial.println(TOC_DO_MAX_BUOC_MOI_GIAY);

  readConfigFile();

  Serial.println("Load toàn bộ dữ liệu thành công");
  khoiDong();

  xTaskCreatePinnedToCore(
    progressTask,
    "ProgressTask",
    4096,
    NULL,
    1,
    &progressTaskHandle,
    1
  );

}

void loop() {
  switch (trangThaiHoatDong){
  case 0:
    btnMenu.tick();
    btnSet.tick();
    btnUp.tick();
    btnDown.tick();
    break;
  case 1: {
    btnMenu.tick();
    btnDown.tick();
    mainRun();
    bool trangThaiHienTaiChanVit = digitalRead(sensorFoot);
    static bool trangThaiCuoiChanVit = false;
    if (trangThaiCuoiChanVit != trangThaiHienTaiChanVit){
      if (trangThaiHienTaiChanVit){
        if (Wait(50)){
          digitalWrite(OutCylinferFoot,HIGH);
        }
      } else {
        digitalWrite(OutCylinferFoot,LOW);
      }
      trangThaiCuoiChanVit = trangThaiHienTaiChanVit;
    } else {
      resetWait();
    }

    static uint32_t lastHandledPulse = 0;
    //static long mucTieuCu = 0;

    // Lấy và reset pulseCount theo block (đếm được rồi xử lý hàng loạt)
    uint32_t pulses = 0;
    noInterrupts();
    pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    if (pulses) {
      // chuyển pulses -> bước (làm một lần, giảm số lần gọi moveTo)
      float buocFloat = tinhBuocMoiXung();
      long buocAdd = (long)roundf(buocFloat * (float)pulses);
      mucTieuCu += buocAdd;
      stepper.moveTo(mucTieuCu);
    }
    // gọi run thường xuyên
    stepper.run();
    getStepperPosition = stepper.currentPosition();
    lost = mucTieuCu - getStepperPosition;
    static bool relayON = false;
    static unsigned long lastRelayON = 0;

    if(!lost && WaitMillis(lastRelayON , DO_TRE_NGAT_UI)){
      digitalWrite(OutCylinferRelay,LOW);
      relayON = false;
      lastRelayON = millis();
    } else if (lost){
      if (relayON){
        lastRelayON = millis();
      } else {
        digitalWrite(OutCylinferRelay,HIGH);
        relayON = true;
      }
    }
    
    /*static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
      lastPrint = millis();
      showProgress(mucTieuCu,getStepperPosition,lost);
    }
    break;*/
  }
    
  case 2:
    mainRun();
    break;
  case 198:     // Về Gốc 1
    trangThaiHoatDong = 1;
    break;
  case 199:     // Về Gốc 2
    if(Wait(2000)){
      trangThaiHoatDong = 1;
      showText("READY", "Đang Hoạt Động");
    }
    break;
  case 200:        //ESTOP dừng khẩn cấp
    btnMenu.tick();
    break;
  case 201:         // Func Test Mode
    btnMenu.tick();
    btnUp.tick();
    btnDown.tick();
    testMode();
    break;
  case 202:        // Func Test Input
    btnMenu.tick();
    testInput();
    break;
  case 203:      // Func Test Output
    btnMenu.tick();
    btnSet.tick();
    btnUp.tick();
    btnDown.tick();
    testOutput();
    break;
  case 204:
    btnMenu.tick();
    handleOTA(); // Xử lý OTA khi điều kiện đúng
    break;  
  default:
    break;
  }
}
