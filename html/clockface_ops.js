var clk_ops =
[
    {
        op_comp_val: OP_MOD  | COMP_EQU | 5, 
        comp_val: 0,
        dst_op_val: 8,
        dst: DST_RGB
    },
    {
        op_comp_val: OP_NONE | COMP_EQU | 0,
        comp_val: 0,
        dst_op_val: 16,
        dst:  DST_RGB
    },
    {
        op_comp_val: OP_NONE | COMP_EQU | 0,
        comp_val: SRC_SEC,
        dst_op_val: 32,
        dst: DST_BLUE
    },
    {
        op_comp_val: OP_NONE | COMP_EQU | 0,
        comp_val: SRC_HOUR_MIN,
        dst_op_val: 64,
        dst: DST_RED
    },
    {
        op_comp_val: OP_SUB  | COMP_EQU | 1,
        comp_val: SRC_MIN,
        dst_op_val: SRC_SEC | OP_DIV | 2,
        dst: DST_GREEN},
    {
        op_comp_val: OP_NONE | COMP_EQU | 0,
        comp_val: SRC_MIN,
        dst_op_val: SRC_SEC | OP_DIV | 2 | OP_NEG,
        dst: DST_MEM | 1
    },
    {
        op_comp_val: OP_NONE | COMP_EQU | 0,
        comp_val: SRC_MIN,
        dst_op_val: SRC_MEM1 | OP_ADD | 30,
        dst: DST_GREEN
    },
    {
        op_comp_val: OP_NONE | COMP_LT  | 0, 
        comp_val: SRC_SEC,
        dst_op_val: 4,
        dst: DST_MEM | 2
    },
    {
        op_comp_val: OP_ADD  | COMP_GT  | 30,
        comp_val: SRC_SEC,
        dst_op_val: SRC_MEM2,
        dst: DST_BLUE
    }
];