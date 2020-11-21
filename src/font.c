#define array_count(a) ((sizeof((a)) / sizeof(*(a))))

#define ALIGN_NONE   -2
#define ALIGN_LEFT   -1
#define ALIGN_CENTER  0
#define ALIGN_RIGHT   1

CTFontRef create_font(char *cstring)
{
    float size = 10.0f;
    char font_properties[2][255] = { {}, {} };
    sscanf(cstring, "%254[^:]:%254[^:]:%f", font_properties[0], font_properties[1], &size);
    CFStringRef font_family_name = CFStringCreateWithCString(NULL, font_properties[0], kCFStringEncodingUTF8);
    CFStringRef font_style_name = CFStringCreateWithCString(NULL, font_properties[1], kCFStringEncodingUTF8);
    CFNumberRef font_size = CFNumberCreate(NULL, kCFNumberFloat32Type, &size);

    const void *keys[] = { kCTFontFamilyNameAttribute, kCTFontStyleNameAttribute, kCTFontSizeAttribute };
    const void *values[] = { font_family_name, font_style_name, font_size };
    CFDictionaryRef attributes = CFDictionaryCreate(NULL, keys, values, array_count(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(attributes);
    CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, 0.0, NULL);

    CFRelease(descriptor);
    CFRelease(attributes);
    CFRelease(font_size);
    CFRelease(font_style_name);
    CFRelease(font_family_name);

    return font;
}

CTLineRef create_line(CTFontRef font, char *cstring, CGRect *bounds, float *ascent, float *descent)
{
    const void *keys[] = { kCTFontAttributeName, kCTForegroundColorFromContextAttributeName };
    const void *values[] = { font, kCFBooleanTrue };

    CFDictionaryRef attributes = CFDictionaryCreate(NULL, keys, values, array_count(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFStringRef string = CFStringCreateWithCString(NULL, cstring, kCFStringEncodingUTF8);
    CFAttributedStringRef attr_string = CFAttributedStringCreate(NULL, string, attributes);
    CTLineRef line = CTLineCreateWithAttributedString(attr_string);

    CGFloat _ascent, _descent;
    CTLineGetTypographicBounds(line, &_ascent, &_descent, NULL);

    *ascent = _ascent;
    *descent = _descent;
    *bounds = CTLineGetBoundsWithOptions(line, kCTLineBoundsUseGlyphPathBounds);

    CFRelease(string);
    CFRelease(attributes);
    CFRelease(attr_string);

    return line;
}

static inline float align_x(struct component *component, CGRect bounds, float x, int alignment)
{
    if (alignment == ALIGN_LEFT) {
        return (component->window.render_frame.size.width * x);
    } else if (alignment == ALIGN_CENTER) {
        return (component->window.frame.size.width * x) - (bounds.size.width / 2);
    } else if (alignment == ALIGN_RIGHT) {
        return (component->window.render_frame.size.width * x) - (bounds.size.width);
    } else {
        return (int)(x + 0.5f);
    }
}

static inline float align_y(struct component *component, float ascent, float descent, float y)
{
    return (component->window.frame.size.height * y) - ((ascent - descent) / 2);
}

void draw_text(struct component *component, char *text, float x, float y, int alignment_x)
{
    CGRect bounds;
    float ascent, descent;
    CTLineRef line = create_line(component->font, text, &bounds, &ascent, &descent);

    static CGRect truncation_bounds;
    static float truncation_ascent, truncation_descent;
    static CTLineRef truncation_line;
    if (!truncation_line) truncation_line = create_line(component->font, "…", &truncation_bounds, &truncation_ascent, &truncation_descent);

    int ax = align_x(component, bounds, x, alignment_x);
    int ay = align_y(component, ascent, descent, y);

    if ((ax <= 0) || (ax + bounds.size.width >= component->window.render_frame.size.width)) {
        float truncated_width = component->window.render_frame.size.width - 2.0f*truncation_bounds.size.width;

        if (alignment_x == ALIGN_LEFT) {
            truncated_width = component->window.render_frame.size.width - ax - 2.0f*truncation_bounds.size.width;
        } else if (alignment_x == ALIGN_RIGHT) {
            truncated_width = bounds.size.width + ax - 2.0f*truncation_bounds.size.width;
        }

        CTLineRef truncated_line = CTLineCreateTruncatedLine(line, truncated_width, kCTLineTruncationEnd, truncation_line);
        if (truncated_line) {
            bounds.size.width = truncated_width;
            ax = align_x(component, bounds, x, alignment_x);
            CFRelease(line);
            line = truncated_line;
        }
    }

    CGContextSetTextPosition(component->window.context, ax, ay);
    CTLineDraw(line, component->window.context);
    CFRelease(line);
}
