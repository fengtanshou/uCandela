/*
 * Compiler specific options and macros
 */

#ifndef COMPILER_H_INC
#define COMPILER_H_INC

#if defined __GNUC__
#define SECTION(x) __attribute__((section(#x)))
#define NAKED  __attribute__((naked))
#define NOINIT __attribute__((section(".noinit")))
#define NORETURN __attribute__((noreturn))
#define INIT_FUNC_3 NAKED SECTION(.init3)
#define INIT_FUNC_8 NAKED SECTION(.init8)
#endif /* defined __GNUC__ */

#endif /* COMPILER_H_INC */

