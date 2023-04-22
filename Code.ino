#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

//OUTPUT PINS VARIABLES
#define gLed 15
#define rLed 16
#define buzz 17

//RFID VARIABLES
#define SS_PIN 10
#define RST_PIN 14
MFRC522 mfrc522(SS_PIN, RST_PIN);

//LCD VARIABLES
LiquidCrystal_I2C lcd(0x27, 16, 2);
bool backlightON;
byte hash[8]{
  B10001,
  B10001,
  B01010,
  B11111,
  B01010,
  B11111,
  B01010,
  B00000
};
byte downArrow[8]{
  B00000,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000
};
byte A[8]{
  B10001,
  B10101,
  B01010,
  B01010,
  B01110,
  B01010,
  B01010,
  B00000
};
byte star[8]{
  B10001,
  B10001,
  B00100,
  B10101,
  B01110,
  B10101,
  B00100,
  B00000
};
byte checkMark[8]{
  B00000,
  B00000,
  B00001,
  B00011,
  B10110,
  B11100,
  B01000,
  B00000
};

//KEYPAD VARIABLES
const byte ROWS = 4; 
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//MENU VARIABLES
byte listCounter;
byte menuList[10];
byte currentOption;

//PIN/CARD VARIABLES
byte PIN[10];
String inputPIN = "";
char customKey;

byte cardCode[10];
bool replacingCard;

//ACTIVE TIMER
unsigned long readTime;
int timer = 20 * 1000;


//VERIFY PIN/Card
void checkCard(){
  bool isGood = true;
  byte i = 0;
  while (i<cardCode[0] && isGood){
    if (cardCode[i+1] == mfrc522.uid.uidByte[i]){
      i++;
    }else {
      isGood = false;
    }
  }
  if (isGood){
    accessGranted(true);
  }else {
    accessGranted(false);
  }
}
bool checkPin(){
  bool isGood = true;
  byte i = 0;
  if (inputPIN.length() == PIN[0]){
    while (i<PIN[0] && isGood){
      if (PIN[i+1] == (byte)inputPIN.charAt(i)){
        i++;
      }else {
        isGood = false;
      }
    }
  }else {
    isGood = false;
  }
  return isGood;
}

//REPLACE PIN/Card
void replacePIN(){
  PIN[0] = inputPIN.length();
  EEPROM.update(EEPROM.read(0)+1, PIN[0]);
  for (byte i = 0; i<PIN[0]; i++){
    PIN[i+1] = (byte)inputPIN.charAt(i);
    EEPROM.update(EEPROM.read(0)+2+i, PIN[i+1]);
  }
  Serial.println(EEPROM.read(EEPROM.read(0)+1));
}
void replaceCard(){
  cardCode[0] = mfrc522.uid.size;
  EEPROM.update(0, cardCode[0]);
  Serial.println(EEPROM.read(0));
  for (byte i = 0; i<mfrc522.uid.size; i++){
    cardCode[i+1] = mfrc522.uid.uidByte[i];
    EEPROM.update(i+1, cardCode[i+1]);
    Serial.println(cardCode[i+1]);
    Serial.println(EEPROM.read(i+1));
  }
}

//ACCESS
void accessGranted(bool isGranted){
  lcdLight(true);
  if (isGranted) {
    digitalWrite(gLed, HIGH);
    lcd.clear();
    lcd.print("      OPEN      ");
    tone(buzz, 690, 200);
    delay(250);
    tone(buzz, 690, 200);
    delay(250);
    tone(buzz, 690, 1500);
    delay(1500);
    reset();
  } else {
    digitalWrite(rLed, HIGH);
    tone(buzz, 200, 1000);
    lcd.clear();
    lcd.print("   TRY  AGAIN   ");
    customKey = '\0';
    inputPIN = "";
    delay(2000);
    digitalWrite(rLed, LOW);
    menuManager(menuList[listCounter]);
  } 
}

void reset(){
  digitalWrite(gLed, LOW);
  digitalWrite(rLed, LOW);
  customKey = '\0';
  inputPIN = "";
  listCounter = 0;
  menuList[listCounter] = 0;
  currentOption = 1;
  menuManager(0);
}

//MENU
void homeMenu(){ // 0
  
    lcd.clear();
    switch(currentOption){
      case 1:
        lcd.setCursor(0, 0); lcd.print("1.Enter PIN");
        break;
      case 2:
        lcd.setCursor(0, 0); lcd.print("2.Settings");
        break;
      default:
        currentOption = 1;
        homeMenu();
        break;
    }
    lcd.setCursor(0, 1); lcd.write(byte(0));
    lcd.setCursor(1, 1); lcd.print("-");
    lcd.setCursor(2, 1); lcd.write(byte(3));
    
    lcd.setCursor(4, 1); lcd.write(byte(1));
    lcd.setCursor(5, 1); lcd.print("-");
    lcd.setCursor(6, 1); lcd.write(byte(4));

    switch(customKey){
      case 'A':
        customKey = '\0';
        ++currentOption;
        homeMenu();
        break;
      case '*':
        customKey = '\0';
        switch(currentOption){
          case 1:
            ++listCounter;
            menuList[listCounter] = 1;
            break;
          case 2:
            ++listCounter;
            menuList[listCounter] = 2;
            break;
        }
        currentOption = 1;
        menuManager(menuList[listCounter]);
        break;
      default:
        break;
    }
}
    

void pinMenu(){ // 1
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("PIN:");
    
  lcd.setCursor(4, 1); lcd.write(byte(1));
  lcd.setCursor(5, 1); lcd.print("-");
  lcd.setCursor(6, 1); lcd.write(byte(4));
    
  lcd.setCursor(8, 1); lcd.write(byte(2));
  lcd.setCursor(9, 1); lcd.print("-CANCEL");
  
  lcd.setCursor(4, 0);
  
  switch(customKey){
    case '*':
      customKey = '\0';
      if(checkPin()){
        inputPIN = ""; 
        switch(menuList[listCounter-1]){
          case 0:
            accessGranted(true);
            break;
          case 2:
            switch(currentOption){
              case 1:
                menuList[listCounter] = 3;
                break;
              case 2:
                menuList[listCounter] = 4;
                break;
            }
            menuManager(menuList[listCounter]);
            break; 
        }
      }else {
        accessGranted(false);
      }
      break;
    case '#':
      customKey = '\0';
      inputPIN = "";
      --listCounter;
      menuManager(menuList[listCounter]);
      break;
    default:
      if (inputPIN.length() < 8){
        inputPIN.concat(String(customKey));
      } else{
      }
      lcd.print(inputPIN);
      break;
  }
}
void settingsMenu(){ // 2
    lcd.clear();
    switch(currentOption){
      case 1:
        lcd.setCursor(0, 0); lcd.print("1.Set New PIN");
        break;
      case 2:
        lcd.setCursor(0, 0); lcd.print("2.Set New Card");
        break;
      default:
        currentOption = 1;
        settingsMenu();
        break;
    }
    lcd.setCursor(0, 1); lcd.write(byte(0));
    lcd.setCursor(1, 1); lcd.print("-");
    lcd.setCursor(2, 1); lcd.write(byte(3));
    
    lcd.setCursor(4, 1); lcd.write(byte(1));
    lcd.setCursor(5, 1); lcd.print("-");
    lcd.setCursor(6, 1); lcd.write(byte(4));

    lcd.setCursor(8, 1); lcd.write(byte(2));
    lcd.setCursor(9, 1); lcd.print("-BACK");

    switch(customKey){
      case 'A':
        customKey = '\0';
        ++currentOption;
        settingsMenu();
        break;
      case '*':
        customKey = '\0';
        ++listCounter;
        menuList[listCounter] = 1;
        menuManager(menuList[listCounter]);
        break;
      case '#':
        customKey = '\0';
        --listCounter;
        currentOption = 1;
        menuManager(menuList[listCounter]);
        break;
      default:
        break;
    }
}

void setPINMenu(){ // 3
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("New PIN:");
  
  lcd.setCursor(4, 1); lcd.write(byte(1));
  lcd.setCursor(5, 1); lcd.print("-");
  lcd.setCursor(6, 1); lcd.write(byte(4));
  
  lcd.setCursor(8, 1); lcd.write(byte(2));
  lcd.setCursor(9, 1); lcd.print("-CANCEL");
  
  lcd.setCursor(8, 0);
  
  switch(customKey){
    case '*': 
      customKey = '\0';
      if(inputPIN != ""){
        replacePIN();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("    NEW  PIN    ");
        lcd.setCursor(0, 1);
        lcd.print("   REGISTERED   ");
        delay(2000);
        reset();
      }
      break;
    case '#':
      customKey = '\0';
      --listCounter;
      menuManager(menuList[listCounter]);
      break;
    default:
      if (inputPIN.length() <= 8){
        inputPIN.concat(String(customKey));
        lcd.print(inputPIN);
      }
      break;
  }
}

void setCardMenu(){ //4
  bool cardRead = false;
  String keyCheck = "";

  replacingCard = true;
  
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Scan New Card...");
  
  lcd.setCursor(8, 1); lcd.write(byte(2));
  lcd.setCursor(9, 1); lcd.print("-CANCEL");
  
  lcd.setCursor(8, 0);
  keyCheck.concat(customKey);

  while (keyCheck == "" && !cardRead){
    if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){
      replaceCard();
      cardRead = true;
      replacingCard = false;
      lcdLight(true);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    NEW CARD    ");
      lcd.setCursor(0, 1);
      lcd.print("   REGISTERED   ");
      tone(buzz, 500, 100);
      delay(2000);
      reset();
    }
  }
  switch(customKey){
    case '#':
      customKey = '\0';
      replacingCard = false;
      --listCounter;
      menuManager(menuList[listCounter]);
      break;
  }
}

//MANAGER
void menuManager(byte menuNo){
  switch (menuNo){
    case 0:
      homeMenu();
      break;
    case 1:
      pinMenu();
      break;
    case 2:
      settingsMenu();
      break;
    case 3:
      setPINMenu();
      break;
    case 4:
      setCardMenu();
  }
}

//LIGHT TURN ON/OFF
void lcdLight(bool high){
  if (high){
    readTime = millis();
    if (!backlightON){
      lcd.backlight();
      backlightON = true;
    }
  }else {
    lcd.noBacklight();
    backlightON = false;
  }
}

// S E T U P
void setup() {
  Serial.begin(9600);

  //LCD SETUP
  lcd.begin(16,2);
  lcd.noBacklight();
  backlightON = false;  
  lcd.clear();
  lcd.createChar(0, A);
  lcd.createChar(1, star);
  lcd.createChar(2, hash);
  lcd.createChar(3, downArrow);
  lcd.createChar(4, checkMark);

  //CARD, PIN SETUP
  for (byte i = 0; i <= EEPROM.read(0); i++){
    cardCode[i] = EEPROM.read(i);  
  }
  for (byte i = 0; i <= EEPROM.read(EEPROM.read(0)+1); i++){  
    PIN[i] = EEPROM.read(EEPROM.read(0)+1+i);
    PIN[i] = 0;
    Serial.print(PIN[i]);
    Serial.print(" ");
  }

  //MENU SETUP
  listCounter = 0;
  menuList[listCounter] = 0;
  currentOption = 1;
  menuManager(0);

  //LED/PIEZO SETUP
  pinMode(gLed, OUTPUT);
  pinMode(rLed, OUTPUT);
  pinMode(buzz, OUTPUT);

  //CARD READER SETUP 
  SPI.begin();    
  mfrc522.PCD_Init();
}

// L O O P
void loop() {
  if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() && !replacingCard){
    checkCard();
  }else {
    customKey = '\0';
    customKey = customKeypad.getKey();
    if (customKey){
      lcdLight(true);
      menuManager(menuList[listCounter]);
    }else if (millis() >= (readTime+timer) && backlightON) {
      lcdLight(false);
      reset();
    }
  }
}
