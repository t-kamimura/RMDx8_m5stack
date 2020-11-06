/* 周波数応答確認用*/
#include <M5Stack.h>
#include <mcp_can_m5.h>
#include <SPI.h>
#include <RMDx8Arduino.h>

/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define SERIAL serialUSB
#else
#define SERIAL Serial
#endif

#define BAUDRATE 115200 //シリアル通信がボトルネックにならないよう，速めに設定しておく
#define LOOPTIME 10     //[ms]
#define ENDTIME 10000   //[ms]
#define TEXTSIZE 2

unsigned char len = 0;
unsigned char cmd_buf[8], reply_buf[8];
long unsigned int rxId;
unsigned char rxBuf[8];
char msgString[128];
unsigned long timer[3];
byte pos_byte[4];

const uint16_t MOTOR_ADDRESS = 0x141; //0x140 + ID(1~32)
const int SPI_CS_PIN = 12;

#define CAN0_INT 15          // Set INT to pin 2
MCP_CAN_M5 CAN0(SPI_CS_PIN); // Set CS to pin 10

int A = 30 * 6 * 100; // [servoHornDeg]*[gearRatio]*[encorderResolution]
double f = 1.0;       //[Hz]
double omega = 2 * 3.14 * f;

void init_can();
void write_can();
void read_can();

void setup()
{
  M5.begin();
  M5.Power.begin();
  Serial.begin(BAUDRATE);
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setTextSize(TEXTSIZE);

  init_can();
  Serial.println("Test CAN...\n");
  timer[0] = millis();
}

void loop()
{
  while (millis() - timer[0] < ENDTIME)
  {
    timer[1] = millis();
    int32_t tgt_pos = A * sin(omega * (timer[1] - timer[0]) * 0.001);

    // M5.Lcd.clear();

    // position control command
    cmd_buf[0] = 0xA4;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x10;
    cmd_buf[4] = tgt_pos & 0xFF;
    cmd_buf[5] = (tgt_pos >> 8) & 0xFF;
    cmd_buf[6] = (tgt_pos >> 16) & 0xFF;
    cmd_buf[7] = (tgt_pos >> 24) & 0xFF;
    write_can();
    // read_can();

    // print
    SERIAL.print(timer[1] - timer[0]);
    SERIAL.print(",");
    SERIAL.print(tgt_pos);

    // read multi turn angle
    cmd_buf[0] = 0x92;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;
    write_can();
    read_can();

    M5.update();

    int64_t present_pos = 0;
    // for (int i = 0; i < 7; i++)
    // {
    //   present_pos = present_pos + (reply_buf[i+1] << (8*i));
    // }
    present_pos = reply_buf[1] + (reply_buf[2] << 8) + (reply_buf[3] << 16) + (reply_buf[4] << 24) + (reply_buf[5] << 32) + (reply_buf[6] << 48);
    present_pos = present_pos * 0.01 / 6;

    // byteならこっち
    // serialDisp(rmd.reply_buf, rmd.pos_buf);

    // 10進数ならこっち
    // SERIAL.print(",");
    // SERIAL.print(present_pos);
    int tgt_angle = tgt_pos * 0.01 / 6;
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.printf("TIM:            ");
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.printf("TIM: %d", timer[1] - timer[0]);
    M5.Lcd.setCursor(0, 70);
    M5.Lcd.printf("TGT:            ");
    M5.Lcd.setCursor(0, 70);
    M5.Lcd.printf("TGT: %d", tgt_angle);
    M5.Lcd.setCursor(0, 100);
    M5.Lcd.printf("POS:            ");
    M5.Lcd.setCursor(0, 100);
    M5.Lcd.printf("POS: %d", present_pos);

    // rmd.serialWriteTerminator();

    timer[2] = millis() - timer[1];
    if (timer[2] < LOOPTIME)
    {
      delay(LOOPTIME - timer[2]);
    }
    else
    {
      // M5.Lcd.printf("TIME SHORTAGE: %d\n", LOOPTIME - timer[2]);
    }
  }

  // stop command
  cmd_buf[0] = 0x81;
  cmd_buf[1] = 0x00;
  cmd_buf[2] = 0x00;
  cmd_buf[3] = 0x00;
  cmd_buf[4] = 0x00;
  cmd_buf[5] = 0x00;
  cmd_buf[6] = 0x00;
  cmd_buf[7] = 0x00;
  write_can();
  read_can();
  delay(500);
  SERIAL.println("Program finish!");
  M5.Lcd.setCursor(0, 130);
  M5.Lcd.printf("Program finish!");
  while (true)
  {
    delay(100);
  }
}

void init_can()
{
  M5.Lcd.setTextSize(TEXTSIZE);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("CAN Test!\n");

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
  byte sndStat = CAN0.sendMsgBuf(MOTOR_ADDRESS, 0, 8, cmd_buf);
  if (sndStat == CAN_OK)
  {
    Serial.println("Message Sent Successfully!");
    // M5.Lcd.printf("Message Sent Successfully!\n");
  }
  else
  {
    Serial.println("Error Sending Message...");
    // M5.Lcd.printf("Error Sending Message...\n");
  }
}

void read_can()
{
  if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, reply_buf); // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
    {
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d \n Data:", (rxId & 0x1FFFFFFF), len);
    }
    else
    {
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d \n Data:", rxId, len);
    }

    Serial.print(msgString);
    // M5.Lcd.printf(msgString);

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
        // M5.Lcd.printf(msgString);
      }
    }

    // M5.Lcd.printf("\n");
    Serial.println();
  }
}

void serialDisp(unsigned char *buf, unsigned char *pos_buf)
{
  // Serial Communication
  SERIAL.print(",");
  SERIAL.print(buf[0]);
  SERIAL.print(",");
  SERIAL.print(buf[1]);
  SERIAL.print(",");
  SERIAL.print(buf[2]);
  SERIAL.print(",");
  SERIAL.print(buf[3]);
  SERIAL.print(",");
  SERIAL.print(buf[4]);
  SERIAL.print(",");
  SERIAL.print(buf[5]);
  SERIAL.print(",");
  SERIAL.print(buf[6]);
  SERIAL.print(",");
  SERIAL.print(buf[7]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[0]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[1]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[2]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[3]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[4]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[5]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[6]);
  SERIAL.print(",");
  SERIAL.print(pos_buf[7]);
}
