//beat trackerでADCを行うサブコア

#if (SUBCORE != 1)
  #error "You select wrong core"
#endif

#include <MP.h>
#include <SPI.h>

/* 設定用の定数 */
const int FFT_POINT = 1024;                         //FFT点数
const int DATA_NUM = 512;                           //一度に送信するデータ数
const int BUF_NUM = 2;                              //バッファのサイズ
const unsigned int SAMPLING_INTERVAL = 23;          //サンプリング間隔[us]
const int8_t SEND_ID = 2;                           //送信ID
const uint8_t ADC_PIN = A5;                         //ADCを行うピン番号
const int ADC_SUBCORE = 1;                          //ADCを行うサブコア番号
const int FFT_SUBCORE = 2;                          //FFTを行うサブコア番号
const int DF_SUBCORE = 3;                           //DFを求めるサブコア番号

/* グローバル変数 */
uint16_t buf[BUF_NUM][DATA_NUM];                    //バッファ
int buf_pointer = 0;                                //現在使用しているバッファを指し示す
int data_pointer = 0;                               //バッファ内部の書き込む場所を指し示す
int old_buf_pointer;

/* 関数 */
unsigned int get_adc(void);                         //ADCの値を読んで書き込む、タイマーで呼び出す

void setup() {
  //サブコア起動
  MP.begin();
  MPLog("start ADC subcore\n");
  int usedMem, freeMem, largestFreeMem;
  MP.GetMemoryInfo(usedMem, freeMem, largestFreeMem);
  MPLog("Used:%4d [KB] / Free:%4d [KB] (Largest:%4d [KB])\n",
        usedMem / 1024, freeMem / 1024, largestFreeMem / 1024);
  delay(100);
  MP.RecvTimeout(MP_RECV_BLOCKING);
  SPI.begin();
  SPI.beginTransaction(SPISettings(3000000, MSBFIRST, SPI_MODE0));
  attachTimerInterrupt(get_adc, SAMPLING_INTERVAL);
}

void loop() {
  //データがたまったとき
  if(data_pointer == DATA_NUM){
    data_pointer = 0;
    old_buf_pointer = buf_pointer;
    buf_pointer = (buf_pointer+1) % BUF_NUM;
    MP.Send(SEND_ID, buf[old_buf_pointer], DF_SUBCORE);
    //MPLog("send data\n");
    //MP.Send(SEND_ID, buf[old_buf_pointer]);
  }
}

unsigned int get_adc(void){
  buf[buf_pointer][data_pointer++] = (uint16_t)(SPI.transfer16(0x7800) & 2047);
  return SAMPLING_INTERVAL;
}
