#include "mylib.h"
#include "videoout.h"
#include "integer.h"
#include "videoout.h"

/*add two vectors*/
vector_t add2(const vector_t *i1,const vector_t *i2){
    vector_t res;

    res.x = i1->x + i2->x;
    res.y = i1->y + i2->y;

    return res;
}

/*sub two vectors*/
vector_t sub2(const vector_t *i1,const vector_t *i2){
    vector_t res;

    res.x = i1->x - i2->x;
    res.y = i1->y - i2->y;

    return res;
}

/*mul vector*/
/*x256*/
vector_t mul_sc256(const vector_t *i1,int i2){
    vector_t res;

    res.x = (i1->x * i2) >> 8;
    res.y = (i1->y * i2) >> 8;

    return res;
}

/*‘¬“x‚ÌŒvŽZ*/
void update_object(object_t *obj){
    obj->vero = add2(&obj->vero,&obj->acc);
    obj->pos = add2(&obj->pos,&obj->vero);
}

/* position 
 * 256 times!
 */   
void drawImage(const vector_t *position,const image_t *img){
    vector_t t;
    
    t = mul_sc256(position,1);

    g_putbmpmn(t.x,t.y,img->size.x,img->size.y,img->bmp);
}

void eraseImage(const vector_t *position,const image_t *img){
    vector_t t;
    
    t = mul_sc256(position,1);

    g_clrbmpmn(t.x,t.y,img->size.x,img->size.y);
}

int judgeArea(int x1,int x2,int x1w,int x2w){
    if(x1 < x2){
        if(x1 + x1w > x2)return 1;
    }else{
        if(x1 < x2 + x2w )return 1;        
    }
    return 0;
}

int isCollision(const object_t *obj1,const object_t *obj2){
    if(judgeArea(obj1->pos.x,obj2->pos.x,obj1->size.x,obj2->size.x)&&
       judgeArea(obj1->pos.y,obj2->pos.y,obj1->size.y,obj2->size.y)){
        return 1;
    }
    return 0;
}

int refrection(const object_t *obj1,const object_t *obj2){
    const int margin = 4*256;
    int ret = 0;
    
    if(isCollision(obj1,obj2)){
        if(obj1->pos.x+obj1->size.x - margin < obj2->pos.x){
            ret = 1;
        }
        if(obj1->pos.x + margin > obj2->pos.x + obj2->size.x){
            ret = 1;
        }
        if(obj1->pos.y < obj2->pos.y){
            if(obj1->pos.y+obj1->size.y - margin < obj2->pos.y ){
                ret |= 2;
            }
        }else{
            if(obj1->pos.y + margin > obj2->pos.y+ obj2->size.y){
                ret |= 2;
            }
        }
    }
    
    return ret;
}
