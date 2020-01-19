//beat trackerのメインコア

//サブコアでないか確認
#ifdef SUBCORE
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
const int BUF_NUM = 2;                              //バッファのサイズ
const unsigned int SAMPLING_INTERVAL = 23;          //サンプリング間隔[us]
const int ADC_SUBCORE = 1;                          //ADCを行うサブコア番号
const int FFT_SUBCORE = 2;                          //FFTを行うサブコア番号
const int DF_SUBCORE = 3;                           //DFを求めるサブコア番号
const int TE_SUBCORE  = 4;
const int BT_SUBCORE  = 5;

/* 関数 */
unsigned int hoge(void);
int emod(int a, int b);

void setup() {
  Serial.begin(115200);
  while(!Serial);
  //MP.RecvTimeout(MP_RECV_BLOCKING);

  //ADCを行うサブコアを起動
  printf("Start ADC subcore\n");
  MP.begin(ADC_SUBCORE);

  //FFTを行うサブコアを起動
  printf("Start FFT subcore\n");
  MP.begin(FFT_SUBCORE);

  //DFを求めるサブコアを起動
  printf("Start DF subcore\n");
  MP.begin(DF_SUBCORE);

  //BTを行うサブコアを起動
  printf("Start BT subcore\n");
  MP.begin(BT_SUBCORE);

}

void loop() {
  static int8_t msg_id;
  static int32_t *r_buf;
  
  MP.Recv(&msg_id, &r_buf, BT_SUBCORE);
  

  //MPLog("%d %d\n", now_pos, next_pos);  
}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}
