#ifndef INCLUDE_CMSDK_CM3_EXT_H
#define INCLUDE_CMSDK_CM3_EXT_H

#include <stdint.h>

// Define framebuffer properties based on the QEMU code

typedef struct {
    unsigned int enable_irq :1;     /* Bit 0: Enable touch interrupt */
    unsigned int reserved   :31;    /* Bits 1-31: Reserved for future features */
} MPS2FBCtrl;


/* Touch register offsets: single point compatiable setting */
/* -------------------------------------------------------- */
#define TOUCH_BASE_ADDRESS ((volatile void*)0x41000000)
#define TOUCH_CTRL   ((volatile MPS2FBCtrl *)(TOUCH_BASE_ADDRESS + 0))
#define TOUCH_X      ((volatile uint32_t *)(TOUCH_BASE_ADDRESS + 8))
#define TOUCH_Y      ((volatile uint32_t *)(TOUCH_BASE_ADDRESS + 12))
#define TOUCH_PRESS  ((volatile uint32_t *)(TOUCH_BASE_ADDRESS + 16))

#define Touch_IRQn GPIO0_7_IRQn
#define TouchIRQ_Handler GPIO0_7_Handler

/* Touch register offsets: single point compatiable define */
/* -------------------------------------------------------- */
typedef struct {
    unsigned int points_mask :16;     /* Bits 0-16: Point mask */
    unsigned int reserved   :16;    /* Bits 5-31: Reserved for future features */
} MPS2FBTouchHeader;

#define MAX_TOUCH_POINTS    10
#define MOUSE_SLOT 0

/* Touch point structure: multi-touch / single-touch define */
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t pressed;
    int32_t track_id;
} MPS2FBTouchPoint;

struct MPS2FBTouch {
    MPS2FBCtrl ctrl;
    MPS2FBTouchHeader header;
    MPS2FBTouchPoint points[MAX_TOUCH_POINTS];
};

#define MPS2FB_TOUCH   ((volatile struct MPS2FBTouch *)(TOUCH_BASE_ADDRESS + 0))



#define FB_BASE_ADDRESS ((volatile uint32_t *)(0x41001000))
#define FB_WIDTH        640
#define FB_HEIGHT       480
#define FB_BPP          32 // Bits per pixel
#define FB_BYTE_PER_PIXEL (FB_BPP / 8 ) // Should be 4
#define FB_BYTES_PER_ROW (FB_WIDTH * FB_BYTE_PER_PIXEL) // 1120 / 4 = 280

#endif  // INCLUDE_CMSDK_CM3_EXT_H
