#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
#define IIF(c) PRIMITIVE_CAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t

#define COMPL(b) PRIMITIVE_CAT(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0

#define DEC(x) PRIMITIVE_CAT(DEC_, x)
#define DEC_0 0
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8
#define DEC_10 9
#define DEC_11 10
#define DEC_12 11
#define DEC_13 12
#define DEC_14 13
#define DEC_15 14
#define DEC_16 15
#define DEC_17 16
#define DEC_18 17
#define DEC_19 18
#define DEC_20 19
#define DEC_21 20
#define DEC_22 21
#define DEC_23 22
#define DEC_24 23
#define DEC_25 24
#define DEC_26 25
#define DEC_27 26
#define DEC_28 27
#define DEC_29 28
#define DEC_30 29
#define DEC_31 30
#define DEC_32 31
#define DEC_33 32
#define DEC_34 33
#define DEC_35 34
#define DEC_36 35
#define DEC_37 36
#define DEC_38 37 
#define DEC_39 38 
#define DEC_40 39 
#define DEC_41 40 
#define DEC_42 41  
#define DEC_43 42 
#define DEC_44 43 
#define DEC_45 44 
#define DEC_46 45 
#define DEC_47 46 
#define DEC_48 47 
#define DEC_49 48 
#define DEC_50 49 
#define DEC_51 50 
#define DEC_52 51 
#define DEC_53 52 
#define DEC_54 53 
#define DEC_55 54 
#define DEC_56 55 
#define DEC_57 56 
#define DEC_58 57 
#define DEC_59 58 
#define DEC_60 59 
#define DEC_61 60 
#define DEC_62 61 
#define DEC_63 62
#define DEC_64 63 

#define ADD2(x) PRIMITIVE_CAT(ADD2_, x)
#define ADD2_0 2
#define ADD2_1 3
#define ADD2_2 4
#define ADD2_3 5
#define ADD2_4 6
#define ADD2_5 7
#define ADD2_6 8
#define ADD2_7  9
#define ADD2_8 10
#define ADD2_9 11
#define ADD2_10 12
#define ADD2_11 13
#define ADD2_12 14
#define ADD2_13 15
#define ADD2_14 16
#define ADD2_15 17
#define ADD2_16 18
#define ADD2_17 19
#define ADD2_18 20
#define ADD2_19 21
#define ADD2_20 22
#define ADD2_21 23
#define ADD2_22 24
#define ADD2_23 25
#define ADD2_24 26
#define ADD2_25 27
#define ADD2_26 28
#define ADD2_27 29
#define ADD2_28 30
#define ADD2_29 31
#define ADD2_30 32
#define ADD2_31 33
#define ADD2_32 34 
#define ADD2_33 35 
#define ADD2_34 36  
#define ADD2_35 37 
#define ADD2_36 38 
#define ADD2_37 39 
#define ADD2_38 40 
#define ADD2_39 41  
#define ADD2_40 42 
#define ADD2_41 43 
#define ADD2_42 44 
#define ADD2_43 45 
#define ADD2_44 46 
#define ADD2_45 47 
#define ADD2_46 48 
#define ADD2_47 49 
#define ADD2_48 50 
#define ADD2_49 51 
#define ADD2_50 52 
#define ADD2_51 53 
#define ADD2_52 54 
#define ADD2_53 55 
#define ADD2_54 56 
#define ADD2_55 57 
#define ADD2_56 58 
#define ADD2_57 59 
#define ADD2_58 60 
#define ADD2_59 61 
#define ADD2_60 62 
#define ADD2_61 63
#define ADD2_62 64
#define ADD2_63 65
#define ADD2_64 66

#ifdef T64
    #define NCLIENTS 7

    #define ADD2_half1(x) PRIMITIVE_CAT(ADD2_half1_, x)
    #define ADD2_half1_0 0
    #define ADD2_half1_1 1
    #define ADD2_half1_2 2
    #define ADD2_half1_3 3
    #define ADD2_half1_4 4
    #define ADD2_half1_5 5
    #define ADD2_half1_6 6

    #define ADD2_half2(x) PRIMITIVE_CAT(ADD2_half2_, x)
    #define ADD2_half2_0 7
    #define ADD2_half2_1 8
    #define ADD2_half2_2 9
    #define ADD2_half2_3 10
    #define ADD2_half2_4 11
    #define ADD2_half2_5 12
    #define ADD2_half2_6 13

#else
    #define NCLIENTS 15

    #define ADD2_half1(x) PRIMITIVE_CAT(ADD2_half1_, x)
    #define ADD2_half1_0 0
    #define ADD2_half1_1 1
    #define ADD2_half1_2 2
    #define ADD2_half1_3 3
    #define ADD2_half1_4 4
    #define ADD2_half1_5 5
    #define ADD2_half1_6 6
    #define ADD2_half1_7 7
    #define ADD2_half1_8 8
    #define ADD2_half1_9 9 
    #define ADD2_half1_10 10 
    #define ADD2_half1_11 11 
    #define ADD2_half1_12 12 
    #define ADD2_half1_13 13 
    #define ADD2_half1_14 14

    #define ADD2_half2(x) PRIMITIVE_CAT(ADD2_half2_, x)
    #define ADD2_half2_0 15
    #define ADD2_half2_1 16
    #define ADD2_half2_2 17
    #define ADD2_half2_3 18
    #define ADD2_half2_4 19
    #define ADD2_half2_5 20
    #define ADD2_half2_6 21
    #define ADD2_half2_7 22
    #define ADD2_half2_8 23
    #define ADD2_half2_9 24
    #define ADD2_half2_10 25 
    #define ADD2_half2_11 26 
    #define ADD2_half2_12 27 
    #define ADD2_half2_13 28 
    #define ADD2_half2_14 29

#endif

#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0,)
#define PROBE(x) x, 1,

#define NOT(x) CHECK(PRIMITIVE_CAT(NOT_, x))
#define NOT_0 PROBE(~)

#define BOOL(x) COMPL(NOT(x))
#define IF(c) IIF(BOOL(c))

#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()

#define EVAL(...)  EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define SERVER_CODE(cid, cflag, ...) SERVER_CODE_IMP(cid, cflag, __VA_ARGS__)

#define CHIP_IMP(id, macro_to_call, ...) macro_to_call(ADD2(id), __VA_ARGS__)
#define CHIP_IMP15_1(id, macro_to_call, ...) macro_to_call(ADD2_half1(id), __VA_ARGS__)
#define CHIP_IMP15_2(id, macro_to_call, ...) macro_to_call(ADD2_half2(id), __VA_ARGS__)
#define UNROLL(id, macro_to_call, ...) macro_to_call(ADD2(id), __VA_ARGS__)

#define REPEAT(count, macro, ...) \
    WHEN(count) \
    ( \
        OBSTRUCT(REPEAT_INDIRECT) () \
        ( \
            DEC(count), macro, __VA_ARGS__ \
        ) \
        OBSTRUCT(macro) \
        ( \
            DEC(count), __VA_ARGS__ \
        ) \
    )
#define REPEAT_INDIRECT() REPEAT

