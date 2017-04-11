#ifdef __cplusplus
extern "C"{
#endif
  
// コンポジットカラー信号出力プログラム 高解像度版4（正方画素）　PIC32MX150F128B用ヘッダーファイル　by K.Tanaka
//　4ビット単位VRAM、16色

#define X_RES	256 // 横方向解像度
#define Y_RES	224 // 縦方向解像度
#define H_WORD	(X_RES/4) // 1行当りのワード数(16bit単位)

// 入力ボタンのポート、ビット定義
#define KEYPORT PORTB
#define KEYUP 0x0400
#define KEYDOWN 0x0080
#define KEYLEFT 0x0100
#define KEYRIGHT 0x0200
#define KEYSTART 0x0800
#define KEYFIRE 0x4000

extern volatile char drawing;		//　表示期間中は-1
extern volatile unsigned short drawcount;		//　1画面表示終了ごとに1足す。アプリ側で0にする。
							// 最低1回は画面表示したことのチェックと、アプリの処理が何画面期間必要かの確認に利用。

extern unsigned short VRAM[];
extern unsigned short *volatile VRAMp; //処理中VRAMアドレス
  
#ifdef __cplusplus
}
#endif
  
namespace video{
  void start_composite(void); //カラーコンポジット出力開始
  void stop_composite(void); //カラーコンポジット出力停止
  void init_composite(void); //カラーコンポジット出力初期化
  void clearscreen(void); //画面クリア
  void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g); //カラーパレット設定

  void clearscreen(void);
  //グラフィック画面クリア
  void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g);
  //グラフィック用カラーパレット設定
 
  void pset(int x,int y,unsigned int c);
  // (x,y)の位置にカラーcで点を描画
 
  void putbmpmn(int x,int y,char m,char n,const unsigned char bmp[]);
  // 横m*縦nドットのキャラクターを座標x,yに表示
  // unsigned char bmp[m*n]配列に、単純にカラー番号を並べる
  // カラー番号が0の部分は透明色として扱う
 
  void clrbmpmn(int x,int y,char m,char n);
  // 縦m*横nドットのキャラクター消去
  // カラー0で塗りつぶし
 
  void gline(int x1,int y1,int x2,int y2,unsigned int c);
  // (x1,y1)-(x2,y2)にカラーcで線分を描画
 
  void hline(int x1,int x2,int y,unsigned int c);
  // (x1,y)-(x2,y)への水平ラインを高速描画
 
  void circle(int x0,int y0,unsigned int r,unsigned int c);
  // (x0,y0)を中心に、半径r、カラーcの円を描画
 
  void boxfill(int x1,int y1,int x2,int y2,unsigned int c);
  // (x1,y1),(x2,y2)を対角線とするカラーcで塗られた長方形を描画
 
  void circlefill(int x0,int y0,unsigned int r,unsigned int c);
  // (x0,y0)を中心に、半径r、カラーcで塗られた円を描画
 
  void putfont(int x,int y,unsigned int c,int bc,unsigned char n);
  //8*8ドットのアルファベットフォント表示
  //座標（x,y)、カラー番号c
  //bc:バックグランドカラー、負数の場合無視
  //n:文字番号
 
  void printstr(int x,int y,unsigned int c,int bc,unsigned char *s);
  //座標(x,y)からカラー番号cで文字列sを表示、bc:バックグランドカラー
 
  void printnum(int x,int y,unsigned char c,int bc,unsigned int n);
  //座標(x,y)にカラー番号cで数値nを表示、bc:バックグランドカラー
 
  void printnum2(int x,int y,unsigned char c,int bc,unsigned int n,unsigned char e);
  //座標(x,y)にカラー番号cで数値nを表示、bc:バックグランドカラー、e桁で表示
 
  unsigned int getcolor(int x,int y);
  //座標(x,y)のVRAM上の現在のパレット番号を返す、画面外は0を返す

}

