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
const int MAX_V1      = 131;
const int MAX_V2      = 66;

/* CMSIS用 */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <cmsis/arm_const_structs.h>

/* グローバル変数 */
static int beat_period = 65;                //ビート間隔[DF sample]
static float32_t W_1[131];                  //対数ガウシアン遷移重み
static float32_t W_2[66];                  //ガウシアン重み
static const float32_t alpha = 0.9;         //新旧情報へのウェイト
static const float32_t eta = 5.0;           //W_1のタイトさ
static float32_t cumu_score[131] = {};
int *interval_pointer;

/* 関数 */
int emod(int a, int b);                     //ユークリッド除法でa%bを求める

void setup(){
  MP.begin();
  MPLog("start BT subcore\n");
  MP.RecvTimeout(MP_RECV_POLLING);
  *interval_pointer = 65;

  //W_1を設定する
  for(int v1 = 1;v1 < 131;++v1){               //v1=0は未定義なので1<=v1<=230で代入する
    W_1[v1] = (float32_t)exp(- pow(eta * log10((float)v1/(float)beat_period), 2.0) * 0.5);
    cumu_score[v1] = 0.0;
  }

  //W_2を設定する
  for(int v2 = 1;v2 < 66;++v2){
    W_2[v2] = exp( - pow((float32_t)v2 - (float32_t)beat_period / 2.0, 2.0) 
                  / (2.0*pow((float32_t)beat_period/2.0, 2.0)));
  }

  pinMode(LED0, OUTPUT);
}

void loop(){
  float32_t *df;
  static float32_t new_inf;
  static int8_t msg_id;
  static int32_t cs_head = 0;
  static int32_t old_beat_pos = 200;
  static int32_t next_beat_pos;
  static int led_state = 0;
  static int beat_interval = 65;

  msg_id = 1;
  while(msg_id == 1){
    MP.Recv(&msg_id, &df, DF_SUBCORE);
  }
  new_inf = (1.0-alpha) * (*df);
  //MPLog("%lf\n", *df);
  //MPLog("%p\n", MP.Virt2Phys(df));

  static float32_t candidate;
  for(int v1 = beat_interval/2;v1 < 2*beat_interval;++v1){
    candidate = new_inf + alpha*W_1[v1]*cumu_score[emod(cs_head-v1, MAX_V1)];
    if(candidate > cumu_score[cs_head]){
      cumu_score[cs_head] = candidate;
    }
  }

  int arg_max_v2;
  float32_t weight_cs_max = 0;
  for(int v2 = 1;v2 <= beat_interval; ++v2){
    if(weight_cs_max < W_2[v2] * cumu_score[emod(old_beat_pos+v2, MAX_V1)]){
      weight_cs_max = W_2[v2] * cumu_score[emod(old_beat_pos+v2, MAX_V1)];
      arg_max_v2 = v2;
    }
  }

  next_beat_pos = emod(old_beat_pos + arg_max_v2, MAX_V1);
  //越した判定(暫定的なもの，あとでしっかり考え直す必要あり)
  if(next_beat_pos == cs_head){
    old_beat_pos = next_beat_pos;
    //MP.Send(1, &old_beat_pos);
    led_state ^= 1;
    digitalWrite(LED0, led_state);

    msg_id = 2;
    MP.Recv(&msg_id, &interval_pointer, TE_SUBCORE);
    if(msg_id ==1){
      MPLog("recv\n");
      beat_interval = *interval_pointer;
  
      //W_1を設定する
      for(int v1 = 1;v1 < 131;++v1){               //v1=0は未定義なので1<=v1<=131で代入する
        W_1[v1] = (float32_t)exp(- pow(eta * log10((float)v1/(float)beat_interval), 2.0) * 0.5);
        cumu_score[v1] = 0.0;
      }
  
      //W_2を設定する
      for(int v2 = 1;v2 < 66;++v2){
        W_2[v2] = exp( - pow((float32_t)v2 - (float32_t)beat_interval / 2.0, 2.0) 
                      / (2.0*pow((float32_t)beat_interval/2.0, 2.0)));
      }
    }
  }

  MPLog("%d %d\n", cs_head, next_beat_pos);
  //MP.Send((int8_t)cs_head, &next_beat_pos);
  cs_head = (cs_head + 1) % MAX_V1;

}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}
