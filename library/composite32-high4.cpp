// コンポジットカラー信号出力プログラム 高解像度版（正方画素）　PIC32MX150F128B用 Ver1.1　by K.Tanaka
// 出力 PORTB
//　解像度256×224ドット
// 16bitの上位4ビットが左ドット、下位が右ドットとなるVRAM形式（16色対応）
// カラーパレット対応
// クロック周波数3.579545MHz×15倍
// 1ドットがカラーサブキャリアの5分の3周期（9クロック）
// 1ドットあたり2回信号出力（0クロック目と4クロック目）


// 出力に74HC04等のインバーターをバッファ代わりに接続する場合は、
// 以下の"#define INVERTER"を有効にする

//#define INVERTER

#include "videoout.h"
#include <plib.h>


//カラー信号出力データ
//
#ifndef INVERTER

// 5bit DA、ストレート出力の場合の信号定数データ
#define C_SYN	0
#define C_BLK	6
#define C_BST1	6
#define C_BST2	3
#define C_BST3	8

#else

// 5bit DA、インバーター出力の場合こちらを有効にする
#define C_SYN	31
#define C_BLK	25
#define C_BST1	25
#define C_BST2	28
#define C_BST3	23
//　インバーター出力用データここまで

#endif

// パルス幅定数
#define V_NTSC		262					// 262本/画面
#define V_SYNC		10					// 垂直同期本数
#define V_PREEQ		18					// ブランキング区間上側
#define V_LINE		Y_RES				// 画像描画区間
#define H_NTSC		3405				// 約63.5μsec
#define H_SYNC		252					// 水平同期幅、約4.7μsec
#define H_CBST		285					// カラーバースト開始位置（水平同期立下りから）
#define H_BACK		483					// 左スペース（水平同期立ち上がりから）


// グローバル変数定義
unsigned short VRAM[H_WORD*Y_RES] __attribute__ ((aligned (4)));
unsigned short *volatile VRAMp; //VRAMと処理中VRAMアドレス
volatile unsigned short LineCount;	// 処理中の行
volatile unsigned short drawcount;	//　1画面表示終了ごとに1足す。アプリ側で0にする。
// 最低1回は画面表示したことのチェックと、アプリの処理が何画面期間必要かの確認に利用。
volatile char drawing;		//　映像区間処理中は-1、その他は0

//カラー信号テーブル
//16色分のカラーパレットテーブル
//5ドット分の信号データを保持。順に出力する
//位相は15分の0,4,9,13,3,7,12,1,6,10の順
unsigned char ClTable[16*16];

extern "C"{

 
const unsigned char FontData[256*8]={
 //フォントデータ、キャラクタコード順に8バイトずつ、上位ビットが左
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x08,0x0C,0xFE,0xFE,0x0C,0x08,0x00,
  0x00,0x20,0x60,0xFE,0xFE,0x60,0x20,0x00,
  0x18,0x3C,0x7E,0x18,0x18,0x18,0x18,0x00,
  0x00,0x18,0x18,0x18,0x18,0x7E,0x3C,0x18,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x30,0x30,0x30,0x30,0x00,0x00,0x30,0x00,
  0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,
  0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00,
  0x18,0x7E,0xD8,0x7E,0x1A,0xFE,0x18,0x00,
  0xE0,0xE6,0x0C,0x18,0x30,0x6E,0xCE,0x00,
  0x78,0xCC,0xD8,0x70,0xDE,0xCC,0x76,0x00,
  0x0C,0x18,0x30,0x00,0x00,0x00,0x00,0x00,
  0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00,
  0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00,
  0xD6,0x7C,0x38,0xFE,0x38,0x7C,0xD6,0x00,
  0x00,0x30,0x30,0xFC,0x30,0x30,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,
  0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x38,0x38,0x00,
  0x00,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00,
  0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
  0x18,0x38,0x78,0x18,0x18,0x18,0x7E,0x00,
  0x7C,0xC6,0x06,0x1C,0x70,0xC0,0xFE,0x00,
  0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00,
  0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00,
  0xFE,0xC0,0xF8,0x0C,0x06,0xCC,0x78,0x00,
  0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00,
  0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00,
  0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00,
  0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00,
  0x00,0x30,0x00,0x00,0x00,0x30,0x00,0x00,
  0x00,0x30,0x00,0x00,0x00,0x30,0x30,0x60,
  0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00,
  0x00,0x00,0xFE,0x00,0xFE,0x00,0x00,0x00,
  0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00,
  0x7C,0xC6,0x06,0x1C,0x30,0x00,0x30,0x00,
  0x3C,0x66,0xDE,0xF6,0xDC,0x60,0x3E,0x00,
  0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,
  0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00,
  0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
  0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00,
  0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00,
  0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00,
  0x3C,0x66,0xC0,0xCE,0xC6,0x66,0x3C,0x00,
  0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,
  0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,
  0x1E,0x0C,0x0C,0x0C,0x0C,0xCC,0x78,0x00,
  0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00,
  0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00,
  0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00,
  0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00,
  0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00,
  0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00,
  0x38,0x6C,0xC6,0xC6,0xDE,0x6C,0x3E,0x00,
  0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00,
  0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00,
  0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,
  0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
  0xC6,0xC6,0xC6,0x6C,0x6C,0x38,0x38,0x00,
  0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00,
  0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00,
  0xCC,0xCC,0xCC,0x78,0x30,0x30,0x30,0x00,
  0xFE,0x06,0x0C,0x38,0x60,0xC0,0xFE,0x00,
  0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,
  0xCC,0xCC,0x78,0xFC,0x30,0xFC,0x30,0x00,
  0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,
  0x30,0x78,0xCC,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00,
  0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x7C,0x0C,0x7C,0xCC,0x7E,0x00,
  0xC0,0xC0,0xFC,0xE6,0xC6,0xE6,0xFC,0x00,
  0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00,
  0x06,0x06,0x7E,0xCE,0xC6,0xCE,0x7E,0x00,
  0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00,
  0x1C,0x36,0x30,0xFC,0x30,0x30,0x30,0x00,
  0x00,0x00,0x7E,0xCE,0xCE,0x7E,0x06,0x7C,
  0xC0,0xC0,0xFC,0xE6,0xC6,0xC6,0xC6,0x00,
  0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00,
  0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0xCC,0x78,
  0xC0,0xC0,0xCC,0xD8,0xF0,0xF8,0xCC,0x00,
  0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,
  0x00,0x00,0xFC,0xB6,0xB6,0xB6,0xB6,0x00,
  0x00,0x00,0xFC,0xE6,0xC6,0xC6,0xC6,0x00,
  0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00,
  0x00,0x00,0xFC,0xE6,0xE6,0xFC,0xC0,0xC0,
  0x00,0x00,0x7E,0xCE,0xCE,0x7E,0x06,0x06,
  0x00,0x00,0xDC,0xE6,0xC0,0xC0,0xC0,0x00,
  0x00,0x00,0x7E,0xC0,0x7C,0x06,0xFC,0x00,
  0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0x00,
  0x00,0x00,0xC6,0xC6,0xC6,0xCE,0x76,0x00,
  0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00,
  0x00,0x00,0x86,0xB6,0xB6,0xB6,0xFC,0x00,
  0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00,
  0x00,0x00,0xC6,0xC6,0xCE,0x7E,0x06,0x7C,
  0x00,0x00,0xFE,0x0C,0x38,0x60,0xFE,0x00,
  0x3C,0x60,0x60,0xC0,0x60,0x60,0x3C,0x00,
  0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x00,
  0xF0,0x18,0x18,0x0C,0x18,0x18,0xF0,0x00,
  0x60,0xB6,0x1C,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
  0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,
  0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,
  0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,
  0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
  0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
  0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,0xE0,
  0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
  0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,
  0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
  0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,
  0x18,0x18,0x18,0x18,0xFF,0x18,0x18,0x18,
  0x18,0x18,0x18,0x18,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xFF,0x18,0x18,0x18,
  0x18,0x18,0x18,0x18,0xF8,0x18,0x18,0x18,
  0x18,0x18,0x18,0x18,0x1F,0x18,0x18,0x18,
  0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,
  0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
  0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
  0x00,0x00,0x00,0x00,0x1F,0x18,0x18,0x18,
  0x00,0x00,0x00,0x00,0xF8,0x18,0x18,0x18,
  0x18,0x18,0x18,0x18,0x1F,0x00,0x00,0x00,
  0x18,0x18,0x18,0x18,0xF8,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x07,0x0C,0x18,0x18,
  0x00,0x00,0x00,0x00,0xE0,0x30,0x18,0x18,
  0x18,0x18,0x18,0x0C,0x07,0x00,0x00,0x00,
  0x18,0x18,0x18,0x30,0xE0,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x78,0x68,0x78,0x00,
  0x78,0x60,0x60,0x60,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x18,0x18,0x18,0x78,0x00,
  0x00,0x00,0x00,0x00,0x60,0x30,0x18,0x00,
  0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,
  0xFE,0x06,0x06,0xFE,0x06,0x0C,0x78,0x00,
  0x00,0x00,0xFC,0x0C,0x38,0x30,0x60,0x00,
  0x00,0x00,0x0C,0x18,0x38,0x78,0x18,0x00,
  0x00,0x00,0x30,0xFC,0xCC,0x0C,0x38,0x00,
  0x00,0x00,0x00,0xFC,0x30,0x30,0xFC,0x00,
  0x00,0x00,0x18,0xFC,0x38,0x78,0xD8,0x00,
  0x00,0x00,0x60,0xFC,0x6C,0x68,0x60,0x00,
  0x00,0x00,0x00,0x78,0x18,0x18,0xFC,0x00,
  0x00,0x00,0x7C,0x0C,0x7C,0x0C,0x7C,0x00,
  0x00,0x00,0x00,0xAC,0xAC,0x0C,0x38,0x00,
  0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x00,
  0xFE,0x06,0x06,0x34,0x38,0x30,0x60,0x00,
  0x06,0x0C,0x18,0x38,0x78,0xD8,0x18,0x00,
  0x18,0xFE,0xC6,0xC6,0x06,0x0C,0x38,0x00,
  0x00,0x7E,0x18,0x18,0x18,0x18,0x7E,0x00,
  0x18,0xFE,0x18,0x38,0x78,0xD8,0x18,0x00,
  0x30,0xFE,0x36,0x36,0x36,0x36,0x6C,0x00,
  0x18,0x7E,0x18,0x7E,0x18,0x18,0x18,0x00,
  0x3E,0x66,0xC6,0x0C,0x18,0x30,0xE0,0x00,
  0x60,0x7E,0xD8,0x18,0x18,0x18,0x30,0x00,
  0x00,0xFE,0x06,0x06,0x06,0x06,0xFE,0x00,
  0x6C,0xFE,0x6C,0x0C,0x0C,0x18,0x30,0x00,
  0x00,0xF0,0x00,0xF6,0x06,0x0C,0xF8,0x00,
  0xFE,0x06,0x0C,0x18,0x38,0x6C,0xC6,0x00,
  0x60,0xFE,0x66,0x6C,0x60,0x60,0x3E,0x00,
  0xC6,0xC6,0x66,0x06,0x0C,0x18,0xF0,0x00,
  0x3E,0x66,0xE6,0x3C,0x18,0x30,0xE0,0x00,
  0x0C,0x78,0x18,0xFE,0x18,0x18,0xF0,0x00,
  0x00,0xD6,0xD6,0xD6,0x0C,0x18,0xF0,0x00,
  0x7C,0x00,0xFE,0x18,0x18,0x30,0x60,0x00,
  0x30,0x30,0x38,0x3C,0x36,0x30,0x30,0x00,
  0x18,0x18,0xFE,0x18,0x18,0x30,0x60,0x00,
  0x00,0x7C,0x00,0x00,0x00,0x00,0xFE,0x00,
  0x00,0x7E,0x06,0x6C,0x18,0x36,0x60,0x00,
  0x18,0x7E,0x0C,0x18,0x3C,0x7E,0x18,0x00,
  0x06,0x06,0x06,0x0C,0x18,0x30,0x60,0x00,
  0x30,0x18,0x0C,0xC6,0xC6,0xC6,0xC6,0x00,
  0xC0,0xC0,0xFE,0xC0,0xC0,0xC0,0x7E,0x00,
  0x00,0xFE,0x06,0x06,0x0C,0x18,0x70,0x00,
  0x00,0x30,0x78,0xCC,0x06,0x06,0x00,0x00,
  0x18,0x18,0xFE,0x18,0xDB,0xDB,0x18,0x00,
  0xFE,0x06,0x06,0x6C,0x38,0x30,0x18,0x00,
  0x00,0x3C,0x00,0x3C,0x00,0x7C,0x06,0x00,
  0x0C,0x18,0x30,0x60,0xCC,0xFC,0x06,0x00,
  0x02,0x36,0x3C,0x18,0x3C,0x6C,0xC0,0x00,
  0x00,0xFE,0x30,0xFE,0x30,0x30,0x3E,0x00,
  0x30,0x30,0xFE,0x36,0x3C,0x30,0x30,0x00,
  0x00,0x78,0x18,0x18,0x18,0x18,0xFE,0x00,
  0xFE,0x06,0x06,0xFE,0x06,0x06,0xFE,0x00,
  0x7C,0x00,0xFE,0x06,0x0C,0x18,0x30,0x00,
  0xC6,0xC6,0xC6,0x06,0x06,0x0C,0x38,0x00,
  0x6C,0x6C,0x6C,0x6E,0x6E,0x6C,0xC8,0x00,
  0x60,0x60,0x60,0x66,0x6C,0x78,0x70,0x00,
  0x00,0xFE,0xC6,0xC6,0xC6,0xC6,0xFE,0x00,
  0x00,0xFE,0xC6,0xC6,0x06,0x0C,0x38,0x00,
  0x00,0xF0,0x06,0x06,0x0C,0x18,0xF0,0x00,
  0x18,0xCC,0x60,0x00,0x00,0x00,0x00,0x00,
  0x70,0xD8,0x70,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,
  0x18,0x18,0x1F,0x18,0x18,0x1F,0x18,0x18,
  0x18,0x18,0xFF,0x18,0x18,0xFF,0x18,0x18,
  0x18,0x18,0xF8,0x18,0x18,0xF8,0x18,0x18,
  0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF,
  0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF,
  0xFF,0x7F,0x3F,0x1F,0x0F,0x07,0x03,0x01,
  0xFF,0xFE,0xFC,0xF8,0xF0,0xE0,0xC0,0x80,
  0x10,0x38,0x7C,0xFE,0xFE,0x38,0x7C,0x00,
  0x6C,0xFE,0xFE,0xFE,0x7C,0x38,0x10,0x00,
  0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10,0x00,
  0x38,0x38,0xFE,0xFE,0xD6,0x10,0x7C,0x00,
  0x00,0x3C,0x7E,0x7E,0x7E,0x7E,0x3C,0x00,
  0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,
  0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,
  0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x03,
  0x83,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x83,
  0xFE,0xB6,0xB6,0xFE,0x86,0x86,0x86,0x00,
  0xC0,0xFE,0xD8,0x7E,0x58,0xFE,0x18,0x00,
  0x7E,0x66,0x7E,0x66,0x7E,0x66,0xC6,0x00,
  0xFE,0xC6,0xC6,0xFE,0xC6,0xC6,0xFE,0x00,
  0x06,0xEF,0xA6,0xFF,0xA2,0xFF,0x0A,0x06,
  0x00,0x38,0x6C,0xC6,0x7C,0x34,0x6C,0x00,
  0xFC,0x6C,0xFE,0x6E,0xF6,0xEC,0x6C,0x78,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
 
  /**********************
   *  Timer2 割り込み処理関数
   ***********************/
  void __ISR(8, ipl5) T2Handler(void)
  {
    asm volatile("#":::"a0");
    asm volatile("#":::"v0");

    //TMR2の値でタイミングのずれを補正
    asm volatile("la	$v0,%0"::"i"(&TMR2));
    asm volatile("lhu	$a0,0($v0)");
    asm volatile("addiu	$a0,$a0,-22");
    asm volatile("bltz	$a0,label1_2");
    asm volatile("addiu	$v0,$a0,-10");
    asm volatile("bgtz	$v0,label1_2");
    asm volatile("sll	$a0,$a0,2");
    asm volatile("la	$v0,label1");
    asm volatile("addu	$a0,$v0");
    asm volatile("jr	$a0");
    asm volatile("label1:");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");

    asm volatile("label1_2:");
    //LATB=C_SYN;
    asm volatile("addiu	$a0,$zero,%0"::"n"(C_SYN));
    asm volatile("la	$v0,%0"::"i"(&LATB));
    asm volatile("sb	$a0,0($v0)");// 同期信号立ち下がり。ここを基準に全ての信号出力のタイミングを調整する

    if(LineCount<V_SYNC){
      // 垂直同期期間
      OC3R = H_NTSC-H_SYNC-3;	// 切り込みパルス幅設定
      OC3CON = 0x8001;
    }
    else{
      OC1R = H_SYNC-3;		// 同期パルス幅4.7usec
      OC1CON = 0x8001;		// タイマ2選択ワンショット
      if(LineCount>=V_SYNC+V_PREEQ && LineCount<V_SYNC+V_PREEQ+V_LINE){
	// 画像描画区間
	OC2R = H_SYNC+H_BACK-3-20;// 画像信号開始のタイミング
	OC2CON = 0x8001;		// タイマ2選択ワンショット
      }
    }
    LineCount++;
    if(LineCount>=V_NTSC) LineCount=0;
    IFS0bits.T2IF = 0;			// T2割り込みフラグクリア
  }
  
  /*********************
   *  OC3割り込み処理関数 垂直同期切り込みパルス
   *********************/
  void __ISR(14, ipl5) OC3Handler(void)
  {
    asm volatile("#":::"v0");
    asm volatile("#":::"v1");
    asm volatile("#":::"a0");

    //TMR2の値でタイミングのずれを補正
    asm volatile("la	$v0,%0"::"i"(&TMR2));
    asm volatile("lhu	$a0,0($v0)");
    asm volatile("addiu	$a0,$a0,%0"::"n"(-(H_NTSC-H_SYNC+22)));
    asm volatile("bltz	$a0,label4_2");
    asm volatile("addiu	$v0,$a0,-10");
    asm volatile("bgtz	$v0,label4_2");
    asm volatile("sll	$a0,$a0,2");
    asm volatile("la	$v0,label4");
    asm volatile("addu	$a0,$v0");
    asm volatile("jr	$a0");

    asm volatile("label4:");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");

    asm volatile("label4_2:");
    // 同期信号のリセット
    //	LATB=C_BLK;
    asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
    asm volatile("la	$v0,%0"::"i"(&LATB));
    asm volatile("sb	$v1,0($v0)");	// 同期信号リセット。同期信号立ち下がりから3153サイクル

    IFS0bits.OC3IF = 0;			// OC3割り込みフラグクリア
  }

  /*********************
   *  OC1割り込み処理関数 水平同期立ち上がり～カラーバースト
   *********************/
  void __ISR(6, ipl5) OC1Handler(void)
  {
    asm volatile("#":::"v0");
    asm volatile("#":::"v1");
    asm volatile("#":::"a0");

    //TMR2の値でタイミングのずれを補正
    asm volatile("la	$v0,%0"::"i"(&TMR2));
    asm volatile("lhu	$a0,0($v0)");
    asm volatile("addiu	$a0,$a0,%0"::"n"(-(H_SYNC+22)));
    asm volatile("bltz	$a0,label2_2");
    asm volatile("addiu	$v0,$a0,-10");
    asm volatile("bgtz	$v0,label2_2");
    asm volatile("sll	$a0,$a0,2");
    asm volatile("la	$v0,label2");
    asm volatile("addu	$a0,$v0");
    asm volatile("jr	$a0");

    asm volatile("label2:");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");

    asm volatile("label2_2:");
    // 同期信号のリセット
    //	LATB=C_BLK;
    asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
    asm volatile("la	$v0,%0"::"i"(&LATB));
    asm volatile("sb	$v1,0($v0)");	// 同期信号リセット。水平同期立ち下がりから252サイクル

    // 28クロックウェイト
    asm volatile("addiu	$a0,$zero,9");
    asm volatile("loop2:");
    asm volatile("addiu	$a0,$a0,-1");
    asm volatile("nop");
    asm volatile("bnez	$a0,loop2");

    // カラーバースト信号 9周期出力
    asm volatile("addiu	$a0,$zero,9");
    asm volatile("la	$v0,%0"::"i"(&LATB));
    asm volatile("loop3:");
    asm volatile("addiu	$v1,$zero,%0"::"n"(C_BST1));
    asm volatile("sb	$v1,0($v0)");	// カラーバースト開始。水平同期立ち下がりから285サイクル
    asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("addiu	$v1,$zero,%0"::"n"(C_BST2));
    asm volatile("sb	$v1,0($v0)");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("addiu	$v1,$zero,%0"::"n"(C_BST3));
    asm volatile("sb	$v1,0($v0)");
    asm volatile("addiu	$a0,$a0,-1");//ループカウンタ
    asm volatile("nop");
    asm volatile("bnez	$a0,loop3");

    asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
    asm volatile("sb	$v1,0($v0)");

    IFS0bits.OC1IF = 0;			// OC1割り込みフラグクリア
  }


  /***********************
   *  OC2割り込み処理関数　映像信号出力
   ***********************/
  void __ISR(10, ipl5) OC2Handler(void)
  {
    // 映像信号出力
    //インラインアセンブラでの破壊レジスタを宣言（スタック退避させるため）
    asm volatile("#":::"v0");
    asm	volatile("#":::"v1");
    asm volatile("#":::"a0");
    asm volatile("#":::"a1");
    asm volatile("#":::"a2");
    asm volatile("#":::"t0");
    asm volatile("#":::"t1");
    asm volatile("#":::"t2");

    //TMR2の値でタイミングのずれを補正
    asm volatile("la	$v0,%0"::"i"(&TMR2));
    asm volatile("lhu	$a0,0($v0)");
    asm volatile("addiu	$a0,$a0,%0"::"n"(-(H_SYNC+H_BACK+22-15)));
    asm volatile("bltz	$a0,label3_2");
    asm volatile("addiu	$v0,$a0,-10");
    asm volatile("bgtz	$v0,label3_2");
    asm volatile("sll	$a0,$a0,2");
    asm volatile("la	$v0,label3");
    asm volatile("addu	$a0,$v0");
    asm volatile("jr	$a0");

    asm volatile("label3:");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
    asm volatile("nop");asm volatile("nop");

    asm volatile("label3_2:");
    //	drawing=-1;
    asm volatile("addiu	$t1,$zero,-1");
    asm volatile("la	$v0,%0"::"i"(&drawing));
    asm volatile("sb	$t1,0($v0)");
    //	t1=VRAMp;
    asm volatile("la	$v0,%0"::"i"(&VRAMp));
    asm volatile("lw	$t1,0($v0)");
    //	v1=ClTable;
    asm volatile("la	$v1,%0"::"i"(ClTable));
    //	v0=&LATB;
    asm volatile("la	$v0,%0"::"i"(&LATB));
    //	t2=0;
    asm volatile("addiu	$t2,$zero,0");

    asm volatile("addiu	$a2,$zero,12"); //ループカウンタ
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");

    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("rotr	$a1,$a1,28");
    //	asm volatile("nop");

    //20ドット×12回ループ
    //-------------------------------------------------
    asm volatile("loop1:"); //ここでLATBに最初の出力。水平同期立ち下がりから735サイクルになるよう調整する
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("addiu	$a2,$a2,-1");//ループカウンタ
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("bnez	$a2,loop1");
    //-------------------------------------------------
    //残16ドット
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,2($a0)");
    asm volatile("lhu	$a1,0($t1)");
    asm volatile("addiu	$t1,$t1,2"); //VRAMp++;
    asm volatile("rotr	$a1,$a1,12");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,3($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,4($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,5($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,6($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,7($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,8($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("rotr	$a1,$a1,28");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,9($a0)");
    asm volatile("ins	$t2,$a1,4,4");
    asm volatile("addu	$a0,$t2,$v1");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,0($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("sb	$t0,0($v0)");
    asm volatile("lbu	$t0,1($a0)");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("sb	$t0,0($v0)");
    //-------------------------------------------------

    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");

    //	LATB=C_BLK;
    asm volatile("addiu	$t0,$zero,%0"::"n"(C_BLK));
    asm volatile("sb	$t0,0($v0)");

    VRAMp+=H_WORD;// 次の行へ
    if(LineCount==V_SYNC+V_PREEQ+V_LINE){ // 1画面最後の描画終了
      drawing=0;
      drawcount++;
      VRAMp=VRAM;
    }
    IFS0bits.OC2IF = 0;			// OC2割り込みフラグクリア
  }
}
void wait60thsec(int s60){
  for(int i=0;i<s60;i++){
    while(drawing);
    while(!drawing);
  }
}

namespace video{

// 画面クリア
void clearscreen(void){
  unsigned int *vp;
  int i;
  vp=(unsigned int *)VRAM;
  for(i=0;i<H_WORD*Y_RES/2;i++) *vp++=0;
}

void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g){
  // カラーパレット設定（5ビットDA、電源3.3V、10ドットで1周のパターン）
  // n:パレット番号、r,g,b:0～255
  // 輝度Y=0.587*G+0.114*B+0.299*R
  // 信号N=Y+0.4921*(B-Y)*sinθ+0.8773*(R-Y)*cosθ
  // 出力データS=(N*0.71[v]+0.29[v])/3.3[v]*64

  int y;
  y=(150*g+29*b+77*r+128)/256;
  ClTable[n*16+0]=(3525*y+   0*((int)b-y)+3093*((int)r-y)+1440*256+32768)/65536;//θ=2Π*0/15
  ClTable[n*16+1]=(3525*y+1725*((int)b-y)- 323*((int)r-y)+1440*256+32768)/65536;//θ=2Π*4/15
  ClTable[n*16+2]=(3525*y-1020*((int)b-y)-2502*((int)r-y)+1440*256+32768)/65536;//θ=2Π*9/15
  ClTable[n*16+3]=(3525*y-1289*((int)b-y)+2069*((int)r-y)+1440*256+32768)/65536;//θ=2Π*13/15
  ClTable[n*16+4]=(3525*y+1650*((int)b-y)+ 956*((int)r-y)+1440*256+32768)/65536;//θ=2Π*3/15
  ClTable[n*16+5]=(3525*y+ 361*((int)b-y)-3025*((int)r-y)+1440*256+32768)/65536;//θ=2Π*7/15
  ClTable[n*16+6]=(3525*y-1650*((int)b-y)+ 956*((int)r-y)+1440*256+32768)/65536;//θ=2Π*12/15
  ClTable[n*16+7]=(3525*y+ 706*((int)b-y)+2825*((int)r-y)+1440*256+32768)/65536;//θ=2Π*1/15
  ClTable[n*16+8]=(3525*y+1020*((int)b-y)-2502*((int)r-y)+1440*256+32768)/65536;//θ=2Π*6/15
  ClTable[n*16+9]=(3525*y-1503*((int)b-y)-1546*((int)r-y)+1440*256+32768)/65536;//θ=2Π*10/15

#ifdef INVERTER
  for(int i=0;i<10;i++) ClTable[n*16+i]^=0x1f;
#endif
}


void start_composite(void){
  // 変数初期設定
  LineCount=0;				// 処理中ラインカウンター
  drawing=0;
  VRAMp=VRAM;

  PR2 = H_NTSC -1; 			// 約63.5usecに設定
  T2CONSET=0x8000;			// タイマ2スタート
}

void stop_composite(void){
  T2CONCLR = 0x8000;			// タイマ2停止
}

// カラーコンポジット出力初期化
void init_composite(void){
  unsigned char i;
  clearscreen();
  //カラーパレット初期化
  for(i=0;i<8;i++){
    set_palette(i,255*(i&1),255*((i>>1)&1),255*(i>>2));
  }
  for(i=0;i<8;i++){
    set_palette(8+i,128*(i&1),128*((i>>1)&1),128*(i>>2));
  }

  // タイマ2の初期設定,内部クロックで63.5usec周期、1:1
  T2CON = 0x0000;				// タイマ2停止状態
  IPC2bits.T2IP = 5;			// 割り込みレベル5
  IFS0bits.T2IF = 0;
  IEC0bits.T2IE = 1;			// タイマ2割り込み有効化

  // OC1の割り込み有効化
  IPC1bits.OC1IP = 5;			// 割り込みレベル5
  IFS0bits.OC1IF = 0;
  IEC0bits.OC1IE = 1;			// OC1割り込み有効化

  // OC2の割り込み有効化
  IPC2bits.OC2IP = 5;			// 割り込みレベル5
  IFS0bits.OC2IF = 0;
  IEC0bits.OC2IE = 1;			// OC2割り込み有効化

  // OC3の割り込み有効化
  IPC3bits.OC3IP = 5;			// 割り込みレベル5
  IFS0bits.OC3IF = 0;
  IEC0bits.OC3IE = 1;			// OC3割り込み有効化

  OSCCONCLR=0x10; // WAIT命令はアイドルモード
  BMXCONCLR=0x40;	// RAMアクセスウェイト0
  INTEnableSystemMultiVectoredInt();
  start_composite();
}
 
// (x,y)の位置にカラーcで点を描画
void pset(int x,int y,unsigned int c){
  unsigned short *ad,d1;
 
  if((unsigned int)x>=X_RES) return;
  if((unsigned int)y>=Y_RES) return;
  c&=15;
  ad=VRAM+y*H_WORD+x/4;
  d1=~(0xf000>>(x%4)*4);
  c<<=(3-(x%4))*4;
  *ad&=d1;
  *ad|=c;
}
 
// 横m*縦nドットのキャラクターを座標x,yに表示
// unsigned char bmp[m*n]配列に、単純にカラー番号を並べる
// カラー番号が0の部分は透明色として扱う
void putbmpmn(int x,int y,char m,char n,const unsigned char bmp[])
{
  int i,j,k;
  unsigned short *vp;
  const unsigned char *p;
  if(x<=-m || x>=X_RES || y<=-n || y>=Y_RES) return; //画面外
  if(m<=0 || n<=0) return;
  if(y<0){ //画面上部に切れる場合
    i=0;
    p=bmp-y*m;
  }
  else{
    i=y;
    p=bmp;
  }
  for(;i<y+n;i++){
    if(i>=Y_RES) return; //画面下部に切れる場合
    if(x<0){ //画面左に切れる場合は残る部分のみ描画
      j=0;
      p+=-x;
      vp=VRAM+i*H_WORD;
    }
    else{
      j=x;
      vp=VRAM+i*H_WORD+x/4;
    }
    k=j%4;
    if(k!=0){ //16ビット境界の左側描画
      if(k==1){
	if(*p) *vp=(*vp&0xf0ff)|(*p<<8);
	p++;
	j++;
	if(j>=x+m) continue;
      }
      if(k<=2){
	if(*p) *vp=(*vp&0xff0f)|(*p<<4);
	p++;
	j++;
	if(j>=x+m) continue;
      }
      if(*p) *vp=(*vp&0xfff0)|*p;
      p++;
      j++;
      if(j>=x+m) continue;
      vp++;
    }
    //ここから16ビット境界の内部および右側描画
    while(1){
      if(j>=X_RES){ //画面右に切れる場合
	p+=x+m-j;
	break;
      }
      if(*p) *vp=(*vp&0x0fff)|(*p<<12);
      p++;
      j++;
      if(j>=x+m) break;
      if(*p) *vp=(*vp&0xf0ff)|(*p<<8);
      p++;
      j++;
      if(j>=x+m) break;
      if(*p) *vp=(*vp&0xff0f)|(*p<<4);
      p++;
      j++;
      if(j>=x+m) break;
      if(*p) *vp=(*vp&0xfff0)|*p;
      p++;
      j++;
      if(j>=x+m) break;
      vp++;
    }
  }
}
 
// 縦m*横nドットのキャラクター消去
// カラー0で塗りつぶし
void clrbmpmn(int x,int y,char m,char n)
{
  int i,j,k;
  unsigned short mask,*vp;
 
  if(x<=-m || x>=X_RES || y<=-n || y>=Y_RES) return; //画面外
  if(m<=0 || n<=0) return;
  if(y<0){ //画面上部に切れる場合
    i=0;
  }
  else{
    i=y;
  }
  for(;i<y+n;i++){
    if(i>=Y_RES) return; //画面下部に切れる場合
    if(x<0){ //画面左に切れる場合は残る部分のみ消去
      j=0;
      vp=VRAM+i*H_WORD;
    }
    else{
      j=x;
      vp=VRAM+i*H_WORD+x/4;
    }
    k=j%4;
    if(k!=0){ //16ビット境界の左側消去
      if(k==1){
	*vp&=0xf0ff;
	j++;
	if(j>=x+m) continue;
      }
      if(k<=2){
	*vp&=0xff0f;
	j++;
	if(j>=x+m) continue;
      }
      *vp&=0xfff0;
      j++;
      if(j>=x+m) continue;
      vp++;
    }
    //ここから16ビット境界の内部消去
    for(;j<x+m-3;j+=4){
      if(j>=X_RES) break; //画面右に切れる場合
      *vp++=0;
    }
    if(j<X_RES){
      k=x+m-j;
      if(k!=0){ //16ビット境界の右側消去
	mask=0xffff>>(k*4);
	*vp&=mask;
      }
    }
  }
}
 
// (x1,y1)-(x2,y2)にカラーcで線分を描画
void gline(int x1,int y1,int x2,int y2,unsigned int c)
{
  int sx,sy,dx,dy,i;
  int e;
 
  if(x2>x1){
    dx=x2-x1;
    sx=1;
  }
  else{
    dx=x1-x2;
    sx=-1;
  }
  if(y2>y1){
    dy=y2-y1;
    sy=1;
  }
  else{
    dy=y1-y2;
    sy=-1;
  }
  if(dx>=dy){
    e=-dx;
    for(i=0;i<=dx;i++){
      pset(x1,y1,c);
      x1+=sx;
      e+=dy*2;
      if(e>=0){
	y1+=sy;
	e-=dx*2;
      }
    }
  }
  else{
    e=-dy;
    for(i=0;i<=dy;i++){
      pset(x1,y1,c);
      y1+=sy;
      e+=dx*2;
      if(e>=0){
	x1+=sx;
	e-=dy*2;
      }
    }
  }
}

// (x1,y)-(x2,y)への水平ラインを高速描画
void hline(int x1,int x2,int y,unsigned int c)
{
  int temp;
  unsigned short d,*ad;
 
  if(y<0 || y>=Y_RES) return;
  if(x1>x2){
    temp=x1;
    x1=x2;
    x2=temp;
  }
  if(x2<0 || x1>=X_RES) return;
  if(x1<0) x1=0;
  if(x2>=X_RES) x2=X_RES-1;
  while(x1&3){
    pset(x1++,y,c);
    if(x1>x2) return;
  }
  d=c|(c<<4)|(c<<8)|(c<<12);
  ad=VRAM+y*H_WORD+x1/4;
  while(x1+3<=x2){
    *ad++=d;
    x1+=4;
  }
  while(x1<=x2) pset(x1++,y,c);
}
 
// (x0,y0)を中心に、半径r、カラーcの円を描画
void circle(int x0,int y0,unsigned int r,unsigned int c)
{
  int x,y,f;
  x=r;
  y=0;
  f=-2*r+3;
  while(x>=y){
    pset(x0-x,y0-y,c);
    pset(x0-x,y0+y,c);
    pset(x0+x,y0-y,c);
    pset(x0+x,y0+y,c);
    pset(x0-y,y0-x,c);
    pset(x0-y,y0+x,c);
    pset(x0+y,y0-x,c);
    pset(x0+y,y0+x,c);
    if(f>=0){
      x--;
      f-=x*4;
    }
    y++;
    f+=y*4+2;
  }
}
void boxfill(int x1,int y1,int x2,int y2,unsigned int c)
// (x1,y1),(x2,y2)を対角線とするカラーcで塗られた長方形を描画
{
  int temp;
  if(x1>x2){
    temp=x1;
    x1=x2;
    x2=temp;
  }
  if(x2<0 || x1>=X_RES) return;
  if(y1>y2){
    temp=y1;
    y1=y2;
    y2=temp;
  }
  if(y2<0 || y1>=Y_RES) return;
  if(y1<0) y1=0;
  if(y2>=Y_RES) y2=Y_RES-1;
  while(y1<=y2){
    hline(x1,x2,y1++,c);
  }
}
void circlefill(int x0,int y0,unsigned int r,unsigned int c)
// (x0,y0)を中心に、半径r、カラーcで塗られた円を描画
{
  int x,y,f;
  x=r;
  y=0;
  f=-2*r+3;
  while(x>=y){
    hline(x0-x,x0+x,y0-y,c);
    hline(x0-x,x0+x,y0+y,c);
    hline(x0-y,x0+y,y0-x,c);
    hline(x0-y,x0+y,y0+x,c);
    if(f>=0){
      x--;
      f-=x*4;
    }
    y++;
    f+=y*4+2;
  }
}
 
//8*8ドットのアルファベットフォント表示
//座標（x,y)、カラー番号c
//bc:バックグランドカラー、負数の場合無視
//n:文字番号
void putfont(int x,int y,unsigned int c,int bc,unsigned char n)
{
  int i,j,k;
  unsigned int d1,mask;
  unsigned char d;
  const unsigned char *p;
  unsigned short *ad;
 
  p=FontData+n*8;
  //p=fontp+n*8;
  c&=15;
  if(bc>=0) bc&=15;
  if(x<0 || x+7>=X_RES || y<0 || y+7>=Y_RES){
    for(i=0;i<8;i++){
      d=*p++;
      for(j=0;j<8;j++){
	if(d&0x80) pset(x+j,y+i,c);
	else if(bc>=0) pset(x+j,y+i,bc);
	d<<=1;
      }
    }
    return;
  }
  ad=VRAM+y*H_WORD+x/4;
  k=x&3;
  for(i=0;i<8;i++){
    d=*p;
    d1=0;
    mask=0;
    for(j=0;j<8;j++){
      d1<<=4;
      mask<<=4;
      if(d&0x80) d1+=c;
      else if(bc>=0) d1+=bc;
      else mask+=15;
      d<<=1;
    }
    if(k==0){
      if(bc>=0){
	*ad=d1>>16;
	*(ad+1)=(unsigned short)d1;
      }
      else{
	*ad=(*ad & (mask>>16))|(d1>>16);
	*(ad+1)=(*(ad+1) & (unsigned short)mask)|(unsigned short)d1;
      }
    }
    else if(k==1){
      if(bc>=0){
	*ad=(*ad & 0xf000)|(d1>>20);
	*(ad+1)=(unsigned short)(d1>>4);
	*(ad+2)=(*(ad+2) & 0x0fff)|((unsigned short)d1<<12);
      }
      else{
	*ad=(*ad & ((mask>>20)|0xf000))|(d1>>20);
	*(ad+1)=(*(ad+1) & (unsigned short)(mask>>4))|(unsigned short)(d1>>4);
	*(ad+2)=(*(ad+2) & (((unsigned short)mask<<12)|0x0fff))|((unsigned short)d1<<12);
      }
    }
    else if(k==2){
      if(bc>=0){
	*ad=(*ad & 0xff00)|(d1>>24);
	*(ad+1)=(unsigned short)(d1>>8);
	*(ad+2)=(*(ad+2) & 0x00ff)|((unsigned short)d1<<8);
      }
      else{
	*ad=(*ad & ((mask>>24)|0xff00))|(d1>>24);
	*(ad+1)=(*(ad+1) & (unsigned short)(mask>>8))|(unsigned short)(d1>>8);
	*(ad+2)=(*(ad+2) & (((unsigned short)mask<<8)|0x00ff))|((unsigned short)d1<<8);
      }
    }
    else{
      if(bc>=0){
	*ad=(*ad & 0xfff0)|(d1>>28);
	*(ad+1)=(unsigned short)(d1>>12);
	*(ad+2)=(*(ad+2) & 0x000f)|((unsigned short)d1<<4);
      }
      else{
	*ad=(*ad & ((mask>>28)|0xfff0))|(d1>>28);
	*(ad+1)=(*(ad+1) & (unsigned short)(mask>>12))|(unsigned short)(d1>>12);
	*(ad+2)=(*(ad+2) & (((unsigned short)mask<<4)|0x000f))|((unsigned short)d1<<4);
      }
    }
    p++;
    ad+=H_WORD;
  }
}

//座標(x,y)からカラー番号cで文字列sを表示、bc:バックグランドカラー
void printstr(int x,int y,unsigned int c,int bc,unsigned char *s){
  while(*s){
    putfont(x,y,c,bc,*s++);
    x+=8;
  }
}

//座標(x,y)にカラー番号cで数値nを表示、bc:バックグランドカラー
void printnum(int x,int y,unsigned char c,int bc,unsigned int n){
  unsigned int d,e;
  d=10;
  e=0;
  while(n>=d){
    e++;
    if(e==9) break;
    d*=10;
  }       
  x+=e*8;
  do{
    putfont(x,y,c,bc,'0'+n%10);
    n/=10;
    x-=8;
  }while(n!=0);
}

//座標(x,y)にカラー番号cで数値nを表示、bc:バックグランドカラー、e桁で表示
void printnum2(int x,int y,unsigned char c,int bc,unsigned int n,unsigned char e){
  if(e==0) return;
  x+=(e-1)*8;
  do{
    putfont(x,y,c,bc,'0'+n%10);
    e--;
    n/=10;
    x-=8;
  }while(e!=0 && n!=0);
  while(e!=0){
    putfont(x,y,c,bc,' ');
    x-=8;
    e--;
  }
}

//座標(x,y)のVRAM上の現在のパレット番号を返す、画面外は0を返す
unsigned int color(int x,int y){
  unsigned short *ad;
 
  if((unsigned int)x>=X_RES) return 0;
  if((unsigned int)y>=Y_RES) return 0;
  ad=VRAM+y*H_WORD+x/4;
  return (*ad >>(3-(x&3))*4) & 0xf;
}
}
