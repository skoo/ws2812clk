#include "clockface.h"
#include "ws2812_spi.h"

#include "stm32f0xx_conf.h"

#define saturate(v,min,max) if (v < min) v = min; if (v > max) v = max

typedef enum
{
    ARC_STOP_OFF = 0,
    ARC_STOP_FULL = 1,
    ARC_STOP_HALF = 2,
    ARC_STOP_QUARTER = 3,
    ARC_STOP_HOUR_MARKER = 4,
} clock_arc_stop_t;

typedef enum
{
    ARC_FADE_NORMAL = 0,
    ARC_FADE_SINGLE = 1,
} clock_arc_fade_t;

typedef struct COLOR
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

typedef struct CLOCKARC
{
    uint8_t len;
    uint8_t stop;
    uint8_t fade:7;
    uint8_t fade_type:1;
} clock_arc_t;

typedef struct CLOCKHAND
{
    color_t color;
    clock_arc_t backward;
    clock_arc_t forward;
} clock_hand_t;

typedef struct CLOCKFACE
{
    clock_hand_t hours;
    clock_hand_t minutes;
    clock_hand_t seconds;

    color_t circle_color;
    color_t hour_marker_color;
} clock_face_t;

const clock_face_t _clk_face __attribute__ ((section (".clockface_data"))) =
{
    .hours =
    {
        .color = { .r = 64 , .g = 0, .b = 0 },

        .backward =
        {
            .len = 0, .stop = 0, .fade_type = 0
        },

        .forward =
        {
            .len = 0, .stop = 0, .fade_type = 0
        },
    },

    .minutes = 
    {
        .color = { .r = 0 , .g = 30, .b = 0 },

        .backward =
        {
            .len = 30,
            .stop = ARC_STOP_HOUR_MARKER,
            .fade_type = ARC_FADE_NORMAL,
            .fade = 3,
        },

        .forward =
        {
            .len = 0, .stop = 0, .fade_type = 0
        },
    },

    .seconds =
    {
        .color = { .r = 0, .g = 0, .b = 32 },
        .backward =
        {
            .len = 0, .stop = 0, .fade_type = 0
        },

        .forward =
        {
            .len = 0, .stop = 0, .fade_type = 0
        },
    },

    .circle_color = { .r = 4, .g = 4, .b = 4 },
    .hour_marker_color = { .r = 13, .g = 13, .b = 13 },
};

static void clockface_draw_arc(const clock_arc_t* arc, const color_t* color, int dir, int position)
{
    int fade_done = 0;
    color_t c = *color;

    for (int len = arc->len; len > 0; len--)
    {
        if (arc->stop == ARC_STOP_HOUR_MARKER)
        {
            if (position % 5 == 0)
                return;
        }
        else if (arc->stop == ARC_STOP_QUARTER)
        {
            if (position % 15 == 0)
                return;
        }
        else if (arc->stop == ARC_STOP_HALF)
        {
            if (position == 0 || position == 30)
                return;
        }
        else if (arc->stop == ARC_STOP_FULL && position == 0)
        {
            return;
        }

        if (!fade_done)
        {
            if (arc->fade_type == ARC_FADE_SINGLE)
                fade_done = 1;

            if (arc->fade)
            {
                c.r /= arc->fade;
                c.g /= arc->fade;
                c.b /= arc->fade;
            }
        }

        position += dir;
        if (position < 0)
            position += 60;
        else if(position > 59)
            position -= 60;

        ws2812_led(position, c.r, c.g, c.b);
    }
}

static void clockface_draw_hand(const clock_hand_t* hand, uint8_t position, int draw_arcs)
{
    if (draw_arcs)
    {
        clockface_draw_arc(&hand->backward, &hand->color, -1, position);
        clockface_draw_arc(&hand->forward, &hand->color, 1, position);
    }

    ws2812_led(position, hand->color.r, hand->color.g, hand->color.b);
}


void clockface_draw(rtc_time_t* t)
{
    unsigned int i;
    uint8_t h = t->hour*5 % 60;
    uint8_t hm = h + t->min/12;

    for (i = 0; i < 60; i++)
    {
        const color_t* c = &_clk_face.circle_color;
        ws2812_led(i, c->r, c->g, c->b);
    }

    clockface_draw_hand(&_clk_face.seconds, t->sec, 1);
    clockface_draw_hand(&_clk_face.minutes, t->min, 1);
    clockface_draw_hand(&_clk_face.hours, hm, 1);

    for (i = 0; i < 60; i++)
    {
        const color_t* c = &_clk_face.hour_marker_color;

        if (i % 5 == 0)
        {
            ws2812_led(i, c->r, c->g, c->b);
        }
    }

    clockface_draw_hand(&_clk_face.seconds, t->sec, 0);
    clockface_draw_hand(&_clk_face.minutes, t->min, 0);
    clockface_draw_hand(&_clk_face.hours, hm, 0);
}

void clockface_fill(uint8_t r, uint8_t g, uint8_t b)
{
    int i;
    for (i = 0; i < 60; i++)
    {
        ws2812_led(i, r, g, b);
    }
}
