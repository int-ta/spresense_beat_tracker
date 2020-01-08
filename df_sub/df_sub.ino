//beat trackerでDFを求めるサブコア

#if (SUBCORE != 3)
  #error "You select wrong core"
#endif

#include <MP.h>

/* CMSIS用 */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <cmsis/arm_const_structs.h>

/* 設定用の定数 */
const int ADC_SUBCORE = 1;                          //ADCを行うサブコア番号
const int FFT_SUBCORE = 2;                          //FFTを行うサブコア番号
const int DF_SUBCORE = 3;                           //DFを求めるサブコア番号
const int FFT_POINT = 1024;                         //FFT点数
const int BUF_SIZE = 3;                             //spe_buf,amp_buf,pha_bufのサイズ

/* 関数 */
int emod(int a, int b);                             //bを法とするオイラー除法で行う

void setup() {
  MP.begin();  
}

void loop() {
  static float32_t spe_buf[BUF_SIZE][2*FFT_POINT];  //スペクトル(複素数)
  static float32_t amp_buf[BUF_SIZE][FFT_POINT];    //振幅(実数)
  static float32_t pha_buf[BUF_SIZE][FFT_POINT];    //位相(実数)
  static int pointer = 3;                           //先頭を指す
  static float32_t *r_buf;                          //受信したデータ      
  static int8_t msg_id;

  //FFT_SUBCOREからスペクトルを受信
  MP.Recv(&msg_id, &r_buf, FFT_SUBCORE);
  pointer = emod(pointer + 1, BUF_SIZE);

  //受信したデータから値渡し
  arm_copy_f32(r_buf, spe_buf[pointer], 2*FFT_POINT);

  arm_cmplx_mag_f32(spe_buf[pointer], amp_buf[pointer], FFT_POINT);

  
}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return a - floor(a/b) * b;
  }
}
