//beat trackerのメインコア

//サブコアでないか確認
#ifdef SUBCORE
  #error "You select wrong core"
#endif

#include <MP.h>

/* 設定用の定数 */
const int FFT_POINT = 1024;                         //FFT点数
const int DATA_NUM = 512;                           //一度に送信するデータ数
const int BUF_NUM = 2;                              //バッファのサイズ
const unsigned int SAMPLING_INTERVAL = 23;          //サンプリング間隔[us]
const int ADC_SUBCORE = 1;                          //ADCを行うサブコア番号
const int FFT_SUBCORE = 2;                          //FFTを行うサブコア番号
const int DF_SUBCORE = 3;                           //DFを求めるサブコア番号
const int TE_SUBCORE  = 4;                          //Tempo Estimationを行うサブコア番号
const int BT_SUBCORE  = 5;                          //Beat Trackingを行うサブコア番号

/* 関数 */
int emod(int a, int b);

void setup() {
  Serial.begin(115200);
  while(!Serial);
  //MP.RecvTimeout(MP_RECV_BLOCKING);

  //ADCを行うサブコアを起動
  printf("Start ADC subcore\n");
  MP.begin(ADC_SUBCORE);

  //FFTを行うサブコアを起動
  //printf("Start FFT subcore\n");
  //MP.begin(FFT_SUBCORE);

  //DFを求めるサブコアを起動
  printf("Start DF subcore\n");
  MP.begin(DF_SUBCORE);

  //TEを行うサブコアを起動
  printf("Start TE subcore\n");
  MP.begin(TE_SUBCORE);

  //BTを行うサブコアを起動
  printf("Start BT subcore\n");
  MP.begin(BT_SUBCORE);


  int usedMem, freeMem, largestFreeMem;
  MP.GetMemoryInfo(usedMem, freeMem, largestFreeMem);
  MPLog("Used:%4d [KB] / Free:%4d [KB] (Largest:%4d [KB])\n",
        usedMem / 1024, freeMem / 1024, largestFreeMem / 1024);

  
}

void loop() {
  
}

int emod(int a, int b){
  if(a > -1){
    return a%b;
  }else{
    return (a - floor((float)a/(float)b) * b);
  }
}
