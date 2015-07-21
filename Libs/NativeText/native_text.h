#ifndef NativeText
#define NativeText

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct native_text native_text;

typedef struct nt_extent
{
    int w, h;
} nt_extent;

enum nt_styles
{
    nt_sans = 0,
    nt_serif = 1,
    nt_mono = 2
};

native_text* nt_init(void);
void nt_free(native_text*);

void nt_buffer(native_text*, char* buffer, int width, int height, int components);
//scaling is the relationship between pixels in the buffer, and screen units used by the
//text drawing functions.
void nt_scaling(native_text*, float sX, float sY);

void nt_size(native_text*, int points);
void nt_style(native_text*, int style);
void nt_color(native_text*, float red, float green, float blue, float alpha);

//length < 0: null-terminated.

//metrics functions
nt_extent nt_get_extent(native_text*, const char* text, ptrdiff_t length);
//character offset corresponding to x-coordinate, clamped to [0, length]
ptrdiff_t nt_hit_test(native_text*, const char* text, ptrdiff_t length, int x);
//x-coordinate corresponding to character offset (range?)
int nt_visual_offset(native_text*, const char* text, ptrdiff_t length, ptrdiff_t index);

//extent may be NULL, in which case the text will not be wrapped. return value is the same as get_extent
nt_extent nt_put_text(native_text*, const char* text, ptrdiff_t length, int x, int y, nt_extent* extent);

    
#ifdef __cplusplus
}
#endif

#endif
