#include "window.h"

extern int g_connection;

void window_init(struct window *window, CGRect frame, int border_width)
{
    window->render_frame = (CGRect) {{0, 0}, frame.size};

    frame.origin.x -= border_width;
    frame.origin.y -= border_width;
    frame.size.width  += (2*border_width);
    frame.size.height += (2*border_width);

    CFTypeRef region;
    CGSNewRegionWithRect(&frame, &region);

    uint64_t set_tags = kCGSStickyTagBit | kCGSIgnoreForExposeTagBit | kCGSDisableShadowTagBit;
    uint64_t clear_tags = 0x200000000000;

    SLSNewWindow(g_connection, 2, 0, 0, region, &window->id);
    SLSSetWindowTags(g_connection, window->id, &set_tags, 64);
    SLSClearWindowTags(g_connection, window->id, &clear_tags, 64);
    SLSSetWindowResolution(g_connection, window->id, 2.0f);
    SLSSetWindowOpacity(g_connection, window->id, 0);
    SLSSetWindowLevel(g_connection, window->id, CGWindowLevelForKey(3));
    window->context = SLWindowContextCreate(g_connection, window->id, 0);

    frame.origin.x = 0;
    frame.origin.y = 0;
    window->frame = frame;

    window->border = CGPathCreateMutable();
    CGRect border = {{ border_width, border_width }, { frame.size.width - 2*border_width, frame.size.height - 2*border_width }};
    CGPathAddRoundedRect(window->border, NULL, border, 8, 8);

    window->background = CGPathCreateMutable();
    CGRect background = {{ 1.5f*border_width, 1.5f*border_width }, { frame.size.width - 3*border_width, frame.size.height - 3*border_width }};
    CGPathAddRoundedRect(window->background, NULL, background, 7, 7);
}
