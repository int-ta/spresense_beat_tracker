//beat trackerでBeat Trackingを行うサブコア

#if (SUBCORE != 5)
  #error "You select wrong core"
#endif

#include <MP.h>

/* 設定用の定数 */

/* グローバル変数 */

/* 関数 */

void setup(){
  MP.begin();
  MPLog("start DT subcore\n");
}

void loop(){
  
}
