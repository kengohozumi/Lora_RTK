//arduino R3は3.3V/5Vどちらでも動く　arduino R4は5vじゃないと動かない
//#include "Arduino_LED_Matrix.h"はNGになる
//#include <zlib_turbo.h>はGzipの解答する関数しかない
//---------------------------------------------------------------------
//    FLINT E220-900T22S-JP Library  demo
//        文字列の送受信　受信時RSSIとSNRを表示
//    2023/10/08 
//    for ArduinoIDE 2.2.1以降
//
// Note: E220-900T22S (JP) is a module customized for Japan.
//       This library does not work with E220-900T22S.
//
//    Copyright (c) 2023 flint.works　　https://flint.works/
//    Released under the MIT license
//    https://opensource.org/licenses/mit-license.php
//
//---------------------------------------------------------------------

//参考https://github.com/yoronneko/qzsl6tool/blob/main/python/rtcmread.py

#include "FLINT_E220_900T22S_JP.h"
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);

//for Arduino Uno R3 ----------------------------------------------------
// ArduinoIDE [ツール]→[ボード]→[Arduino Uno]
// FLINT LoRa無線シールド(E220-900T22S for Arduino) SW9=D5/D6
//シールドピン設定
#define RX_pin 6
#define TX_pin 5
#define Lora_power_pin 11//?
#define Aux_status_pin 3
#define M0_pin 11
#define M1_pin 10
#include <SoftwareSerial.h>
SoftwareSerial mySerial(RX_pin, TX_pin); // Arduino RX <-- e220 TX, Arduino TX --> e220 RX
FLINT_E220_900T22S_JP LoRa1(&mySerial,LORA_POWER_PIN_Disabled,Lora_power_pin,AUX_STATUS_PIN_ENABLE,Aux_status_pin,M0_pin,M1_pin);
//      LORA_POWER_PIN_Disabled or LORA_POWER_PIN_ENABLE
//      AUX_STATUS_PIN_Disabled or AUX_STATUS_PIN_ENABLE
//-------------------------------------------------------------------------
//CRC
#define POLYCRC24Q 0x1864CFBU

int i,x;
uint8_t data;
uint8_t dataBuffer[1029];

int16_t data_tmp;
int16_t dataBuffer_tmp[1029];
uint8_t dataBuffer_tmp_1[1029];//分けた後格納する

uint8_t low_byte,high_byte;

char hexChar[3]; 
int messageLength = 0;
int messageLength_tmp = 0;
int bufferIndex = 0;
int bufferIndex_tmp = 0;
int bufferIndex_tmp_1 = 0;
int flag = 0;
String string_RTCMdata = "";
int messageType = 0; 
int type_count = 0;
bool CRC_check = false;

typedef struct { 
  uint8_t key;  
  String value; 
} dataDictionary;

const dataDictionary myDictionaryArr[] = {
    {0x00, "a"}, {0xFF, "b"}, {0x80, "c"}, {0xD3, "d"}, {0x20, "e"},
    {0x1E, "f"}, {0x7F, "g"}, {0x60, "h"}, {0x01, "i"}, {0x45, "j"},
    {0xC0, "k"}, {0xA0, "l"}, {0x16, "m"}, {0x43, "n"}, {0xF0, "o"},
    {0x9A, "p"}, {0xFA, "q"}, {0x55, "r"}, {0x14, "s"}, {0x92, "t"},
    {0x02, "u"}, {0x53, "v"}, {0x40, "w"}, {0xF8, "x"}, {0xFE, "y"},
    {0x05, "z"}, {0x88, "G"}, {0x09, "H"}, {0xFC, "I"}, {0x6D, "J"},
    {0xFD, "K"}, {0x96, "L"}, {0x46, "M"}, {0xEB, "N"}, {0xBA, "O"},
    {0x82, "P"}, {0x44, "Q"}, {0xBF, "R"}, {0xDF, "S"}, {0x06, "T"},
    {0xD4, "U"}, {0xC3, "V"}, {0x98, "W"}, {0x08, "X"}, {0x13, "Y"},
    {0x77, "Z"}
};

typedef struct {
    String key;
    String value;
} StringDictionary;

void uint8ToHexChar(uint8_t num, char* hexChar) {
    sprintf(hexChar, "%02X", num); // 2桁の16進数としてchar配列に保存
}

//RLE
String RLE_encode(String data) {
    String encoded = "";
    int count = 1;

    for (int i = 1; i <= data.length(); i++) {
        if (i < data.length() && data[i] == data[i - 1] && count < 9) {
            count++;
        } else {
            encoded += (count > 1 ? String(count) : "") + data[i - 1];
            count = 1;
        }
    }
    return encoded;
}

//上位36番目を1文字に変換　エンコード(RLE用)
String Karnaughmap_encode_36_RLE(String data) {
  StringDictionary myDictionaryArr[] = {
    {"OG", "a"}, {"DJ", "b"}, {"IG", "c"}, {"HE", "d"}, {"NF", "e"},
    {"MG", "f"}, {"GH", "g"}, {"KL", "h"}, {"CG", "i"}, {"AG", "j"},
    {"HM", "k"}, {"KJ", "l"}, {"FG", "m"}, {"PA", "n"}, {"FA", "o"},
    {"LL", "p"}, {"HK", "q"}, {"PI", "r"}, {"GI", "s"}, {"LJ", "t"},
    {"KC", "u"}, {"FO", "v"}, {"FE", "w"}, {"GL", "x"}, {"OO", "y"},
    {"GP", "z"}, {"FC", "Q"}, {"MD", "R"}, {"FD", "S"}, {"PM", "T"},
    {"KM", "U"}, {"EB", "V"}, {"BA", "W"}, {"OI", "X"}, {"KK", "Y"},
    {"BF", "Z"}
    };

    String encoded = "";
    
    for (int i = 0; i < data.length(); i += 2) {
        String hexPair = data.substring(i, i + 2);
        bool found = false;

        for (int j = 0; j < sizeof(myDictionaryArr) / sizeof(myDictionaryArr[0]); j++) {
            if (hexPair == myDictionaryArr[j].key) {  // 修正: uint8_t 比較を String 比較に変更
                encoded += myDictionaryArr[j].value;
                found = true;
                break;
            }
        }
        
        if (!found) {  // 見つからない場合はそのまま追加
            encoded += hexPair;
        }
    }

    return encoded;
}

// 0~9 を G~P に変換
String a_to_1_encode(String data) {
    String encoded = "";
    for (int i = 0; i < data.length(); i++) {
        char c = data[i];
        encoded += (isdigit(c)) ? char('G' + (c - '0')) : c;
    }
    return encoded;
}
//G~Pを0~9に変換
String a_to_1_decode(String data) {
    String decoded = "";
    for (int i = 0; i < data.length(); i++) {
        char c = data[i];
        decoded += (c >= 'G' && c <= 'P') ? char('0' + (c - 'G')) : c;
    }
    return decoded;
}

String Karnaughmap_encode(String data) {
    String encoded = "";
    
    for (int i = 0; i < data.length(); i += 2) {
        String hexPair = data.substring(i, i + 2);
        uint8_t hexValue = strtol(hexPair.c_str(), NULL, 16);

        bool found = false;
        for (int j = 0; j < sizeof(myDictionaryArr) / sizeof(myDictionaryArr[0]); j++) {
            if (hexValue == myDictionaryArr[j].key) {
                encoded += myDictionaryArr[j].value;
                found = true;
                break;
            }
        }
        if (!found) {// 見つからない場合はそのまま追加
            encoded += hexPair;
        }
    }
    
    return encoded;
}

int decide_RTCM(int RTCM_count){//124→94
      if(RTCM_count == 1){x = 0x464;}// RTCM 1124 ok
      else if(RTCM_count == 0){x = 0x446;}// RTCM 1094 ok
    return x;
}

//CRC ok
bool CRC_calculate(const uint8_t *buff, int buff_len) {
    unsigned int crc = 0;
    for (int y = 0; y < buff_len - 3; y++) { // 末尾3バイトを除く
        crc ^= (unsigned int)buff[y] << 16;
        for (int z = 0; z < 8; z++) {
            if ((crc <<= 1) & 0x1000000) {
                crc ^= POLYCRC24Q;
            }
        }
    }

    uint32_t receivedCRC = (buff[buff_len - 3] << 16) |
                           (buff[buff_len - 2] << 8) |
                           buff[buff_len - 1];

    return (crc & 0xFFFFFF) == receivedCRC; // 24ビット比較
}


void setup() {
    //Serial.begin(9600);//RTCMの受信が間に合わない
    Serial.begin(115200);//115200にすると間に合う
    
    pinMode(3,INPUT);

    lcd.init();       // initialize the LCD
    lcd.clear();      // clear the LCD display
    lcd.backlight();  // Make sure backlight is on

    //ライブラリ設定----------------------------------------
    LoRa1.begin();
    LoRa1.reset(200);       //モジュールをリセット200ms

    //LoRaモジュール設定
    
    //ADDH
    LoRa1.Register.Addh = 0x00;

    //ADDL
    LoRa1.Register.Addl = 0x00;
    
    //UART Serial Port Rate(bps) ArduinoとLoRaモジュール間のボーレート
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE1200;
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE2400;
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE4800;
    LoRa1.Register.Uart_rate = UART_RATE::UART_RATE9600;  //9600のみ対応     // (default)
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE19200;
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE38400;
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE57600;
    //LoRa1.Register.Uart_rate = UART_RATE::UART_RATE115200;
    
    //Air Data Rate 送信機　受信機　双方の伝送レートは同じでなければなりません。伝送レートが高いほど、遅延が小さくなりますが、伝送距離が短くなります。
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE15625;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE9375;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE5469;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE3125;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE1758;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE31250;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE18750;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE10938;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE6250;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE3516;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE1953;
    LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE62500;     // (default)
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE37500;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE21875;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE12500;//トラ技参考
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE7031;
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE3906;//
    //LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE2148;//近くでも遅くなる

    //サブパケット長
    LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET200;     // (default)
    //LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET128;
    //LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET64;
    //LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET32;

    //RSSI 環境ノイズの有効化
    //LoRa1.Register.Rssi_noise = RSSI_NOISE::RSSI_NOISE_Disabled;     // (default)
    LoRa1.Register.Rssi_noise = RSSI_NOISE::RSSI_NOISE_Enabled;

    //送信出力電力
    //LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER13;     // (default)
    //LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER12;
    //LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER7;
    //LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER0;

    //周波数チャンネルの指定
    LoRa1.Register.Ch = 0x0A;    //BW:125kHzでCH:15-37を選択した場合、BW:250kHzでCH:14-36を選択した場合、およびBW:500kHzでCH:13-30を選択した場合、32byte(29byte)に制限

    //RSSIバイトの有効化
    LoRa1.Register.Rssi_byte = RSSI_BYTE::RSSI_BYTE_Disabled;     // (default)
    //LoRa1.Register.Rssi_byte = RSSI_BYTE::RSSI_BYTE_Enabled;     //このライブラリでは未対応

    //送信方法
    //LoRa1.Register.Transmission_method = TRANSMISSION_METHOD::TRANSMISSION_METHOD_TRANSPARENT; //おすすめしません
    LoRa1.Register.Transmission_method = TRANSMISSION_METHOD::TRANSMISSION_METHOD_FIXED;

    //WOR サイクル
    //LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE500;
    //LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE1000;
    //LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE1500;
    //LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE2000;     // (default)
    //LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE2500;
    //LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE3000;

    //Key High Byte
    //LoRa1.Register.Crypt_h = 0x00;

    //Key Low Byte
    //LoRa1.Register.Crypt_l = 0x00;

    //一時作業レジスタの書き込み（モジュールの電源OFFで設定が消えます。（リセットでも））
    CODE Ret = LoRa1.set_temporary_register();

    //レジスタ値の書き込み（モジュールの電源OFFで設定が消えません）
    //CODE Ret = LoRa1.set_register();
    

    if(Ret == CODE_COMPLETE){
        Serial.println("RegOK");
    }else if(Ret == CODE_FORMAT_ERROR){
        Serial.println("Reg NG!!!!!!!");
    }else if(Ret == CODE_UNKNOWN_ERROR){
        Serial.println("Reg NG!!!!!!!!!");
    }
}

void loop() {
while(1){///← 

  bufferIndex_tmp = 0;
  bufferIndex_tmp_1 = 0;
  bool RTCM_flag = false;

  while(1){//←RTCM1メッセージを探すwhile(1)
    if(bufferIndex_tmp > 700){break;}//700byte
      if ((data_tmp = Serial.read()) != -1) {
        dataBuffer_tmp[bufferIndex_tmp++] = data_tmp;
        continue;//→
      }
    }

  for(int u = 0;u <= bufferIndex_tmp;u++){
    low_byte = dataBuffer_tmp[u] & 0xFF;//下位ビット取得する
    dataBuffer_tmp_1[bufferIndex_tmp_1++]=low_byte;
  }

  int currentIndex = 0;

  while (1) {//←受信したRTCM解析
    if (dataBuffer_tmp_1[currentIndex] == 0xD3) {//D3発見
      int messagestart = currentIndex;//D3が始まるindex番号
      if (currentIndex > bufferIndex_tmp_1-10){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.println("no message");          
        break;
      } // データ不足を確認 
      if (currentIndex + 5 <= bufferIndex_tmp_1){
        if (dataBuffer_tmp_1[messagestart + 1] == 0x00) {//次が00か確認
          messageLength = dataBuffer_tmp_1[messagestart + 2];          
          if(messagestart + messageLength +5 <= bufferIndex_tmp_1){
            uint8_t byte4 = dataBuffer_tmp_1[messagestart + 3];
            uint8_t byte5 = dataBuffer_tmp_1[messagestart + 4];           
            messageType = ((byte4 << 4) | (byte5 >> 4)) & 0xFFF;
            if (messageType == 1094 || messageType == 1124) {//指定メッセージタイプをチェック
              bufferIndex = 0;
              for(int s = messagestart; s<messagestart + messageLength + 6;s++){//ここがあってない
                dataBuffer[bufferIndex++] = dataBuffer_tmp_1[s];
              }
              CRC_check = CRC_calculate(dataBuffer,bufferIndex);
              if(CRC_check == true){
                if(flag == 0 && messageType == 1094){//RTCM1094発見
                  string_RTCMdata = "";
                  for (int a = 2; a <= bufferIndex; a++) {//先頭なのでD3 00を除外 
                    uint8ToHexChar(dataBuffer[a], hexChar);
                    string_RTCMdata += hexChar;//メッセージを保存
                  }
                  flag = 1;
                  currentIndex = messagestart + messageLength + 6; // 次のメッセージに移動(D3:1 00:1 le:1 messagelength CRC:3:messageLength + 6)   
                  continue;//→                   
                }
                else if(flag == 1 && messageType == 1124){//RTCM1124発見
                  for (int b = 0; b <= bufferIndex; b++) {
                    uint8ToHexChar(dataBuffer[b], hexChar);
                    string_RTCMdata += hexChar;//メッセージを保存
                  }
                  flag = 2;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.println("message ok!");                  
                  break;
                }
                else{//ダブりRTCM
                  currentIndex = messagestart + messageLength + 6; // 次のメッセージに移動(D3:1 00:1 le:1 messagelength CRC:3:messageLength + 6)   
                  continue;//→                   
                }
              }//if(CRC_check == true){
              else{//CRCの計算が合わなかった
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.println("CRC error");
                flag = 0;
                string_RTCMdata = "";
                currentIndex = messagestart + messageLength + 6; // 次のメッセージに移動(D3:1 00:1 le:1 messagelength CRC:3:messageLength + 6)      
                continue;//→  
              }                                          
            }//if (messageType == 1094 || messageType == 1124) {
            else{//求めているRTCM type以外のメッセージ
              currentIndex = messagestart + messageLength + 6; // 次のメッセージに移動(D3:1 00:1 le:1 messagelength CRC:3:messageLength + 6)   
              continue;//→                   
            }            
          }//if(messagestart + messageLength +5 <= bufferIndex_tmp_1){
          else{
            break;
          }
        }//if (dataBuffer_tmp_1[messagestart + 1] == 0x00) {
        else{
          currentIndex++;//D3の次が00じゃなかったら次のインデクス番号を見る
          continue; //→             
        }
      }//if (currentIndex + 5 <= bufferIndex_tmp_1){
      else{
        break;
      } 
    }//if (dataBuffer_tmp_1[currentIndex] == 0xD3) {
    else{
      currentIndex++;//D3の次が00以外の場合、次のインデクス番号を見る
      continue; //→        
    } 
  }

  if(string_RTCMdata.length() > 0 && flag == 2){//求めているメッセージ2つたまったのでLora送信
    flag = 0;
    //////////////圧縮処理/////////////////////////////////
    // 0~9 を G~P に変換 ok
    String encoded_data_1_to_G = a_to_1_encode(string_RTCMdata);
    // Karnaughmap Encode (36) String ok
    String encoded_data_36 = Karnaughmap_encode_36_RLE(encoded_data_1_to_G);
    // RLE Encode ok
    String rleEncoded = RLE_encode(encoded_data_36);
      
    char charArray[rleEncoded.length() + 1];
    rleEncoded.toCharArray(charArray, rleEncoded.length() + 1);

    int len = strlen(charArray);
    //int maxlen = 30;
    int maxlen = 45;//max 1メッセージあたり47byte ok
    int bunkatu = (len + maxlen - 1) / maxlen;        
    int i = 1, part = 1;
    int value = digitalRead(3);//busy(送信中)0　 not busy 1
    if(value != 0){
      while(1){
        if(i+maxlen > len){
          int remainingLength = (len - 1) % maxlen + 1;
          String str1 = "";
          char extracted1[len-i];
          strncpy(extracted1, charArray + i-1, len); // encodedString + i番目のインデックスからlen文字をコピー
          str1.concat(String(part));
          str1.concat("a");//2桁の場合
          str1.concat(String(bunkatu));
          str1.concat("a");//2桁の場合
          str1.concat(String(remainingLength));
          str1.concat("a");
          str1.concat(extracted1);
          //str1.concat('\0');//ok
          char str1Char[str1.length() + 1];
          str1.toCharArray(str1Char, str1.length() + 1);
          LoRa1.send_string(0x00,0x00,0x0A,str1Char);
          delay(300);
          str1 = "";
          break;
          }
        else{
          String str2 = "";
          int Part = part;
          str2.concat(String(Part));//ok
          str2.concat("a");//2桁の場合
          str2.concat(String(bunkatu));//ok
          str2.concat("a");//2桁の場合
          str2.concat(String(maxlen));
          str2.concat("a");
          char extracted2[maxlen+1];
          strncpy(extracted2, charArray + i-1, i+maxlen-1); // i番目のインデックスからi+maxlen-1文字をコピー
          extracted2[maxlen] = '\0';
          str2.concat(extracted2);//
          //str2.concat('\0');//ok　
          char str2Char[str2.length() + 1];
          str2.toCharArray(str2Char, str2.length() + 1);
          LoRa1.send_string(0x00,0x00,0x0A,str2Char);
          delay(300);
          bunkatu = (len + maxlen - 1) / maxlen;
          //maxlen = 30;
          maxlen = 45;//max 1メッセージあたり47byte
          part = part + 1;
          str2 = "";
        }
        i = i + maxlen;
      }
      string_RTCMdata = "";//メッセージを送信したので初期化
      messageLength = 0;
      bufferIndex = 0;
      bufferIndex_tmp = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.println("Lora ok");
    }
  }
  else{
    string_RTCMdata = "";
    messageLength = 0;
    bufferIndex = 0;
    bufferIndex_tmp = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("no Lora");
  }
  while(Serial.available() > 0){char t = Serial.read();}//余分に送られてくるものは捨てる。 
}
}