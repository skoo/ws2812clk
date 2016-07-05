
var SRC_NONE        = 0x0000;
var SRC_SEC         = 0x0100;
var SRC_MIN         = 0x0200;
var SRC_HOUR        = 0x0300;
var SRC_HOUR_MIN    = 0x0400;
var SRC_INDEX       = 0x0500;
var SRC_MEM         = 0x0800;
var SRC_MEM0        = 0x0800;
var SRC_MEM1        = 0x0900;
var SRC_MEM2        = 0x0A00;
var SRC_MEM3        = 0x0B00;

var OP_NONE         = 0x0000;
var OP_MOD          = 0x1000;
var OP_ADD          = 0x2000;
var OP_SUB          = 0x3000;
var OP_MUL          = 0x4000;
var OP_DIV          = 0x5000;
var OP_NEG          = 0x8000;

var COMP_NEQU       = 0x0000;
var COMP_EQU        = 0x0100;
var COMP_GT         = 0x0200;
var COMP_LT         = 0x0400;
var COMP_GTE        = 0x0300;
var COMP_LTE        = 0x0500;

var SRC_MASK        = 0x0f00;
var SRC_MEM_MASK    = 0x0800;
var OP_MASK         = 0xf000;
var COMP_MASK       = 0x0f00;
var VAL_MASK        = 0x00ff;

var DST_RED         = 0x10;
var DST_GREEN       = 0x20;
var DST_BLUE        = 0x40;
var DST_RGB         = 0x70;
var DST_MEM         = 0x80;

var DST_RGB_MASK    = 0x70;
var DST_MEM_MASK    = 0x80;

function saturate(v, min, max)
{
    if (v < min)
        return min;
    if (v > max)
        return max;

    return v;
}

var _mem = new Array();

function clk_op(op, val, wrap)
{
    var op_val =  op & VAL_MASK;

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

function clockface_draw(ops, h, m, s, brightness, dots)
{
    var i;
    var h = (h*5) % 60;
    var hm = h +  Math.floor(m/12);

    for (i = 0; i < 60; i++)
    {
        var r = 0;
        var g = 0;
        var b = 0;

        _mem[0] = 0;
        _mem[1] = 0;
        _mem[2] = 0;
        _mem[3] = 0;

        var op;
        for (op = 0; op < ops.length; op++)
        {
            var val = clk_op(ops[op].op_comp_val, i, 1);

            var comp = ops[op].comp_val & 0x00ff;

            switch (ops[op].comp_val & SRC_MASK)
            {
            case SRC_SEC:
                comp = s;
                break;
            case SRC_MIN:
                comp = m;
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

            var diff = val - comp;
            if (diff >= 30)
                diff -= 60;
            else if (diff < -30)
                diff += 60;

            var match = 0;

            switch (ops[op].op_comp_val & COMP_MASK)
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
                var src = ops[op].dst_op_val & 0xff;

                if (ops[op].dst_op_val & SRC_MEM_MASK)
                {
                    src = _mem[(ops[op].dst_op_val >> 8) & 3];
                }
                else
                {
                    switch (ops[op].dst_op_val & SRC_MASK)
                    {
                    case SRC_SEC:
                        src = s;
                        break;
                    case SRC_MIN:
                        src = m;
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

                var dst_val = clk_op(ops[op].dst_op_val, src, 0);

                if (ops[op].dst & DST_MEM_MASK)
                {
                    _mem[ops[op].dst & 3] = dst_val;
                }
                else
                {
                    if (ops[op].dst & DST_RED)
                        r += dst_val;
                    if (ops[op].dst & DST_GREEN)
                        g += dst_val;
                    if (ops[op].dst & DST_BLUE)
                        b += dst_val;
                }
            }
        }

        r = saturate(r * brightness, 0, 255);
        g = saturate(g * brightness, 0, 255);
        b = saturate(b * brightness, 0, 255);

        dots[i] = { r: r, g: g, b: b };
    }
}
