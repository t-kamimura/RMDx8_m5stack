/* FreeRTOS test*/

#include <M5Stack.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define TEXTSIZE 2

// define two tasks for Blink & AnalogRead
void TaskA(void *pvParameters);
void TaskB(void *pvParameters);

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

    // Now set up two tasks to run independently.
    xTaskCreateUniversal(
        TaskA,
        "TaskA" // A name just for humans
        ,
        1024 // This stack size can be checked & adjusted by reading the Stack Highwater
        ,
        NULL, 2 // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,
        NULL, ARDUINO_RUNNING_CORE);

    xTaskCreateUniversal(
        TaskB,
        "TaskB",
        1024 // Stack size
        ,
        NULL,
        1 // Priority
        ,
        NULL,
        ARDUINO_RUNNING_CORE);

    // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
    // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskA(void *pvParameters) // This is a task.
{
    (void)pvParameters;

    while (1) // A Task shall never return or exit.
    {
        M5.Lcd.setCursor(0, 40);
        M5.Lcd.printf("AAAAAAA");
        Serial.print('A');
        vTaskDelay(99);                 // one tick delay (15ms) in between reads for stability
        M5.Lcd.setCursor(0, 40);
        M5.Lcd.printf("aaaaaaa");
        Serial.println('a');
        vTaskDelay(99);                 // one tick delay (15ms) in between reads for stability
    }
}

void TaskB(void *pvParameters) // This is a task.
{
    (void)pvParameters;

    while (1) // A Task shall never return or exit.
    {
        M5.Lcd.setCursor(0, 70);
        M5.Lcd.printf("BBBBBBB");
        Serial.print('B');
        vTaskDelay(200);
        M5.Lcd.setCursor(0, 70);
        M5.Lcd.printf("bbbbbbb");
        Serial.println('b');
        vTaskDelay(200);
    }
}