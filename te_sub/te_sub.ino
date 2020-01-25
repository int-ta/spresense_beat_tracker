//beat trackerでTempo Estimationを行うサブコア

#if (SUBCORE != 4)
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
const int DF_SUBCORE  = 3;                          //Detection Functionを求めるサブコア番号
const int TE_SUBCORE  = 4;                          //Tempo Estimationを行うサブコア番号
const int BT_SUBCORE  = 5;                          //Beat Trackingを行うサブコア番号
const int HOP_SIZE = 128;
const int FLAME_SIZE = 512;
const int TAU_END = 66;
const int TAU_BEGIN = 32; 

/* グローバル変数 */
static float32_t FilterCoe[FLAME_SIZE][TAU_END-TAU_BEGIN];

/* 関数 */
int emod(int a, int b);
void moving_ave(float32_t *, float32_t *, int);
float32_t hwr(float32_t);

void setup(){
  MP.begin();
  MPLog("start TE subcore\n");

  //フィルター係数を初期化
  for(int l = 0;l < FLAME_SIZE;++l){
    for(int tau_i = 0;tau_i < TAU_END-TAU_BEGIN;++tau_i){
      FilterCoe[l][tau_i] = 0.0;
    }
  }

  //フィルター係数を設定
  for(int tau_i = 0;tau_i < TAU_END-TAU_BEGIN;++tau_i){
    for(int p = 1;p <= 4;++p){
      int tau = tau_i + TAU_BEGIN;
      for(int v = 1-p;v <= p-1;++v){
        FilterCoe[tau*p-v][tau_i] = 1.0/(2.0*(float32_t)p - 1);
      }
    }
  }

  //フィルターに対数ガウシアン重みをかける
  for(int tau_i = 0;tau_i < TAU_END-TAU_BEGIN;++tau_i){
    float32_t tau = (float32_t)(tau_i + TAU_BEGIN);
    float32_t weight = tau / (43.0*43.0) * exp(-tau*tau/(2.0*43.0*43.0));
    for(int l = 0;l < FLAME_SIZE;++l){
      FilterCoe[l][tau_i] *= weight;
    }
  }
  
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
  
  MP.Recv(&msg_id, &r_buf, DF_SUBCORE);
  int write_head = emod(flame_head-HOP_SIZE, FLAME_SIZE);
  for(int i = 0;i < HOP_SIZE;++i){
    df_flame[write_head+i] = r_buf[i];
  }

  //移動平均スレッショルドを求める
  moving_ave(df_flame, df_flame_ma, flame_head);
  
  //修正DFフレームを求める
  int l_prime;
  for(int l = 0;l < FLAME_SIZE;++l){
    l_prime = (l+flame_head) % FLAME_SIZE;
    mod_df_flame[l] = hwr(df_flame[l_prime]-df_flame[l_prime]);
  }

  //正規化自己相関を求める
  for(int l = 0;l < FLAME_SIZE;++l){
    auto_col[l] = 0.0;
    for(int m = l;m < FLAME_SIZE;++m){
      auto_col[l] += mod_df_flame[m] * mod_df_flame[m-l];
    }
    auto_col[l] /= (float32_t)(FLAME_SIZE - l - 1); 
  }

  float32_t output_max = 0.0;
  int arg_max_tau;
  for(int tau_i = 0;tau_i < TAU_END-TAU_BEGIN;++tau_i){
    output[tau_i] = 0.0;
    for(int l = 0;l < FLAME_SIZE;++l){
      output[tau_i] += auto_col[l]*FilterCoe[l][tau_i]; 
    }
    if(output_max < output[tau_i]){
      arg_max_tau = tau_i + TAU_BEGIN;
      output_max = output[tau_i];
    }
  }

  MPLog("%lf\n", arg_max_tau);


}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}

void moving_ave(float32_t *df_flame, float32_t *df_flame_ma, int flame_head){
  int center, head, tail;
  float32_t sum = 0.0;
  const int flame_tail = emod(flame_head+1, FLAME_SIZE);

  center = flame_head;
  tail = center + 8;
  for(int i = center+1;i <= tail;++i){
    sum += df_flame[i];
  }

  //headがflameに入りきるまで
  for(head = flame_head;center-head < 8;center++, tail++){
    sum += df_flame[tail];
    df_flame_ma[center] = sum/17.0;
  }

  //tailがflame_tailにいたるまで
  while(tail == flame_head){
    sum += df_flame[tail];
    sum -= df_flame[emod(head-1, FLAME_SIZE)];
    df_flame_ma[center] = sum/17.0;

    tail = (tail+1)%FLAME_SIZE;
    head = (head+1)%FLAME_SIZE;
    center = (center+1)%FLAME_SIZE;
  }

  while(center != flame_head){
    sum -= df_flame[emod(head-1, FLAME_SIZE)];
    df_flame_ma[center] = sum/17.0;

    center = (center+1)%FLAME_SIZE;
    head = (head+1)%FLAME_SIZE;
  }

  return;
}

float32_t hwr(float32_t x){
  if(x > 0.0) return x;
  else        return 0.0;
}