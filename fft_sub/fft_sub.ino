//beat trackerでFFTを行うサブコア

#if (SUBCORE != 2)
  #error "You select wrong core"
#endif

#include <MP.h>

/* CMSIS用 */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

/* 設定用の定数 */
const int FFT_POINT = 1024;                         //FFT点数
const int DATA_NUM = 512;                           //一度に送信するデータ数
const unsigned int SAMPLING_INTERVAL = 11600;       //サンプリング間隔[us]
const int ADC_SUBCORE = 1;                          //ADCを行うサブコア番号
const int BUF_NUM = 2;                              //バッファの数

/* グローバル変数 */
static float32_t buf[BUF_NUM][DATA_NUM];            //受信したデータを保持するバッファ

void setup() {
  //サブコア起動
  MP.begin();

  //bufを初期化
  for(int i = 0;i < BUF_NUM;++i){
    for(int j = 0;j < DATA_NUM;++j){
      buf[i][j] = 0.0;
    }
  }
  
}

void loop() {
  static int8_t msg_id;
  uint16_t *r_buf;
  static int buf_pos = 0;

  //データを受け取る
  MP.Recv(&msg_id, &r_buf, ADC_SUBCORE);

  //受け取ったデータを移し替える
  for(int i = 0;i < DATA_NUM;++i){
    buf[buf_pos][i] = r_buf[i];
  }
  buf_pos = (buf_pos + 1) % BUF_NUM;
  
}
