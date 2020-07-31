#include "image.h"
#include "pms5003ST.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SSD1306Wire.h> // legacy: #include "SSD1306.h"
#include <SoftwareSerial.h>
#include <Wire.h> // Only needed for Arduino 1.6.5 and earlier

#define S8_RXPIN D6
#define S8_TXPIN D5
#define PMS_RXPIN D7
#define LEFT_BUTTON D3
#define RIGHT_BUTTON D4

SoftwareSerial S8(S8_RXPIN, S8_TXPIN);
SoftwareSerial PMS_Serial(PMS_RXPIN, D0);
PMS::DATA pms_data;
PMS pms(PMS_Serial, &pms_data);

// const char *ssid = "Wireless-2.4GHz";
// const char *passwd = "83885877";
// const char *mqtt_server = "192.168.0.2";
// const char *mqtt_username = "homeassistant";
// const char *mqtt_password = "myhome";

String mqtt_server = "";
String mqtt_username = "";
String mqtt_passwd = "";

const IPAddress apIP(192, 168, 1, 1);
const char *apSSID = "ESP8266_SETUP";
boolean settingMode;
String ssidList;

DNSServer dnsServer;
ESP8266WebServer webServer(80);

WiFiClient espClient;
PubSubClient client(espClient);

SSD1306Wire display(0x3c, SDA, SCL);

static int WiFiStatus;
static int MQTTStatus;

/* CO2 */
byte command_read_co2[] = {0xFE, 0x44, 0x00, 0x08, 0x02, 0x9F, 0x25};
byte response[] = {0, 0, 0, 0, 0, 0, 0};
int co2_value_scale = 1;
unsigned long co2 = 0;

bool co2_send_command(Stream *serial = &S8, byte *cmd = command_read_co2,
                      int cmd_len = 7, byte *resp = response,
                      int resp_len = 7) {
  while (!serial->available()) {
    serial->write(cmd, cmd_len);
    delay(50);
  }

  int timeout = 0;
  while (serial->available() < 7) {
    timeout++;
    if (timeout > 10) {
      while (serial->available())
        serial->read();
      return false;
    }
    delay(50);
  }
  for (int i = 0; i < resp_len; i++)
    resp[i] = serial->read();
  return true;
}

unsigned long co2_read_response(byte *resp = response,
                                int scale = co2_value_scale) {
  return scale * (resp[3] * 256 + resp[4]);
}

/* Network */
/*
void setup_wifi() {
  delay(50);
  Serial.println("WiFi connecting...");
  WiFi.begin(ssid, passwd);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    WiFiStatus = 0;
    return;
  }
  Serial.println("WiFi Connected.");
  WiFiStatus = 1;
  delay(100);
  Serial.print("IP Addr: ");
  Serial.println(WiFi.localIP());
}
*/

void reconnect() {
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "AirSniffer-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  // if you MQTT broker has clientID,username and password
  // please change following line to    if
  // (client.connect(clientId,userName,passWord))
  boolean connect_status = false;
  if (mqtt_username == "") {
    connect_status = client.connect(clientId.c_str());
  } else {
    if (mqtt_passwd == "")
      connect_status =
          client.connect(clientId.c_str(), mqtt_username.c_str(), NULL);
    else
      connect_status = client.connect(clientId.c_str(), mqtt_username.c_str(),
                                      mqtt_passwd.c_str());
  }
  if (connect_status) {
    Serial.println("connected");
    MQTTStatus = 1;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    MQTTStatus = 0;
  }
}

/* Draw status bar */
// only need to call when status changed
void draw_statusbar() {
  display.setColor(BLACK);
  display.fillRect(0, 0, 128, 12);
  display.setColor(WHITE);
  if (MQTTStatus) {
    display.drawFastImage(12, 1, 8, 16, upload);
  }
  if (WiFiStatus) {
    display.drawFastImage(0, 1, 16, 16, wifi);
    display.setFont(ArialMT_Plain_10);
    display.drawString(24, 0, WiFi.localIP().toString().c_str());
  }
  display.drawFastImage(118, 1, 16, 16, logo);
  display.display();
  display.drawLine(0, 12, 128, 12);
}

void ICACHE_RAM_ATTR onTimerISR();

static int displaying = 0;
static int display_sence = 0;

String urlDecode(String input);
String makePage(String title, String contents);
void setupMode();
void startWebServer();
boolean checkConnection();
boolean restoreConfig();

void setup() {
  Serial.begin(9600);
  Serial.println("Initialization --Begin--");

  WiFiStatus = 0;
  MQTTStatus = 0;

  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(50000); // 10ms

  S8.begin(9600);
  S8.flush();
  PMS_Serial.begin(9600);
  PMS_Serial.flush();

  display.init();
  displaying = 1;
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.clear();
  display.drawString(0, 0, "AirSniffer");
  display.drawString(32, 16, "Initializing");
  display.display();

  // setup_wifi();
  // client.setServer(mqtt_server, 1883);

  EEPROM.begin(512);
  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();

  draw_statusbar();

  Serial.println("Initialization --Done--");
}

void publish_it(unsigned long value, const char *name) {
  char addr_buf[32];
  sprintf(addr_buf, "Air/%s", name);
  char buf[32];
  sprintf(buf, "%ld", value);
  if (client.publish(addr_buf, buf))
    ;
  else
    Serial.println(" Failed...");
}

void publish_it_float(float value, const char *name, int dig) {
  char addr_buf[32];
  sprintf(addr_buf, "Air/%s", name);
  char buf[32];
  sprintf(buf, "%.*f", dig, value);
  if (client.publish(addr_buf, buf))
    ;
  else
    Serial.println(" Failed...");
}

void mqtt_publish() {
  if (co2)
    publish_it(co2, "CO2");
  if (pms_data.TEMP) {
    publish_it_float(pms_data.TEMP / 10.0f, "TEMP", 1);
    publish_it_float(pms_data.HCHO / 1000.0f, "HCHO", 3);
    publish_it_float(pms_data.HUMD / 10.0f, "HUMD", 1);
    publish_it(pms_data.PM_FE_UG_1_0, "PM1.0-FE");
    publish_it(pms_data.PM_FE_UG_2_5, "PM2.5-FE");
    publish_it(pms_data.PM_FE_UG_10_0, "PM10-FE");
    publish_it(pms_data.PM_AE_UG_1_0, "PM1.5");
    publish_it(pms_data.PM_AE_UG_2_5, "PM2.5");
    publish_it(pms_data.PM_AE_UG_10_0, "PM10");
    publish_it(pms_data.ABOVE_0dot3_um, ">0.3um");
    publish_it(pms_data.ABOVE_0dot5_um, ">0.5um");
    publish_it(pms_data.ABOVE_1dot0_um, ">1.0um");
    publish_it(pms_data.ABOVE_2dot5_um, ">2.5um");
    publish_it(pms_data.ABOVE_5dot0_um, ">5.0um");
    publish_it(pms_data.ABOVE_10_um, ">10um");
  }
}

void data_read() {
  // Serial.println("Reading Data.");

  // Read co2
  co2 = 0;
  if (co2_send_command()) {
    co2 = co2_read_response();
    // Serial.print("Co2 got data: ");
    // Serial.println(co2);
  } else {
    // Serial.println("Co2 no data");
    co2 = 0;
  }
}

#define BTN_PRESSING 1
#define BTN_RELEASE 0
#define BTN_NOSURE 2

static int btn_status_left = BTN_RELEASE;
static int btn_status_right = BTN_RELEASE;
static int btn_left_changed = 0;
static int btn_right_changed = 0;

void ICACHE_RAM_ATTR onTimerISR() {

  int vl = digitalRead(LEFT_BUTTON);
  int vr = digitalRead(RIGHT_BUTTON);

  switch (btn_status_left) {
  case BTN_NOSURE:
    if (vl == LOW) {
      btn_status_left = BTN_PRESSING;
      btn_left_changed = 1;
    } else {
      btn_status_left = BTN_RELEASE;
    }
    break;
  case BTN_RELEASE:
    if (vl == LOW)
      btn_status_left = BTN_NOSURE;
    break;
  case BTN_PRESSING:
    if (vl == HIGH) {
      btn_status_left = BTN_RELEASE;
      btn_left_changed = 1;
    }
    break;
  }

  switch (btn_status_right) {
  case BTN_NOSURE:
    if (vr == LOW) {
      btn_status_right = BTN_PRESSING;
      btn_right_changed = 1;
    } else {
      btn_status_right = BTN_RELEASE;
    }
    break;
  case BTN_RELEASE:
    if (vr == LOW)
      btn_status_right = BTN_NOSURE;
    break;
  case BTN_PRESSING:
    if (vr == HIGH) {
      btn_status_right = BTN_RELEASE;
      btn_right_changed = 1;
    }
    break;
  }

  timer1_write(50000); // 10ms
}

int read_button() {

  /*
  display.setColor(BLACK);
  display.fillRect(0, 54, 21, 10);
  display.setColor(WHITE);
  if (vl == HIGH)
    display.drawString(0, 54, "0");
  else
    display.drawString(0, 54, "1");
  if (vr == HIGH)
    display.drawString(11, 54, "0");
  else
    display.drawString(11, 54, "1");
  display.display();
  */
  return 0;
}

static int need_to_redraw = 0;
char msg[128];
long last_read = 0;
int read_delay = 2000;
long last_btn_read = 0;
int btn_read_delay = 10;
long last_mqtt = 0;
int mqtt_delay = 10000;

const char *func_list[] = {"< Turnoff Disp", "< Restart", "< CO2 Measure",
                           "< MQTT Update", "< PMS Detail"};
static int current_func = 0;
static int func_count = 5;

int i;

void print_pms_data() {
  display.setColor(BLACK);
  display.fillRect(0, 13, 128, 64 - 13);
  display.setColor(WHITE);
  display.setFont(ArialMT_Plain_10);
  char buf[32];
  if (pms_data.TEMP) {
    sprintf(buf, "PM1.0: %d ug/m3", pms_data.PM_AE_UG_1_0);
    display.drawString(0, 13, buf);
    sprintf(buf, "PM2.5: %d ug/m3", pms_data.PM_AE_UG_2_5);
    display.drawString(0, 23, buf);
    sprintf(buf, "PM10 : %d ug/m3", pms_data.PM_AE_UG_10_0);
    display.drawString(0, 33, buf);
    sprintf(buf, "HCHO : %.3f mg/m3", pms_data.HCHO / 1000.0f);
    display.drawString(0, 43, buf);
    sprintf(buf, "TEMP: %.1f`C HUMD: %.1f%%", pms_data.TEMP / 10.0f,
            pms_data.HUMD / 10.0f);
    display.drawString(0, 53, buf);
  } else {
    display.drawString(0, 13, "PMS NO DATA");
  }
  display.display();
}

void loop() {
  if (settingMode) {
    dnsServer.processNextRequest();
  }
  webServer.handleClient();
  if (WiFiStatus && mqtt_server != "" && !client.connected()) {
    reconnect();
    need_to_redraw = 1;
  }
  if (client.connected()) {
    client.loop();
  }

  pms.loop();

  long now = millis();
  // only read co2
  if (now - last_read > read_delay) {
    last_read = now;
    data_read();
    need_to_redraw = 1;
  }
  // if pms was ok
  if (pms.status == pms.STATUS_OK) {
    need_to_redraw = 1;
  }

  if (now - last_mqtt > mqtt_delay) {
    last_mqtt = now;
    if (MQTTStatus && WiFiStatus)
      mqtt_publish();
  }

  if (btn_left_changed) {
    if (btn_status_left == BTN_PRESSING) {
      if (displaying && display_sence == 0) {
        current_func++;
        if (current_func >= func_count)
          current_func = 0;
        need_to_redraw = 1;
      } else if (displaying == 0) {
        display.displayOn();
        displaying = 1;
      } else if (display_sence == 1) {
        display_sence = 0;
        display.setColor(BLACK);
        display.fillRect(0, 0, 128, 64);
        display.setColor(WHITE);
        need_to_redraw = 1;
      }
    }
    btn_left_changed = 0;
  }

  if (btn_right_changed) {
    if (btn_status_right == BTN_PRESSING) {
      if (displaying && display_sence == 0) {
        switch (current_func) {
        case 0:
          display.displayOff();
          displaying = 0;
          break;
        case 1:
          S8.end();
          PMS_Serial.end();
          display.end();
          ESP.restart();
          break;
        case 2:
          last_read = now;
          data_read();
          need_to_redraw = 1;
          break;
        case 3:
          last_mqtt = now;
          if (MQTTStatus && WiFiStatus)
            mqtt_publish();
          break;
        case 4:
          display_sence = 1;
          need_to_redraw = 1;
        default:
          break;
        }
      } else if (displaying == 0) {
        display.displayOn();
        displaying = 1;
      } else if (display_sence == 1) {
        display_sence = 0;
        display.setColor(BLACK);
        display.fillRect(0, 0, 128, 64);
        display.setColor(WHITE);
        need_to_redraw = 1;
      }
    }
    btn_right_changed = 0;
  }

  if (need_to_redraw && display_sence == 0) {
    draw_statusbar();
    display.setFont(ArialMT_Plain_16);
    display.setColor(BLACK);
    display.fillRect(0, 13, 128, 32);
    display.setColor(WHITE);
    char buf[8];
    if (co2)
      sprintf(buf, "CO2: %ld ppm", co2);
    else
      sprintf(buf, "CO2: NODATA");
    display.drawString(0, 13, buf);
    if (pms_data.TEMP)
      sprintf(buf, "PM2.5: %d ug/m3", pms_data.PM_AE_UG_2_5);
    else
      sprintf(buf, "PMS: NODATA");
    display.drawString(0, 13 + 16, buf);
    if (co2) {
      display.setColor(BLACK);
      display.fillRect(96 - 4, 45, 32 + 4, 16 + 4);
      display.setColor(WHITE);
      display.drawRect(96 - 4, 45, 32 + 4, 16 + 3);

      if (co2 < 800) {
        display.drawFastImage(96 - 2, 45 + 2, 32, 16, lianghao);
      } else if (co2 < 1200) {
        display.drawFastImage(96 - 2, 45 + 2, 16, 16, jiao);
        display.drawFastImage(96 - 2 + 16, 45 + 2, 16, 16, gao);
      } else if (co2 < 2000) {
        display.drawFastImage(96 - 2, 45 + 2, 16, 16, guo);
        display.drawFastImage(96 - 2 + 16, 45 + 2, 16, 16, gao);
      } else if (co2 < 5000) {
        display.drawFastImage(96 - 2 + 16, 45 + 2, 16, 16, gao);
        display.drawFastImage(96 - 2, 45 + 2, 16, 16, chao);
      } else {
        display.drawFastImage(96 - 2, 45 + 2, 32, 16, siwang);
      }
    }
    // print current function
    display.setFont(ArialMT_Plain_10);
    display.setColor(BLACK);
    display.fillRect(0, 45, 80, 12);
    display.setColor(WHITE);
    display.drawString(0, 45, func_list[current_func]);
    display.display();
  } else if (need_to_redraw && display_sence == 1) {
    draw_statusbar();
    print_pms_data();
  }
}

boolean restoreConfig() {
  Serial.println("Reading EEPROM...");
  String ssid = "";
  String pass = "";
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i = 32; i < 96; ++i) {
      pass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(pass);
    WiFi.begin(ssid.c_str(), pass.c_str());

    uint16_t mqtt_port = 0;
    for (int i = 96; i < 128; ++i) {
      mqtt_server += char(EEPROM.read(i));
    }
    Serial.print("MQTT Server: ");
    Serial.println(mqtt_server);

    for (int i = 128; i < 160; ++i) {
      mqtt_username += char(EEPROM.read(i));
    }
    Serial.print("MQTT Username: ");
    Serial.println(mqtt_username);

    for (int i = 160; i < 192; ++i) {
      mqtt_passwd += char(EEPROM.read(i));
    }
    Serial.print("MQTT Password: ");
    Serial.println(mqtt_passwd);

    mqtt_port = EEPROM.read(193) << 8 | EEPROM.read(192);
    Serial.print("MQTT Port: ");
    Serial.println(mqtt_port);

    client.setServer(mqtt_server.c_str(), mqtt_port);
    return true;
  } else {
    Serial.println("Config not found.");
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while (count < 30) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      WiFiStatus = 1;
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  WiFiStatus = 0;
  return false;
}

void startWebServer() {
  if (settingMode) {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s =
          "<h1>Wi-Fi & MQTT Settings</h1><p>Please enter your password by "
          "selecting the SSID.</p>";

      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select "
           "name=\"ssid\">";
      s += ssidList;
      s += "</select><br><br>Password: <input name=\"pass\" length=64 "
           "type=\"password\">";
      s += "<br><br>MQTT Server: <input name=\"mqttserver\" length=32 "
           "type=\"text\">";
      s += "<br><br>MQTT Username: <input name=\"mqttusername\" length=32 "
           "type=\"text\">";
      s += "<br><br>MQTT Password: <input name=\"mqttpassword\" length=32 "
           "type=\"password\">";
      s += "<br><br>MQTT Port: <input name=\"mqttport\" length=32 "
           "type=\"number\" value=\"1883\">";

      s += "<input type=\"submit\">";
      s += "</form>";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      for (int i = 0; i < 194; ++i) {
        EEPROM.write(i, 0);
      }
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      Serial.println(pass);
      String mqtt_server = urlDecode(webServer.arg("mqttserver"));
      String mqtt_username = urlDecode(webServer.arg("mqttusername"));
      String mqtt_password = urlDecode(webServer.arg("mqttpassword"));
      String mqtt_port = urlDecode(webServer.arg("mqttport"));
      Serial.print("MQTT Server/Username/Password/Port: ");
      Serial.println(mqtt_server);
      Serial.println(mqtt_username);
      Serial.println(mqtt_password);
      Serial.println(mqtt_port);

      Serial.println("Writing SSID to EEPROM...");
      for (uint i = 0; i < ssid.length(); ++i) {
        EEPROM.write(i, ssid[i]);
      }

      Serial.println("Writing Password to EEPROM...");
      for (uint i = 0; i < pass.length(); ++i) {
        EEPROM.write(32 + i, pass[i]);
      }

      Serial.println("Writing MQTT Infomation to EEPROM...");
      for (uint i = 0; i < mqtt_server.length(); ++i) {
        EEPROM.write(96 + i, mqtt_server[i]);
      }
      for (uint i = 0; i < mqtt_username.length(); ++i) {
        EEPROM.write(128 + i, mqtt_username[i]);
      }
      for (uint i = 0; i < mqtt_server.length(); ++i) {
        EEPROM.write(160 + i, mqtt_password[i]);
      }
      uint16_t port = (uint16_t)mqtt_port.toInt() & 0xFFFF;
      EEPROM.write(192, (uint8_t)(port & 0xFF));
      EEPROM.write(193, (uint8_t)((port >> 8) & 0xFF));

      EEPROM.commit();
      Serial.println("Write EEPROM done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Settings", s));
      delay(500);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  } else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>AirSniffer Webpages</h1><p><a href=\"/reset\">Reset "
                 "All Settings</a></p>";
      if (co2) {
        s += "<p>CO2: ";
        s += co2;
        s += " ppm</p>";
      }
      if (pms_data.TEMP) {
        s += "<p>PM1.0: ";
        s += pms_data.PM_AE_UG_1_0;
        s += " ug/m3</p>";
        s += "<p>PM2.5: ";
        s += pms_data.PM_AE_UG_2_5;
        s += " ug/m3</p>";
        s += "<p>PM10 : ";
        s += pms_data.PM_AE_UG_10_0;
        s += " ug/m3</p>";
        s += "<p>HCHO: ";
        s += pms_data.HCHO / 1000.0f;
        s += " mg/m3</p>";
        s += "<p>TEMP: ";
        s += pms_data.TEMP / 10.0f;
        s += " `C</p>";
        s += "<p>HUMD: ";
        s += pms_data.HUMD / 10.0f;
        s += " %</p>";
      }
      webServer.send(200, "text/html", makePage("AirSniffer", s));
    });
    webServer.on("/reset", []() {
      for (int i = 0; i < 194; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      String s = "<h1>Settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Settings", s));
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(53, "*", apIP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s +=
      "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}