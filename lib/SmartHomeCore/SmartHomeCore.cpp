#include <SmartHomeCore.h>

//ap config
String _ssidInit	      	   = "SmartHomeCore";
String _passwordInit    	   = "123123123";
int wifiChannel                = 8;
int maxConnection			   = 1;
int hiddenWifi				   = 0;

//common config
int _serialSpeed               = 115200;
int _udpPort                   = 6510;
int _httpPort                  = 80;
int _jsonLength	               = 2048;
String _otaHostName            = "SmartHomeCore";
const char* www_username       = "admin";
const char* www_password       = "admin";
float version                  = 1.2;

DynamicJsonDocument json(_jsonLength);
IPAddress apIP(77, 77, 77, 77);
IPAddress subnet(255, 255, 255, 0);

void shCore::registrateEvent(String uri, void(*function)())
{
	_server.on(uri,function);
}
void shCore::sendToServer(int code, String contentType, String str)
{
	_server.send(code, contentType, str);
}
void shCore::coreInit(void) {

	//ArduinoOTA.setHostname(otaHostName);
	//ArduinoOTA.begin();

	SPIFFS.begin();
  
	Serial.begin(_serialSpeed);
  
	pinMode(LED_BUILTIN, OUTPUT);

	wifiInit();

	_udp.begin(_udpPort);

	if (!openFile("devices.conf")) { createOrErase("devices.conf", ""); Serial.println("devices.conf"); };

	Serial.println("starting:");

}

void shCore::coreHandle(void) {
  _server.handleClient();//ждём клиентов
	String str = tryToReceive();
	if (str.length() != 0)
	{
		Serial.printf("Recived packet!\n");
		Serial.println(str);

		if (str == "initial")
		{
			Serial.println("tryToSend to Mobile");
			tryToSend(_udp.remoteIP(), _udpPort, WiFi.localIP()? WiFi.localIP().toString(): WiFi.softAPIP().toString());

			blink(4, 5);
		}
		else
		{
			blink(1, 1);		
		}
	}
	
  //ArduinoOTA.handle();  //ждём обнов
  blink(1,5);
}
//работа с устройствами
void shCore::wifiInit()
{
	File f = openFile("wifiConf.conf");
	if (!f)
	{
		WiFi.softAPConfig(apIP, apIP, subnet);
		bool connect  = WiFi.softAP(_ssidInit, _passwordInit, wifiChannel, hiddenWifi, maxConnection);
		IPAddress myIP = WiFi.softAPIP();
		if(connect){Serial.println("Успешно");} else{Serial.println("Ошибка");}
		Serial.println("AP IP address: ");
		Serial.println(myIP);
		_server.on("/", htmlAccessPoint);
		_server.on("/getWifiList", sendWifiList);
		_server.on("/configure", configWiFi);

		blink(2, 100);
	}
	else
	{
		json.clear();

		Serial.println("Client starting...");

		String str = f.readString();
		Serial.println(str);

		deserializeJson(json, str);
		String wifiSSID = json["wifiSSID"];
		String wifiPass = json["wifiPass"];

		WiFi.mode(WIFI_AP_STA);
		WiFi.begin(wifiSSID, wifiPass);

		int i = 25;
		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");

			if (i == 0) {
				Serial.println("remove /wifiConf.conf");
				clearAll();
				delay(1000);
			}
			else { i--; }
		}
		Serial.println(WiFi.localIP());
		blink(5, 100);
	}
	_server.onNotFound(handleNotFound);          //  Cтраница ошибки
	_server.on("/", html);
	_server.on("/clearAll", clearAll);
	_server.on("/info", api);
	filesHandling();
	_server.begin(_httpPort);
}
//события http-сервера
void shCore::htmlAccessPoint()
{
	File f = SPIFFS.open("/htmlAccessPoint.html", "r");
	if (!f) {
		Serial.println("file open failed");
	}
	else {
		Serial.println("send page ok! ");
		Serial.print(_server.uri());
		Serial.print(" ");
		Serial.println(f.size());
	}
	_server.streamFile(f,"text/html");
	f.close();
}
void shCore::handleNotFound(){ //страница ошибок
 
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += _server.uri();
  message += "\nMethod: ";
  message += (_server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += _server.args();
  message += "\n";
  for (int i=0; i<_server.args(); i++){
    message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
  }
  _server.send(404, "text/plain", message);
 
}

void shCore::api(){         
 if(!_server.authenticate(www_username, www_password))
 return _server.requestAuthentication();   
 String api = "Smart Home Module (temperature)<br><br>";
 api += "WiFi RSSI<br>";
 api += WiFi.RSSI();
 api += "<br>";
 api += "Ram<br>";
 api += ESP.getFreeHeap();
 api += "<br>";
 api += "Chip ID<br>";
 api += ESP.getChipId();
 api += "<br>";
 api += "SDK version<br>";
 api += ESP.getSdkVersion();
 api += "<br>";
 api += "Release version<br>";
 api += version;
 api += "<br>";
 api += "<button onclick='fetch(\"/clearAll\");'>Reset Config</button>";
 api += "<br>";
 _server.send(200, "text/html", api);
};
void shCore::html() {
	//if (!_server.authenticate(www_username, www_password))
	//	return _server.requestAuthentication();
		File f = SPIFFS.open("/index.html", "r");
		if (!f) {
			Serial.println("file open failed");
		}
		else {

			Serial.println("send page ok! ");
			Serial.print(_server.uri());
			Serial.print(" ");
			Serial.println(f.size());
		}
		_server.streamFile(f,"text/html");
		f.close();

}
void shCore::clearAll()
{
	SPIFFS.remove("/wifiConf.conf");
	SPIFFS.remove("/device.conf");
	SPIFFS.remove("/devices.conf");

	_server.send(200, "text/html", "clear Ok");
	Serial.println("clear Ok");

	WiFi.disconnect();
	delay(1000);
	ESP.restart();
}
void shCore::sendWifiList()
{
	String http;
	int n = WiFi.scanNetworks();
	if (n == 0)
	{
		http += "<option value='-1'>No Networks!</option>";
	}
	else
	{
		Serial.println("FindWifi:");
		http += "<option selected>Choose...</option>";
		for (int i = 0; i < n; ++i)
		{
			http += "<option value='";
			http += WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
			Serial.println(WiFi.SSID(i));
			delay(1);

		}
	}
	_server.send(200, "text/plain", http);
}

//общие функции
String shCore::tryToReceive()
{
	int udpPacketSize = _udp.parsePacket();
	String str;
	if (udpPacketSize)
	{
		str = _udp.readString();
	}
	return str;
}
void shCore::tryToSend(IPAddress remoteIp, int udpPort, String text)
{
	_udp.beginPacket(remoteIp, udpPort);
	char buf[256];
	text.toCharArray(buf, 256, 0);
	_udp.write(buf);
	_udp.endPacket();

}
void shCore::configWiFi()
{
	String str;
	String wifiSSID = _server.arg("SSID");
	String wifiPass = _server.arg("pass");
	json["wifiSSID"] = wifiSSID;
	json["wifiPass"] = wifiPass;
	serializeJson(json, str);
	createOrErase("wifiConf.conf", str);
	Serial.println("New wifiConf!");
	Serial.println(str);

	Serial.println("restart");

	WiFi.disconnect();
	WiFi.softAPdisconnect();
	delay(2000);

	ESP.restart();
}
void shCore::filesHandling()
{
	Dir dir = SPIFFS.openDir("/");
	Serial.println("Add file to listeaner");
	while (dir.next()) {
		_server.on(dir.fileName(), fileDownload);
		Serial.println(dir.fileName());
	}
}
void shCore::fileDownload()
{
	File f = SPIFFS.open(_server.uri(), "r");
	if (!f) {
		Serial.println("file open failed");
		Serial.println(_server.uri());
	}
	else { 
		Serial.println("send page ok! "); 
		Serial.print(_server.uri());
		Serial.print(" ");
		Serial.println(f.size());
	}
	_server.streamFile(f, getContentType(_server.uri()),HTTP_GET);
	f.close();
}
String shCore::getContentType(String filename) {
	if (_server.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}
File shCore::openFile(String Filename)
{
	File f = SPIFFS.open("/" + Filename, "r");
	if (!f) {
		Serial.println("file open failed");
		Serial.println(Filename);
	}
	return f;
}
void shCore::createOrErase(String Filename, String Text)
{
	File f = SPIFFS.open("/" + Filename, "w");
	if (!f) {
		Serial.println("file open failed");
		Serial.println(Filename);
	}
	f.print(Text);
	f.close();
}
void shCore::blink(int num, int delayMs)
{
	for (int i = 0; i < num; i++)
	{
		digitalWrite(LED_BUILTIN, LOW);
		delay(delayMs);
		digitalWrite(LED_BUILTIN, HIGH);
		delay(delayMs);
	}

}

//set
void shCore::setSerialSpeed(int serialSpeed){_serialSpeed = serialSpeed;}
void shCore::setSSIDwifiAP(String ssid){_ssidInit = ssid;}
void shCore::setPassWiFiAP(String pass){_passwordInit = pass;}
void shCore::setUDPport(int port){_udpPort = port;}
void shCore::setHTTPport(int port){_httpPort = port;}
void shCore::setOTAname(String name){_otaHostName = name;}