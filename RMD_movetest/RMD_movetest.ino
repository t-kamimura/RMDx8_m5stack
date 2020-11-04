#include <M5Stack.h>
#include <mcp_can_m5.h>

#define MOTOR_ADDRESS 0x141 //0x140 + ID(1~32)
#define BAUDRATE 115200     //シリアル通信がボトルネックにならないよう，速めに設定しておく
#define LOOPTIME 5
#define TEXTSIZE 1

int pos = 300;
unsigned char len = 0;
unsigned char cmd_buf[8], reply_buf[8];
unsigned long timer[3];

/* variable for CAN */
byte data[8] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
long unsigned int rxId;
unsigned char rxBuf[8];
char msgString[128];

#define CAN0_INT 15  // Set INT to pin 2
MCP_CAN_M5 CAN0(12); // Set CS to pin 10

void init_can();
void write_can();
void read_can();

void setup()
{
  M5.begin();
  M5.Power.begin();
  Serial.begin(BAUDRATE);
  // Serial2.begin(BAUDRATE, SERIAL_8N1, 16, 17);
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setTextSize(TEXTSIZE);

  init_can();
  Serial.println("Test CAN...");
  timer[0] = millis();
}

void loop()
{
  if (M5.BtnA.wasPressed())
  {
    M5.Lcd.clear();
    M5.Lcd.printf("CAN Test B!\n");
    init_can();
  }
  write_can();
  read_can();
  M5.update();
  timer[1] = millis();
  Serial.print(timer[1] - timer[0]);
  M5.Lcd.printf("%d: ", timer[1] - timer[0]);
  delay(100);
}

void init_can()
{
  M5.Lcd.setTextSize(TEXTSIZE);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("CAN Test B!\n");

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK)
  {
    Serial.println("MCP2515 Initialized Successfully!");
  }
  else
  {
    Serial.println("Error Initializing MCP2515...");
  }

  CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
}

void write_can()
{
  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, data);
  if (sndStat == CAN_OK)
  {
    Serial.println("Message Sent Successfully!");
    M5.Lcd.printf("Message Sent Successfully!\n");
  }
  else
  {
    Serial.println("Error Sending Message...");
    M5.Lcd.printf("Error Sending Message...\n");
  }
  delay(200); // send data per 200ms
}

void read_can()
{
  if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
    {
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    }
    else
    {
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
    }

    Serial.print(msgString);
    M5.Lcd.printf(msgString);

    if ((rxId & 0x40000000) == 0x40000000)
    { // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    }
    else
    {
      for (byte i = 0; i < len; i++)
      {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
        M5.Lcd.printf(msgString);
      }
    }

    M5.Lcd.printf("\n");
    Serial.println();
  }
}