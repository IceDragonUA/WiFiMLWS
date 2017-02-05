/*
  Created by Igor Jarc
  See http://iot-playground.com for details
  Please use community forum on website do not contact author directly

  External libraries:
  - https://github.com/adamvr/arduino-base64

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
*/

#include "Arduino.h"
#include <EspBase64.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// EasyIoT server definitions
#define IOT_USERNAME    "admin"
#define IOT_PASSWORD    "smartsoft"
#define IOT_IP_ADDRESS  "homeautomation.smartsoft.com"
#define IOT_DEV_NAME    "motionlightblock"
#define IOT_PORT        9091
#define USER_PWD_LEN     40
#define DEFAULT_IP       "0.0.0.0"

#define IOT_MOTION_1    "N1S0"
#define IOT_MOTION_2    "N2S0"
#define IOT_MOTION_3    "N3S0"
#define IOT_MOTION_4    "N4S0"

#define IOT_SWITCH_1    "N5S0"
#define IOT_SWITCH_2    "N6S0"
#define IOT_SWITCH_3    "N7S0"
#define IOT_SWITCH_4    "N8S0"

#define IOT_INPUT_1     0
#define IOT_INPUT_2     1
#define IOT_INPUT_3     2
#define IOT_INPUT_4     3

#define CONTROL_ON       0
#define CONTROL_OFF      1

#define A0 17
#define D0 16
#define D1 5
#define D2 4
#define D4 2
#define D5 14
#define D6 12
#define D7 13

bool firstInputState;
bool secondInputState;
bool thirdInputState;
bool fourthInputState;

bool firstOutputState;
bool secondOutputState;
bool thirdOutputState;
bool fourthOutputState;

bool isAutomationModeFirstSwitch;
bool isAutomationModeSecondSwitch;
bool isAutomationModeThirdSwitch;
bool isAutomationModeFourthSwitch;

byte checkIP;
unsigned int timer;
unsigned int checkTimerIP;

String command = "";

WiFiServer localServer(80);
IPAddress easyIotIpAddress;

char userNameEncoded[USER_PWD_LEN];

void initPins() {
    pinMode(D0, INPUT);
    pinMode(D1, INPUT);
    pinMode(D2, INPUT);
    pinMode(D4, OUTPUT);
    pinMode(D5, OUTPUT);
    pinMode(D6, OUTPUT);
    pinMode(D7, OUTPUT);
    digitalWrite(D4, CONTROL_OFF);
    digitalWrite(D5, CONTROL_OFF);
    digitalWrite(D6, CONTROL_OFF);
    digitalWrite(D7, CONTROL_OFF);
    firstInputState = !digitalRead(D0);
    secondInputState = !digitalRead(D1);
    thirdInputState = !digitalRead(D2);
    fourthInputState = false;
    firstOutputState = false;
    secondOutputState = false;
    thirdOutputState = false;
    fourthOutputState = false;
    isAutomationModeFirstSwitch = true;
    isAutomationModeSecondSwitch = true;
    isAutomationModeThirdSwitch = true;
    isAutomationModeFourthSwitch = true;
    timer = 0;
    checkIP = 0;
    checkTimerIP = 0;
}

void initCredentials() {
    char userName[USER_PWD_LEN];
    String str = String(IOT_USERNAME) + ":" + String(IOT_PASSWORD);
    str.toCharArray(userName, USER_PWD_LEN);
    memset(userNameEncoded, 0, sizeof(userNameEncoded));
    base64_encode(userNameEncoded, userName, strlen(userName));
}

void setupIpAddress() {
    WiFi.hostByName(IOT_IP_ADDRESS, easyIotIpAddress);
    if (easyIotIpAddress.toString().equals("127.0.1.1")) {
        WiFi.hostByName(IOT_IP_ADDRESS, easyIotIpAddress);
    }
}

void wifiConnect() {
    WiFiManager wifiManager;
    wifiManager.autoConnect(IOT_DEV_NAME);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    setupIpAddress();
    localServer.begin();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        // wait for serial port to connect. Needed for Leonardo only
    }
    initPins();
    initCredentials();
    wifiConnect();
}

void sendCommand(String command, WiFiClient serverClient, String eiotNode) {
    serverClient.print(String("POST ") +
                       "/Api/EasyIoT/Control/Module/Virtual/" +
                       eiotNode + "/" + command + " HTTP/1.1\r\n" +
                       "Host: " + IOT_IP_ADDRESS + "\r\n" +
                       "Connection: close\r\n" +
                       "Authorization: Basic " + userNameEncoded + " \r\n" +
                       "Content-Length: 0\r\n" +
                       "\r\n");
    serverClient.flush();
}

void controlWithoutServer(bool inputState, int inputNumber, bool isAutomationModeFirstSwitch,
                          bool isAutomationModeSecondSwitch, bool isAutomationModeThirdSwitch,
                          bool isAutomationModeFouthSwitch) {
    if (inputState && isAutomationModeFirstSwitch && inputNumber == 0) {
        digitalWrite(D4, CONTROL_ON);
    } else if (inputState && isAutomationModeSecondSwitch && inputNumber == 1) {
        digitalWrite(D5, CONTROL_ON);
    } else if (inputState && isAutomationModeThirdSwitch && inputNumber == 2) {
        digitalWrite(D6, CONTROL_ON);
    } else if (inputState && isAutomationModeFouthSwitch && inputNumber == 3) {
        digitalWrite(D7, CONTROL_ON);
    } else if (!inputState && isAutomationModeFouthSwitch && inputNumber == 3) {
        digitalWrite(D7, CONTROL_OFF);
    } else if (!inputState && isAutomationModeThirdSwitch && inputNumber == 2) {
        digitalWrite(D6, CONTROL_OFF);
    } else if (!inputState && isAutomationModeSecondSwitch && inputNumber == 1) {
        digitalWrite(D5, CONTROL_OFF);
    } else if (!inputState && isAutomationModeFirstSwitch && inputNumber == 0) {
        digitalWrite(D4, CONTROL_OFF);
    }
}

void startSessionsByNode(bool inputState, int inputNumber, String eiotNode) {
    WiFiClient serverClient;
    Serial.println(easyIotIpAddress);

    if (easyIotIpAddress.toString().equals("0.0.0.0")) {
        controlWithoutServer(inputState, inputNumber, isAutomationModeFirstSwitch, isAutomationModeSecondSwitch,
                             isAutomationModeThirdSwitch, isAutomationModeFourthSwitch);
        serverClient.stop();
        return;
    }

    if (!serverClient.connect(easyIotIpAddress, IOT_PORT)) {
        String defaultIP = String(DEFAULT_IP);
        char *defaultIPAddress = &defaultIP[0];
        easyIotIpAddress.fromString(defaultIPAddress);
        controlWithoutServer(inputState, inputNumber, isAutomationModeFirstSwitch, isAutomationModeSecondSwitch,
                             isAutomationModeThirdSwitch, isAutomationModeFourthSwitch);
        serverClient.stop();
        return;
    }

    if (inputState && isAutomationModeFirstSwitch && inputNumber == 0) {
        command = "ControlOn";
        digitalWrite(D4, CONTROL_ON);
        sendCommand(command, serverClient, eiotNode);
    } else if (inputState && isAutomationModeSecondSwitch && inputNumber == 1) {
        command = "ControlOn";
        digitalWrite(D5, CONTROL_ON);
        sendCommand(command, serverClient, eiotNode);
    } else if (inputState && isAutomationModeThirdSwitch && inputNumber == 2) {
        command = "ControlOn";
        digitalWrite(D6, CONTROL_ON);
        sendCommand(command, serverClient, eiotNode);
    } else if (inputState && isAutomationModeFourthSwitch && inputNumber == 3) {
        command = "ControlOn";
        digitalWrite(D7, CONTROL_ON);
        sendCommand(command, serverClient, eiotNode);
    } else if (!inputState && isAutomationModeFourthSwitch && inputNumber == 3) {
        command = "ControlOff";
        digitalWrite(D7, CONTROL_OFF);
        sendCommand(command, serverClient, eiotNode);
    } else if (!inputState && isAutomationModeThirdSwitch && inputNumber == 2) {
        command = "ControlOff";
        digitalWrite(D6, CONTROL_OFF);
        sendCommand(command, serverClient, eiotNode);
    } else if (!inputState && isAutomationModeSecondSwitch && inputNumber == 1) {
        command = "ControlOff";
        digitalWrite(D5, CONTROL_OFF);
        sendCommand(command, serverClient, eiotNode);
    } else if (!inputState && isAutomationModeFirstSwitch && inputNumber == 0) {
        command = "ControlOff";
        digitalWrite(D4, CONTROL_OFF);
        sendCommand(command, serverClient, eiotNode);
    }
    serverClient.stop();
}

bool checkDigitalInputState(bool inputState, bool oldInputState, int inputNumber, String sensorId, String switchId) {
    if (inputState != oldInputState) {
        startSessionsByNode(inputState, inputNumber, sensorId);
        startSessionsByNode(inputState, inputNumber, switchId);
    }
    return inputState;
}

void readDigitalInputs() {
    firstInputState = checkDigitalInputState(
            (bool) digitalRead(D0), firstInputState, IOT_INPUT_1, String(IOT_MOTION_1), String(IOT_SWITCH_1)
    );
    secondInputState = checkDigitalInputState(
            (bool) digitalRead(D1), secondInputState, IOT_INPUT_2, String(IOT_MOTION_2), String(IOT_SWITCH_2)
    );
    thirdInputState = checkDigitalInputState(
            (bool) digitalRead(D2), thirdInputState, IOT_INPUT_3, String(IOT_MOTION_3), String(IOT_SWITCH_3)
    );
}

bool checkAnalogInputState(int inputState, int inputNumber, String sensorId, String switchId) {
    if (inputState > 256) {
        startSessionsByNode(true, inputNumber, sensorId);
        startSessionsByNode(true, inputNumber, switchId);
        return true;
    }
    startSessionsByNode(false, inputNumber, sensorId);
    startSessionsByNode(false, inputNumber, switchId);
    return false;
}

void readAnalogInputs() {
    if (timer == 1024) {
        timer = 0;
        fourthInputState = checkAnalogInputState(
                analogRead(A0), IOT_INPUT_4, String(IOT_MOTION_4), String(IOT_SWITCH_4)
        );
    }
    timer++;
}

void readIpAddress() {
    if (easyIotIpAddress.toString().equals("0.0.0.0")) {
        if (checkTimerIP == 65535) {
            checkTimerIP = 0;
            if (checkIP == 15) {
                checkIP = 0;
                setupIpAddress();
            }
            checkIP++;
        }
        checkTimerIP++;
    }
}

void loop() {
    Serial.println(easyIotIpAddress);
    WiFiClient browserClient = localServer.available();
    if (!browserClient) {
        readDigitalInputs();
        readAnalogInputs();
        readIpAddress();
        return;
    }

    // Wait until the client sends some data
    while (!browserClient.available()) {
        delay(1);
    }

    // Read the first line of the request
    String request = browserClient.readStringUntil('\r');

    // Match the request
    if (request.indexOf("/gpio/1") != -1) {
        digitalWrite(D4, CONTROL_ON);
        firstOutputState = true;
    } else if (request.indexOf("/gpio/3") != -1) {
        digitalWrite(D5, CONTROL_ON);
        secondOutputState = true;
    } else if (request.indexOf("/gpio/5") != -1) {
        digitalWrite(D6, CONTROL_ON);
        thirdOutputState = true;
    } else if (request.indexOf("/gpio/7") != -1) {
        digitalWrite(D7, CONTROL_ON);
        fourthOutputState = true;
    } else if (request.indexOf("/gpio/6") != -1) {
        digitalWrite(D7, CONTROL_OFF);
        fourthOutputState = false;
    } else if (request.indexOf("/gpio/4") != -1) {
        digitalWrite(D6, CONTROL_OFF);
        thirdOutputState = false;
    } else if (request.indexOf("/gpio/2") != -1) {
        digitalWrite(D5, CONTROL_OFF);
        secondOutputState = false;
    } else if (request.indexOf("/gpio/0") != -1) {
        digitalWrite(D4, CONTROL_OFF);
        firstOutputState = false;
    } else if (request.indexOf("/gpio/Q") != -1) {
        isAutomationModeFirstSwitch = false;
    } else if (request.indexOf("/gpio/W") != -1) {
        isAutomationModeSecondSwitch = false;
    } else if (request.indexOf("/gpio/E") != -1) {
        isAutomationModeThirdSwitch = false;
    } else if (request.indexOf("/gpio/R") != -1) {
        isAutomationModeFourthSwitch = false;
    } else if (request.indexOf("/gpio/A") != -1) {
        isAutomationModeFirstSwitch = true;
    } else if (request.indexOf("/gpio/B") != -1) {
        isAutomationModeSecondSwitch = true;
    } else if (request.indexOf("/gpio/C") != -1) {
        isAutomationModeThirdSwitch = true;
    } else if (request.indexOf("/gpio/D") != -1) {
        isAutomationModeFourthSwitch = true;
    } else {
        browserClient.stop();
        return;
    }
    Serial.println(request);

    String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
    response += (firstInputState) ? "MOTION 1: ON\r\n" : "MOTION 1: OFF\r\n";
    response += (secondInputState) ? "MOTION 2: ON\r\n" : "MOTION 2: OFF\r\n";
    response += (thirdInputState) ? "MOTION 3: ON\r\n" : "MOTION 3: OFF\r\n";
    response += (fourthInputState) ? "MOTION 4: ON\r\n" : "MOTION 4: OFF\r\n";
    response += (firstOutputState) ? "SWITCH 1: ON\r\n" : "SWITCH 1: OFF\r\n";
    response += (secondOutputState) ? "SWITCH 2: ON\r\n" : "SWITCH 2: OFF\r\n";
    response += (thirdOutputState) ? "SWITCH 3: ON\r\n" : "SWITCH 3: OFF\r\n";
    response += (fourthOutputState) ? "SWITCH 4: ON\r\n" : "SWITCH 4: OFF\r\n";
    response += (isAutomationModeFirstSwitch) ? "AUTOLIGHT 1: ON\r\n" : "AUTOLIGHT 1: OFF\r\n";
    response += (isAutomationModeSecondSwitch) ? "AUTOLIGHT 2: ON\r\n" : "AUTOLIGHT 2: OFF\r\n";
    response += (isAutomationModeThirdSwitch) ? "AUTOLIGHT 3: ON\r\n" : "AUTOLIGHT 3: OFF\r\n";
    response += (isAutomationModeFourthSwitch) ? "AUTOLIGHT 4: ON\r\n" : "AUTOLIGHT 4: OFF\r\n";
    response += "</html>\n";
    Serial.println(response);

    browserClient.flush();
    browserClient.print(response);
    delay(10);
}

