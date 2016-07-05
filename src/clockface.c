#include "clockface.h"
#include "ws2812_spi.h"

#include "stm32f0xx_conf.h"

#define SRC_NONE        0x0000
#define SRC_SEC         0x0100
#define SRC_MIN         0x0200
#define SRC_HOUR        0x0300
#define SRC_HOUR_MIN    0x0400
#define SRC_INDEX       0x0500
#define SRC_MEM         0x0800
#define SRC_MEM0        0x0800
#define SRC_MEM1        0x0900
#define SRC_MEM2        0x0A00
#define SRC_MEM3        0x0B00

#define OP_NONE         0x0000
#define OP_MOD          0x1000
#define OP_ADD          0x2000
#define OP_SUB          0x3000
#define OP_MUL          0x4000
#define OP_DIV          0x5000
#define OP_NEG          0x8000

#define COMP_NEQU       0x0000
#define COMP_EQU        0x0100
#define COMP_GT         0x0200
#define COMP_LT         0x0400
#define COMP_GTE        0x0300
#define COMP_LTE        0x0500

#define SRC_MASK        0x0f00
#define SRC_MEM_MASK    0x0800
#define OP_MASK         0xf000
#define COMP_MASK       0x0f00
#define VAL_MASK        0x00ff

#define DST_RED     0x10
#define DST_GREEN   0x20
#define DST_BLUE    0x40
#define DST_RGB     0x70
#define DST_MEM     0x80

#define DST_RGB_MASK 0x70
#define DST_MEM_MASK 0x80

#define saturate(v,min,max) if (v < min) v = min; if (v > max) v = max

typedef struct _CLK_OP
{
    uint16_t    op_comp_val;
    int16_t     comp_val;
    uint16_t     dst_op_val;
    uint8_t     dst;
} clock_op_t;

const clock_op_t _clk_ops[] __attribute__ ((section (".clockface_data"))) =
{
    { OP_MOD  | COMP_EQU   | 5, 0,                 8,   DST_RGB},
    { OP_NONE  | COMP_EQU  | 0, 0,                 16,   DST_RGB},
    { OP_NONE | COMP_EQU   | 0, SRC_SEC,         32,   DST_BLUE},
    { OP_NONE | COMP_EQU   | 0, SRC_HOUR_MIN,    64,   DST_RED},
    { OP_SUB  | COMP_EQU   | 1, SRC_MIN,         SRC_SEC | OP_DIV | 2,          DST_GREEN},
    { OP_NONE | COMP_EQU   | 0, SRC_MIN,         SRC_SEC | OP_DIV | 2 | OP_NEG, DST_MEM | 1},
    { OP_NONE | COMP_EQU   | 0, SRC_MIN,         SRC_MEM1 | OP_ADD | 30,         DST_GREEN},
    { OP_NONE | COMP_LT    | 0, SRC_SEC,         4,       DST_MEM | 2},
    { OP_ADD  | COMP_GT    | 30, SRC_SEC,        SRC_MEM2, DST_BLUE},
};

static int16_t _mem[4];

static int16_t clk_op(uint16_t op, int16_t val, int wrap)
{
    int16_t op_val =  op & VAL_MASK;

    switch (op & (OP_MASK ^ OP_NEG))
    {
    case OP_MOD:
        val %= op_val;
        break;
    case OP_ADD:
        val += op_val;
        break;
    case OP_SUB:
        val -= op_val;
        break;
    case OP_MUL:
        val *= op_val;
        break;
    case OP_DIV:
        val /= op_val;
        break;
    }

    if (op & OP_NEG)
    {
        val *= -1;
    }

    if (wrap)
    {
        if (val >= 60)
            val -= 60;

        if (val <= -60)
            val = -60;
    }

    return val;
}

void clockface_draw(rtc_time_t* t)
{
    unsigned int i;
    uint8_t h = t->hour*5 % 60;
    uint8_t hm = h + t->min/12;

    for (i = 0; i < 60; i++)
    {
        int16_t r = 0;
        int16_t g = 0;
        int16_t b = 0;

        _mem[0] = 0;
        _mem[1] = 0;
        _mem[2] = 0;
        _mem[3] = 0;

        unsigned int op;
        for (op = 0; op < sizeof(_clk_ops)/sizeof(clock_op_t); op++)
        {
            int16_t val = clk_op(_clk_ops[op].op_comp_val, i, 1);

            int16_t comp = _clk_ops[op].comp_val & 0x00ff;

            switch (_clk_ops[op].comp_val & SRC_MASK)
            {
            case SRC_SEC:
                comp = t->sec;
                break;
            case SRC_MIN:
                comp = t->min;
                break;
            case SRC_HOUR:
                comp = h;
                break;
            case SRC_HOUR_MIN:
                comp = hm;
                break;
            case SRC_INDEX:
                comp = i;
            case SRC_MEM:
                comp = _mem[comp & 0x03];
                break;
            }

            int diff = val - comp;
            if (diff >= 30)
                diff -= 60;
            else if (diff < -30)
                diff += 60;

            int match = 0;

            switch (_clk_ops[op].op_comp_val & COMP_MASK)
            {
            case COMP_NEQU:
                match = diff != 0;
                break;
            case COMP_EQU:
                match = diff == 0;
                break;
            case COMP_GT:
                match = diff > 0;
                break;
            case COMP_LT:
                match = diff < 0;
                break;
            case COMP_GTE:
                match = diff >= 0;
                break;
            case COMP_LTE:
                match = diff <= 0;
                break;
            }

            if (match)
            {
                int16_t src = _clk_ops[op].dst_op_val & 0xff;

                if (_clk_ops[op].dst_op_val & SRC_MEM_MASK)
                {
                    src = _mem[(_clk_ops[op].dst_op_val >> 8) & 3];
                }
                else
                {
                    switch (_clk_ops[op].dst_op_val & SRC_MASK)
                    {
                    case SRC_SEC:
                        src = t->sec;
                        break;
                    case SRC_MIN:
                        src = t->min;
                        break;
                    case SRC_HOUR:
                        src = h;
                        break;
                    case SRC_HOUR_MIN:
                        src = hm;
                    case SRC_INDEX:
                        src = i;
                        break;

                    }
                }

                int16_t dst_val = clk_op(_clk_ops[op].dst_op_val, src, 0);

                if (_clk_ops[op].dst & DST_MEM_MASK)
                {
                    _mem[_clk_ops[op].dst & 3] = dst_val;
                }
                else
                {
                    if (_clk_ops[op].dst & DST_RED)
                        r += dst_val;
                    if (_clk_ops[op].dst & DST_GREEN)
                        g += dst_val;
                    if (_clk_ops[op].dst & DST_BLUE)
                        b += dst_val;
                }
            }
        }

       saturate(r, 0, 255);
       saturate(g, 0, 255);
       saturate(b, 0, 255);

        ws2812_led(i, r, g, b);
    }
}

void clockface_fill(uint8_t r, uint8_t g, uint8_t b)
{
    int i;
    for (i = 0; i < 60; i++)
    {
        ws2812_led(i, r, g, b);
    }
}
