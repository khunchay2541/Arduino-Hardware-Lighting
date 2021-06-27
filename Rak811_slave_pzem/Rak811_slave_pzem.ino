/********************************************************
 * This demo is only supported after RUI firmware version 3.0.0.13.X on RAK811
 * Master Board Uart Receive buffer size at least 128 bytes. 
 * Board:   number "0" or "32"
 * Measure: Temp & Huminity by DHT11
 ********************************************************/
//node 1
#include "RAK811.h"
#include "SoftwareSerial.h"
//#include "DHT.h"
#include <PZEM004Tv30.h>

PZEM004Tv30 pzem(11, 12); //tx rx

#define DHTTYPE DHT11 
#define DHTPIN 2
#define TXpin 5   // Set the virtual serial port pins
#define RXpin 4
#define ATSerial Serial
SoftwareSerial DebugSerial(RXpin,TXpin);    // Declare a virtual serial port

RAK811 RAKLoRa(ATSerial,DebugSerial);
//DHT dht(DHTPIN, DHTTYPE);



//for Sw set number of box
//const int sw_mode_pin = 11;   

void f2a(float number, char* dest){
 char num[20];
 int last_index, st;
  dtostrf(number,3,2,num);
 char buffer[4*sizeof(num)]; //sized for the worst case scenario of each being in the hundreds plus a space between each and a null
 char* buffPtr = buffer;
 
 for(byte i = 0; i < sizeof(num) - 1; i++){
   itoa((int)num[i],buffPtr,16); //convert the next character to a string and store it in the buffer
   buffPtr += strlen(buffPtr); //move on to the position of the null character
   *buffPtr = ' '; //replace with a space
 }
 buffPtr--; //move back a character to where the final space (' ') is
 *buffPtr++ = '\0'; //replace it with a null to terminate the string
 
 for (int i = 0;i<12;i++){
  if (buffer[i]=='e') last_index = i;
 }
 for (int i = 0;i<last_index+5;i++){
  dest[i]=buffer[i];
 }
}


char send_lora[40];
int i_a, i_v;  
float v, a; 
String a_str, v_str;
int ind_a, ind_v;
char result[12];
char result1[12];
bool sw_mode = false;
String own_number;
//================== SUTUP ===========================
void setup() {
  //pinMode(sw_mode_pin,INPUT_PULLUP);
  DebugSerial.begin(9600);
  while(DebugSerial.available())
  {
    DebugSerial.read(); 
  }
//  dht.begin();
  ATSerial.begin(9600); 
  DebugSerial.println("Start rev....");

    sw_mode = true;
    own_number = "31";
    DebugSerial.println(" Mode 0 ");
    
}


void loop() {  

 //---------ex pzem
 
float voltage = pzem.voltage();
    Serial.print("Voltage: "); Serial.print(voltage); Serial.println("V");
float current = pzem.current();
    Serial.print("Current: "); Serial.print(current); Serial.println("A");

//-----------------------------

   String ret = RAKLoRa.rk_recvP2PData();
    if(ret != NULL)
    {   
      
    //  if((ret.substring(ret.length()-2,ret.length()))=="30") // <=== NUMBER OF STATION NODE
     if((ret.substring(ret.length()-2,ret.length()))== own_number) // <=== NUMBER OF STATION NODE    
         { 
             // ๑) อ่าน อุณหภูมิ
              delay(2000);
             // h = dht.readHumidity();
             // t = dht.readTemperature();
                  // ที่ให้ voltage >1 เพราะ ในกรณีที่ volt เป็น NAN พอตั้งเงื่อนไข != NAN แล้ว พอปิดเครื่องได้ค่า nan มาแล้วโปรแกรมไม่ทำงานคำสั่ง else
                  if (voltage > 1){
                    a = pzem.current();
                    v = pzem.voltage();
                  } else {
                    a = 9.99;
                    v = 999.99;
                   }
             
              // ๒)เปลี่ยนเป็น สตริงและหาความยาวไว้
              v_str = String(v);
              Serial.print(v_str);
              ind_v = v_str.length();
              a_str = String(a);
              Serial.print(a_str);
              ind_a = a_str.length();
            
             // ๒)ใช้ ฟังชั่นก์ f2a เพื่อเปลี่ยนค่าเป็น ascii code
              f2a(v, result);
            
             // ๓)ใส่ระหัสศูนย์ที่ท้าย อาเรย์ เพราะค่าที่ได้เดิมจะไม่มีกรณีตัวท้ายมีค่าเป็นศูนย์
           
              f2a(a, result1);
        
              // ๔) จัดการเรื่องฟอร์แมทเพื่อส่งผ่าน Lora (ต้องไม่เกิน 64 byte) 
      //  ฟอร์แมท์ คือ  [หมายเลขตนเอง][เว้นวรรค][อุณหภูมิ (สมมุติ 4 ตัว)][เว้นวรรค][ความชื่น (สมมุติ 3 ตัว)][เว้นวรรค]['\0'] 
      //  ตัวอย่างเช่น สมมุติตัวเราเป็นเหมายเลข "0" เราอ่าน อุณภูมิได้ 22.3 ความชื่น 44 เราก็จะได้ผลลัพท์ค่าที่เราจะส่งกลับคือ (ASCII)
     //            [00]      [ ]                  [22.2]              [ ]          [44]          [ ]   ['\0']
     //  ['3' '0' '3' '0']['2' '0']['3' '2' '3' '2' '2' 'e' '3' '3']['2' '0']['3' '4' '3' '4']['2' '0']['\0']
              // เริ่มข้อมูลส่งกลับโดย หมายเลขเบอร์ตนเอง ( 00, 32)
             if(sw_mode){
              send_lora[0]='3';send_lora[1]='0';   //
              send_lora[2]='3';send_lora[3]='1';   //  number 1 nodeที่ 1
              send_lora[4]='2';send_lora[5]='0';   //
             }else{
              send_lora[0]='3';send_lora[1]='3';  //
              send_lora[2]='3';send_lora[3]='2';  //  number 32
              send_lora[4]='2';send_lora[5]='0';  //
              
             }
                //จากนั้นก็เติมต่อ ด้วยข้อมูล tmp (5 หลัก) & Humi ( 6 หลัก) ก่อนส่งค่าผ่าน LoRa กลับไปสู่ตัวแม่
                  //ใส่ค่าอุณหภูมิ
              for(i_v = 6;i_v<(6+ind_v*2);i_v++) 
                         send_lora[i_v]=result[i_v-6];
                 // เว้นวรรด หนึ่งอักษร
              send_lora[i_v]='2';send_lora[i_v+1]='0';
              for(i_a = i_v+2;i_a<((i_v+2)+ind_a*2);
                          i_a++) send_lora[i_a]= result1[i_a-(i_v+2)];
                 // ปิดท้ายด้วย เว้นวรรคและ "\0" เพื่อให้เป็น String
              send_lora[i_a]='2';
              send_lora[i_a+1]='0';
              send_lora[i_a+2]='\0';
             
             
              DebugSerial.println(send_lora);
              DebugSerial.println("====================");
             RAKLoRa.rk_sendP2PData(send_lora);
          if (RAKLoRa.rk_sendP2PData(send_lora))   // print out to check if send success
            {   
                String ret = RAKLoRa.rk_recvP2PData();
                if(ret != NULL)
                   {     
                     DebugSerial.println(ret);
                   }
            }
         }
         DebugSerial.println(ret.substring(ret.length()-2,ret.length())); // print out input number 
    }
}
