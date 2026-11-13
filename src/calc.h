#ifndef CALC_H
#define CALC_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file calc.h
 * @brief The arithmetic of the calculator: a pocket calculator that takes the keys one at a time.
 *
 * Numbers are fixed point: CALC_SCALE units in one, so the 6 decimal places are exact and no floating point is needed. An operand has
 * at most CALC_DIGITS digits; what does not fit (or a division by zero) is an error that the next key clears.
 */

#define CALC_SCALE      1000000LL           /**< Units in one */
#define CALC_DIGITS     11                  /**< Digits of an operand: 99999999999 is the greatest number */
#define CALC_TEXT       24                  /**< Size of a text of the display (with the end) */

typedef struct
{
    int64_t acc;                /* What the operations before the pending one gave */
    char    op;                 /* The pending operation: '+' '-' '*' '/' or 0 */
    bool    typing;             /* A number is being typed: `text` is it; otherwise `text` is the result */
    bool    fresh;              /* `acc` has a value (the first operand, or a result) */
    bool    error;
    char    text[CALC_TEXT];    /* What the big line shows */
    char    expr[CALC_TEXT * 2];/* What the small line above shows: the operand and the operation waiting */
} calc_t;

/** @brief The state of a calculator that was just switched on: 0 */
void calc_reset(calc_t* calc);

/**
 * @brief A key: '0'..'9' '.', '+' '-' '*' '/' '=' '%' (percent of what is shown), 'n' (change the sign), 'b' (erase a digit),
 *        'c' (clear all). Other keys are ignored.
 * @return true when something shown changed
 */
bool calc_key(calc_t* calc, char key);

#endif
