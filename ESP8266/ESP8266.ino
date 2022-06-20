#define BLINKER_WIFI//通讯方式
#include <Blinker.h>
//#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"

//#define HOME
#define SCHOOL
//#define PHONE

#ifdef SCHOOL
char auth[] = "53acee473caf";//这里填写设备密钥
char ssid[] = "XBSN";//这里填写wifi
char pswd[] = "5e53)1T0";//这里填写wifi码
#endif

//暂存数据
int velocity = 0;
int num_dir = 0;

//flag,记录电源开关,0关，1开
int flag = 0;

//新建组件对象
BlinkerButton Gree_power("btn-pwr");//电源开关组件
BlinkerButton Gree_dir("btn-dir");//风向组件
BlinkerSlider Slider1("ran-wen");//风速调节滑块
BlinkerNumber temp_num("num-temp");//温度显示模块
BlinkerNumber humid_num("num-humid");//湿度显示模块

#ifdef SCHOOL
// Set your Static IP address
IPAddress local_IP(192, 168, 137, 66);
// Set your Gateway IP address
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

IPAddress primaryDNS(114, 114, 114, 114);
IPAddress secondaryDNS(8, 8, 8, 8);
//SoftwareSerial mySerial(3, 1);

//初始化
void setup()
{
  Serial.begin(9600);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("网络配置失败：STA Failed to configure");
  }

  BLINKER_DEBUG.stream(Serial);
  Gree_power.attach(Gree_power_callback);
  Gree_dir.attach(Gree_dir_callback);
  Slider1.attach(Slider1_callback);
  Blinker.attachHeartbeat(heartbeat);
  Blinker.attachData(dataRead);
  Blinker.begin(auth, ssid, pswd);
}

String str1 = "";
void loop()
{
  Blinker.run();
  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      delay(20);
      str1 += char(Serial.read());
    }
    // 温湿度报文格式temp:xx humid:xx(接收来自stm32温湿度反馈)
    if (str1.substring(0, 4) == "temp") {
      if (!flag) {
        temp_num.color("#FFF700");
        humid_num.color("#00DDFF");
      } else {
        temp_num.color("#595959");
        humid_num.color("#595959");
      }
      temp_num.print(str1.substring(5, 7).toInt());
      humid_num.print(str1.substring(14).toInt());
    }
    str1 = "";
  }
}

// 输入框输入的数据由这个回调函数获取
void dataRead(const String & data)
{
  /*
     strstr(a,b)可以在a中查找b的子串，找不到就返回null
     但是a和b的类型都得是char指针，所以要数据类型转换
     data不能直接转为char*，需要先转为charArray才可以
     data转为charArray又要从const String先转为String
  */
  String str = String(data) + " ";
  char dataarray[data.length()];
  char *datachar = dataarray;
  str.toCharArray(datachar, str.length());
  String datanum = num(str);

  if (strstr(datachar, "关闭")) {
    Serial.println("关闭");
  } else if (strstr(datachar, "打开")) {
    Serial.println("打开");
  } else if (strstr(datachar, "阈值")) {
    String bound = datanum ? "温度阈值:" + datanum : "";
    if (bound != "") {
      Serial.println(bound);
    }
  } else if (strstr(datachar, "模式")) {
    String bound = datanum ? "模式:" + datanum : "";
    if (bound != "") {
      Serial.println(bound);
    }
  }
}

// 提取数字
String num(String &str) {
  String ans = "";
  for (int q = 0; q < str.length(); q++)
  {
    if (str[q] >= '0' && str[q] <= '9')
    {
      ans += str[q];
    }
  }
  if (ans != "") {
    return ans;
  } else {
    return "";
  }
}

//电源开关
void Gree_power_callback(const String &state)
{
  // BLINKER_LOG("开关当前状态: ", state);

  if (state == BLINKER_CMD_BUTTON_TAP)
  {
    if (!flag) {
      flag = 1;
      Gree_power.color("#00FF00");
      Slider1.color("#0000FF");
      Gree_dir.color("#00FFFF");
      temp_num.color("#FFF700");
      humid_num.color("#00DDFF");
      Gree_power.text("开启");
      Serial.println("打开");
    } else {
      flag = 0;
      Gree_power.color("#595959");
      Slider1.color("#595959");
      Gree_dir.color("#595959");
      temp_num.color("#595959");
      humid_num.color("#595959");
      Gree_power.text("关闭");
      Serial.println("关闭");
    }
  }
  
  //print可以更新界面图标
  temp_num.print();
  humid_num.print();
  Gree_dir.print();
  Gree_power.print();
  Slider1.print();
}

//风速、风向心跳包
void heartbeat()
{
  switch (num_dir)
  {
    case 0:
      Gree_dir.text("逆时针");
      break;

    case 1:
      Gree_dir.text("顺时针");
      break;
  }
}

//风向调节
void Gree_dir_callback(const String & state)
{
  if (state == BLINKER_CMD_BUTTON_TAP)
  {
    num_dir = !num_dir;
    switch (num_dir)
    {
      case 0:
        Gree_dir.text("逆时针");
        Gree_dir.icon("fas fa-undo-alt");
        if (flag) {
          Gree_dir.color("#00FFFF");
        } else {
          Gree_dir.color("#595959");
        }
        Serial.println("逆时针");
        break;

      case 1:
        Gree_dir.text("顺时针");
        Gree_dir.icon("fas fa-redo-alt");
        if (flag) {
          Gree_dir.color("#00FFFF");
        } else {
          Gree_dir.color("#595959");
        }
        Serial.println("顺时针");
        break;
    }
    Gree_dir.print();
  }
}

//风扇风速(电机有个电机死区，这个区域内的PWM频率不会使电机转动)
void Slider1_callback(int32_t value)
{
  String str = "风速:" + String(value);
  velocity = value;
  if (flag) {
    Slider1.color("#0000FF");
  } else {
    Slider1.color("#595959");
  }
  Slider1.print(velocity);
  Serial.println(str);
}
