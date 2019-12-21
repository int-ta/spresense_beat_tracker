//beat trackerのメインコア

//サブコアでないか確認
#ifdef SUBCORE
  #error "You select wrong core"
#endif

#include <MP.h>

/* 設定用の定数 */
const int FFT_POINT = 1024;                         //FFT点数
const int DATA_NUM = 512;                           //一度に送信するデータ数
const int BUF_NUM = 2;                              //バッファのサイズ
const unsigned int SAMPLING_INTERVAL = 11600;       //サンプリング間隔[us]
const int ADC_SUBCORE = 1;                          //ADCを行うサブコアの番号の指定
const int FFT_SUBCORE = 2;                          //FFTを行うサブコアの番号の指定

void setup() {
  Serial.begin(115200);
  while(!Serial);
  MP.RecvTimeout(MP_RECV_BLOCKING);

  //ADCを行うサブコアを起動
  printf("Start ADC subcore\n");
  MP.begin(ADC_SUBCORE);
}

void loop() {
  /*
  int8_t msg_id;
  uint16_t *r_buf;
  int err;
  err = MP.Recv(&msg_id, &r_buf, ADC_SUBCORE);
  for(int i = 0;i < DATA_NUM;++i){
    printf("%3d : %d\n", i, r_buf[i]);
  }
  */
}
