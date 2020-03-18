#pragma once
#include <Arduino.h>
//for wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>

namespace shCore{

    static ESP8266WebServer _server;
    static WiFiUDP _udp;

    void registrateEvent(String uri, void(*function)());
    void sendToServer(int code, String contentType, String str);

    void coreInit(void);
    void coreHandle(void);
    void wifiInit(void);

    //события http сервера
    void handleNotFound(void);
    //html-страница для режима инициализации
    void htmlAccessPoint(void);
    //инормационная страница
    void api(void);
    //html-страница для нормального режима работы
    void html(void);
    //сброс до заводских настроек
    void clearAll(void);
    //отправляем список доступных wifi сетей
    void sendWifiList(void);

    //пробуем принять udp
    String tryToReceive(void);
    //пробуем отправить udp
    void tryToSend(IPAddress remoteIp, int udpPort, String text);
    //удалённая настройка wifi
    void configWiFi(void);
    //создаём события для всех файлов в памяти
    void filesHandling(void);
    //событие загрузки файла с сервера
    void fileDownload(void);
    //определяем тип файла
    String getContentType(String filename);
    File openFile(String Filename);
    void createOrErase(String Filename, String Text);
    void blink(int num, int delayMs);

    //set
    void setSerialSpeed(int serialSpeed);
    void setSSIDwifiAP(String ssid);
    void setPassWiFiAP(String pass);
    void setUDPport(int port);
    void setHTTPport(int port);
    void setOTAname(String name);
    void setMainPage(String pageName);
    void sendMainPage();
};