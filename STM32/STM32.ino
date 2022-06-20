#include <dht11.h>
#include <SoftwareSerial.h>
#define DHT11PIN PD6     // 定义DHT11的数据引脚

const int IA = PB14; // 电机IA口
const int IB = PB13; // 电机IB口
const int buzzer = PC7; // 蜂鸣器
const int SW = PA0; // 开关风扇按钮
const int modeSW = PC13; // 模式切换按钮
dht11 DHT11; // 定义温湿度传感器
bool fanMode = 0; // 0为开关控制模式，1为温度阈值控制开关模式
byte fanNow = 0; // 定义风扇当前状态，0为关，1为正向，2为逆向
bool isChange = false; // 接收到esp8266的指令后是否有改变某些值
int boundary = 20; // 温度阈值
byte speed1 = 255;  // L9110转速设为255
String str = ""; // 初始化为空串以接收串口信息
SoftwareSerial mySerial(PA10, PA9); // RX, TX
SoftwareSerial wifi(PB11, PB10); // wifi通信串口

void setup()
{
  //初始化
  mySerial.begin(9600); //设置波特率为9600
  wifi.begin(9600);
  pinMode(IA, OUTPUT); // 定义IA脚输出
  pinMode(IB, OUTPUT); // 定义IB脚输出
  pinMode(SW, INPUT); // 定义按钮为输入
  pinMode(buzzer, OUTPUT); // 定义蜂鸣器输出
  digitalWrite(buzzer, HIGH); // 有源蜂鸣器一通电就不停，要先停一下(低电平触发，高电平中止)
  attachInterrupt(SW, changeSW, RISING); //按下开关按钮时发生RISING，运行changeSW函数(中断函数)
  attachInterrupt(modeSW, changeModeSW, RISING); //按下模式切换按钮时发生RISING，运行changeModeSW函数(中断函数)
  wifi.listen(); // SoftwareSerial只能接收一个串口发来的数据，这里设置为wifi的串口
}

// count用于计数，通过计数来计时，每隔一段时间发送数据给esp8266
int count = 0;
void loop()
{
  if (count++ % 100 == 0) {
    String state = "temp:" + String(int(DHT11.temperature)) + " humid:" + String(int(DHT11.humidity));
    wifi.println(state);
    mySerial.print("已发送消息：");
    mySerial.println(state);
  }

  // 读取DHT11的信息，并根据"当前温度是否超过阈值与当前电机状态的关系"来更新isChange
  DHT11.read(DHT11PIN);
  if (fanMode == 1) {
    // 温度阈值控制模式
    if (DHT11.temperature >= boundary && !fanNow) {
      // 如果当前温度超过了阈值却没启动电机
      fanNow = 1;
      isChange = true;
      mySerial.println("温控模式：超过阈值温度——自启动电机");
    } else if (DHT11.temperature < boundary && fanNow) {
      // 如果当前温度低于了阈值却没停止电机
      fanNow = 0;
      isChange = true;
      mySerial.println("温控模式：低于阈值温度——自关闭电机");
    }
  }

  // 当有发生改变状态时才执行操作（节约运行开销）
  if (isChange) {
    if (fanNow == 0) {
      stop();
    } else if (fanNow == 1) {
      // 更改为顺时针转动
      forward();
    } else {
      // 更改为逆时针转动
      backward();
    }
    isChange = false;
  }

  if (wifi.available() > 0) {
    // 循环接收串口数据（来自ESP8266的指令）
    while (wifi.available() > 0) {
      delay(10);
      str += char(wifi.read());
    }

    // 这里处理8266传来的指令，strstr()仅支持char*的匹配，因此把str→dataArray→char*
    mySerial.print("收到8266的指令为：");
    char dataArray[str.length()];
    str.toCharArray(dataArray, str.length());
    char *data = dataArray;
    mySerial.println(str);

    if (strstr(data, "打开")) {
      isChange = true;
      fanNow = 1;
      mySerial.print("fanNow:");
      mySerial.println(fanNow);
    } else if (strstr(data, "关闭")) {
      isChange = true;
      fanNow = 0;
      mySerial.print("fanNow:");
      mySerial.println(fanNow);
    } else if (strstr(data, "温度阈值")) {
      isChange = true;
      boundary = str.substring(str.indexOf(":") + 1).toInt();
      mySerial.print("boundary:");
      mySerial.println(boundary);
    } else if (strstr(data, "风速")) {
      isChange = true;
      speed1 = str.substring(str.indexOf(":") + 1).toInt();
      mySerial.print("speed1:");
      mySerial.println(speed1);
    }  else if (strstr(data, "模式")) {
      isChange = true;
      fanMode = str.substring(str.indexOf(":") + 1).toInt();
      mySerial.print("fanMode:");
      mySerial.println(fanMode);
    } else if (strstr(data, "顺时针")) {
      isChange = true;
      fanNow = 1;
      mySerial.print("fanNow:");
      mySerial.println(fanNow);
    } else if (strstr(data, "逆时针")) {
      isChange = true;
      fanNow = 2;
      mySerial.print("fanNow:");
      mySerial.println(fanNow);
    }
    str = "";
  }
}

// 开关中断函数（仅在开关控制模式下能手动开关）
void changeSW() {
  isChange = true;
  if (fanNow == 0) {
    // 从关状态改为开状态默认为顺时针
    fanNow = 1;
  } else {
    fanNow = 0;
  }
}

// 模式切换中断函数
void changeModeSW() {
  isChange = true;
  fanMode = !fanMode;
}

// 逆时针旋转
void backward()
{
  analogWrite(IA, speed1);
  analogWrite(IB, 0);
  mySerial.println("风扇逆转");
  digitalWrite(buzzer, LOW); //发声音
  delay(50);//延时
  digitalWrite(buzzer, HIGH); //不发声音
}

// 顺时针旋转
void forward()
{
  analogWrite(IA, 0);
  analogWrite(IB, speed1);
  mySerial.println("风扇正转");
  digitalWrite(buzzer, LOW); //发声音
  delay(50);//延时
  digitalWrite(buzzer, HIGH); //不发声音
}

// 停止旋转
void stop()
{
  analogWrite(IB, 0);
  analogWrite(IA, 0);
  mySerial.println("风扇停止");
  digitalWrite(buzzer, LOW); //发声音
  delay(50);//延时
  digitalWrite(buzzer, HIGH); //不发声音
}
