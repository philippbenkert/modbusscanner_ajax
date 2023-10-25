#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   Graphical settings
 *====================*/

/* Horizontal and vertical resolution of the library.*/
#define LV_HOR_RES_MAX          (480)
#define LV_VER_RES_MAX          (320)

/* Color depth:
 * - 1:  1 byte per pixel
 * - 8:  RGB332
 * - 16: RGB565
 * - 32: ARGB8888
 */
#define LV_COLOR_DEPTH     16

/* Swap the 2 bytes of RGB565 color. Useful if the display has a 8 bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP   0

/*=====================
   Memory settings
 *=====================*/

/* Size of the memory used by `lv_mem_alloc` in bytes (>= 2kB)*/
#define LV_MEM_SIZE    (32U * 1024U)

/* Use the standard `memcpy` and `memset` instead of LVGL's own functions */
#define LV_MEM_CUSTOM      0

/*==================
 *  Compiler settings
 *==================*/

/* Define a custom attribute to `lv_tick_inc` function */
#define LV_ATTRIBUTE_TICK_INC

/* Define a custom attribute to `lv_task_handler` function */
#define LV_ATTRIBUTE_TASK_HANDLER

/* With this define you can tag attributes with `LV_ATTRIBUTE_FAST_MEM` before a declaration to place it into the RAM
 * It's required for (some) Cortex-M3, M4 and M7 processors */
#define LV_ATTRIBUTE_FAST_MEM

/*==================
 *  Misc. setting
 *==================*/

/* Input device settings*/
#define LV_USE_INDEV   1
#define LV_INDEV_DEF_TYPE  LV_INDEV_TYPE_POINTER

/* Log settings */
#define LV_USE_LOG      1
#if LV_USE_LOG
#  define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#endif  /*LV_USE_LOG*/

/*==================
 *  Features
 *==================*/

/* Image decoder and cache */
#define LV_IMG_CF_INDEXED   1
#define LV_IMG_CF_ALPHA     1

/* Use other fonts */
#define LV_FONT_MONTSERRAT_8     1
#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_16    1

/* ... many more configurations ... */

#endif /*LV_CONF_H*/
