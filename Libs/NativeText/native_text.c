#include "native_text.h"

#if defined(__APPLE__)

#import <CoreText/CoreText.h>
#import <CoreGraphics/CGBitmapContext.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct native_text
{
    CGContextRef context;
    
    float sX, sY;
    CTFontRef font;
    CGColorRef color;
} native_text;

native_text* nt_init(void)
{
    native_text* ret = malloc(sizeof(native_text));
    ret->context = NULL;
    ret->sX = ret->sY = 1;

    ret->font = CTFontCreateWithName(CFSTR("Helvetica Neue"), 12, NULL);
    ret->color = CGColorCreateGenericRGB(0, 0, 0, 1);
    
    return ret;
}

void nt_free(native_text* nt)
{
    if (nt->context)
        CFRelease(nt->context);
    
    CFRelease(nt->font);
    free(nt);
}

void nt_buffer(native_text* nt, char* buffer, int width, int height, int components)
{
    CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    nt->context = CGBitmapContextCreate(buffer, width, height, 8, width*components, colorspace, kCGImageAlphaPremultipliedLast);
    
    //OpenGL coordinates
    CGContextTranslateCTM(nt->context, 0, height);
    CGContextScaleCTM(nt->context, 1.0, -1.0);
    
    CGContextScaleCTM(nt->context, nt->sX, nt->sY);
    
    CGColorSpaceRelease(colorspace);
}

void nt_scaling(native_text* nt, float sX, float sY)
{
    if (nt->context)
        CGContextScaleCTM(nt->context, sX, sY);
    
    nt->sX = sX;
    nt->sY = sY;
}

void nt_style(native_text* nt, int style)
{
    CTFontRef new;
    switch (style)
    {
        case nt_serif:
            new = CTFontCreateCopyWithFamily(nt->font, 0.0, NULL, CFSTR("Baskerville"));
            break;
        case nt_mono:
            new = CTFontCreateCopyWithFamily(nt->font, 0.0, NULL, CFSTR("Menlo"));
            break;
        case nt_sans:
        default:
            new = CTFontCreateCopyWithFamily(nt->font, 0.0, NULL, CFSTR("Helvetica Neue"));
            break;
    }
    CFRelease(nt->font);
    nt->font = new;
}

void nt_size(native_text* nt, int points)
{
    CTFontRef new = CTFontCreateCopyWithAttributes(nt->font, points, NULL, NULL);
    CFRelease(nt->font);
    nt->font = new;
}

void nt_color(native_text* nt, float red, float green, float blue, float alpha)
{
    CFRelease(nt->color);
    nt->color = CGColorCreateGenericRGB(red, green, blue, alpha);
}

//length < 0: null-terminated.
CFMutableAttributedStringRef nt_str_len_to_attr_string(const char* text, ptrdiff_t length)
{
    if (length < 0)
        length = (ptrdiff_t)strlen(text);
    
    CFStringRef str = CFStringCreateWithBytes(NULL, (const unsigned char*)text, length, kCFStringEncodingUTF8, false);
    assert(str && "String is not valid UTF8");
    CFMutableAttributedStringRef attrString = CFAttributedStringCreateMutable(NULL, 0);
    
    CFAttributedStringReplaceString(attrString, CFRangeMake(0, 0), str);
    CFRelease(str);
    
    return attrString;
}

nt_extent nt_get_extent(native_text* nt, const char* text, ptrdiff_t length)
{
    CFMutableAttributedStringRef attrString = nt_str_len_to_attr_string(text, length);
    CFAttributedStringSetAttribute(attrString, CFRangeMake(0, CFAttributedStringGetLength(attrString)), kCTFontAttributeName, nt->font);
    
    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    CGFloat ascent, descent;
    CGFloat width = CTLineGetTypographicBounds(line, &ascent, &descent, NULL);
    
    CFRelease(attrString);
    CFRelease(line);
    
    nt_extent ret = {0, 0};
    ret.w = width + .5f;
    ret.h = ascent + descent + .5f;
    return ret;
}

nt_extent nt_put_text(native_text* nt, const char* text, ptrdiff_t length, int x, int y, nt_extent* extent)
{
    assert(nt->context);
    
    CFMutableAttributedStringRef attrString = nt_str_len_to_attr_string(text, length);
    
    CFRange wholeString = CFRangeMake(0, CFAttributedStringGetLength(attrString));
    CFAttributedStringSetAttribute(attrString, wholeString, kCTFontAttributeName, nt->font);
    CFAttributedStringSetAttribute(attrString, wholeString, kCTForegroundColorAttributeName, nt->color);
    
    CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString(attrString);
    CFRelease(attrString);
    
    CGSize maxBox = {CGFLOAT_MAX, CGFLOAT_MAX};
    if (extent)
    {
        maxBox.width = extent->w;
        maxBox.height = extent->h;
    }
    
    CFRange fitRange;
    CGSize size = CTFramesetterSuggestFrameSizeWithConstraints(framesetter, CFRangeMake(0, 0), NULL, maxBox, &fitRange);
    
    CGPathRef path = CGPathCreateWithRect(CGRectMake(x, y, size.width, size.height), NULL);
    CTFrameRef frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);
    CFRelease(framesetter);
    CFRelease(path);
    
    CTFrameDraw(frame, nt->context);
    CFRelease(frame);
    
    nt_extent ret = {0, 0};
    ret.w = size.width;
    ret.h = size.height;
    return ret;
}

#endif //__APPLE__
