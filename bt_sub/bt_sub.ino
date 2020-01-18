//beat trackerでBeat Trackingを行うサブコア

#if (SUBCORE != 5)
  #error "You select wrong core"
#endif

#include <MP.h>
#include <math.h>

/* 設定用の定数 */
const int ADC_SUBCORE = 1;
const int FFT_SUBCORE = 2;
const int DF_SUBCORE  = 3;
const int TE_SUBCORE  = 4;
const int BT_SUBCORE  = 5;
const int MAX_V1      = 231;
const int MAX_V2      = 116;

/* CMSIS用 */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <cmsis/arm_const_structs.h>

/* グローバル変数 */
static int beat_period = 115;               //ビート間隔[DF sample]
static float32_t W_1[231];                  //対数ガウシアン遷移重み
static float32_t W_2[116];                  //ガウシアン重み
static const float32_t alpha = 0.9;         //新旧情報へのウェイト
static const float32_t eta = 5.0;           //W_1のタイトさ

/* 関数 */
int emod(int a, int b);                     //ユークリッド除法でa%bを求める

void setup(){
  MP.begin();
  MPLog("start DT subcore\n");

  //W_1を設定する
  for(int v1 = 1;v1 < 231;++v1){               //v=0は未定義なので1<=v<=230で代入する
    W_1[v1] = exp(- pow(eta * log10(-(float32_t)v1/(float32_t)beat_period),2.0) * 0.5);
  }

  //W_2を設定する
  for(int v2 = 1;v2 < 116;++v2){
    W_2[v2] = exp( - pow((float32_t)v2 - (float32_t)beat_period / 2.0, 2.0) 
                  / (2.0*pow((float32_t)beat_period/2.0, 2.0)));
  }
}

void loop(){
  float32_t *df;
  static float32_t new_inf;
  static int8_t msg_id;
  static float32_t cumu_score[231] = {};
  static int cs_head = 0;
  static int old_beat_pos = 200;
  static int next_beat_pos;

  MP.Recv(&msg_id, &df, DF_SUBCORE);
  new_inf = (1.0-alpha) * (*df);
  MPLog("%lf\n", *df);
  //MPLog("%p\n", MP.Virt2Phys(df));

  for(int v1 = 58;v1 < 231;++v1){
    cumu_score[cs_head] = max(cumu_score[cs_head], 
                              new_inf + alpha*W_1[v1]*cumu_score[emod(cs_head-v1, MAX_V1)]);
  }

  int arg_max_v2;
  int weight_cs_max = 0;
  for(int v2 = 1;v2 <= beat_period; ++v2){
    if(weight_cs_max < W_2[v2] * cumu_score[emod(old_beat_pos+v2, MAX_V1)]){
      weight_cs_max = W_2[v2] * cumu_score[emod(old_beat_pos+v2, MAX_V1)];
      arg_max_v2 = v2;
    }
  }
  
  next_beat_pos = old_beat_pos + arg_max_v2;

  //MP.Send((int8_t)cs_head, &next_beat_pos);
  cs_head = (cs_head + 1) % MAX_V1;

}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - (ceil(a/b)-1) * b);
  }
}
