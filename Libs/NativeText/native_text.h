#ifndef NativeText
#define NativeText

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
void nt_scaling(native_text*, float sX, float sY);

void nt_size(native_text*, int points);
void nt_style(native_text*, int style);

//length < 0: null-terminated.

nt_extent nt_get_extent(native_text*, char* text, int length);

//extent may be NULL, in which case the text will not be wrapped
nt_extent nt_put_text(native_text*, char* text, int length, int x, int y, nt_extent* extent);

#endif
