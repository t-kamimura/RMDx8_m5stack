/* FreeRTOS test*/

#include <mcp_can_m5.h>
#include <SPI.h>
#include <M5Stack.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define BAUDRATE 115200 //シリアル通信がボトルネックにならないよう，速めに設定しておく
#define LOOPTIME 10     //[ms]
#define ENDTIME 10000   //[ms]
#define TEXTSIZE 2
#define KP 0.25
#define KD 0.01
// #define TGT_POS 0

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

// variables for controller
const int A = 30 * 6 * 100; // [servoHornDeg]*[gearRatio]*[encorderResolution]
const double f = 1.0;       //[Hz]
const double omega = 2 * 3.14 * f;
int32_t tgt_pos = 0;

int64_t present_pos = 0;
int32_t pos_old = 0;
int32_t vel = 0;

// define functions
void init_can();
void write_can();
void read_can();

// define two tasks
void TaskReadWrite(void *pvParameters);
void TaskDataProcess(void *pvParameters);

// variables for task communication
QueueHandle_t xQueue;

// the setup function runs once when you press reset or power the board
void setup()
{
    M5.begin();
    M5.Power.begin();
    // initialize serial communication at 115200 bits per second:
    Serial.begin(115200);
    delay(1000);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setTextSize(TEXTSIZE);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("RTOS Test!\n");
    timer[0] = millis();


    // create queue : type = int32_t, size = 5
     xQueue = xQueueCreate(5, sizeof(int32_t));

     if(xQueue != NULL)
     {
        // Now set up two tasks to run independently.
        xTaskCreateUniversal(TaskReadWrite, "TaskReadWrite", 1024, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
        xTaskCreateUniversal(TaskDataProcess, "TaskDataProcess", 1024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
         Serial.println("tasks registered");
     }
     else
     {
         while(1)
         {
             Serial.println("rtos queue create error, stopped");
             delay(1000);
         }
     }

    // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
    // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskReadWrite(void *pvParameters) // This is a task.
{
    (void)pvParameters;

    while (1) // A Task shall never return or exit.
    {
        while (millis() - timer[0] < ENDTIME)
        {
            timer[1] = millis();

            BaseType_t xStatus;

            tgt_pos = A * sin(omega * (timer[1] - timer[0]) * 0.001);

            // M5.Lcd.clear();

            // position control command
            cmd_buf[0] = 0xA3;
            cmd_buf[1] = 0x00;
            cmd_buf[2] = 0x00;
            cmd_buf[3] = 0x00;
            cmd_buf[4] = tgt_pos & 0xFF;
            cmd_buf[5] = (tgt_pos >> 8) & 0xFF;
            cmd_buf[6] = (tgt_pos >> 16) & 0xFF;
            cmd_buf[7] = (tgt_pos >> 24) & 0xFF;
            write_can();

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

            // M5.update();
            int64_t SendValue;
            // SendValue = reply_buf[1] + (reply_buf[2] << 8) + (reply_buf[3] << 16) + (reply_buf[4] << 24) + (reply_buf[5] << 32) + (reply_buf[6] << 40) + (reply_buf[7] << 48);
            SendValue = 1;

            xStatus = xQueueSend(xQueue, &SendValue, 0);    // キューを送信

            // if(xStatus != pdPASS) // send error check
            // {
            //     while(1)
            //     {
            //         Serial.println("rtos queue send error, stopped");
            //         delay(1000);
            //     }
            // }

            timer[2] = millis() - timer[1];
            if (timer[2] < LOOPTIME)
            {
                delay(LOOPTIME - timer[2]);
            }
            else
            {
                M5.Lcd.printf("TIME SHORTAGE: %d\n", LOOPTIME - timer[2]);
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
        Serial.println("Program finish!");
        M5.Lcd.setCursor(0, 160);
        M5.Lcd.printf("Program finish!");
        while (true)
        {
            delay(100);
        }
    }
}

void TaskDataProcess(void *pvParameters) // This is a task.
{
    (void)pvParameters;

    while (1) // A Task shall never return or exit.
    {
        M5.update();
        BaseType_t xStatus;
        int32_t ReceivedValue = 0;
        const TickType_t xTicksToWait = 500U; // [ms]

        xStatus = xQueueReceive(xQueue, &ReceivedValue, xTicksToWait);

         Serial.println("check if data is received");

         if(xStatus == pdPASS) // receive error check
         {
             Serial.print("received data : ");
             Serial.println(ReceivedValue);
            //  present_pos = reply_buf[1] + (reply_buf[2] << 8) + (reply_buf[3] << 16) + (reply_buf[4] << 24) + (reply_buf[5] << 32) + (reply_buf[6] << 40) + (reply_buf[7] << 48);
            //  present_pos = ReceivedValue[1] + (ReceivedValue[2] << 8) + (ReceivedValue[3] << 16) + (ReceivedValue[4] << 24) + (ReceivedValue[5] << 32) + (ReceivedValue[6] << 40) + (ReceivedValue[7] << 48);
            present_pos = 0;
            present_pos = present_pos * 0.01 / 6;
            vel = (present_pos - pos_old) / (LOOPTIME * 0.001);

            // M5.Lcd.setCursor(0, 40);
            // M5.Lcd.printf("TIM:            ");
            // // M5.Lcd.setCursor(0, 40);
            // // M5.Lcd.printf("TIM: %d", timer[1] - timer[0]);
            // M5.Lcd.setCursor(0, 70);
            // M5.Lcd.printf("TGT:            ");
            // // M5.Lcd.setCursor(0, 70);
            // // M5.Lcd.printf("TGT: %d", tgt_pos);
            // M5.Lcd.setCursor(0, 100);
            // M5.Lcd.printf("POS:            ");
            // M5.Lcd.setCursor(0, 100);
            // M5.Lcd.printf("POS: %d", present_pos);
            // M5.Lcd.setCursor(0, 130);
            // M5.Lcd.printf("VEL:            ");
            // M5.Lcd.setCursor(0, 130);
            // M5.Lcd.printf("VEL: %d", vel);
         }
         else
         {
             if(uxQueueMessagesWaiting(xQueue) != 0)
             {
                 while(1)
                 {
                     Serial.println("rtos queue receive error, stopped");
                     delay(1000);
                 }
             }
         }

        vTaskDelay(1);
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
