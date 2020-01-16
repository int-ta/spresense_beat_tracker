//beat trackerでTempo Estimationを行うサブコア

#if (SUBCORE != 4)
  #error "You select wrong core"
#endif

#include <MP.h>

/* 設定用の定数 */

/* グローバル定数 */

/* 関数 */

void setup(){
  MP.begin();
  MPLog("start TE subcore\n");
}

void loop(){
  
}
