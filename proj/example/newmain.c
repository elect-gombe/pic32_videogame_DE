#define _SUPPRESS_PLIB_WARNING
// TETRIS（キャラクターベース） Rev.1.0 2013/8/22
// 　with テキストVRAMコンポジット出力システム for PIC32MX1xx/2xx by K.Tanaka
// rev.2 PS/2キーボードシステムに対応
#include <plib.h>
#include <stdlib.h>
#include "videoout.h"

#include "ff.h"
#include "mylib.h"
#include <stdint.h>
#include <xc.h>

// 入力ボタンのポート、ビット定義
#define KEYPORT PORTB
#define KEYUP 0x0400
#define KEYDOWN 0x0080
#define KEYLEFT 0x0100
#define KEYRIGHT 0x0200
#define KEYSTART 0x0800
#define KEYFIRE 0x4000

#define SAMPLING_FREQ 32000
#define OUTPUT_FREQ 100000
#define CLOCK_FREQ (3.58*1000000*15)


#define SIZEOFSOUNDBF 2048

#define HEIGHT 192


//外付けクリスタル with PLL (15倍)
#pragma config PMDL1WAY = OFF, IOL1WAY = OFF
#pragma config FPLLIDIV = DIV_1, FPLLMUL = MUL_15, FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL, FSOSCEN = OFF, POSCMOD = XT, OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1, FWDTEN = OFF, JTAGEN = OFF, ICESEL = ICS_PGx1

uint16_t VRAMA[256*224/4];
uint16_t *VRAM = VRAMA;

typedef unsigned int uint;

void audiotask(void);

unsigned char sounddata[SIZEOFSOUNDBF] = {0};

FATFS fatfs;

#define PADDLE_Y ((224-20)*256)
#define PADDLE_WIDTH (80*256)

const uint8_t ballimgred[16*16]={
    0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,
    0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,
    0,0,0,2,2,2,2,2,2,2,2,2,10,0,0,0,
    0,0,2,2,2,2,2,2,2,2,2,2,2,10,0,0,
    0,2,2,2,2,2,2,2,2,2,2,2,2,10,10,0,
    0,2,2,2,7,7,2,2,2,2,2,2,2,10,10,0,
    2,2,2,2,7,2,2,2,2,2,2,2,2,10,10,10,
    2,2,2,2,2,2,2,2,2,2,2,2,10,10,10,10,
    2,2,2,2,2,2,2,2,2,2,2,2,10,10,10,10,
    10,2,2,2,2,2,2,2,2,2,2,2,10,10,10,10,
    0,10,2,2,2,2,2,2,2,2,10,10,10,10,10,0,
    0,10,10,10,2,2,2,10,10,10,10,10,10,10,10,0,
    0,0,10,10,10,10,10,10,10,10,10,10,10,10,0,0,
    0,0,0,10,10,10,10,10,10,10,10,10,10,0,0,0,
    0,0,0,0,10,10,10,10,10,10,10,10,0,0,0,0,
    0,0,0,0,0,0,10,10,10,10,0,0,0,0,0,0,
};
const uint8_t ballimgpink[16*16]={
    0,0,0,0,0,0,3,3,3,3,0,0,0,0,0,0,
    0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,
    0,0,0,3,3,3,3,3,3,3,3,3,11,0,0,0,
    0,0,3,3,3,3,3,3,3,3,3,3,3,11,0,0,
    0,3,3,3,3,3,3,3,3,3,3,3,3,11,11,0,
    0,3,3,3,7,7,3,3,3,3,3,3,3,11,11,0,
    3,3,3,3,7,3,3,3,3,3,3,3,3,11,11,11,
    3,3,3,3,3,3,3,3,3,3,3,3,11,11,11,11,
    3,3,3,3,3,3,3,3,3,3,3,3,11,11,11,11,
    11,3,3,3,3,3,3,3,3,3,3,3,11,11,11,11,
    0,11,3,3,3,3,3,3,3,3,11,11,11,11,11,0,
    0,11,11,11,3,3,3,11,11,11,11,11,11,11,11,0,
    0,0,11,11,11,11,11,11,11,11,11,11,11,11,0,0,
    0,0,0,11,11,11,11,11,11,11,11,11,11,0,0,0,
    0,0,0,0,11,11,11,11,11,11,11,11,0,0,0,0,
    0,0,0,0,0,0,11,11,11,11,0,0,0,0,0,0,
};
const uint8_t ballimggreen[16*16]={
    0,0,0,0,0,0,4,4,4,4,0,0,0,0,0,0,
    0,0,0,0,4,4,4,4,4,4,4,4,0,0,0,0,
    0,0,0,4,4,4,4,4,4,4,4,4,12,0,0,0,
    0,0,4,4,4,4,4,4,4,4,4,4,4,12,0,0,
    0,4,4,4,4,4,4,4,4,4,4,4,4,12,12,0,
    0,4,4,4,7,7,4,4,4,4,4,4,4,12,12,0,
    4,4,4,4,7,4,4,4,4,4,4,4,4,12,12,12,
    4,4,4,4,4,4,4,4,4,4,4,4,12,12,12,12,
    4,4,4,4,4,4,4,4,4,4,4,4,12,12,12,12,
    12,4,4,4,4,4,4,4,4,4,4,4,12,12,12,12,
    0,12,4,4,4,4,4,4,4,4,12,12,12,12,12,0,
    0,12,12,12,4,4,4,12,12,12,12,12,12,12,12,0,
    0,0,12,12,12,12,12,12,12,12,12,12,12,12,0,0,
    0,0,0,12,12,12,12,12,12,12,12,12,12,0,0,0,
    0,0,0,0,12,12,12,12,12,12,12,12,0,0,0,0,
    0,0,0,0,0,0,12,12,12,12,0,0,0,0,0,0,
};
const uint8_t ballimgcyan[16*16]={
        0,0,0,0,0,0,5,5,5,5,0,0,0,0,0,0,
    0,0,0,0,5,5,5,5,5,5,5,5,0,0,0,0,
    0,0,0,5,5,5,5,5,5,5,5,5,13,0,0,0,
    0,0,5,5,5,5,5,5,5,5,5,5,5,13,0,0,
    0,5,5,5,5,5,5,5,5,5,5,5,5,13,13,0,
    0,5,5,5,7,7,5,5,5,5,5,5,5,13,13,0,
    5,5,5,5,7,5,5,5,5,5,5,5,5,13,13,13,
    5,5,5,5,5,5,5,5,5,5,5,5,13,13,13,13,
    5,5,5,5,5,5,5,5,5,5,5,5,13,13,13,13,
    13,5,5,5,5,5,5,5,5,5,5,5,13,13,13,13,
    0,13,5,5,5,5,5,5,5,5,13,13,13,13,13,0,
    0,13,13,13,5,5,5,13,13,13,13,13,13,13,13,0,
    0,0,13,13,13,13,13,13,13,13,13,13,13,13,0,0,
    0,0,0,13,13,13,13,13,13,13,13,13,13,0,0,0,
    0,0,0,0,13,13,13,13,13,13,13,13,0,0,0,0,
    0,0,0,0,0,0,13,13,13,13,0,0,0,0,0,0,
};

const uint8_t ballimgyellow[16*16]={
    0,0,0,0,0,0,6,6,6,6,0,0,0,0,0,0,
    0,0,0,0,6,6,6,6,6,6,6,6,0,0,0,0,
    0,0,0,6,6,6,6,6,6,6,6,6,14,0,0,0,
    0,0,6,6,6,6,6,6,6,6,6,6,6,14,0,0,
    0,6,6,6,6,6,6,6,6,6,6,6,6,14,14,0,
    0,6,6,6,7,7,6,6,6,6,6,6,6,14,14,0,
    6,6,6,6,7,6,6,6,6,6,6,6,6,14,14,14,
    6,6,6,6,6,6,6,6,6,6,6,6,14,14,14,14,
    6,6,6,6,6,6,6,6,6,6,6,6,14,14,14,14,
    14,6,6,6,6,6,6,6,6,6,6,6,14,14,14,14,
    0,14,6,6,6,6,6,6,6,6,14,14,14,14,14,0,
    0,14,14,14,6,6,6,14,14,14,14,14,14,14,14,0,
    0,0,14,14,14,14,14,14,14,14,14,14,14,14,0,0,
    0,0,0,14,14,14,14,14,14,14,14,14,14,0,0,0,
    0,0,0,0,14,14,14,14,14,14,14,14,0,0,0,0,
    0,0,0,0,0,0,14,14,14,14,0,0,0,0,0,0,
};

const uint8_t ballimgblue[16*16]={
    0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,
    0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,0,1,1,1,1,1,1,1,1,1,9,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,9,0,0,
    0,1,1,1,1,1,1,1,1,1,1,1,1,9,9,0,
    0,1,1,1,7,7,1,1,1,1,1,1,1,9,9,0,
    1,1,1,1,7,1,1,1,1,1,1,1,1,9,9,9,
    1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,
    1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,
    9,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,
    0,9,1,1,1,1,1,1,1,1,9,9,9,9,9,0,
    0,9,9,9,1,1,1,9,9,9,9,9,9,9,9,0,
    0,0,9,9,9,9,9,9,9,9,9,9,9,9,0,0,
    0,0,0,9,9,9,9,9,9,9,9,9,9,0,0,0,
    0,0,0,0,9,9,9,9,9,9,9,9,0,0,0,0,
    0,0,0,0,0,0,9,9,9,9,0,0,0,0,0,0,
};

const uint8_t *const ballimg[]={
    ballimgcyan,
    ballimggreen,
    ballimgpink,
    ballimgred,
    ballimgyellow,
    ballimgblue,
};

typedef struct{
    int hp;
    object_t obj;
} block_t;

const uint8_t s_break[]={
#include "music_csv/break.h"
    0
};

const uint8_t s_hitblk[]={
#include "music_csv/hitblk.h"
    0
};
const uint8_t s_hitpad[]={
#include "music_csv/hitpad.h"
    0
};
#define MAX_BLOCKS 30
#define MAX_BALLS 20

block_t blocks[MAX_BLOCKS+1]={
    {
    .hp = 10,
    .obj={
        .pos={
            .x = 0,
            .y = 0,
        },
        .size={
            .x = 48*256,
            .y = 16*256,
        },
    }
    },
    {
    .hp = 10,
    .obj={
        .pos={
            .x = 50*256,
            .y = 0,
        },
        .size={
            .x = 48*256,
            .y = 16*256,
        },
    }
    },
};

void musicTask(void) ;

void myg_pset(int x,int y,unsigned int c)
// (x,y)の位置にカラーcで点を描画
{
	unsigned short *ad,d1;

	c&=15;
	ad=gVRAM+y*G_H_WORD+(x>>2);
	d1=~(0xf000>>(x&0x3)*4);
	c<<=(3-(x&0x3))*4;
	*ad&=d1;
	*ad|=c;
}

void myg_hline(unsigned int x1,unsigned int x2,int y,unsigned int c)
// (x1,y)-(x2,y)への水平ラインを高速描画
// 領域外チェック無し
// x1 < x2
{
	unsigned short d,*ad;

	while(x1&3){
		myg_pset(x1++,y,c);
		if(x1>x2) return;
	}
	d=c|(c<<4)|(c<<8)|(c<<12);
	ad=gVRAM+y*G_H_WORD+(x1>>2);
	while(x1+3<=x2){
		*ad++=d;
		x1+=4;
	}
	while(x1<=x2) myg_pset(x1++,y,c);
}

void myg_boxfill(int x1,int y1,int x2,int y2,unsigned int c)
// (x1,y1),(x2,y2)を対角線とするカラーcで塗られた長方形を描画
// 領域外チェック無しなので気をつけること
// x1 < x2
// y1 < y2
{
	while(y1<=y2){
		myg_hline(x1,x2,y1++,c);
	}
}

void drawBlock(block_t *blk){
    object_t *obj;
    vector_t x2;
    obj = &(blk->obj);
    uint8_t color;
    if(blk->hp!=-1){
        color = blk->hp;
    }
    
    x2 = add2(&obj->pos,&obj->size);
    myg_boxfill(obj->pos.x>>8,obj->pos.y>>8,(x2.x>>8)-1,(x2.y>>8)-1,color);
}

void eraseBlock(block_t *blk){
    object_t *obj;
    vector_t x2;
    obj = &(blk->obj);

    g_clrbmpmn(obj->pos.x>>8,obj->pos.y>>8,(obj->size.x>>8),(obj->size.y>>8));
}

typedef struct {
    object_t obj;
    int exist;
    int rate;
    int max_speed;
    int imgcol;
} ball_t;

ball_t balls[MAX_BALLS]={
};

int ballWall(object_t *bp){
    int ret = 0;
    if(bp->pos.x <= WALL_LEFT + WALL_WIDTH){
        ret = 1;
    }
    else if(bp->pos.x + bp->size.x > WALL_RIGHT){
        ret = 1;
    }
    if(bp->pos.y < WALL_UP + WALL_WIDTH){
        ret |= 2;
    }
    return ret;
}

unsigned long randomxor128(void){ 
    static unsigned long x=123456789,y=362436069,z=521288629,w=88675123; 
    unsigned long t; 
    t=(x^(x<<11));x=y;y=z;z=w; 
    return( w=(w^(w>>19))^(t^(t>>8)) ); 
}

void title(void){
    g_clearscreen();
    g_printstr(0,50,7,8,"Multi-Ball Breakout Game");
    g_printstr(0,70,7,8,"Press Start key to start.");
    g_printstr(0,150,7,8,"Made By Gombe.");

    
    image_t img={
        .size = {
            .x = 16,
            .y = 16,
        }
    };
    int i;
    vector_t v={
        .x = 0,
        .y = 256*100,
    };
    const vector_t vel={
        .x = 256*30,
        .y = 0,
    };
    
    
    for(i=0;i<6;i++){
        img.bmp = ballimg[i];
        drawImage(&v,&img);
        v = add2(&v,&vel);
    }
    while(PORTB&KEYSTART) {
        randomxor128();
        audiotask();
    }
}

int geneball(ball_t *ball,int level,vector_t *pos){
    int i;
    for(i=0;i<MAX_BALLS;i++){
        if(!ball[i].exist)break;
    }
    if(i==MAX_BALLS)return -1;

    ball[i].obj.pos.x = pos->x;
    ball[i].obj.pos.y = pos->y;
    ball[i].obj.size.x = 16*256;
    ball[i].obj.size.y = 16*256;
    ball[i].obj.vero.x = 256+(((i+7)*317614)&0xFF);
    ball[i].obj.vero.y = ball[i].max_speed-ball[i].obj.vero.x;
    ball[i].exist = 1;
    ball[i].rate = level+10;
//    ball[i].imgcol = 5;

    ball[i].imgcol = (randomxor128()%(level+1))%6;
    ball[i].max_speed = ball[i].imgcol*100+600+level*50;
    switch(ball[i].imgcol){
        case 0:
            ball[i].obj.acc.x = 0;
            ball[i].obj.acc.y = 0;
            break;
        case 1:
            ball[i].obj.acc.x = (randomxor128()%2)*2-2;
            ball[i].obj.acc.y = 0;
            break;
        case 2:
            ball[i].obj.acc.x = 0;
            ball[i].obj.acc.y = 3;
            break;
        case 3:
            ball[i].obj.acc.x = 0;
            ball[i].obj.acc.y = 5;
            break;
        case 4:
            ball[i].obj.acc.x = 0;
            ball[i].obj.acc.y = -1;
            break;
        case 5:
            ball[i].obj.acc.x = 0;
            ball[i].obj.acc.y = -2;
            break;
            
    }

    return 0;
}

void initball(ball_t *ball,int level,int ballnum){
    int i;
    vector_t pos;
    for(i=0;i<MAX_BALLS;i++){
        ball[i].exist=0;
    }
    for(i=0;i</*ballnum*/MAX_BALLS;i++){
        pos.x = (i*7987645%200+25)*256;
        pos.y = (30)*256;
        geneball(ball,level,&pos);
    }

}

void initblock(block_t *blocks,int level){
    int i;
    for(i=0;i<MAX_BLOCKS;i++){
        blocks[i].hp = i / 12+3+level - (randomxor128()%5);
        if(blocks[i].hp < 0)blocks[i].hp=0;
        blocks[i].hp&=0xF;
        blocks[i].obj.pos.x = ((i%6) * 200 / 6 + 28)*256;
        blocks[i].obj.pos.y = ((i/6) * 16+30)*256;
        blocks[i].obj.size.x = (200 / 6-3) *256;
        blocks[i].obj.size.y = (13) *256;
    }
}

void printscore(int score){
    g_printstr(10,1,7,8,"score:");
    g_printnum(100,1,7,8,score);
}

//you must set stop data!
#define SOUNDCH 6

FIL fbgm;
int sound_kind;
const uint8_t stop = 0;
const uint8_t *cur_sound[SOUNDCH] = {
    &stop,&stop, &stop,&stop, &stop, &stop,
};

void sound(const uint8_t *sound){
    int i;
    for(i=0;i<SOUNDCH;i++){
        if(*(cur_sound[i])==0)break;
    }
    if(i==SOUNDCH)
        return;
    cur_sound[i] = sound;
}

/*mix type : add*/
void soundmixing(uint8_t *buff){
    int j;
    uint8_t *tbuf = buff;
    for(j=0;j<SOUNDCH;j++){
        buff = tbuf;
        if(*(cur_sound[j])==0)continue;
        int i;
        i = SIZEOFSOUNDBF/2;
        do{
            *buff++ += (*(cur_sound[j]++)-128)>>1;
        }while(--i&&*(cur_sound[j]));
        ;
    }
}

int main(void){
    int score = 0;
    int stage = 1;
    int ballnum = 3;
    FRESULT res;

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

    RPB13R = 5; //RPB13ピンにOC4を割り当て
    OC4R = 0;
    OC4CON = 0x000e; // Timer3ベース、PWMモード
    OC4CONSET = 0x8000; //OC4スタート
    T3CON = 0x0000; // プリスケーラ1:1
    PR3 = 256;
    T3CONSET = 0x8000; // タイマ3スタート

    T4CONbits.SIDL = 0;
    T4CONbits.TCKPS = 3;
    T4CONbits.T32 = 0;
    T4CONbits.TCS = 0;
    TMR4 = 0; 
    
    PR4 = CLOCK_FREQ / 8 / SAMPLING_FREQ;
    
    
    T4CONbits.ON = 1;

    DmaChnOpen(0, 0, DMA_OPEN_AUTO);

    DmaChnSetEventControl(0, DMA_EV_START_IRQ(_TIMER_4_IRQ));

    DmaChnSetTxfer(0, sounddata, (void*) &OC4RS, sizeof (sounddata), 1, 1);

    DmaChnEnable(0);
    init_composite(); // ビデオ出力システムの初期化

#define FILENAME "bgm.raw"    
    printstr("SD INIT...");
    if (disk_initialize(0) != 0) {
        printstr("SD INIT ERR");
        while (1) ;
    }
    if (res = f_mount(&fatfs, "", 0) != FR_OK) {
        printstr("SD INIT ERR");
        while (1) ;
    } else {
#define PATH "./blk_data"
        res = f_chdir(PATH);
        if(res == FR_NO_PATH){
            printstr("DIR <"PATH"> NOT FOUND");
            while (1) ;            
        }
        res = f_open(&fbgm, FILENAME, FA_READ);
        if (res != FR_OK) {
            printstr("FILE <"FILENAME"> NOT FOUND");
            while (1) ;
        }
    }
    

    init_graphic(VRAMA);
    set_graphmode(1);

    image_t img={
        .size = {
            .x = 16,
            .y = 16,
        }
    };
    

    title();

    int ref=0,ret,i;
    g_clearscreen();
 
    int paddlewidth = 120*256;
    while(1){
    g_clearscreen();
    g_boxfill(WALL_RIGHT>>8,WALL_UP>>8,(WALL_RIGHT>>8)+(WALL_WIDTH>>8),PADDLE_Y>>8,4);
    g_boxfill(WALL_LEFT>>8,WALL_UP>>8,(WALL_LEFT>>8)+(WALL_WIDTH>>8),PADDLE_Y>>8,4);
    g_boxfill(WALL_LEFT>>8,WALL_UP>>8,WALL_RIGHT>>8,((WALL_UP-WALL_WIDTH)>>8),4);
    initball(balls,stage,ballnum);
    initblock(blocks,stage);
    for(i=0;i<MAX_BLOCKS;i++){
        if(blocks[i].hp){
            drawBlock(&blocks[i]);
        }
    }
    int b;
    int paddlepos = 0*256;
    while(1){
        printscore(score);
        if(!(PORTB & KEYRIGHT)){
            paddlepos += 1000;
            if(paddlepos > 224*256){
                paddlepos = -paddlewidth;
            }
        }
        if(!(PORTB & KEYLEFT)){
            paddlepos -= 1000;
            if(paddlepos < -paddlewidth){
                paddlepos = 224*256;
            }
        }
        {
            int e=0;
            for(i=0;i<MAX_BALLS;i++)if(balls[i].exist)e=1;
            if(!e){
                g_printstr(10,120,7,8,"Game Over");
                g_printstr(0,130,7,8,"Press Start key to restart.");
                score = 0;
                stage = 1;
                ballnum=3;
                paddlewidth = 120*256;
                while(PORTB&KEYSTART)musicTask();
                break;
            }
            e=0;
            for(i=0;i<MAX_BLOCKS;i++)if(blocks[i].hp)e=1;
            if(!e){
                stage++;
                ballnum += stage/2+1;
                g_printstr(10,120,7,8,"Game Clear");
                g_printstr(10,130,7,8,"You Get");
                g_printnum(80,130,7,8,ballnum);
                g_printstr(100,130,7,8,"x1000 Points!");
                g_printstr(0,140,7,8,"Goto Next Stage! Press Start key.");
                score += ballnum*1000;
                paddlewidth -= paddlewidth>>8;
                printscore(score);
                if(ballnum > MAX_BALLS)ballnum = MAX_BALLS;
                while(PORTB&KEYSTART)musicTask();
                break;
            }
        }
        g_set_palette(0,0,0,0);
        for(b=0;b<MAX_BALLS;b++){
            if(balls[b].exist==0)continue;
            update_object(&balls[b].obj);
            ref = 0;
            for(i=0;i<MAX_BLOCKS;i++){
                if(blocks[i].hp){
                    ret = refrection(&blocks[i].obj,&balls[b].obj);
                    if(ret){
                        balls[b].rate += balls[b].rate;
                        if(balls[b].rate > 1000)balls[b].rate = 1000;
                        score += balls[b].rate;
                        ref |= ret;
                        blocks[i].hp--;
                        if(blocks[i].hp==0){
                            sound(s_break);
                            eraseBlock(&blocks[i]);
                            if(randomxor128()%10==0){
                                geneball(balls,stage,&blocks[i].obj.pos);
                                ballnum++;
                            }
                        }else{
                            sound(s_hitblk);
                            drawBlock(&blocks[i]);
                        }
                    }
                }
            }
            ret = ballWall(&balls[b].obj);
            if(ret & 2){
                if(balls[b].imgcol >= 4){
                    if(balls[b].obj.vero.y > -400)
                        balls[b].obj.vero.y = -400;
                }
            }
            ref |= ret;

            if(ref & 1){
                balls[b].obj.vero.x = -balls[b].obj.vero.x;
            }
            if(ref & 2){
                balls[b].obj.vero.y = -balls[b].obj.vero.y;            
            }
            if(ref) update_object(&balls[b].obj);
            if(balls[b].obj.pos.y+balls[b].obj.size.y > PADDLE_Y){
                if(balls[b].obj.pos.x > paddlepos&&balls[b].obj.pos.x < paddlepos + paddlewidth){
                    sound(s_hitpad);
                    balls[b].obj.vero.x -= (paddlepos + paddlewidth / 2 - balls[b].obj.pos.x)/(paddlewidth/512);
                    if(balls[b].obj.vero.x > balls[b].max_speed - 200){
                        balls[b].obj.vero.x = balls[b].max_speed - 200;
                    }else if(balls[b].obj.vero.x < -balls[b].max_speed + 200){
                        balls[b].obj.vero.x = -balls[b].max_speed + 200;
                    }
                    balls[b].obj.vero.y = abs(balls[b].obj.vero.x) - balls[b].max_speed;
                    balls[b].max_speed++;
                    balls[b].rate = stage+10;
                }else{
                    balls[b].exist = 0;
                    ballnum--;
                }
            }
        }
        for(b=0;b<MAX_BALLS;b++){
            if(balls[b].exist){
                img.bmp = ballimg[balls[b].imgcol];
                drawImage(&(balls[b].obj.pos),&img);
            }
        }
        myg_boxfill(0,PADDLE_Y>>8 ,255,((PADDLE_Y)>>8) + 5,8);
        g_boxfill((paddlepos>>8 )+10,PADDLE_Y>>8 ,((paddlepos + paddlewidth)>>8)-10,((PADDLE_Y)>>8) + 5,7);
        while(!drawing);
        while(drawing){
            musicTask();
        }
        for(b=0;b<MAX_BALLS;b++){
            if(balls[b].exist)
                eraseImage(&(balls[b].obj.pos),&img);
        }
    }
    }
    
}

void musicTask(void) {
    audiotask();
}

void audiotask(void) {
    static uint prevtrans = 1;
    uint8_t *buff;
    UINT read;

#ifdef SIMMODE
    buff = &sounddata[0];
#else
    buff = NULL; //&sounddata[0];
#endif
    if (DmaChnGetEvFlags(0) & DMA_EV_SRC_HALF) {
        DmaChnClrEvFlags(0, DMA_EV_SRC_HALF);
        if (prevtrans == 2) {
            prevtrans = 1;
            buff = &sounddata[0];
        }
    } else if (DmaChnGetEvFlags(0) & DMA_EV_SRC_FULL) {
        DmaChnClrEvFlags(0, DMA_EV_SRC_FULL);
        if (prevtrans == 1) {
            prevtrans = 2;
            buff = &sounddata[SIZEOFSOUNDBF / 2];
        }
    }

    if (buff) {
        int i;
        for (i = 0; i < SIZEOFSOUNDBF / 2; i++) {
            buff[i] = 128;
        }

        f_read(&fbgm, buff, SIZEOFSOUNDBF / 2, &read);
        if(read==0){
            f_open(&fbgm,FILENAME,FA_READ);
        }
        soundmixing(buff);
    }
}
