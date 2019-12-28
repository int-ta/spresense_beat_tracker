//beat trackerでADCを行うサブコア

#if (SUBCORE != 1)
  #error "You select wrong core"
#endif

#include <MP.h>

/* 設定用の定数 */
const int FFT_POINT = 1024;                         //FFT点数
const int DATA_NUM = 512;                           //一度に送信するデータ数
const int BUF_NUM = 2;                              //バッファのサイズ
const unsigned int SAMPLING_INTERVAL = 11600;       //サンプリング間隔[us]
const int8_t SEND_ID = 2;                           //送信ID
const uint8_t ADC_PIN = PIN_A5;                     //ADCを行うピン番号
const int FFT_SUBCORE = 2;                          //FFTを行うサブコアの番号の指定

/* グローバル変数 */
uint16_t buf[BUF_NUM][DATA_NUM];                    //バッファ
int buf_pointer = 0;                                //現在使用しているバッファを指し示す
int data_pointer = 0;                               //バッファ内部の書き込む場所を指し示す

/* 関数 */
unsigned int get_adc(void);                         //ADCの値を読んで書き込む、タイマーで呼び出す

void setup() {
  //サブコア起動
  MP.begin();
  attachTimerInterrupt(get_adc, SAMPLING_INTERVAL);
}

void loop() {
  //データがたまったとき
  if(data_pointer == DATA_NUM){
    data_pointer = 0;
    MP.Send(SEND_ID, MP.Virt2Phys(buf[buf_pointer]), FFT_SUBCORE);
    //MP.Send(SEND_ID, MP.Virt2Phys(buf[buf_pointer]));
    buf_pointer = (buf_pointer+1) % BUF_NUM;
  }
}

unsigned int get_adc(void){
  buf[buf_pointer][data_pointer++] = analogRead(ADC_PIN);
  return SAMPLING_INTERVAL;
}
