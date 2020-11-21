#ifndef COMPONENT_H
#define COMPONENT_H

struct color
{
    int p;
    float r;
    float g;
    float b;
    float a;
};

enum component_type
{
    COMPONENT_SHELL,
    COMPONENT_TIME,
    COMPONENT_POWER,
};

struct component
{
    enum component_type type;
    struct window window;
    int border_width;
    struct color border_color;
    struct color fg_color;
    struct color bg_color;
    CTFontRef font;
};

struct component_shell
{
    struct component component;
    char *command;
    char *output;
    int refresh_rate;
};

struct component_time
{
    struct component component;
    char icon[32];
    char output[256];
    int refresh_rate;
};

struct component_power
{
    struct component component;
    char icon[32];
    char output[256];
    int percent;
    int refresh_rate;
};

void *component_create_shell(float x, float y, float w, float h, uint32_t fg, uint32_t bg, uint32_t bd, char *command);
void *component_create_time(float x, float y, float w, float h, uint32_t fg, uint32_t bg, uint32_t bd);
void *component_create_power(float x, float y, float w, float h, uint32_t fg,  uint32_t bg, uint32_t bd);

void component_update(struct component *component);
void component_render(struct component *component);

#endif
