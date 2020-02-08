//beat trackerでTempo Estimationを行うサブコア

#if (SUBCORE != 4)
  #error "You select wrong core"
#endif

#include <MP.h>
#include <math.h>

/* CMSIS用 */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <cmsis/arm_const_structs.h>

/* 設定用の定数 */
const int ADC_SUBCORE = 1;                          //ADCを行うサブコア番号
const int FFT_SUBCORE = 2;                          //FFTを行うサブコア番号
const int DF_SUBCORE  = 3;                          //Detection Functionを求めるサブコア番号
const int TE_SUBCORE  = 4;                          //Tempo Estimationを行うサブコア番号
const int BT_SUBCORE  = 5;                          //Beat Trackingを行うサブコア番号
const int HOP_SIZE = 128;
const int FLAME_SIZE = 512;
const int TAU_END = 66;
const int TAU_BEGIN = 32;
const int TAU_SIZE = 34; 

/* グローバル変数 */
//static float32_t FilterCoe[TAU_SIZE][FLAME_SIZE];
int arg_max_tau = 52;

/* 関数 */
int emod(int a, int b);
void moving_ave(float32_t *df_flame, float32_t *df_flame_ma, int flame_head);
float32_t hwr(float32_t x);

void setup(){
  MP.begin();
  MPLog("start TE subcore\n");
  //MP.Send(1, &arg_max_tau, BT_SUBCORE);
  /*
  //float32_t tmp;
  //フィルター係数を初期化
  for(int l = 0;l < 512;++l){
    for(int tau_i = 0;tau_i < 34;++tau_i){
      FilterCoe[tau_i][l] = 0.0;
    }
  }

  
  //フィルター係数を設定
  for(int tau_i = 0;tau_i < TAU_END-TAU_BEGIN;++tau_i){
    for(int p = 1;p <= 4;++p){
      int tau = tau_i + TAU_BEGIN;
      float32_t tau_f = (float32_t)(tau_i + TAU_BEGIN);
      float32_t weight = tau_f / (43.0*43.0) * exp(-tau_f*tau_f/(2.0*43.0*43.0));
      for(int v = 1-p;v <= p-1;++v){
        FilterCoe[tau_i][tau*p-v] = 1.0/(2.0*(float32_t)p - 1) * weight;
        //tmp = FilterCoe[tau_i][tau*p-v];
        //MPLog("%.3lf\n", tmp);
      }
    }
  }
  */
  
}

void loop(){
  static uint8_t msg_id;
  static float32_t *r_buf;
  static float32_t df_flame[FLAME_SIZE];
  static float32_t df_flame_ma[FLAME_SIZE];
  static float32_t mod_df_flame[FLAME_SIZE];
  static int flame_head = 0;
  static float32_t auto_col[FLAME_SIZE];
  static float32_t output[TAU_END-TAU_BEGIN];
  static float32_t output_max;
  
  MP.Recv(&msg_id, &r_buf, DF_SUBCORE);
  int write_head = emod(flame_head-HOP_SIZE, FLAME_SIZE);
  
  for(int i = 0;i < HOP_SIZE;++i){
    df_flame[write_head+i] = r_buf[i];
  }
  
  //移動平均スレッショルドを求める
  moving_ave(df_flame, df_flame_ma, flame_head);
    
  //修正DFフレームを求める
  static int l_prime;
  for(int l = 0;l < FLAME_SIZE;++l){
    l_prime = (l+flame_head) % FLAME_SIZE;
    mod_df_flame[l] = hwr(df_flame[l_prime]-df_flame_ma[l_prime]);
  }
  
  //正規化自己相関を求める
  for(int l = 0;l < FLAME_SIZE;++l){
    auto_col[l] = 0.0;
    for(int m = l;m < FLAME_SIZE;++m){
      auto_col[l] += mod_df_flame[m] * mod_df_flame[m-l];
    }
    auto_col[l] /= (float32_t)(FLAME_SIZE - l - 1); 
  }

  
  output_max = 0.0;
  for(int tau_i = 0;tau_i < TAU_SIZE;++tau_i){
    output[tau_i] = 0.0;

    for(int p = 1;p <= 4;++p){
      int tau = tau_i + TAU_BEGIN;
      float32_t tau_f = (float32_t)(tau_i + TAU_BEGIN);
      float32_t weight = tau_f / (43.0*43.0) * exp(-tau_f*tau_f/(2.0*43.0*43.0));
      for(int v = 1-p;v <= p-1;++v){
        output[tau_i] += auto_col[tau*p-v] * (1.0/(2.0*(float32_t)p - 1) * weight);
      }
    }

    if(output_max < output[tau_i]){
      arg_max_tau = tau_i + TAU_BEGIN;
      output_max = output[tau_i];
    }
  }
  MPLog("%.3lf\n", 60.0/((float32_t)arg_max_tau * 0.0113));
  MP.Send(1, &arg_max_tau, BT_SUBCORE);

  flame_head = (flame_head+HOP_SIZE)%FLAME_SIZE;
  
}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}

void moving_ave(float32_t *df_flame, float32_t *df_flame_ma, int flame_head){
  float32_t ext_df_flame[FLAME_SIZE+16];
  for(int i = 0;i < FLAME_SIZE;++i){
    ext_df_flame[(flame_head+i+8)%(FLAME_SIZE+16)] = df_flame[(flame_head+i)%FLAME_SIZE];
  }

  //先頭と後ろに0をつけたext_df_flameを作る
  for(int i = 0;i < 8;++i){
    ext_df_flame[flame_head+i] = 0.0;
    ext_df_flame[emod(flame_head-1-i, FLAME_SIZE+16)] = 0.0;
  }

  float32_t sum = 0.0;
  for(int i = 0;i < 17;++i){
    sum += ext_df_flame[flame_head+i];
  }

  for(int i = 0;i < FLAME_SIZE;++i){
    df_flame_ma[(flame_head+i)%FLAME_SIZE] = sum / 17.0;
    sum -= ext_df_flame[(flame_head+i)%(FLAME_SIZE+16)];
    sum += ext_df_flame[(flame_head+i+9)%(FLAME_SIZE+16)];
  }

  return;
}

float32_t hwr(float32_t x){
  if(x > 0.0) return x;
  else        return 0.0;
}
