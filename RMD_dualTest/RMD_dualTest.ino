/* CAN通信テスト */
#include <M5Stack.h>
#include <mcp_can_m5.h>
#include <SPI.h>
#include <RMDX8_M5.h>

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
#define KP 0.5
#define KD 0.2
#define TGT_POS 0

// unsigned char len = 0;
// unsigned char cmd_buf[8], reply_buf[8];
// long unsigned int rxId;
// unsigned char rxBuf[8];
// char msgString[128];
unsigned long timer[3];
// byte pos_byte[4];

const uint16_t MOTOR_ADDRESS_1 = 0x141; //0x140 + ID(1~32)
const uint16_t MOTOR_ADDRESS_2 = 0x143; //0x140 + ID(1~32)
const int SPI_CS_PIN = 12;

#define CAN0_INT 15          // Set INT to pin 2
MCP_CAN_M5 CAN0(SPI_CS_PIN); // Set CS to pin 10
RMDX8_M5 myMotor1(CAN0, MOTOR_ADDRESS_1);
RMDX8_M5 myMotor2(CAN0, MOTOR_ADDRESS_2);

int A = 5 * 6 * 100; // (servoHornDeg)*(gearRatio)*(encorderResolution)
double f = 1.0;       // [Hz]
double omega = 2 * 3.14 * f;

void init_can();

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
    Serial.println("Test CAN...");
    timer[0] = millis();

    myMotor1.readPosition();
}

void loop()
{
    while (millis() - timer[0] < ENDTIME)
    {
        timer[1] = millis();
        int32_t tgt_pos = A * sin(omega * (timer[1] - timer[0]) * 0.001);

        // read multi turn angle
        // pos_buf = myMotor2.present_pos;
        myMotor1.readPosition();
        delay(1);
        myMotor2.readPosition();
        delay(1);
        myMotor1.writePosition(tgt_pos);
        delay(1);
        myMotor2.writePosition(-tgt_pos);
        delay(1);

        M5.update();

        // vel = (myMotor2.present_pos - pos_buf)/(LOOPTIME*0.01);

        // DEBUG(この処理重いので注意)
        // M5.Lcd.setCursor(0, 40);
        // M5.Lcd.printf("TIM:            ");
        // M5.Lcd.setCursor(0, 40);
        // M5.Lcd.printf("TIM: %d", timer[1] - timer[0]);
        // M5.Lcd.setCursor(0, 70);
        // M5.Lcd.printf("POS:            ");
        // M5.Lcd.setCursor(0, 70);
        // M5.Lcd.printf("POS: %d", myMotor2.present_pos);
        // M5.Lcd.setCursor(0, 100);
        // M5.Lcd.printf("HRN:            ");
        // M5.Lcd.setCursor(0, 100);
        // M5.Lcd.printf("HRN: %d", myMotor2.present_angle);
        // M5.Lcd.setCursor(0, 130);
        // M5.Lcd.printf("VEL:            ");
        // M5.Lcd.setCursor(0, 130);
        // M5.Lcd.printf("VEL: %d", vel);
        Serial.print("TIM: ");
        Serial.print(timer[1] - timer[0]);
        // Serial.print(" POS: ");
        // Serial.print(myMotor1.present_pos);

        // Serial.print(" TGT: ");
        // Serial.print(tgt_pos);

        Serial.print(" HRN1: ");
        Serial.print(myMotor1.present_angle);
        Serial.print(" HRN2: ");
        Serial.print(myMotor2.present_angle);
        Serial.println("");


        timer[2] = millis() - timer[1];
        if (timer[2] < LOOPTIME)
        {
            delay(LOOPTIME - timer[2]);
        }
        else
        {
            SERIAL.print("TIME SHORTAGE");
            SERIAL.println(LOOPTIME - timer[2]);
            // M5.Lcd.printf("TIME SHORTAGE: %d\n", LOOPTIME - timer[2]);
        }
    }

    // stop command
    myMotor1.stop();
    delay(500);
    SERIAL.println("Program finish!");
    M5.Lcd.setCursor(0, 160);
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
