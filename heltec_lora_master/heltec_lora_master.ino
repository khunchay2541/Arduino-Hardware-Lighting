//***************** TEST LoRa Hitech ESP32 LED *********************************

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
//              #include <WiFi.h>
//              #include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include<Arduino.h>

SSD1306 display(0x3c, 4, 15);

#define SS 18
#define RST 14
#define DI0 26
///////////////////////////////////////////////////////////////////////////////////////////////

// ***** ส่วนตรงนี้กำหนดช่อง WIFI เพื่อส่งขึ้น Cloud  (อาจใช้ตัว router WIFI 4G ได้เลย) *****************

#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>

// Set these to run example.
#define FIREBASE_HOST "https://project-lighting-status-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "gFL2cICRlkLZkpbTP598HKQQ3KdJpvVBqmUfJPGY"
#define WIFI_SSID "WiFiname"
#define WIFI_PASSWORD "password"

///////////////////////////////////////////////////////////////////////////////////////////////

#define LED_PIN 22
//-------------------------------------------------------------------------------------------

// ส่วนตรงนี้กำหนด LoRA ความถี่ที่ใช้ในไทย
// และค่าความแรงและอื่นๆ
#define BAND 923000000.00
#define spreadingFactor 9
#define SignalBandwidth 62.5E3
#define SignalBandwidth 31.25E3
#define SignalBandwidth 125E3
#define preambleLength 8
#define codingRateDenominator 8

/////////////////////////////////////////////

//ตรงนี้กำหนดโปรแกรมใช้งาน WIFI และ MQTT

//////////////////////////////////////////////

long lastMsg = 0;
byte counter = 0;     // ใช้เริ่มนับสแกน แต่ละตัวรับ LoRA
byte node_num = 0;    // จำนวน โนดทั้งหมด (มี สองโหมด คือ โหมดศูนย์ มีเลขอุปกรณ์คือ 0..31 และโหมดหนึ่ง มีเลขอุปกรณ์คือ 32...63)
byte end_counter = 0; // เก็บหมายเลขสุดท้ายของจำนวนโหนดที่เลือกไว้ โดย node_num
String data;
String scan_num;
     //ขนาดของอะเรย์ต่อไปนี้จะต้องมีขนาดที่แปลตามจำนวนเสาหรือข้อมูลที่เราต้องการใช้ ปัจจุบันตัวอย่างจำนวน 2 โนด โดยโนดหมายเลข "0" เป็นการวัดค่าต่างๆ เช่น อุณหภูมิ ที่สถานี
     // และ หมายเลข "1" เป็น เสาต้นที่หนึ่ง ซึ่งมีค่าที่ต้องการวัดคือ Volt, Amp ส่งกลับมาเพื่อประมวลผลค่าต่างๆอีกที เช่น หากมีการเปิดไฟส่องสว่างแต่อ่านค่ากระแสได้น้อยมาก
     // หรือเท่ากันศูนย์ก็แสดงว่าหลอดขาด, หรือไม่มี Volt ก็แสดงว่าสายไฟขาด, หรือหากยังไม่มีการเปิดไฟส่องสว่างแต่กลับวัดได้ค่ากระแสไหลในระดับหนึ่งก็แสดงว่าไฟรั่ว ที่เสานั้น
     // เหล่านี้เป็นต้น ส่วนกรณีตัวโมดูลวัดเสียหายไม่ทำงาน ตัวแม่นี้จะกำหนดคำว่า "DOWN" ขึ้นสู่ MQTT Cloud เพื่อแสดงให้ทราบว่าตัววัดเสียหายไม่ทำงาน..
     // โดยตัวแปร V[..], A[..], S[..] ก็คือ Volt, Amp และ Status ของตัวโมดูลวัด ตามลำดับ
String V[64]={"V00","V01","V02","V03","V04","V05","V06","V07","V08","V09","V10","V11","V12","V13","V14","V15",
              "V16","V17","V18","V19","V20","V21","V22","V23","V24","V25","V26","V27","V28","V29","V30","V31",
              "V32","V33","V34","V35","V36","V37","V38","V39","V40","V41","V42","V43","V44","V45","V46","V47",
              "V48","V49","V50","V51","V52","V53","V54","V55","V56","V57","V58","V59","V60","V61","V62","V63"};
String A[64]={"A00","A01","A02","A03","A04","A05","A06","A07","A08","A09","A10","A11","A12","A13","A14","A15",
              "A16","A17","A18","A19","A20","A21","A22","A23","A24","A25","A26","A27","A28","A29","A30","A31",
              "A32","A33","A34","A35","A36","A37","A38","A39","A40","A41","A42","A43","A44","A45","A46","A47",
              "A48","A49","A50","A51","A52","A53","A54","A55","A56","A57","A58","A59","A60","A61","A62","A63"};
String S[64]={"S00","S01","S02","S03","S04","S05","S06","S07","S08","S09","S10","S11","S12","S13","S14","S15",
              "S16","S17","S18","S19","S20","S21","S22","S23","S24","S25","S26","S27","S28","S29","S30","S31",
              "S32","S33","S34","S35","S36","S37","S38","S39","S40","S41","S42","S43","S44","S45","S46","S47",
              "S48","S49","V50","S51","S52","S53","S54","S55","S56","S57","S58","S59","S60","S61","S62","S63"};
String Station_sensor[2]={"Temp","Humi"};    //กำหนดขนาดให้แปลเปลี่ยนไปตามจำนวนค่าที่เราต้องการวัดที่สถานีได เช่น เพื่อค่า PM 2.5 เป็นต้น
char json_char6[500];      //ตรงนี้ อาจต้องเพิ่มขนาดหากเราต้องการวัดค่ามากขึ้น หรือเพิ่มจำนวนเสาไฟมากขึ้น แต่ต้องระวังหน่วยความจำไม่พอ
String json_format6;      //กำหนดรูปแบบ json ส่ง MQTT server
char json_char1[500];      //ตรงนี้ อาจต้องเพิ่มขนาดหากเราต้องการวัดค่ามากขึ้น หรือเพิ่มจำนวนเสาไฟมากขึ้น แต่ต้องระวังหน่วยความจำไม่พอ
String json_format1;      //กำหนดรูปแบบ json ส่ง MQTT server
char json_char2[500];      //ตรงนี้ อาจต้องเพิ่มขนาดหากเราต้องการวัดค่ามากขึ้น หรือเพิ่มจำนวนเสาไฟมากขึ้น แต่ต้องระวังหน่วยความจำไม่พอ
String json_format2;      //กำหนดรูปแบบ json ส่ง MQTT server
char json_char3[500];      //ตรงนี้ อาจต้องเพิ่มขนาดหากเราต้องการวัดค่ามากขึ้น หรือเพิ่มจำนวนเสาไฟมากขึ้น แต่ต้องระวังหน่วยความจำไม่พอ
String json_format3;      //กำหนดรูปแบบ json ส่ง MQTT server
char json_char4[500];      //ตรงนี้ อาจต้องเพิ่มขนาดหากเราต้องการวัดค่ามากขึ้น หรือเพิ่มจำนวนเสาไฟมากขึ้น แต่ต้องระวังหน่วยความจำไม่พอ
String json_format4;      //กำหนดรูปแบบ json ส่ง MQTT server
char json_char5[500];      //ตรงนี้ อาจต้องเพิ่มขนาดหากเราต้องการวัดค่ามากขึ้น หรือเพิ่มจำนวนเสาไฟมากขึ้น แต่ต้องระวังหน่วยความจำไม่พอ
String json_format5;      //กำหนดรูปแบบ json ส่ง MQTT server
bool sw_mode = false;   // กำหนด Mode "0" เป็นค่า default ก่อน คือ หมายเลขเครื่องจะเป็น 0..31

//---------------------- SETUP ----------------------------------------------------------------------------------------------------------------
void setup() {
/////////////////////////////
   Serial.begin(9600);

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
   
  //------------ Lora setup --------------------------------
  pinMode(25,OUTPUT); //Send success, LED will bright 1 second
  //set for thump wheel switch
  pinMode(36, INPUT_PULLUP);
  pinMode(37, INPUT_PULLUP);
  pinMode(38, INPUT_PULLUP);
  pinMode(39, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
  pinMode(33, INPUT_PULLUP);
  pinMode(0, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);
  node_num = 2 ;  // อ่านจำนวนทั่งหมดของโนด ที่จะต้องแสกน  (อาจกำหนดเองตายตัวเลยก็ได้)
  //Serial.begin(9600);
  while (!Serial); //If just the the basic function, must connect to a computer
  // pin for select mode
  pinMode(2,INPUT_PULLUP);
  if(digitalRead(2)){counter = 0; sw_mode = false;}  // อ่านค่า โมดสวิทช์ ( mode 0, 1)
  else {counter = 32; sw_mode = true;}
  end_counter = counter + node_num +1;

  Serial.println(counter);
  Serial.println(end_counter);
  Serial.println("................................");


// Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5,5,"LoRa Sender");
  display.display();
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  Serial.println("LoRa Sender");
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.print("LoRa Spreading Factor: ");
  Serial.println(spreadingFactor);
  LoRa.setSpreadingFactor(spreadingFactor);
  Serial.print("LoRa Signal Bandwidth: ");
  Serial.println(SignalBandwidth);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setCodingRate4(codingRateDenominator);
  LoRa.setPreambleLength(preambleLength);
  Serial.println("LoRa Initial OK!");
  display.drawString(5,20,"LoRa Initializing OK!");
  display.display();
  delay(2000);
}
//--------------------------- LOOP MAIN -------------------------------
void loop() {  

     /////===========  LOOP RECIEVED DATA FROM LORA (POLE) FOREVER =======================
  //[1] send request node 
  //== Show on Serial debug
      Serial.print("Sending node number: ");
      Serial.println(counter); 

  //== show Node number on LED ==
      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.drawString(3, 5, "Sending node : ");
      display.drawString(50, 30, String(counter));
      display.display();
  
  // ======== Send Node number on Lora ===================
      LoRa.beginPacket();
      LoRa.print(counter);
      LoRa.endPacket(); 
  
 //[2]=============== wait for response 3 seccons ====================
// try to parse packet
      int count_time = 0;
      bool flag_time = false;
   
    while(count_time < 350 && !flag_time)  // ค่า count_time จะใช้สำหรับรอค่าที่มาจากเสาไฟฟ้าที่ต้องการติดต่อด้วยค่าโดยประมาณ 4-5 วินาที หากพ้นเวลานี้ถือว่าเสีย
        { 
        int packetSize = LoRa.parsePacket();
          if (packetSize) {    
                    display.clear();
                    display.setFont(ArialMT_Plain_16);
                    display.drawString(3, 0, "Received Ack"); // <= Show on LED
                    display.display();
                    while (LoRa.available()) 
                         {
                           data = LoRa.readString();
                           Serial.print("data is : ");
                           Serial.println(data);       // <= Show on Serial Debug
                           display.display();
                          }
                   scan_num = String(counter);
                   if (scan_num.length()==1) scan_num = "0"+scan_num; //เพิ่ม ศูนย์ ไว้ข้างหน้าอีกหนึ่งหลัก กรณี หมายเลขเป็น 0-9
                   if (data.substring(0,2).equals(scan_num)) //หากค่าที่รับได้ตรงหมายเลขที่เรียกไปก็ดำเนินการส่ง MQTT ต่อไป
                        {
                          Serial.println("  OK..rev data..............................!");
                          data = data.substring(2,data.length());
                          display.drawString(20,22,data);  //<== Show on LED
                          display.display();
                          delay(1000);
                          display.drawString(20,22,"           ");
                          display.drawString(35,22,"           ");
                          display.display();
                          flag_time = true;
                        }
                  }
        count_time++;
        Serial.print(" cout_time = ");    //ตรงนี้ไม่ต้องตัดออกเพราะใช้ ชลอการอ่านการตอบสนองของเสาไฟที่ต้องการติดต่อด้วย (แทนการใช้ delay())
        Serial.println(count_time);
      }
          // ตรวจสอบก่อนว่า รอเกินเวลาหรือเปล่า หากเกินก็แสดงผลออก LED ว่า โนดเสียหายแล้ว (Node down...)
   if(count_time >= 350){
        Serial.println(" Node down...!");
        Serial.println(" Scan next node...");
        display.drawString(20,43,"            ");
        display.drawString(20,43," Node down..");
        display.display();
        delay(1000);
        display.drawString(20,43,"            ");
        display.display();
        S[counter]="Down";
        V[counter]= "None";
        A[counter]= "None"; 
        
        if(counter == 1){
          Firebase.set("node1/1/V",V[1]);
          Firebase.set("node1/1/A",A[1]);
          Firebase.set("node1/1/S",S[1]);
        }  else if(counter == 2){
          Firebase.set("node1/2/V",V[2]);
          Firebase.set("node1/2/A",A[2]);
          Firebase.set("node1/2/S",S[2]);
        }  else if(counter == 3){
          Firebase.set("node1/3/V",V[3]);
          Firebase.set("node1/3/A",A[3]);
          Firebase.set("node1/3/S",S[3]);
          
                                     }   
                    }else{         // หากไม่ใช่ก็ทำการส่ง MQTT ต่อไป
                            digitalWrite(25, HIGH); // turn the LED on (HIGH is the voltage level)
                            delay(1000); // wait for a second
      
                      //=========== manag data send to MQTT ===========================
                            if ((counter==0 && sw_mode == false)||(counter==32 && sw_mode == true)){ //กรณีเป็นตัวที่ ศูนย์ คือวัด อุณหภูมิ กับ ความชื่น ก็กำหนดให้เอาอักษรแสดงคือ 0-4 (00.00) และ 6-10 (00.00)
                               Station_sensor[0]=data.substring(0,5);
                               Station_sensor[1]=data.substring(6,11);
                            }else{   //กรณีวัดโวล์และกระแส ก็กำหนดให้เอาอักษรมาแสดงเป็น  0-5 (000.00) Volt และ 7-12 (000.00) Amp 
                                     V[counter]=data.substring(1,6);
                                     A[counter]=data.substring(8,12);
                                     S[counter]="OK";
                                     Serial.print(V[counter]);
                                     Serial.print(A[counter]);
                                     Serial.print(S[counter]);
                                      if (V[counter] ==  "999.9"){
                                        S[counter]="BAD";
                                      }else {
                                        S[counter]="OK";
                                      }
                                     
                                 
                                }
                                // พิมพ์ค่าดูเฉยๆ
                               // Serial.print(" Station_sensor[0]= ");Serial.println(Station_sensor[0]);
                                //Serial.print(" Station_sensor[1]= ");Serial.println(Station_sensor[1]);
                                
                                //ตรงนี้กำหนดเพิ่มค่าที่ต้องการวัดที่สถานีได้ตามต้องการ โดยการเปลี่ยนแปลงขนาด Array
                                if (sw_mode == false){
                                    if(counter == 0){
                                        Firebase.set("node1/0/t",Station_sensor[0]);
                                        Firebase.set("node1/0/h",Station_sensor[1]);
                                     }else if(counter == 1){
                                       Firebase.set("node1/1/V",V[1]);
                                       Firebase.set("node1/1/A",A[1]);
                                       Firebase.set("node1/1/S",S[1]);
                                     }  else if(counter == 2){
                                       Firebase.set("node1/2/V",V[2]);
                                       Firebase.set("node1/2/A",A[2]);
                                       Firebase.set("node1/2/S",S[2]);
                                     }  else if(counter == 3){
                                       Firebase.set("node1/3/V",V[3]);
                                       Firebase.set("node1/3/A",A[3]);
                                       Firebase.set("node1/3/S",S[3]);
                                     }                          
                               }else{
                                    //--------------------------
                               }

                            delay(200);    
                            digitalWrite(25, LOW); // turn the LED off by making the voltage LOW
                            delay(1000); // wait for a second
                        }
     
   counter++;
  if (counter == end_counter && sw_mode == false) counter = 0;
  if (counter == end_counter && sw_mode == true) counter = 32;
 
   delay(300);
   // กลับขึ้นไปวนรับข้อมูลส่งขึ้น Cloud ในรอบต่อไป....
    }
