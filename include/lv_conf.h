#pragma once
#define LV_CONF_H

#define LV_COLOR_DEPTH          16
#define LV_DPI_DEF              130

#define LV_DRAW_BUF_STRIDE_ALIGN  4
#define LV_DRAW_BUF_ALIGN         4

#define LV_USE_LOG                0
#define LV_USE_PERF_MONITOR       0

#define LV_USE_DRAW_SW            1
#define LV_DRAW_SW_SUPPORT_RGB565 1

// Fonts
#define LV_FONT_MONTSERRAT_10     1
#define LV_FONT_MONTSERRAT_12     1
#define LV_FONT_MONTSERRAT_14     1
#define LV_FONT_MONTSERRAT_16     1
#define LV_FONT_MONTSERRAT_18     1
#define LV_FONT_MONTSERRAT_20     1
#define LV_FONT_MONTSERRAT_24     1
#define LV_FONT_MONTSERRAT_28     1

// Input
#define LV_USE_KEYBOARD           1
#define LV_USE_POINTER            1

// Layout
#define LV_USE_FLEX               1
#define LV_USE_GRID               1
#define LV_USE_ANIMATION          1
#define LV_USE_SNAPSHOT           1

#define LV_USE_STDLIB_MALLOC      LV_STDLIB_BUILTIN
#define LV_MEM_SIZE               (256 * 1024U)
#define LV_MEM_POOL_INCLUDE       <esp_heap_caps.h>
#define LV_MEM_POOL_ALLOC(size)   heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
