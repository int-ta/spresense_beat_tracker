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
const int DATA_NUM = 512;                           //一度に受信するデータ数
const int BUF_SIZE = 3;                             //spe_buf,amp_buf,pha_bufのサイズ
const int HOP_SIZE = 128;                           //TEに一度に送るDFのサンプル数
const int SOUND_DATA_SIZE = 2;
const uint16_t BIAS = 345;                          //sound_dataに含まれるバイアス成分

/* グローバル変数 */
const arm_cfft_instance_f32 *instance(0);           //FFT用のinstance
float32_t window[FFT_POINT];                        //ハニング窓を格納

/* 関数 */
int emod(int a, int b);                             //bを法とするユークリッド除法で行う

void setup() {
  MP.begin(); 
  MP.RecvTimeout(MP_RECV_BLOCKING);
  MPLog("Start DF subcore\n"); 

  instance = &arm_cfft_sR_f32_len1024;

  //窓関数を作成
  float32_t coe;
  for(int i = 0;i < FFT_POINT;++i){
    coe = ((float32_t)i) / ((float32_t)FFT_POINT);
    window[i] = 0.5 -0.5*arm_cos_f32(2.0*PI*coe);
  }
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
  static float32_t df_bt[BUF_SIZE];                 //BT subcoreに送るためのDF
  static float32_t df_te[BUF_SIZE][HOP_SIZE];       //TE subcoreに送るためのDF
  static int8_t msg_id;
  static float32_t sound_data[SOUND_DATA_SIZE][DATA_NUM];
  static int sound_data_tail = 1;                   //新しsound_dataの場所
  static int sound_data_head = 0;

  uint16_t *r_buf;                                  //受信したデータを一時保管する
  MP.Recv(&msg_id, &r_buf, ADC_SUBCORE);

  sound_data_tail = (sound_data_tail+1)%SOUND_DATA_SIZE;
  sound_data_head = (sound_data_head+1)%SOUND_DATA_SIZE;
  df_bt_pointer = (df_bt_pointer+1) % BUF_SIZE;

  //受信したデータを移し替えつつ，バイアス成分を取り除く
  for(int i = 0;i < DATA_NUM;++i){
    sound_data[sound_data_tail][i] = (float32_t)(r_buf[i] - BIAS);
    sound_data[sound_data_tail][i] *= 0.1;
  }
  //MPLog("%.3lf\n", sound_data[sound_data_tail][0]);

  for(int i = 0;i < DATA_NUM;++i){
    spe_buf[df_bt_pointer][2*i] = sound_data[sound_data_head][i] * window[i];
    spe_buf[df_bt_pointer][2*i+1] = 0.0;
  }
  for(int i = 0;i < DATA_NUM;++i){
    spe_buf[df_bt_pointer][2*(DATA_NUM+i)] = sound_data[sound_data_tail][i] * window[DATA_NUM+i];
    spe_buf[df_bt_pointer][2*(DATA_NUM+i)+1] = 0.0;
  }
  //MPLog("%lf\n", sound_data[sound_data_head][0]);
  //MPLog("%lf\n", sound_data[sound_data_tail][0]);

  arm_cfft_f32(instance, spe_buf[df_bt_pointer], 0, 1);  
  //MPLog("%lf\n", spe_buf[df_bt_pointer][2]);

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

  //MPLog("%lf\n", df_bt[df_bt_pointer]);
  MP.Send(2, &df_bt[df_bt_pointer], BT_SUBCORE);

}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}
