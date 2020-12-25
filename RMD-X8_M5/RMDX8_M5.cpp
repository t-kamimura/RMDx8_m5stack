
#include "Arduino.h"
#include "RMDX8_M5.h"
#include <mcp_can_m5.h>

// constructor
RMDX8_M5::RMDX8_M5(MCP_CAN_M5 &CAN, const uint16_t motor_addr)
    :_CAN(CAN){
        MOTOR_ADDRESS = motor_addr;
    }


void RMDX8_M5::canSetup() {
    while (CAN_OK != _CAN.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ))
        {
            Serial.println("CAN BUS Shield init fail");
            Serial.println("Init CAN BUS Shield again");
            delay(100);
        }
    Serial.println("CAN BUS Shield init ok!");
    _CAN.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
}


void RMDX8_M5::readPID() {
    cmd_buf[0] = 0x30;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    posKp = reply_buf[2];
    posKi = reply_buf[3];
    velKp = reply_buf[4];
    velKi = reply_buf[5];
    curKp = reply_buf[6];
    curKi = reply_buf[7];
}


void RMDX8_M5::writePID(uint8_t anglePidKp, uint8_t anglePidKi, uint8_t speedPidKp, uint8_t speedPidKi, uint8_t iqPidKp, uint8_t iqPidKi) {
    cmd_buf[0] = 0x31;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = anglePidKp;
    cmd_buf[3] = anglePidKi;
    cmd_buf[4] = speedPidKp;
    cmd_buf[5] = speedPidKi;
    cmd_buf[6] = iqPidKp;
    cmd_buf[7] = iqPidKi;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();
}


void RMDX8_M5::writeEncoderOffset(uint16_t offset) {
    cmd_buf[0] = 0x91;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    cmd_buf[6] = offset & 0xFF;
    cmd_buf[7] = (offset >> 8) & 0xFF;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();
}


void RMDX8_M5::readPosition() {
    cmd_buf[0] = 0x92;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    //pos_buf = present_angle;
    present_pos = 0;
    if (reply_buf[0] == 0x92) {
        present_pos = reply_buf[1] + (reply_buf[2] << 8) + (reply_buf[3] << 16) + (reply_buf[4] << 24) + (reply_buf[5] << 32) + (reply_buf[6] << 48);
    }
    else {
        Serial.print(" Read Error!");
    }
    present_angle = present_pos * 0.01 / 6;
}


void RMDX8_M5::clearState() {
    cmd_buf[0] = 0x80;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    writeCmd(cmd_buf);
    delay(1);
    readBuf();
}


/**
 * current is int16_t type, the value range:-2000~2000, corresponding to the actual torque current range -12.5A ~ 12.5A.
 * (the bus current and the actual torque of motor vary with different motors)
 */
void RMDX8_M5::writeCurrent(int16_t current) {
    cmd_buf[0] = 0xA1;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = current & 0xFF;
    cmd_buf[5] = (current >> 8) & 0xFF;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    temperature = reply_buf[1];
    present_current = ((int16_t)reply_buf[3] << 8) + reply_buf[2];
    present_velocity = ((int16_t)reply_buf[5] << 8) + reply_buf[4];
    encoder_pos = ((uint16_t)reply_buf[7] << 8) + reply_buf[6];
}


/**
 * velocity is int32_t type, which corresponds to the actual speed of 0.01 dps/LSB.
 */
void RMDX8_M5::writeVelocity(int32_t velocity) {
    cmd_buf[0] = 0xA2;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = velocity & 0xFF;
    cmd_buf[5] = (velocity >> 8) & 0xFF;
    cmd_buf[6] = (velocity >> 16) & 0xFF;
    cmd_buf[7] = (velocity >> 24) & 0xFF;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    temperature = reply_buf[1];
    present_current = ((int16_t)reply_buf[3] << 8) + reply_buf[2];
    present_velocity = ((int16_t)reply_buf[5] << 8) + reply_buf[4];
    encoder_pos = ((uint16_t)reply_buf[7] << 8) + reply_buf[6];
}


/**
 * # Position control command 1, multi turns
 * position is int32_t type, and the actual position is 0.01 degree/LSB, 36000 represents 360°.
 * The motor rotation direction is determined by the difference between the target position and the current position.
 */
void RMDX8_M5::writePosition(int32_t position) {
    cmd_buf[0] = 0xA3;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = position & 0xFF;
    cmd_buf[5] = (position >> 8) & 0xFF;
    cmd_buf[6] = (position >> 16) & 0xFF;
    cmd_buf[7] = (position >> 24) & 0xFF;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    temperature = reply_buf[1];
    present_current = ((int16_t)reply_buf[3] << 8) + reply_buf[2];
    present_velocity = ((int16_t)reply_buf[5] << 8) + reply_buf[4];
    encoder_pos = ((uint16_t)reply_buf[7] << 8) + reply_buf[6];
}


/**
 * # Position control command 2, multi turns
 * In addition to Position control command 1, the following functions have been added.
 * The control value max_speed limits the maximum speed at which the motor rotates, uint16_t type, corresponding to the actual speed of 1 dps/LSB.
 */
void RMDX8_M5::writePosition(int32_t position, uint16_t max_speed) {
    cmd_buf[0] = 0xA4;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = max_speed & 0xFF;
    cmd_buf[3] = (max_speed >> 8) & 0xFF;
    cmd_buf[4] = position & 0xFF;
    cmd_buf[5] = (position >> 8) & 0xFF;
    cmd_buf[6] = (position >> 16) & 0xFF;
    cmd_buf[7] = (position >> 24) & 0xFF;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    temperature = reply_buf[1];
    present_current = ((int16_t)reply_buf[3] << 8) + reply_buf[2];
    present_velocity = ((int16_t)reply_buf[5] << 8) + reply_buf[4];
    encoder_pos = ((uint16_t)reply_buf[7] << 8) + reply_buf[6];
}


/**
 * # Position control command 3, single turn
 * position is uint16_t type, the value range is 0~35999, and the actual position is 0.01 degree/LSB, the actual angle range is 0°~359.99°.
 * The control value spin_direction sets the direction in which the motor rotates, which is uint8_t type, 0x00 for CW and 0x01 for CCW.
 */
void RMDX8_M5::writePosition(uint16_t position, uint8_t spin_direction) {
    cmd_buf[0] = 0xA5;
    cmd_buf[1] = spin_direction;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = position & 0xFF;
    cmd_buf[5] = (position >> 8) & 0xFF;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    temperature = reply_buf[1];
    present_current = ((int16_t)reply_buf[3] << 8) + reply_buf[2];
    present_velocity = ((int16_t)reply_buf[5] << 8) + reply_buf[4];
    encoder_pos = ((uint16_t)reply_buf[7] << 8) + reply_buf[6];
}


/**
 * # Position control command 4, single turn
 * position is uint16_t type, the value range is 0~35999, and the actual position is 0.01 degree/LSB, the actual angle range is 0°~359.99°.
 * The control value max_speed limits the maximum speed at which the motor rotates, uint16_t type, corresponding to the actual speed of 1 dps/LSB.
 * The control value spin_direction sets the direction in which the motor rotates, which is uint8_t type, 0x00 for CW and 0x01 for CCW.
 */
void RMDX8_M5::writePosition(uint16_t position, uint16_t max_speed, uint8_t spin_direction) {
    cmd_buf[0] = 0xA6;
    cmd_buf[1] = spin_direction;
    cmd_buf[2] = max_speed & 0xFF;
    cmd_buf[3] = (max_speed >> 8) & 0xFF;
    cmd_buf[4] = position & 0xFF;
    cmd_buf[5] = (position >> 8) & 0xFF;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
    readBuf();

    temperature = reply_buf[1];
    present_current = ((int16_t)reply_buf[3] << 8) + reply_buf[2];
    present_velocity = ((int16_t)reply_buf[5] << 8) + reply_buf[4];
    encoder_pos = ((uint16_t)reply_buf[7] << 8) + reply_buf[6];
}

void RMDX8_M5::stop() {
    cmd_buf[0] = 0x81;
    cmd_buf[1] = 0x00;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;
    cmd_buf[6] = 0x00;
    cmd_buf[7] = 0x00;

    // Send message
    writeCmd(cmd_buf);
    delay(1);
}

// General function
void RMDX8_M5::serialWriteTerminator() {
    Serial.write(13);
    Serial.write(10);
}


// Private
void RMDX8_M5::readBuf() {
    //delayMicroseconds(600);    // 1000us
    /*
    if (CAN_MSGAVAIL == _CAN.checkReceive()) {
        _CAN.readMsgBuf(&rxId, & len, tmp_buf);
        if (tmp_buf[0] == buf[0]) {
            reply_buf[0] = tmp_buf[0];
            reply_buf[1] = tmp_buf[1];
            reply_buf[2] = tmp_buf[2];
            reply_buf[3] = tmp_buf[3];
            reply_buf[4] = tmp_buf[4];
            reply_buf[5] = tmp_buf[5];
            reply_buf[6] = tmp_buf[6];
            reply_buf[7] = tmp_buf[7];
        }
    }
    */
    _CAN.readMsgBuf(&rxId, &len, reply_buf);      // Read data: len = data length, buf = data byte(s)

}


void RMDX8_M5::writeCmd(unsigned char *buf) {
    // CAN通信で送る
    unsigned char sendState = _CAN.sendMsgBuf(MOTOR_ADDRESS, 0, 8, buf);
    if (sendState != CAN_OK) {
        Serial.println("Error Sending Message...");
        Serial.println(sendState);
    }
}
