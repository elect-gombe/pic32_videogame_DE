#define _SUPPRESS_PLIB_WARNING
// TETRIS（キャラクターベース） Rev.1.0 2013/8/22
// 　with テキストVRAMコンポジット出力システム for PIC32MX1xx/2xx by K.Tanaka
// rev.2 PS/2キーボードシステムに対応
#include <plib.h>
#include <stdlib.h>
#include "videoout.h"
#include "vector2.h"
#include "xprintf.h"

#include <stdint.h>
#include <xc.h>

class hello{
private:
  int num;
public:
  hello(int n);
  int main();
};


using namespace video;

int xp=0;
void _putc(char ch){
  if(ch == '\n'){
    xp/=30;
    xp*=30;
    xp+=30;
    return;
  }else if(ch == '\0')return;
  
  video::putfont((xp%30)*8,(xp/30)*10,7,0,ch);
  xp++;
}

hello::hello(int n){
  this->num = n;
}  

int hello::main(){
  String s;
  OSCConfig(OSC_POSC_PLL, OSC_PLL_MULT_15, OSC_PLL_POST_1, 0);
  // 周辺機能ピン割り当て
  SDI2R = 2; //RPA4:SDI2
  RPB5R = 4; //RPB5:SDO2

  //ポートの初期設定
  TRISA = 0x0010; // RA4は入力
  CNPUA = 0x0010; // RA4をプルアップ
  ANSELA = 0x0000; // 全てデジタル
  TRISB = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT; // ボタン接続ポート入力設定
  CNPUBSET = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT; // プルアップ設定
  ANSELB = 0x0000; // 全てデジタル
  LATACLR = 2; // RA1=0（ボタンモード）

  init_composite();

  xdev_out(_putc);

  vector2 vector(10*256,10*256);
  vector+=vector2(5*256,5*256);
  vector.print();
  vector-=vector2(5*256,5*256);
  vector.print();
  (-vector).print();
  (+vector).print();
  (vector*=512).print();
  (vector*=128).print();
  
  xprintf("this is example 1\n");
  for(int i=0;i<this->num;i++){
    xprintf("HelloWorld\n");
  }
  
  while(1){
    asm("wait");
  }

  return 0;
}



hello Hello(3);
int main(void){

  return Hello.main();
}
