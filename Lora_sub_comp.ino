//R3は3.3V/5Vどちらでも動く　R4は5vじゃないと動かない
////#include "Arduino_LED_Matrix.h"はsub側では"!!!!!!!Reg NG_UNKNOWN!!!!!!!!!"になり、受信できなくなる
//#include  "zlib_turbo.h" gzipの解凍しか関数がない
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

//https://flint.works/p/e220-900t22s-for-arduino/
#include "FLINT_E220_900T22S_JP.h"

//for Arduino Uno R3 ----------------------------------------------------
// ArduinoIDE [ツール]→[ボード]→[Arduino Uno]
// FLINT LoRa無線シールド(E220-900T22S for Arduino) SW9=D5/D6
//シールドピン設定
#define SW_1 12
#define RX_pin 6
#define TX_pin 5
#define Lora_power_pin 11
#define Aux_status_pin 3
#define M0_pin 11
#define M1_pin 10
#include <SoftwareSerial.h>
SoftwareSerial mySerial(RX_pin, TX_pin); // Arduino RX <-- e220 TX, Arduino TX --> e220 RX
FLINT_E220_900T22S_JP LoRa1(&mySerial,LORA_POWER_PIN_Disabled,Lora_power_pin,AUX_STATUS_PIN_ENABLE,Aux_status_pin,M0_pin,M1_pin);
//      LORA_POWER_PIN_Disabled or LORA_POWER_PIN_ENABLE
//      AUX_STATUS_PIN_Disabled or AUX_STATUS_PIN_ENABLE
//-------------------------------------------------------------------------

typedef struct {
    String key;
    String value;
} StringDictionary;

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

String RLE_decode(String data) {
    String decoded = "";
    for (int i = 0; i < data.length(); i++) {
        if (isdigit(data[i])) {
            int count = data[i] - '0';
            char value = data[++i];
            for (int j = 0; j < count; j++) {
                decoded += value;
            }
        } else {
            decoded += data[i];
        }
    }
    return decoded;
}

String Karnaughmap_decode_36_RLE(String data) {

    String decoded = "";
    
    for (int i = 0; i < data.length(); i++) {
        String charToDecode = data.substring(i, i + 1);
        bool found = false;

        for (int j = 0; j < sizeof(myDictionaryArr) / sizeof(myDictionaryArr[0]); j++) {
            if (charToDecode == myDictionaryArr[j].value) {
                decoded += myDictionaryArr[j].key;  // 修正: String型キーを返す
                found = true;
                break;
            }
        }
        
        if (!found) {  // 見つからない場合はそのまま追加
            decoded += charToDecode;
        }
    }

    return decoded;
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

void setup() {
    Serial.begin(115200);

    pinMode(3, INPUT);//AUXステータスを取得
    
    //ライブラリ設定----------------------------------------
    LoRa1.begin();
    LoRa1.reset(200);//モジュールをリセット200ms

    //LoRaモジュール設定
    
    //ADDH
    LoRa1.Register.Addh = 0x00;

    //ADDL
    LoRa1.Register.Addl = 0x00;
    
    //UART Serial Port Rate(bps)  ArduinoとLoRaモジュール間のボーレート
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE1200;
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE2400;
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE4800;
    LoRa1.Register.Uart_rate = UART_RATE::UART_RATE9600;  //9600のみ対応     // (default)
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE19200;
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE38400;
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE57600;
//  LoRa1.Register.Uart_rate = UART_RATE::UART_RATE115200;
    
    //Air Data Rate 送信機　受信機　双方の伝送レートは同じでなければなりません。伝送レートが高いほど、遅延が小さくなりますが、伝送距離が短くなります。
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE15625;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE9375;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE5469;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE3125;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE1758;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE31250;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE18750;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE10938;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE6250;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE3516;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE1953;
  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE62500;// (default)
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE37500;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE21875;
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE12500;//トラ技
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE7031;
//    LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE3906;//
//  LoRa1.Register.Air_rate = AIR_RATE::AIR_RATE2148;//近くでも遅くなる

    //サブパケット長
//  LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET200;     // (default)
//  LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET128;
//  LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET64;
//  LoRa1.Register.Sub_packet = SUB_PACKET::SUB_PACKET32;

    //RSSI 環境ノイズの有効化
//  LoRa1.Register.Rssi_noise = RSSI_NOISE::RSSI_NOISE_Disabled;     // (default)
    LoRa1.Register.Rssi_noise = RSSI_NOISE::RSSI_NOISE_Enabled;

    //送信出力電力
//  LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER13;     // (default)
//  LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER12;
//  LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER7;
//  LoRa1.Register.Transmit_power = TRANSMIT_POWER::TRANSMIT_POWER0;

    //周波数チャンネルの指定
    LoRa1.Register.Ch = 0x0A;//0x0Aじゃないと送信不能  BW:125kHzでCH:15-37を選択した場合、BW:250kHzでCH:14-36を選択した場合、およびBW:500kHzでCH:13-30を選択した場合、32byte(29byte)に制限
    //125Hz SF12にしたい　0x0F;  //915Mhz (900 + 0x0F * 1MHz = 915MHz)
    //RSSIバイトの有効化
    LoRa1.Register.Rssi_byte = RSSI_BYTE::RSSI_BYTE_Disabled;     // (default)
//  LoRa1.Register.Rssi_byte = RSSI_BYTE::RSSI_BYTE_Enabled;     //このライブラリでは未対応

    //送信方法
//  LoRa1.Register.Transmission_method = TRANSMISSION_METHOD::TRANSMISSION_METHOD_TRANSPARENT; //おすすめしません
    LoRa1.Register.Transmission_method = TRANSMISSION_METHOD::TRANSMISSION_METHOD_FIXED;

    //WOR サイクル
//  LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE500;
//  LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE1000;
//  LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE1500;
//  LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE2000;     // (default)
//  LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE2500;
//  LoRa1.Register.Wor_cycle = WOR_CYCLE::WOR_CYCLE3000;

    //Key High Byte
//  LoRa1.Register.Crypt_h = 0x00;

    //Key Low Byte
//  LoRa1.Register.Crypt_l = 0x00;

    //一時作業レジスタの書き込み（モジュールの電源OFFで設定が消えます。（リセットでも））
    CODE Ret = LoRa1.set_temporary_register();

    //レジスタ値の書き込み（モジュールの電源OFFで設定が消えません）
    //CODE Ret = LoRa1.set_register();

    if(Ret == CODE_COMPLETE){
        Serial.println("!!!!!!!RegOK!!!!!!!");
    }else if(Ret == CODE_FORMAT_ERROR){
        Serial.println("!!!!!!!Reg NG_FORMAT!!!!!!!");
    }else if(Ret == CODE_UNKNOWN_ERROR){
        Serial.println("!!!!!!!Reg NG_UNKNOWN!!!!!!!!!");
    }

    //SW10(D12)のピン設定
    pinMode(SW_1, INPUT_PULLUP);

}
void loop() {
    char Ret[40];
    int part = 0;      
    int bunkatsu = 0;  
    int leng = 0;      
    int a = 0;
    int count = 0;         
    int tmp_bunkatsu = 0;
    bool flag = false;
    String gousei = "";
    String RTCM1005_data = "D300133ED0000336D1314F8A87D6375EA9089EA218CBD489EC";//ok
    int RTCM1005_count = 0;
    String gzip_decompress_data;
while(1){
  while(1){
    int value = digitalRead(3);////受信した瞬間(Busy状態) 0(Low) 受信していない間 1(high)
    if(value != 0){
    if(LoRa1.receive_string(Ret)){
      if(Ret[1] == 'a' && Ret[3] == 'a'){
        part = int(Ret[0]- '0');
        bunkatsu = int(Ret[2]- '0'); 
        if(Ret[6] == 'a'){
          leng = int(Ret[4]- '0')*10 + int(Ret[5]- '0');
          a = 7;
        }
        else if(Ret[5] == 'a'){
          leng = int(Ret[4]- '0');
          a = 6;    
        }        
      }
      else if(Ret[1] == 'a' && Ret[4] == 'a'){
        part = int(Ret[0]- '0');
        bunkatsu = int(Ret[2]- '0')*10 + int(Ret[3]- '0');        
        leng = int(Ret[5]- '0')*10 + int(Ret[6]- '0');
        a = 8;
      }
      else if(Ret[2] == 'a' && Ret[5] == 'a'){
        part = int(Ret[0]- '0')*10 + int(Ret[1]- '0'); 
        bunkatsu = int(Ret[3]- '0')*10 + int(Ret[4]- '0');        
        if(Ret[8] == 'a'){
          leng = int(Ret[6]- '0')*10 + int(Ret[7]- '0');
          a = 9;
        }
        else if(Ret[7] == 'a'){
          leng = int(Ret[6]- '0');
          a = 8;         
        } 
      }
      if(part == 1){
        flag = true;
        tmp_bunkatsu = bunkatsu;
      }

      if(bunkatsu != tmp_bunkatsu){
        gousei = "";
        //Serial.println("-----bunkatsu error!-----");
        break;
      }
      else{//Serial.println("-----bunkatsu ok!-----");}
        if(flag == true){
          if(part <= bunkatsu){
            for(int i = a;i <= leng+a-1;i++){
              gousei += Ret[i];
            }
          }
          if(part == bunkatsu){
            //Serial.println("Receive Completed!!");
            break;
          }  
        }  
      }
    }
    }
  }

  if(gousei != ""){
    RTCM1005_count = RTCM1005_count +1;

    /////////////解凍処理////////////////////////////////////////
    // RLE Decode　ok
    String rleDecoded = RLE_decode(gousei); 
    // Karnaughmap Decode (36) String ok
    String decoded_data_36 = Karnaughmap_decode_36_RLE(rleDecoded);
    // G~P を 0~9 に戻す ok
    String decoded_data = a_to_1_decode(decoded_data_36);
    /////////////////////////////////////////////////////////////

    String RTCM_First_message ="D300";
    String RTCM_msgs = RTCM_First_message + decoded_data;

    //Serial.println(RTCM_msgs);

    for (int i = 0; i < RTCM_msgs.length()-2; i += 2) {//Loraの終端00が含まれているため-2
      String hex_pair = RTCM_msgs.substring(i, i + 2);
      char hex_byte = (char) strtol(hex_pair.c_str(), NULL, 16);
      Serial.write(hex_byte);
    }
           
    if(RTCM1005_count % 15 == 0){//15回に１回
      for (int i = 0; i < RTCM1005_data.length(); i += 2) {//Loraの終端00が含まれているため-2
        String hex_pair = RTCM1005_data.substring(i, i + 2);
        char hex_byte = (char) strtol(hex_pair.c_str(), NULL, 16);
        Serial.write(hex_byte);
      }
    }

  }
  gousei = "";      
}
}