/* So do mach:
ESP32 DEV KIT V1
ADC1: GPIOs 32 - 39 : note as *
ADC2: GPIOs 0, 2, 4, 12 - 15 and 25 - 27 : note as # (not use when Wifi started)
Inputonly: note as IN
Note x for used in this project
                                           IN  IN  IN  IN
           #   #   #   #   #   #   *   *   *   *  D39 D36 
  VIN-GND-D13-D12-D14-D27-D26-D25-D33-D32-D35-D34- VN- VP- EN
          x   x   x   x           x   x   x   x   
  ===========================================================
              x               x   x   x   x           x   x
  3V3-GND-D15-D02-D04-RX2-TX2-D05-D18-D19-D21-RXD-TXD-D22-D23
          X    #   #  D16 D17             SDA D03 D01 SCL
                                           |____I2C____|
Khong ton tai cac chan sau:                                          
 D6, D7, D8, D9, D10, D11, D20, D24, D28, D29, D30, D31, D37, D38, D40                   
RFID - RC522
    SDA - D05 - Cam
    SCK - D18 - Vang
    MOSI - D23 - Xanh Lá
    MISO - D19 - Xanh Lam
    IRQ - XXX - Tím
    RST - D2 - Xám
    GDN - GND - Đen
    3V3 - 3V3 - Trắng
Oled screen (I2C)
    SDA - D21 - Vang
    SCL - D22 - Cam
Servo
    In - D13 (Out)
LDR
    Out - D35 (In)
DHT11
    Out - D26 (In)
Led
    GND - GND
    VCC - D12 (Out)
Led DieuHoa
    GND - GND
    VCC - D14 (Out)  
Buzzer
    GND - GND
    VCC - D27 (Out) 
Ir Remote
    Data - D15 (In)
Button A
    In  - R(10k) - GND
    Out - D33 (In)
Button B
    In  - R(10k) - GND
    Out - D32 (In)
Button C
    In  - R(10k) - GND
    Out - D25 (In)

*/
#define BLYNK_TEMPLATE_ID "TMPL6Axj_8H35"
#define BLYNK_TEMPLATE_NAME "Nha thong minh  BTL IoT  N03"
#define BLYNK_AUTH_TOKEN "7R8DrwAGMsVPuD1pmEsjjKqi4aCzOJp3"
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>

#define IR_RECEIVE_PIN 34
#define SEND_PWM_BY_TIMER
#ifdef IR_TIMER_USE_ESP32
hw_timer_t *timer;
void IRTimer(); // defined in IRremote.cpp
#endif

#include <IRremote.hpp>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MFRC522.h>
#include <PlayNote.h>
#include "DHTesp.h"
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

const char* ssid = "Dung";
const char* pass = "khongcomatkhau!";

// const char* ssid = "Viet Dung";
// const char* pass = "MonHauVD121";

// const char* server_name = "http://192.168.0.102:8080/process";
const char* server_name = "https://host-esp32-http-post.onrender.com/process";

unsigned long lastHttpRequestTime = 0;
const unsigned long httpInterval = 1000;

// Định nghĩa pin kết nối
#define SS_PIN 5
#define RST_PIN 2
#define SERVO_PIN 13
#define BUZZER_PIN 27
#define LDR_PIN 35
#define DHT11_PIN 26
#define LED_PIN 12
#define DieuHoa_PIN 14
#define BUTTON_Cua_PIN 33
#define BUTTON_ThemNg_PIN 32
#define BUTTON_Den_PIN 25
// #define READ_IR_ANALOG_FROM_OTHER_ESP32 15

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Servo
Servo myServo;
const int ViTriDongCua = 150;
const int ViTriMoCua = 110;

PlayNote playNote;

DHTesp dhtSensor;

// Biến lưu trạng thái hệ thống
String savedUIDs[] = { " 04 43 0E C5 20 02 89", " 13 29 B2 E2", " 04 63 F8 BA 20 02 89", " 04 13 CC B7 20 02 89", "", "", "", "", "", "" };  // UID thẻ đã lưu
String usernames[] = { "Viet Dung", "Van Kien", "Hai Long", "The Anh", "", "", "", "", "", "" };
int currentPosServo = ViTriDongCua;
int savedUsers = 4;          // số lượng người dùng đã lưu
bool addUserMode = false;    // chế độ thêm người dùng
bool hasPeopleMode = false;  // chế độ con người dùng trong phong
bool isDoorClosed = true;
bool isTurnOffLight = false;
bool isTurnOffDieuHoa = false;
bool pressedButtonThemNguoi = false;

unsigned long lastClearTime = 0;
const unsigned long clearInterval = 8000;
unsigned long lastMessageTime = 0;
const unsigned long displayDuration = 3000;

//Blynk
int prevV4Value = LOW;
BLYNK_WRITE(V4) {
  int p = param.asInt();
  if (p != prevV4Value) {
    prevV4Value = p;
    if (isTurnOffLight) {
      isTurnOffLight = false;
      displayMessage("Bat den...");
      buttonBatDenSound();
    } else {
      isTurnOffLight = true;
      displayMessage("Tat den...");
      buttonTatDenSound();
    }
  }
}

int prevV5Value = LOW;
BLYNK_WRITE(V5) {
  int p = param.asInt();
  if (p != prevV5Value) {
    prevV5Value = p;
    if (isTurnOffDieuHoa) {
      isTurnOffDieuHoa = false;
      displayMessage("Bat dieu hoa...");
      buttonBatDieuHoaSound();
    } else {
      isTurnOffDieuHoa = true;
      displayMessage("Tat dieu hoa...");
      buttonTatDieuHoaSound();
    }
  }
}

int pin_control_den = 0;
int prev_pin_control_den = 0;
void setPinDenAutoFromWeb() {
  if (pin_control_den != prev_pin_control_den) {
    prev_pin_control_den = pin_control_den;
    if (isTurnOffLight) {
      isTurnOffLight = false;
      displayMessage("Bat den...");
      buttonBatDenSound();
    } else {
      isTurnOffLight = true;
      displayMessage("Tat den...");
      buttonTatDenSound();
    }
  }
}
int pin_control_dieuhoa = 0;
int prev_pin_control_dieuhoa = 0;
void setPinDieuHoaAutoFromWeb() {
  if (pin_control_dieuhoa != prev_pin_control_dieuhoa) {
    prev_pin_control_dieuhoa = pin_control_dieuhoa;
    if (isTurnOffDieuHoa) {
      isTurnOffDieuHoa = false;
      displayMessage("Bat dieu hoa...");
      buttonBatDieuHoaSound();
    } else {
      isTurnOffDieuHoa = true;
      displayMessage("Tat dieu hoa...");
      buttonTatDieuHoaSound();
    }
  }
}

BlynkTimer blynkTimer;

int nhietDoGuiDi = 25;
int doAmGuiDi = 65;
void myTimer() {
  // displayMessage("Nhiet do: " + String(nhietDoGuiDi, 1) + "'C\n" + "Do am: " + String(doAmGuiDi, 1) + "%");
  Blynk.virtualWrite(V2, nhietDoGuiDi);
  Blynk.virtualWrite(V3, doAmGuiDi);
  if (hasPeopleMode) {
    Blynk.virtualWrite(V0, "Có người");
  } else {
    Blynk.virtualWrite(V0, "Không có người");
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();         // khởi động SPI bus
  mfrc522.PCD_Init();  // khởi động RFID
  mfrc522.PCD_DumpVersionToSerial();



  playNote.setBuzzerPin(BUZZER_PIN);
  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(DieuHoa_PIN, OUTPUT);
  pinMode(BUTTON_Cua_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ThemNg_PIN, INPUT_PULLUP);
  pinMode(BUTTON_Den_PIN, INPUT_PULLUP);

  dhtSensor.setup(DHT11_PIN, DHTesp::DHT11);

  // Khởi động màn hình OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();

  myServo.attach(SERVO_PIN);
  myServo.write(ViTriDongCua);  // ban đầu servo ở vị trí đóng cửa
  delay(1000);
  pinMode(SERVO_PIN, INPUT);

  IrReceiver.begin(IR_RECEIVE_PIN, false);
    // IrReceiver.begin(IR_RECEIVE_PIN);
  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.printf("at pin %d\n", IR_RECEIVE_PIN);

  WiFi.begin(ssid, pass);
  Serial.print("conecting");
  displayMessage("conecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  Serial.println("");
  Serial.println("Connected to WiFi network with IP Address: " + String(WiFi.localIP()));
  displayMessage("Connected to WiFi network with IP Address: " + String(WiFi.localIP()));
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  blynkTimer.setInterval(1000L, myTimer);
  // Khởi động hoàn tất
  Serial.println("Hệ thống sẵn sàng.");
  displayMessage("He thong san sang.");
}

void loop() {
  Blynk.run();
  blynkTimer.run();
  if (!addUserMode) {
    // mfrc522.PCD_Init();
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String cardUID = getCardUID();
      Serial.printf("Card UID: |%s|\n", cardUID);
      //Serial.println(cardUID);

      // Kiểm tra thẻ có trong hệ thống không
      int userIndex = checkUser(cardUID);
      if (userIndex != -1) {
        // Đúng UID
        handleSuccess(cardUID, userIndex);
      } else {
        // Sai UID
        handleFailure();
      }

      mfrc522.PICC_HaltA();  // Ngắt giao tiếp với thẻ
      // IrReceiver.restartTimer();
    }
  }


  // Kiểm tra nút A để khóa cửa
  if (digitalRead(BUTTON_Cua_PIN) == LOW) {
    if (isDoorClosed) {
      isDoorClosed = false;
      displayMessage("Dang mo cua...");
      buttonCuaMoSound();
      openDoor();
    } else {
      isDoorClosed = true;
      displayMessage("Dang dong cua...");
      buttonCuaDongSound();
      lockDoor();
    }
  }

  // Kiểm tra nút B để vào chế độ thêm người dùng mới
  if (digitalRead(BUTTON_ThemNg_PIN) == LOW) {
    buttonThemNgSound();
    if (addUserMode) {
      addUserMode = false;
    } else {
      addUserMode = true;
    }
    pressedButtonThemNguoi = false;
    addUser();
  }

  if (digitalRead(BUTTON_Den_PIN) == LOW) {
    if (isTurnOffLight) {
      isTurnOffLight = false;
      displayMessage("Bat den...");
      buttonBatDenSound();
    } else {
      isTurnOffLight = true;
      displayMessage("Tat den...");
      buttonTatDenSound();
    }
  }

  // dieuKhienDen();

  if (IrReceiver.decode()) {

        /*
         * Print a summary of received data
         */
        if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
            Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
            // We have an unknown protocol here, print extended info
            IrReceiver.printIRResultRawFormatted(&Serial, true);
            IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
        } else {
            IrReceiver.resume(); // Early enable receiving of the next IR frame
            IrReceiver.printIRResultShort(&Serial);
            IrReceiver.printIRSendUsage(&Serial);
        }
        Serial.println();

        /*
         * Finally, check the received data and perform actions according to the received command
         */
       if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
      }
      else {
        if (IrReceiver.decodedIRData.command == 0x45) {
            Serial.println("On/Off");
            if(isTurnOffLight)
            {
              isTurnOffLight = false;
              displayMessage("Bat den...");
              buttonBatDenSound();
            }
            else
            {
              isTurnOffLight = true;
              displayMessage("Tat den...");
              buttonTatDenSound();
            }
        } else if (IrReceiver.decodedIRData.command == 0x47) {
            Serial.println("Speed");
            if(isTurnOffDieuHoa)
            {
              isTurnOffDieuHoa = false;
              displayMessage("Bat dieu hoa...");
              buttonBatDieuHoaSound();
            }
            else
            {
              isTurnOffDieuHoa = true;
              displayMessage("Tat dieu hoa...");
              buttonTatDieuHoaSound();
            }
        }else if (IrReceiver.decodedIRData.command == 0x7) {
            Serial.println("Timer");
            if(addUserMode)
            {
              addUserMode = false;
            }
            else
            {
              addUserMode = true;
            }
            pressedButtonThemNguoi = false;
            addUser();
        }else if (IrReceiver.decodedIRData.command == 0x9) {
            Serial.println("Swing");
            if(isDoorClosed)
            {
              isDoorClosed = false;
              displayMessage("Dang mo cua...");
              buttonCuaMoSound();
              openDoor();
            }
            else
            {
              isDoorClosed = true;
              displayMessage("Dang dong cua...");
              buttonCuaDongSound();
              lockDoor();
            }
        }
      }
  }
  // int pin34 = analogRead(READ_IR_ANALOG_FROM_OTHER_ESP32);
  // Serial.printf("pin34 %d\n", pin34);
  // if (pin34 == 1) {
  //   Serial.println("On/Off");
  //   if (isTurnOffLight) {
  //     isTurnOffLight = false;
  //     displayMessage("Bat den...");
  //     buttonBatDenSound();
  //   } else {
  //     isTurnOffLight = true;
  //     displayMessage("Tat den...");
  //     buttonTatDenSound();
  //   }
  // } else if (pin34 == 2) {
  //   Serial.println("Speed");
  //   if (isTurnOffDieuHoa) {
  //     isTurnOffDieuHoa = false;
  //     displayMessage("Bat dieu hoa...");
  //     buttonBatDieuHoaSound();
  //   } else {
  //     isTurnOffDieuHoa = true;
  //     displayMessage("Tat dieu hoa...");
  //     buttonTatDieuHoaSound();
  //   }
  // } else if (pin34 == 3) {
  //   Serial.println("Timer");
  //   if (addUserMode) {
  //     addUserMode = false;
  //   } else {
  //     addUserMode = true;
  //   }
  //   pressedButtonThemNguoi = false;
  //   addUser();
  // } else if (pin34 == 4) {
  //   Serial.println("Swing");
  //   if (isDoorClosed) {
  //     isDoorClosed = false;
  //     displayMessage("Dang mo cua...");
  //     buttonCuaMoSound();
  //     openDoor();
  //   } else {
  //     isDoorClosed = true;
  //     displayMessage("Dang dong cua...");
  //     buttonCuaDongSound();
  //     lockDoor();
  //   }
  // }

  dieuKhienDen();
  dieuKhienDieuHoa();
  if (millis() - lastMessageTime >= displayDuration && lastMessageTime != 0) {
    displayMessage("He thong san sang\n\nNhiet do: " + String(nhietDoGuiDi) + "'C\n" + "Do am: " + String(doAmGuiDi) + "%");
    lastClearTime = millis();  // Reset the timer
  }

  httpSync();
}




String getCardUID() {
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  return uidString;
}

int checkUser(String uid) {
  for (int i = 0; i < savedUsers; i++) {
    Serial.printf("Nguoi dung da luu UID: |%s|\n", savedUIDs[i]);
    if (savedUIDs[i] == uid) {

      return i;  // trả về index của người dùng
    }
  }
  return -1;  // không tìm thấy người dùng
}

void handleSuccess(String uid, int userIndex) {
  displayMessage("Doc the thanh cong.");
  readDoneSound();
  delay(500);

  // Hiển thị thông tin người dùng


  if (hasPeopleMode) {
    if (isDoorClosed) {
      displayMessage("Xin chao " + usernames[userIndex] + ".\nXin moi vao.");
      openDoor();
      hasPeopleMode = true;
      isDoorClosed = false;
    } else {
      displayMessage("Tam biet " + usernames[userIndex] + ".\nHen gap lai.");
      lockDoor();
      hasPeopleMode = false;
      isDoorClosed = false;
    }
  } else {
    displayMessage("Xin chao " + usernames[userIndex] + ".\nXin moi vao.");
    openDoor();
    hasPeopleMode = true;
    isDoorClosed = false;
  }
}

void handleFailure() {
  displayMessage("The khong hop le.");
  errorSound();
}

void openDoor() {
  // Mở khóa cửa
  Serial.println("Mo khoa cua");
  smoothMove(ViTriDongCua, ViTriMoCua, 15);
  //delay(2500);
  openSound();
  displayMessage("Cua da duoc mo!.");
  delay(100);
}

void lockDoor() {
  Serial.println("Dong cua");
  smoothMove(ViTriMoCua, ViTriDongCua, 15);
  //delay(2500); // khóa cửa
  closeSound();
  displayMessage("Cua da duoc khoa.");
}


bool readButtonThemNguoi() {
  if (digitalRead(BUTTON_ThemNg_PIN) == LOW) {
    Serial.println("Da bam huy them nguoi!");
    buttonHuyThemNgSound();
    pressedButtonThemNguoi = true;
    addUserMode = false;
    return true;
  }
  return false;
}

void addUser() {
  if (addUserMode) {
    displayMessage("Che do them nguoi dung.");
    while (!readButtonThemNguoi()) {
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        // if(!pressedButtonThemNguoi)
        {
          String newUID = getCardUID();
          int checkedUser = checkUser(newUID);
          if (checkedUser == -1) {
            savedUIDs[savedUsers] = newUID;
            usernames[savedUsers] = "Nguoi dung " + String(char(savedUsers - 4 + 'A'));
            savedUsers++;

            Serial.println("Da them: " + usernames[savedUsers - 1]);
            displayMessage("Da them: " + usernames[savedUsers - 1]);
            thanhCongSound();
            delay(500);
            displayMessage("Thoat che do them nguoi dung.");
            addUserMode = false;
            break;
          } else {
            errorSound();
            Serial.println("Trung the: " + usernames[checkedUser] + "\nXin moi thu lai!");
            displayMessage("Trung the: " + usernames[checkedUser] + "\nXin moi thu lai!");
            // addUserMode = false;
            // break;
          }
        }
      }
    }
    if (pressedButtonThemNguoi) {
      Serial.println("Huy che do them nguoi!");
      displayMessage("Huy che do them nguoi!");
      addUserMode = false;
    }
    mfrc522.PICC_HaltA();
  }
}

int prevLightState = 0;
void dieuKhienDen() {

  if (hasPeopleMode && !isTurnOffLight) {
    // Kiểm tra độ sáng
    int lightLevel = digitalRead(LDR_PIN);
    // Serial.printf("Cam bien anh sang: %d\n", lightLevel);
    if (lightLevel == 1) {  // trời tối
      if (prevLightState != lightLevel) {
        prevLightState = lightLevel;
        digitalWrite(LED_PIN, HIGH);  // bật đèn
        displayMessage("Den phong bat.");
      }
    } else {
      if (prevLightState != lightLevel) {
        digitalWrite(LED_PIN, LOW);
        displayMessage("Den phong tat.");
      }
    }

    prevLightState = lightLevel;
  } else {
    prevLightState = 0;
    digitalWrite(LED_PIN, LOW);
  }
}

int prevDieuHoaState = 0;
void dieuKhienDieuHoa() {
  int currDieuHoaState = 0;
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float nhietDo = data.temperature;
  float doAm = data.humidity;
  nhietDoGuiDi = nhietDo;
  doAmGuiDi = doAm;
  if (hasPeopleMode && !isTurnOffDieuHoa) {
    // Serial.println("Nhiet do: " + String(nhietDo, 1) + "°C");
    // Serial.println("Do am: " + String(doAm, 1) + "%");
    // Serial.printf("Cam bien anh sang: %d\n", lightLevel);
    if (nhietDo > 30 || doAm > 90) {
      currDieuHoaState = 1;
    } else {
      currDieuHoaState = 0;
    }
    if (currDieuHoaState == 1) {  // trời nong
      if (prevDieuHoaState != currDieuHoaState) {
        displayMessage("Nhiet do: " + String(nhietDo, 1) + "'C\n" + "Do am: " + String(doAm, 1) + "%");
        prevDieuHoaState = currDieuHoaState;
        digitalWrite(DieuHoa_PIN, HIGH);  // bật dieu hoa
        displayMessage("Dieu hoa bat.");
      }
    } else {
      if (prevDieuHoaState != currDieuHoaState) {
        displayMessage("Nhiet do: " + String(nhietDo, 1) + "'C\n" + "Do am: " + String(doAm, 1) + "%");
        digitalWrite(DieuHoa_PIN, LOW);
        displayMessage("Dieu hoa tat.");
      }
    }

    prevDieuHoaState = currDieuHoaState;
  } else {
    prevDieuHoaState = 0;
    digitalWrite(DieuHoa_PIN, LOW);
  }
}

void displayMessage(String message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
  lastMessageTime = millis();
  // Serial.println(message);
}


void httpSync() {
  unsigned long currentMillis = millis();

  // Check if 1 second has passed since the last HTTP request
  if (currentMillis - lastHttpRequestTime >= httpInterval) {
    lastHttpRequestTime = currentMillis;  // Update the last request time

    // Send HTTP request
    // Serial.println("Sending HTTP request...");
    HTTPClient http;
    http.begin(server_name);
    http.addHeader("Content-type", "text/plain");

    String httpRequestData = "nhietdo=" + String(nhietDoGuiDi) + ", doAm=" + String(doAmGuiDi) + ", coNguoi=" + String(hasPeopleMode ? "true" : "false");
    int httpResponseCode = http.POST(httpRequestData);

    // if (httpResponseCode > 0) {
    //   Serial.print("Good HTTP response code: ");
    // } else {
    //   Serial.print("Bad HTTP response code: ");
    // }
    // Serial.println(httpResponseCode);

    String response = http.getString();
    // Serial.print("Response: ");
    // Serial.println(response);
    updateToPin(response);
    http.end();  // Close connection
  }
}

void updateToPin(String s) {
  s.replace(",", " ");
  s.replace("=", " ");
  int index = 0;  // Variable to track parsing position
  while (index < s.length()) {
    // Extract the next token
    int nextSpace = s.indexOf(' ', index);
    if (nextSpace == -1) nextSpace = s.length();
    String token = s.substring(index, nextSpace);
    index = nextSpace + 1;

    // Check the token and assign the corresponding value
    if (token == "den") {
      pin_control_den = s.substring(index, s.indexOf(' ', index)).toInt();
    } else if (token == "dieuhoa") {
      pin_control_dieuhoa = s.substring(index, s.indexOf(' ', index)).toInt();
    }
  }
  setPinDenAutoFromWeb();
  setPinDieuHoaAutoFromWeb();
}


void openSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes openSong[] = { { "1*", 1 }, { "3*", 1 }, { "5*", 1 }, { "1**", 1 } };
  int lenOpenSong = sizeof(openSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(openSong, lenOpenSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void closeSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes closeSong[] = { { "1**", 1 }, { "5*", 1 }, { "3*", 1 }, { "1*", 1 } };
  int lenCloseSong = sizeof(closeSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(closeSong, lenCloseSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void errorSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes errorSong[] = { { "1*", 0.5 }, { "3*", 0.5 }, { "1*", 1 } };
  int lenErrorSong = sizeof(errorSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(errorSong, lenErrorSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void readDoneSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes readDoneSong[] = { { "1*", 0.5 }, { "2*", 0.5 }, { "5*", 0.5 }, { "1*", 2 } };
  int lenReadDoneSong = sizeof(readDoneSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(readDoneSong, lenReadDoneSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}
void buttonCuaMoSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonCuaMoSong[] = { { "1*", 0.5 }, { "2*", 0.5 }, { "3*", 1 } };
  int lenbuttonCuaMoSong = sizeof(buttonCuaMoSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonCuaMoSong, lenbuttonCuaMoSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}
void buttonCuaDongSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonCuaDongSong[] = { { "3*", 0.5 }, { "2*", 0.5 }, { "1*", 1 } };
  int lenbuttonCuaDongSong = sizeof(buttonCuaDongSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonCuaDongSong, lenbuttonCuaDongSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}
void buttonThemNgSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonThemNguoiSong[] = { { "3*", 0.5 }, { "5*", 1 } };
  int lenbuttonThemNguoiSong = sizeof(buttonThemNguoiSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonThemNguoiSong, lenbuttonThemNguoiSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}
void buttonHuyThemNgSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonHuyThemSong[] = { { "5*", 0.5 }, { "1*", 1 } };
  int lenbuttonHuyThemSong = sizeof(buttonHuyThemSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonHuyThemSong, lenbuttonHuyThemSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void buttonBatDenSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonBatDenSong[] = { { "1*", 0.5 }, { "5*", 1 } };
  int lenbuttonBatDenSong = sizeof(buttonBatDenSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonBatDenSong, lenbuttonBatDenSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void buttonTatDenSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonTatDenSong[] = { { "5*", 0.5 }, { "1*", 1 } };
  int lenbuttonTatDenSong = sizeof(buttonTatDenSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonTatDenSong, lenbuttonTatDenSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void buttonBatDieuHoaSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonBatDieuHoaSong[] = { { "1*", 0.5 }, { "3*", 1 } };
  int lenbuttonBatDieuHoaSong = sizeof(buttonBatDieuHoaSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonBatDieuHoaSong, lenbuttonBatDieuHoaSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void buttonTatDieuHoaSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes buttonTatDieuHoaSong[] = { { "3*", 0.5 }, { "1*", 1 } };
  int lenbuttonTatDieuHoaSong = sizeof(buttonTatDieuHoaSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(buttonTatDieuHoaSong, lenbuttonTatDieuHoaSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void thanhCongSound() {
  //myServo.attach(SERVO_PIN);
  struct PlayNote::notes thanhCongSong[] = { { "1*", 1 }, { "2*", 1 }, { "5*", 2 } };
  int lenthanhCongSong = sizeof(thanhCongSong) / sizeof(struct PlayNote::notes);
  playNote.playSong(thanhCongSong, lenthanhCongSong);
  //myServo.attach(SERVO_PIN);
  // IrReceiver.restartTimer();
}

void smoothMove(int startPos, int endPos, int delayTime) {
  Serial.printf("currentPosServo: %d, start: %d, end: %d\n", currentPosServo, startPos, endPos);
  String mes1 = "currentPosServo: " + String(currentPosServo) + ", start: " + String(startPos) + ", end: " + String(endPos) + "\n";
  // displayMessage(mes1);
  if (currentPosServo == startPos) {
    // displayMessage(mes1);
    myServo.attach(SERVO_PIN);
    if (startPos < endPos) {
      for (int pos = startPos; pos <= endPos; pos++) {
        // Serial.printf("Pos: %d\n", pos);
        myServo.write(pos);
        delay(delayTime);
      }
    } else {
      for (int pos = startPos; pos >= endPos; pos--) {
        // Serial.printf("Pos: %d\n", pos);
        myServo.write(pos);
        delay(delayTime);
      }
    }
    delay(1000);
    pinMode(SERVO_PIN, INPUT);
    currentPosServo = endPos;
  }
  // IrReceiver.restartTimer();
}


/*
Hãy viết một đoạn code Arduino có sử dụng các linh kiện sau:
1. RFID RC522 với thư viện <MFRC522.h>
2. Động cơ MG 996R với thư viện <ESP32Servo.h>
3. Một dàn đèn led mô phỏng đèn trong nhà "oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)"
4. Một màn hình oled để hiển thị sử dụng thư viện <Adafruit_GFX.h>, <Adafruit_SSD1306.h>, <Wire.h>
5. Một cảm biến ánh sáng LDR
6. Một buzzer passive để kêu thông báo
7. Hai nút bấm
Chức năng:
Hệ thống sẽ sử dụng RFID RC522 để đọc các thẻ NFC với các UID đã lưu trong hệ thống
ví dụ :"Card UID: 04 43 0E C5 20 02 89; PICC type: MIFARE Ultralight or Ultralight C"
"Card UID: 13 29 B2 E2; PICC type: MIFARE 1KB"
Khi thẻ NFC đọc thành công buzzer sẽ kêu 1 tiếng bíp 440hz. Và màn hình hiện Đọc Thẻ thành công.
Nếu đúng UID thì động cơ sẽ xoay 90 độ để mở khóa cửa. Và màn hình led thông báo Tên người dùng và xin mời vào.
  - Sau đó đèn trong phòng sẽ bật sáng nếu cảm biến ánh sáng LDR nhận thấy tối
  - Sau đó Khi ấn nút A sẽ xoay -90 độ để khóa cửa. Và buzzer sẽ kêu 1 tiếng bíp.

Nếu sai UID thì buzzer kêu 2 tiếng bíp và Màn hình hiện thông báo không có thông tin người dùng.

Nút B khi ấn sẽ đưa hệ thống vào chế độ thêm người dùng mới (Lưu UID thẻ vào hệ thống với tên người dùng đặt theo dạng "Nguoi dung A", "Nguoi dung B",...)
*/
