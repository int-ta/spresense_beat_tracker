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
const int DF_SUBCORE = 3;                           //Detection Functionを求めるサブコア番号
const int TE_SUBCORE  = 4;                          //Tempo Estimationを行うサブコア番号
const int BT_SUBCORE  = 5;                          //Beat Trackingを行うサブコア番号
const int FFT_POINT = 1024;                         //FFT点数
const int BUF_SIZE = 3;                             //spe_buf,amp_buf,pha_bufのサイズ
const int HOP_SIZE = 128;                           //TEに一度に送るDFのサンプル数

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
  static int df_bt_pointer = 3;                           //dfの先頭を指す
  static int df_te_pointer = 0;                     //df_teの保存先
  static int df_te_i = 0;                           //df_teのイテレータ
  static float32_t *r_buf;                          //受信したデータ
  static float32_t df_bt[BUF_SIZE];                 //BT subcoreに送るためのDF
  static float32_t df_te[BUF_SIZE][HOP_SIZE];       //TE subcoreに送るためのDF
  static int8_t msg_id;

  //FFT_SUBCOREからスペクトルを受信
  MP.Recv(&msg_id, &r_buf, FFT_SUBCORE);
  df_bt_pointer = emod(df_bt_pointer + 1, BUF_SIZE);

  //受信したデータから値渡し
  for(int i = 0;i < 2*FFT_POINT;++i){
    spe_buf[df_bt_pointer][i] = r_buf[i];
  }

  //振幅を取り出す
  arm_cmplx_mag_f32(spe_buf[df_bt_pointer], amp_buf[df_bt_pointer], FFT_POINT);

  //位相を取り出す
  for(int i = 0;i < 2*FFT_POINT;++i){
    if(i%2 == 0){   //実部
      cos_buf[df_bt_pointer][i/2] = spe_buf[df_bt_pointer][i] / amp_buf[df_bt_pointer][i/2];
    }else{          //虚部
      sin_buf[df_bt_pointer][i/2] = spe_buf[df_bt_pointer][i] / amp_buf[df_bt_pointer][i/2];
    }
  }

  //予想スペクトルを求める
  int m_1 = emod(df_bt_pointer-1, BUF_SIZE);
  int m_2 = emod(df_bt_pointer-2, BUF_SIZE);
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

  //DFを求める
  df_bt[df_bt_pointer] = 0.0;
  static float32_t spe_sub[2*FFT_POINT];
  arm_sub_f32(spe_buf[df_bt_pointer], pred_spe, spe_sub, 2*FFT_POINT);
  static float32_t spe_sub_norm[FFT_POINT];
  arm_cmplx_mag_squared_f32(spe_sub, spe_sub_norm, FFT_POINT);
  for(int i = 0;i < FFT_POINT;++i){
    df_bt[df_bt_pointer] += spe_sub_norm[i];
  }

  df_te[df_te_pointer][df_te_i++] = df_bt[df_bt_pointer];
  if(df_te_i == HOP_SIZE){
    df_te_i = 0;
    MP.Send(1, &df_te[df_te_pointer], TE_SUBCORE);
    df_te_pointer = emod(df_te_pointer+1, BUF_SIZE);
  } 
  
  MP.Send(1, &df_bt[df_bt_pointer], BT_SUBCORE);

}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}
