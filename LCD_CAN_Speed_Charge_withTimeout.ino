#include <Adafruit_MCP2515.h>
#include <LiquidCrystal_I2C.h>

#define Led A0   // LED pin

// --- CAN settings ---
#define CS_PIN     10
#define CAN_BAUD   500000
#define TARGET_ID  0x10088A5A

Adafruit_MCP2515 mcp(CS_PIN);
uint8_t canData[8];
float Voltage = 0;
float Speed = 0;

// --- LCD settings (16x2 I²C at 0x27, adjust if needed) ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Blink control variables for LCD ---
unsigned long previousMillisBlinkLCD = 0;
const unsigned long blinkIntervalLCD = 400; // ms
bool backlightState = true;

// --- Blink control variables for LED ---
unsigned long previousMillisBlinkLED = 0;
const unsigned long blinkIntervalLED = 400; // ms
bool LedState = false;

// --- CAN timeout tracking ---
unsigned long lastCanTime = 0;
const unsigned long canTimeout = 1000; // 1 second
bool canTimeoutFlag = false;
// --- Function to handle LCD blinking ---
void blinkBacklight() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisBlinkLCD >= blinkIntervalLCD) {
    previousMillisBlinkLCD = currentMillis;
    backlightState = !backlightState;
    if (backlightState) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }
}

// --- Function to handle LED blinking ---
void blinkLED() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisBlinkLED >= blinkIntervalLED) 
  {
    previousMillisBlinkLED = currentMillis;
    LedState = !LedState;
    digitalWrite(Led, LedState ? HIGH : LOW);
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("MCP2515 + LCD CAN Receiver");

  if (!mcp.begin(CAN_BAUD)) {
    Serial.println("Error initializing MCP2515.");
    while (1) delay(10);
  }
  Serial.println("MCP2515 initialized.");

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.print("CAN Ready");
  delay(1000);
  lcd.clear();

  pinMode(Led, OUTPUT);
  digitalWrite(Led, LOW);
}

void loop() {
  int packetSize = mcp.parsePacket();
  
  if (packetSize && mcp.packetId() == TARGET_ID) {
    lastCanTime = millis();   // mark last reception time
    int i = 0;
    while (mcp.available() && i < 8) {
      canData[i++] = mcp.read();
    }
    while (i < 8) {
      canData[i++] = 0x00;
    }

    // Decode values
    uint16_t RawSpeed = (canData[3] << 8) | canData[2];  
    Speed = RawSpeed * 0.03f;   

    uint16_t RawVoltage = (canData[1] << 8) | canData[0];  
    Voltage = RawVoltage * 0.1f;   
  }

  // --- CAN timeout check ---
  if (millis() - lastCanTime > canTimeout) {
    canTimeoutFlag = true;
  } else {
    canTimeoutFlag = false;
  }
  if(canTimeoutFlag){
    lcd.setCursor(0, 0);
    lcd.print("No CAN Data   ");
    }else{
    // --- Update LCD text ---
    /*lcd.setCursor(0, 0);
    lcd.print("Speed:");
    lcd.print(Speed,1);
    lcd.print("Km/h   "); // spaces to overwrite old chars*/
    char line1[17];
    char speedStr[8];
    char voltStr[10];
    
    // Convert floats to strings
    dtostrf(Speed, 5, 2, speedStr);   // width=5, precision=2
    dtostrf(Voltage, 6, 2, voltStr);  // width=6, precision=2
    
    snprintf(line1, sizeof(line1), "Speed:%sKm/h", speedStr);
    lcd.setCursor(0, 0);
    lcd.print(line1);
    
    char line2[17];
    snprintf(line2, sizeof(line2), "Battery:%sV", voltStr);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    }

  // --- Backlight control ---
  if (Speed > 30.0f) {
    blinkBacklight();
  } else {
    lcd.backlight();
  }

  // --- LED control ---
  if (canTimeoutFlag || Voltage <= 386) {
    blinkLED();
  } else {
    digitalWrite(Led, LOW);
  }
}
