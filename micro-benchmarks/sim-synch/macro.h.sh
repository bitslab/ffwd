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

#define ADD1(x) PRIMITIVE_CAT(ADD1_, x)
#define ADD1_0 1
#define ADD1_1 2
#define ADD1_2 3
#define ADD1_3 4
#define ADD1_4 5
#define ADD1_5 6
#define ADD1_6 7
#define ADD1_7  8
#define ADD1_8  9
#define ADD1_9 10
#define ADD1_10 11
#define ADD1_11 12
#define ADD1_12 13
#define ADD1_13 14
#define ADD1_14 15
#define ADD1_15 16
#define ADD1_16 17
#define ADD1_17 18
#define ADD1_18 19
#define ADD1_19 20
#define ADD1_20 21
#define ADD1_21 22
#define ADD1_22 23
#define ADD1_23 24
#define ADD1_24 25
#define ADD1_25 26
#define ADD1_26 27
#define ADD1_27 28
#define ADD1_28 29
#define ADD1_29 30
#define ADD1_30 31
#define ADD1_31 32
#define ADD1_32 33
#define ADD1_33 34
#define ADD1_34 35
#define ADD1_35 36
#define ADD1_36 37
#define ADD1_37 38
#define ADD1_38 39
#define ADD1_39 40
#define ADD1_40 41
#define ADD1_41 42
#define ADD1_42 43
#define ADD1_43 44
#define ADD1_44 45
#define ADD1_45 46
#define ADD1_46 47
#define ADD1_47 48
#define ADD1_48 49
#define ADD1_49 50
#define ADD1_50 51
#define ADD1_51 52
#define ADD1_52 53
#define ADD1_53 54
#define ADD1_54 55
#define ADD1_55 56
#define ADD1_56 57
#define ADD1_57 58
#define ADD1_58 59
#define ADD1_59 60
#define ADD1_60 61
#define ADD1_61 62
#define ADD1_62 63
#define ADD1_63 64
#define ADD1_64 65

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

#define ADD2Y(x) PRIMITIVE_CAT(ADD2Y_, x)
#define ADD2Y_0 4
#define ADD2Y_1 5
#define ADD2Y_2 6
#define ADD2Y_3 7
#define ADD2Y_4 8
#define ADD2Y_5 9
#define ADD2Y_6 10
#define ADD2Y_7 11 
#define ADD2Y_8 12
#define ADD2Y_9 13
#define ADD2Y_10 14
#define ADD2Y_11 15
#define ADD2Y_12 16
#define ADD2Y_13 17
#define ADD2Y_14 18
#define ADD2Y_15 19
#define ADD2Y_16 20
#define ADD2Y_17 21
#define ADD2Y_18 22
#define ADD2Y_19 23
#define ADD2Y_20 24
#define ADD2Y_21 25
#define ADD2Y_22 26
#define ADD2Y_23 27
#define ADD2Y_24 28
#define ADD2Y_25 29
#define ADD2Y_26 30
#define ADD2Y_27 31
#define ADD2Y_28 32
#define ADD2Y_29 33
#define ADD2Y_30 34 
#define ADD2Y_31 35
#define ADD2Y_32 36
#define ADD2Y_33 37 
#define ADD2Y_34 38  
#define ADD2Y_35 39 
#define ADD2Y_36 40 
#define ADD2Y_37 41 
#define ADD2Y_38 42  
#define ADD2Y_39 43  
#define ADD2Y_40 44
#define ADD2Y_41 45 
#define ADD2Y_42 46 
#define ADD2Y_43 47 
#define ADD2Y_44 48 
#define ADD2Y_45 49 
#define ADD2Y_46 50 
#define ADD2Y_47 51 
#define ADD2Y_48 52 
#define ADD2Y_49 53 
#define ADD2Y_50 54 
#define ADD2Y_51 55 
#define ADD2Y_52 56 
#define ADD2Y_53 57 
#define ADD2Y_54 58 
#define ADD2Y_55 59 
#define ADD2Y_56 60 
#define ADD2Y_57 61 
#define ADD2Y_58 62 
#define ADD2Y_59 63 
#define ADD2Y_60 64 
#define ADD2Y_61 65
#define ADD2Y_62 66

#ifdef NODES

    #define ADD15_1(x) PRIMITIVE_CAT(ADD15_1_, x)
    #define ADD15_1_0 4
    #define ADD15_1_1 5
    #define ADD15_1_2 6
    #define ADD15_1_3 7
    #define ADD15_1_4 8
    #define ADD15_1_5 9
    #define ADD15_1_6 10
    #define ADD15_1_7 11 
    #define ADD15_1_8 12
    #define ADD15_1_9 13
    #define ADD15_1_10 14
    #define ADD15_1_11 15
    #define ADD15_1_12 16
    #define ADD15_1_13 17
    #define ADD15_1_14 18

    #define ADD15_2(x) PRIMITIVE_CAT(ADD15_2_, x)
    #define ADD15_2_0 19
    #define ADD15_2_1 20
    #define ADD15_2_2 21
    #define ADD15_2_3 22
    #define ADD15_2_4 23
    #define ADD15_2_5 24
    #define ADD15_2_6 25
    #define ADD15_2_7 26
    #define ADD15_2_8 27
    #define ADD15_2_9 28
    #define ADD15_2_10 29
    #define ADD15_2_11 30
    #define ADD15_2_12 31
    #define ADD15_2_13 32
    #define ADD15_2_14 33

    #define ADD15_3(x) PRIMITIVE_CAT(ADD15_3_, x)
    #define ADD15_3_0 34 
    #define ADD15_3_1 35
    #define ADD15_3_2 36
    #define ADD15_3_3 37 
    #define ADD15_3_4 38  
    #define ADD15_3_5 39 
    #define ADD15_3_6 40 
    #define ADD15_3_7 41 
    #define ADD15_3_8 42  
    #define ADD15_3_9 43  
    #define ADD15_3_10 44
    #define ADD15_3_11 45 
    #define ADD15_3_12 46 
    #define ADD15_3_13 47 
    #define ADD15_3_14 48 

    #define ADD15_4(x) PRIMITIVE_CAT(ADD15_4_, x)
    #define ADD15_4_0 49 
    #define ADD15_4_1 50 
    #define ADD15_4_2 51 
    #define ADD15_4_3 52 
    #define ADD15_4_4 53 
    #define ADD15_4_5 54 
    #define ADD15_4_6 55 
    #define ADD15_4_7 56 
    #define ADD15_4_8 57 
    #define ADD15_4_9 58 
    #define ADD15_4_10 59 
    #define ADD15_4_11 60 
    #define ADD15_4_12 61 
    #define ADD15_4_13 62 
    #define ADD15_4_14 63 

    #define ADD15A_1(x) PRIMITIVE_CAT(ADD15A_1_, x)
    #define ADD15A_1_0 2
    #define ADD15A_1_1 3
    #define ADD15A_1_2 4
    #define ADD15A_1_3 5
    #define ADD15A_1_4 6
    #define ADD15A_1_5 7
    #define ADD15A_1_6 8
    #define ADD15A_1_7 9 
    #define ADD15A_1_8 10
    #define ADD15A_1_9 11
    #define ADD15A_1_10 12 
    #define ADD15A_1_11 13 
    #define ADD15A_1_12 14
    #define ADD15A_1_13 15
    #define ADD15A_1_14 16

    #define ADD15A_2(x) PRIMITIVE_CAT(ADD15A_2_, x)
    #define ADD15A_2_0 17
    #define ADD15A_2_1 18
    #define ADD15A_2_2 19
    #define ADD15A_2_3 20
    #define ADD15A_2_4 21
    #define ADD15A_2_5 22
    #define ADD15A_2_6 23
    #define ADD15A_2_7 24
    #define ADD15A_2_8 25
    #define ADD15A_2_9 26
    #define ADD15A_2_10 27 
    #define ADD15A_2_11 28 
    #define ADD15A_2_12 29
    #define ADD15A_2_13 30
    #define ADD15A_2_14 31

#elif FRAMES
    #define ADD15_1(x) PRIMITIVE_CAT(ADD15_1_, x)
    #define ADD15_1_0 4
    #define ADD15_1_1 5
    #define ADD15_1_2 6
    #define ADD15_1_3 7
    #define ADD15_1_4 8
    #define ADD15_1_5 9
    #define ADD15_1_6 10

    #define ADD15_2(x) PRIMITIVE_CAT(ADD15_2_, x)
    #define ADD15_2_0 11
    #define ADD15_2_1 12
    #define ADD15_2_2 13
    #define ADD15_2_3 14
    #define ADD15_2_4 15
    #define ADD15_2_5 16
    #define ADD15_2_6 17

    #define ADD15_3(x) PRIMITIVE_CAT(ADD15_3_, x)
    #define ADD15_3_0 18 
    #define ADD15_3_1 19
    #define ADD15_3_2 20
    #define ADD15_3_3 21
    #define ADD15_3_4 22 
    #define ADD15_3_5 23 
    #define ADD15_3_6 24 

    #define ADD15_4(x) PRIMITIVE_CAT(ADD15_4_, x)
    #define ADD15_4_0 25
    #define ADD15_4_1 26 
    #define ADD15_4_2 27 
    #define ADD15_4_3 28 
    #define ADD15_4_4 29 
    #define ADD15_4_5 30 
    #define ADD15_4_6 31 

    #define ADD15A_1(x) PRIMITIVE_CAT(ADD15A_1_, x)
    #define ADD15A_1_0 2
    #define ADD15A_1_1 3
    #define ADD15A_1_2 4
    #define ADD15A_1_3 5
    #define ADD15A_1_4 6
    #define ADD15A_1_5 7
    #define ADD15A_1_6 8

    #define ADD15A_2(x) PRIMITIVE_CAT(ADD15A_2_, x)
    #define ADD15A_2_0 9
    #define ADD15A_2_1 10
    #define ADD15A_2_2 11
    #define ADD15A_2_3 12
    #define ADD15A_2_4 13
    #define ADD15A_2_5 14
    #define ADD15A_2_6 15
#elif LINES
    #define ADD15_1(x) PRIMITIVE_CAT(ADD15_1_, x)
    #define ADD15_1_0 4
    #define ADD15_1_1 5
    #define ADD15_1_2 6
    #define ADD15_1_3 7
    #define ADD15_1_4 8
    #define ADD15_1_5 9
    #define ADD15_1_6 10

    #define ADD15_2(x) PRIMITIVE_CAT(ADD15_2_, x)
    #define ADD15_2_0 11
    #define ADD15_2_1 12
    #define ADD15_2_2 13
    #define ADD15_2_3 14
    #define ADD15_2_4 15
    #define ADD15_2_5 16
    #define ADD15_2_6 17

    #define ADD15_3(x) PRIMITIVE_CAT(ADD15_3_, x)
    #define ADD15_3_0 18 
    #define ADD15_3_1 19
    #define ADD15_3_2 20
    #define ADD15_3_3 21
    #define ADD15_3_4 22 
    #define ADD15_3_5 23 
    #define ADD15_3_6 24 

    #define ADD15_4(x) PRIMITIVE_CAT(ADD15_4_, x)
    #define ADD15_4_0 25
    #define ADD15_4_1 26 
    #define ADD15_4_2 27 
    #define ADD15_4_3 28 
    #define ADD15_4_4 29 
    #define ADD15_4_5 30 
    #define ADD15_4_6 31 

    #define ADD15A_1(x) PRIMITIVE_CAT(ADD15A_1_, x)
    #define ADD15A_1_0 2
    #define ADD15A_1_1 3
    #define ADD15A_1_2 4
    #define ADD15A_1_3 5
    #define ADD15A_1_4 6
    #define ADD15A_1_5 7
    #define ADD15A_1_6 8

    #define ADD15A_2(x) PRIMITIVE_CAT(ADD15A_2_, x)
    #define ADD15A_2_0 9
    #define ADD15A_2_1 10
    #define ADD15A_2_2 11
    #define ADD15A_2_3 12
    #define ADD15A_2_4 13
    #define ADD15A_2_5 14
    #define ADD15A_2_6 15

#endif

// #ifdef FRAMES
//     #ifdef ALL
//         #define CHIP0 14
//         #define CHIP1 14
//         #define CHIP2 14
//         #define CHIP3 14
//     #else
//         #define CHIP0 7
//         #define CHIP1 7
//         #define CHIP2 7
//         #define CHIP3 7
//     #endif

// #elif NODES
//     #ifdef ALL
//         #define CHIP0 30
//         #define CHIP1 30
//         #define CHIP2 30
//         #define CHIP3 30
//     #elif YIELD
//         #define CHIP0 60
//         #define CHIP1 60
//         #define CHIP2 60
//         #define CHIP3 60
//     #else
//         #define CHIP0 15
//         #define CHIP1 15
//         #define CHIP2 15
//         #define CHIP3 15
//     #endif
// #endif


#ifdef FRAMES        
    #define NCLIENTS 7
#elif LINES
    #define NCLIENTS 7
#elif NODES
    #define NCLIENTS 15
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

#ifdef ALL
    #define CHIP_IMP(id, macro_to_call, ...) macro_to_call(ADD2(id), __VA_ARGS__)
    #define CHIP_IMP15_1(id, macro_to_call, ...) macro_to_call(ADD15A_1(id), __VA_ARGS__)
    #define CHIP_IMP15_2(id, macro_to_call, ...) macro_to_call(ADD15A_2(id), __VA_ARGS__)
    #define UNROLL(id, macro_to_call, ...) macro_to_call(ADD2(id), __VA_ARGS__)
#elif YIELD
    #define CHIP_IMP(id, macro_to_call, ...) macro_to_call(ADD2Y(id), __VA_ARGS__)
    #define CHIP_IMP15_1(id, macro_to_call, ...) macro_to_call(ADD15_1(id), __VA_ARGS__)
    #define CHIP_IMP15_2(id, macro_to_call, ...) macro_to_call(ADD15_2(id), __VA_ARGS__)
    #define CHIP_IMP15_3(id, macro_to_call, ...) macro_to_call(ADD15_3(id), __VA_ARGS__)
    #define CHIP_IMP15_4(id, macro_to_call, ...) macro_to_call(ADD15_4(id), __VA_ARGS__)
    #define UNROLL(id, macro_to_call, ...) macro_to_call(ADD2Y(id), __VA_ARGS__)
#else
    #define CHIP_IMP(id, macro_to_call, ...) macro_to_call(ADD1(id), __VA_ARGS__)
    #define UNROLL(id, macro_to_call, ...) macro_to_call(ADD1(id), __VA_ARGS__)
#endif

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

