#include <SoftwareSerial.h> //跟ESP8266通訊用
#include <WiFiEsp.h> //連接wifi跟網頁伺服器用

SoftwareSerial ESP8266(3, 4);  //設定序列埠物件 (TX, RX)

#define WIFI_SSID "CCClass"  //填入WiFi帳號
#define WIFI_PASSWORD "CCClass123"  //填入WiFi密碼

int WiFi_Status = WL_IDLE_STATUS; //ＷiFi狀態 WL:連線 IDEL:閒置
WiFiEspClient espClient;  //設定WiFiEspClient物件
WiFiEspServer server(80); //設定網頁伺服器物件

String reqString = "";  //網頁請求資料
int brightness = 0; //呼吸燈亮度狀態
int fadeAmount = 5; //每次漸變的量
int delayDuration = 30; //脈衝間隔
int ledPin = 5; //LED腳位

void setup() {
  Serial.begin(9600); //設定序列埠傳輸速率(9600bps)
  pinMode(ledPin, OUTPUT); // 設定燈光輸出
  wifi_setting(); //wifi設定
  server.begin(); //啟動網頁伺服器
}

void wifi_setting () {
  ESP8266.begin(9600); //設定ESP8266傳輸速率(9600bps)
  WiFi.init(&ESP8266); //初始化ESP模組
  Serial.print("進行WiFi設定!\r\n");
  
  do{ //不管wifi狀態如何，都得執行一次wifi連接
     Serial.println("WiFi 連接中 ...");
     WiFi_Status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //更新wifi連接狀態
     delay(500);
  } while (WiFi_Status != WL_CONNECTED); //若wifi狀態屬沒連接，則重複迴圈
  
  Serial.println("ＷiFi 連接成功!");
  Serial.print("IP 位址:");
  Serial.println(WiFi.localIP()); //輸出Arduino所獲得的IP地址，務必記住！
  Serial.println("WiFi 設定結束\r\n");
  
  doBreathLED(); //通過呼吸燈通知wifi已連接成功
}

void doBreathLED () {
  do {
    analogWrite(ledPin, brightness); //發送脈衝
    brightness = brightness + fadeAmount; //更新亮度狀態
    
    if (brightness <= 0 || brightness >= 255) fadeAmount = -fadeAmount ; //更新正負
    
    delay(delayDuration); //脈衝間隔
  } while (brightness != 0); //製作堵塞回圈，使呼吸燈完成一個輪迴
  
  digitalWrite(ledPin, LOW); //使LED完全熄滅
}

void doBlinkLED() {
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
}

void loop() {
  WiFiEspClient client = server.available(); //建立客戶端與伺服端連線
  
  if (client) {  //若連線有效   
    while (client.connected()) { //當客戶端與伺服端連線
      if (client.available()) { //若客戶端的請求是有用的
        char c = client.read();
        reqString += c; //把請求資料合併
        
        if (c == '\n') { //當請求資料收完
          if (reqString.indexOf( "?breath" ) > 0) { //檢查有沒有 ?flash
            for (int i = 0; i < 3; i++) doBreathLED();
            Serial.println( "Led Flash" );
            
            client.print(  //伺服端回覆客戶端停止連線
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "\r\n");
            break ;
          } else if (reqString.indexOf( "?blink" ) > 0) { //檢查有沒有 ?flash
            for (int i = 0; i < 3; i++) doBlinkLED();
            Serial.println( "Led Blink" );
            
            client.print(  //伺服端回覆客戶端停止連線
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "\r\n");
            break ;
          }
          
          //伺服端傳送HTML網頁給客戶端
          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<meta charset=UTF-8><title>Arduino Web Server</title><script>function flash2Arduino(e){var t;(t=window.XMLHttpRequest?new XMLHttpRequest:new ActiveXObject('Microsoft.XMLHTTP')).open('GET','?'+e,!0),t.send()}</script><div align=center><h1>呼叫機器</h1><button onclick='flash2Arduino(\"breath\")'type=button>呼吸</button> <button onclick='flash2Arduino(\"blink\")'type=button>閃爍</button></div>\r\n");
          break ;
        }
      }
    }
    
    delay(10);  //預留傳送時間
    client.stop();  //停止這次連線
    reqString = "";  //清空請求資料
  }
}
