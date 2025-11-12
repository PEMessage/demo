#ifndef INCLUDE_CMSDK_CM3_EXT_H
#define INCLUDE_CMSDK_CM3_EXT_H

// Define framebuffer properties based on the QEMU code
#define TOUCH_BASE_ADDRESS ((volatile void*)0x41000000)
#define TOUCH_X      ((volatile uint32_t *)(TOUCH_BASE_ADDRESS + 0))
#define TOUCH_Y      ((volatile uint32_t *)(TOUCH_BASE_ADDRESS + 4))
#define TOUCH_PRESS  ((volatile uint32_t *)(TOUCH_BASE_ADDRESS + 8))

#define FB_BASE_ADDRESS ((volatile uint32_t *)(0x41001000))
#define FB_WIDTH        640
#define FB_HEIGHT       480
#define FB_BPP          32 // Bits per pixel
#define FB_BYTE_PER_PIXEL (FB_BPP / 8 ) // Should be 4
#define FB_BYTES_PER_ROW (FB_WIDTH * FB_BYTE_PER_PIXEL) // 1120 / 4 = 280

#endif  // INCLUDE_CMSDK_CM3_EXT.h_
