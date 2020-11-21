#include "component.h"

#define COMPONENT_FUNC(name) void name(struct component *component)
typedef COMPONENT_FUNC(component_func);

static struct color color_from_hex(uint32_t color)
{
    struct color result;
    result.p = color;
    result.r = ((color >> 0x10) & 0xff) / 255.0f;
    result.g = ((color >> 0x08) & 0xff) / 255.0f;
    result.b = ((color >> 0x00) & 0xff) / 255.0f;
    result.a = ((color >> 0x18) & 0xff) / 255.0f;
    return result;
}

static CGRect calculate_bounds(uint32_t did, float x, float y, float w, float h)
{
    CGRect bounds = CGDisplayBounds(did);
    int cx = (int)((bounds.origin.x + (bounds.size.width  * x)) + 0.5f);
    int cy = (int)((bounds.origin.y + (bounds.size.height * y)) + 0.5f);
    int cw = (int)((w * bounds.size.width) + 0.5f);
    int ch = (int)((h * bounds.size.height) + 0.5f);
    return (CGRect) {{cx, cy}, {cw, ch}};
}

static void component_init(struct component *component, enum component_type type, float x, float y, float w, float h, uint32_t fg, uint32_t bg, uint32_t bd)
{
    uint32_t did = CGMainDisplayID();
    CGRect frame = calculate_bounds(did, x, y, w, h);

    component->type = type;
    component->border_width = 2;
    component->border_color = color_from_hex(bd);
    component->fg_color = color_from_hex(fg);
    component->bg_color = color_from_hex(bg);
    component->font = create_font("Fantasque Sans Mono:Bold:12.0");

    window_init(&component->window, frame, component->border_width);
    CGContextSetLineWidth(component->window.context, component->border_width);
    CGContextSetRGBStrokeColor(component->window.context, component->border_color.r, component->border_color.g, component->border_color.b, component->border_color.a);
    CGContextSetRGBFillColor(component->window.context, component->bg_color.r, component->bg_color.g, component->bg_color.b, component->bg_color.a);
}

void *component_create_shell(float x, float y, float w, float h, uint32_t fg, uint32_t bg, uint32_t bd, char *command)
{
    struct component_shell *shell = malloc(sizeof(struct component_shell));
    memset(shell, 0, sizeof(struct component_shell));
    component_init(&shell->component, COMPONENT_SHELL, x, y, w, h, fg, bg, bd);
    shell->command = command;
    return &shell->component;
}

void *component_create_time(float x, float y, float w, float h, uint32_t fg, uint32_t bg, uint32_t bd)
{
    struct component_time *time = malloc(sizeof(struct component_time));
    memset(time, 0, sizeof(struct component_time));
    component_init(&time->component, COMPONENT_TIME, x, y, w, h, fg, bg, bd);
    return &time->component;
}

void *component_create_power(float x, float y, float w, float h, uint32_t fg, uint32_t bg, uint32_t bd)
{
    struct component_power *power = malloc(sizeof(struct component_power));
    memset(power, 0, sizeof(struct component_power));
    component_init(&power->component, COMPONENT_POWER, x, y, w, h, fg, bg, bd);
    return &power->component;
}

static COMPONENT_FUNC(component_update_shell)
{
    struct component_shell *data = (struct component_shell *) component;

    int cursor = 0;
    int bytes_read = 0;
    char *result = NULL;
    char buffer[BUFSIZ];

    FILE *handle = popen(data->command, "r");
    if (!handle) return;

    while ((bytes_read = read(fileno(handle), buffer, sizeof(buffer)-1)) > 0) {
        char *temp = realloc(result, cursor+bytes_read+1);
        if (!temp) goto err;

        result = temp;
        memcpy(result+cursor, buffer, bytes_read);
        cursor += bytes_read;
    }

    if (result && bytes_read != -1) {
        result[cursor] = '\0';
        data->output = result;
    } else {
err:
        if (result) free(result);
    }

    pclose(handle);
}

static COMPONENT_FUNC(component_update_time)
{
    struct component_time *data = (struct component_time *) component;

    time_t rawtime;
    time(&rawtime);

    struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo) {
        snprintf(data->output, sizeof(data->output), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    }

    snprintf(data->icon, sizeof(data->icon), "⏱ ");;
}

static COMPONENT_FUNC(component_update_power)
{
    struct component_power *data = (struct component_power *) component;

    CFTypeRef ps_info = IOPSCopyPowerSourcesInfo();
    CFTypeRef ps_list = IOPSCopyPowerSourcesList(ps_info);

    bool has_battery = false;
    bool is_charging = false;
    int cur_capacity = 0;
    int max_capacity = 0;
    int percent = 0;

    int ps_count = ps_list ? CFArrayGetCount(ps_list) : 0;
    for (int i = 0; i < ps_count; ++i) {
        CFDictionaryRef ps = IOPSGetPowerSourceDescription(ps_info, CFArrayGetValueAtIndex(ps_list, i));
        if (!ps) continue;

        CFTypeRef ps_type = CFDictionaryGetValue(ps, CFSTR(kIOPSTypeKey));
        if (!ps_type || !CFEqual(ps_type, CFSTR(kIOPSInternalBatteryType))) continue;

        CFTypeRef ps_cur = CFDictionaryGetValue(ps, CFSTR(kIOPSCurrentCapacityKey));
        if (!ps_cur) continue;

        CFTypeRef ps_max = CFDictionaryGetValue(ps, CFSTR(kIOPSMaxCapacityKey));
        if (!ps_max) continue;

        CFTypeRef ps_charging = CFDictionaryGetValue(ps, CFSTR(kIOPSPowerSourceStateKey));
        if (!ps_charging) continue;

        CFNumberGetValue((CFNumberRef) ps_cur, kCFNumberSInt32Type, &cur_capacity);
        CFNumberGetValue((CFNumberRef) ps_max, kCFNumberSInt32Type, &max_capacity);
        is_charging = !CFEqual(ps_charging, CFSTR(kIOPSBatteryPowerValue));
        has_battery = true;
        percent = (int)((double) cur_capacity / (double) max_capacity * 100);
        break;
    }

    if (ps_list) CFRelease(ps_list);
    if (ps_info) CFRelease(ps_info);

    data->percent = percent;

    if (has_battery) {
        snprintf(data->output, sizeof(data->output), "%' ' 3d%%", percent);
        if (is_charging) {
            snprintf(data->icon, sizeof(data->icon), "⚡️");
        } else {
            snprintf(data->icon, sizeof(data->icon), "");
        }
    } else {
        snprintf(data->output, sizeof(data->output), "A/C");
        snprintf(data->icon, sizeof(data->icon), "⚡️");
    }
}

static component_func *component_update_map[] =
{
    [COMPONENT_SHELL] = component_update_shell,
    [COMPONENT_TIME]  = component_update_time,
    [COMPONENT_POWER] = component_update_power
};

void component_update(struct component *component)
{
    component_update_map[component->type](component);
}

static COMPONENT_FUNC(component_render_shell)
{
    struct component_shell *data = (struct component_shell *) component;
    CGContextSetRGBFillColor(component->window.context, component->fg_color.r, component->fg_color.g, component->fg_color.b, component->fg_color.a);
    if (data->output) draw_text(component, data->output, 0.50, 0.50, ALIGN_CENTER);
}

static COMPONENT_FUNC(component_render_time)
{
    struct component_time *data = (struct component_time *) component;
    CGContextSetRGBFillColor(component->window.context, component->fg_color.r, component->fg_color.g, component->fg_color.b, component->fg_color.a);
    draw_text(component, data->icon, 0.10, 0.525, ALIGN_LEFT);
    draw_text(component, data->output, 0.90, 0.50, ALIGN_RIGHT);
}

static COMPONENT_FUNC(component_render_power)
{
#if 0
    struct component_power *data = (struct component_power *) component;

    float x = 0.40 * component->window.render_frame.size.width;
    float w = 0.50 * component->window.render_frame.size.width;
    float y = 0.28 * component->window.render_frame.size.height;
    float h = 0.56 * component->window.render_frame.size.height;
    float percent = data->percent * 0.01;

    CGMutablePathRef battery_outline = CGPathCreateMutable();
    CGRect outline = {{ x, y }, { w, h }};
    CGPathAddRoundedRect(battery_outline, NULL, outline, 2, 2);

    CGMutablePathRef battery_fill = CGPathCreateMutable();
    CGRect fill = {{ x+0.5f, y+0.5f }, { (w*percent)-0.5f, h-0.5f }};
    CGPathAddRoundedRect(battery_fill, NULL, fill, 1, 1);

    float g = percent * 1.0f;
    CGContextSetRGBFillColor(component->window.context, 1.0f-g, g, 0.125f, 0.50f);
    CGContextAddPath(component->window.context, battery_fill);
    CGContextFillPath(component->window.context);

    CGContextSetRGBStrokeColor(component->window.context, component->border_color.r, component->border_color.g, component->border_color.b, component->border_color.a);
    CGContextSetLineWidth(component->window.context, 1);
    CGContextAddPath(component->window.context, battery_outline);
    CGContextStrokePath(component->window.context);

    CGContextSetRGBFillColor(component->window.context, component->fg_color.r, component->fg_color.g, component->fg_color.b, component->fg_color.a);
    draw_text(component, data->icon, 0.10, 0.525, ALIGN_LEFT);
    draw_text(component, data->output, 0.525, 0.50, ALIGN_CENTER);
#else
    struct component_power *data = (struct component_power *) component;

    float x = 0.10 * component->window.render_frame.size.width;
    float w = 0.35 * component->window.render_frame.size.width;
    float y = 0.28 * component->window.render_frame.size.height;
    float h = 0.56 * component->window.render_frame.size.height;
    float percent = data->percent * 0.01;

    CGMutablePathRef battery_outline = CGPathCreateMutable();
    CGRect outline = {{ x, y }, { w, h }};
    CGPathAddRoundedRect(battery_outline, NULL, outline, 2, 2);

    CGMutablePathRef battery_fill = CGPathCreateMutable();
    CGRect fill = {{ x+0.5f, y+0.5f }, { (w*percent)-0.5f, h-0.5f }};
    CGPathAddRoundedRect(battery_fill, NULL, fill, 1, 1);

    float g = percent * 1.0f;
    CGContextSetRGBFillColor(component->window.context, 1.0f-g, g, 0.125f, 0.75f);
    CGContextAddPath(component->window.context, battery_fill);
    CGContextFillPath(component->window.context);

    CGContextSetRGBStrokeColor(component->window.context, component->border_color.r, component->border_color.g, component->border_color.b, component->border_color.a);
    CGContextSetLineWidth(component->window.context, 1);
    CGContextAddPath(component->window.context, battery_outline);
    CGContextStrokePath(component->window.context);

    CGContextSetRGBFillColor(component->window.context, component->fg_color.r, component->fg_color.g, component->fg_color.b, component->fg_color.a);
    draw_text(component, data->icon, 0.25, 0.545, ALIGN_CENTER);
    draw_text(component, data->output, 0.85, 0.50, ALIGN_RIGHT);
#endif
}

static component_func *component_render_map[] =
{
    [COMPONENT_SHELL] = component_render_shell,
    [COMPONENT_TIME]  = component_render_time,
    [COMPONENT_POWER] = component_render_power
};

void component_render(struct component *component)
{
    SLSDisableUpdate(g_connection);
    SLSOrderWindow(g_connection, component->window.id, 0, 0);
    CGContextClearRect(component->window.context, component->window.frame);

    CGContextSetRGBFillColor(component->window.context, component->bg_color.r, component->bg_color.g, component->bg_color.b, component->bg_color.a);
    CGContextAddPath(component->window.context, component->window.background);
    CGContextFillPath(component->window.context);

    component_render_map[component->type](component);

    CGContextSetRGBStrokeColor(component->window.context, component->border_color.r, component->border_color.g, component->border_color.b, component->border_color.a);
    CGContextSetLineWidth(component->window.context, component->border_width);
    CGContextAddPath(component->window.context, component->window.border);
    CGContextStrokePath(component->window.context);

    CGContextFlush(component->window.context);
    SLSOrderWindow(g_connection, component->window.id, 1, 0);
    SLSReenableUpdate(g_connection);
}
