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
int emod(int a, int b);                             //bを法とするユークリッド除法で行う

void setup() {
  MP.begin(); 
  MP.RecvTimeout(MP_RECV_BLOCKING);
  MPLog("Start DF subcore\n"); 
}

void loop() {
  static float32_t spe_buf[BUF_SIZE][2*FFT_POINT];  //スペクトル(複素数)
  static float32_t pred_spe[2*FFT_POINT];           //予想スペクトル(複素数)
  static float32_t amp_buf[BUF_SIZE][FFT_POINT];    //振幅(実数)
  static float32_t cos_buf[BUF_SIZE][FFT_POINT];    //位相(実部)
  static float32_t sin_buf[BUF_SIZE][FFT_POINT];    //位相(虚部)
  static int pointer = 3;                           //先頭を指す
  static float32_t *r_buf;                          //受信したデータ
  static float32_t df[BUF_SIZE];   
  static int8_t msg_id;

  //FFT_SUBCOREからスペクトルを受信
  MP.Recv(&msg_id, &r_buf, FFT_SUBCORE);
  pointer = emod(pointer + 1, BUF_SIZE);

  //受信したデータから値渡し
  for(int i = 0;i < 2*FFT_POINT;++i){
    spe_buf[pointer][i] = r_buf[i];
  }

  //振幅を取り出す
  arm_cmplx_mag_f32(spe_buf[pointer], amp_buf[pointer], FFT_POINT);

  //位相を取り出す
  for(int i = 0;i < 2*FFT_POINT;++i){
    if(i%2 == 0){   //実部
      cos_buf[pointer][i/2] = spe_buf[pointer][i] / amp_buf[pointer][i/2];
    }else{          //虚部
      sin_buf[pointer][i/2] = spe_buf[pointer][i] / amp_buf[pointer][i/2];
    }
  }
  //MPLog("culc phase\n");

  //予想スペクトルを求める
  int m_1 = emod(pointer-1, BUF_SIZE);
  int m_2 = emod(pointer-2, BUF_SIZE);
  for(int i = 0;i < 2*FFT_POINT;++i){
    int k = i/2;

    float32_t cos_2phi_m1 = 1.0 - 2.0*sin_buf[m_1][k]*sin_buf[m_1][k];
    float32_t sin_2phi_m1 = 2.0*sin_buf[m_1][k]*cos_buf[m_1][k];
    
    if(i%2 == 0){   //実部
      pred_spe[i] = amp_buf[m_1][k] * ( cos_2phi_m1*cos_buf[m_2][k] + sin_2phi_m1*sin_buf[m_2][k] );
    }else{          //虚部
      pred_spe[i] = amp_buf[m_1][k] * ( sin_2phi_m1*cos_buf[m_2][k] + cos_2phi_m1*sin_buf[m_2][k] );
    }
  }
  //MPLog("culc pred_spe\n");

  //DFを求める
  df[pointer] = 0.0;
  static float32_t spe_sub[2*FFT_POINT];
  arm_sub_f32(spe_buf[pointer], pred_spe, spe_sub, 2*FFT_POINT);
  static float32_t spe_sub_norm[FFT_POINT];
  arm_cmplx_mag_squared_f32(spe_sub, spe_sub_norm, FFT_POINT);
  for(int i = 0;i < FFT_POINT;++i){
    df[pointer] += spe_sub_norm[i];
  }
  //MPLog("culc DF\n");

  MPLog("%d, %lf\n", pointer, (float)df[pointer]);
  //MP.Send(1, amp_buf[pointer]);

}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - (ceil(a/b)-1) * b);
  }
}
