/* 
 * File:   vector.h
 * Author: gombe
 *
 * Created on 2017/01/05, 19:54
 */

#ifndef VECTOR_H
#define	VECTOR_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <stdint.h>

    typedef struct{
        int x;
        int y;
    } vector_t;

    typedef struct{
        vector_t pos;
        vector_t vero;
        vector_t acc;
        vector_t size;
    } object_t;
    
    typedef struct{
        const uint8_t *bmp;
        vector_t size;
    } image_t;
    
#define WALL_WIDTH (3*256)
#define WALL_LEFT (0*256)
#define WALL_RIGHT (246*256)
#define WALL_UP (12*256)
#define WALL_COL 4

vector_t add2(const vector_t *i1,const vector_t *i2);
    
#ifdef	__cplusplus
}
#endif

#endif	/* VECTOR_H */

