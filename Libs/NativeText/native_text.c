#include "native_text.h"

#if defined(__APPLE__)

#import <CoreText/CoreText.h>
#import <CoreGraphics/CGBitmapContext.h>

#include <stdlib.h>
#include <string.h>

typedef struct native_text
{
    CGContextRef context;
    
    float sX, sY;
    CTFontRef font;
} native_text;

native_text* nt_init(void)
{
    native_text* ret = malloc(sizeof(native_text));
    ret->context = NULL;
    ret->sX = ret->sY = 1;
    
    ret->font = CTFontCreateWithName(CFSTR("Helvetica Neue"), 12, NULL);
    
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
            new = CTFontCreateCopyWithFamily(nt->font, 0.0, NULL, CFSTR("Monaco"));
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

//length < 0: null-terminated.
CFMutableAttributedStringRef nt_str_len_to_attr_string(char* text, int length)
{
    if (length < 0)
        length = (int)strlen(text);
    
    CFStringRef str = CFStringCreateWithBytes(NULL, (const unsigned char*)text, length, kCFStringEncodingUTF8, false);
    CFMutableAttributedStringRef attrString = CFAttributedStringCreateMutable(NULL, 0);
    
    CFAttributedStringReplaceString(attrString, CFRangeMake(0, 0), str);
    CFRelease(str);
    
    return attrString;
}

nt_extent nt_get_extent(native_text* nt, char* text, int length)
{
    nt_extent ret = {0, 0};
    
    if (!nt->context)
        return ret;
    
    CFMutableAttributedStringRef attrString = nt_str_len_to_attr_string(text, length);
    CFAttributedStringSetAttribute(attrString, CFRangeMake(0, CFAttributedStringGetLength(attrString)), kCTFontAttributeName, nt->font);
    
    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    CGRect rect = CTLineGetImageBounds(line, nt->context);
    
    CFRelease(attrString);
    CFRelease(line);
    
    ret.w = rect.size.width;
    ret.h = rect.size.height;
    return ret;
}

nt_extent nt_put_text(native_text* nt, char* text, int length, int x, int y, nt_extent* extent)
{
    nt_extent ret = {0, 0};
    
    if (!nt->context)
        return ret;
    
    CFMutableAttributedStringRef attrString = nt_str_len_to_attr_string(text, length);
    CFAttributedStringSetAttribute(attrString, CFRangeMake(0, CFAttributedStringGetLength(attrString)), kCTFontAttributeName, nt->font);
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
    
    ret.w = size.width;
    ret.h = size.height;
    
    return ret;
}

#endif //__APPLE__
