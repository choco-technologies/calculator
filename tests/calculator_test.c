#define DMOD_ENABLE_REGISTRATION ON
#include "dmod_test.h"
#include "calc.h"
#include <string.h>

/* The keys are pressed one after the other, what the big line shows is looked at */
static const char* run(calc_t* calc, const char* keys)
{
    calc_reset(calc);
    for (const char* k = keys; *k != '\0'; k++)
        (void)calc_key(calc, *k);
    return calc->text;
}

#define EXPECT_RESULT(keys, text) \
    do { calc_t c; DMOD_TEST_EXPECT_TRUE(strcmp(run(&c, keys), text) == 0); } while (0)

DMOD_TEST_STEP(starts_with_zero)
{
    calc_t c;
    calc_reset(&c);
    DMOD_TEST_EXPECT_TRUE(strcmp(c.text, "0") == 0);
    DMOD_TEST_EXPECT_TRUE(c.expr[0] == '\0');
}

DMOD_TEST_STEP(typing_a_number)
{
    EXPECT_RESULT("123", "123");
    EXPECT_RESULT("007", "7");              /* no zeros in front */
    EXPECT_RESULT(".5", "0.5");
    EXPECT_RESULT("1.2.3", "1.23");         /* one point */
    EXPECT_RESULT("1234567890123", "12345678901");     /* CALC_DIGITS digits */
    EXPECT_RESULT("0.1234567", "0.123456");            /* 6 decimal places */
}

DMOD_TEST_STEP(the_four_operations)
{
    EXPECT_RESULT("2+3=", "5");
    EXPECT_RESULT("9-12=", "-3");
    EXPECT_RESULT("6*7=", "42");
    EXPECT_RESULT("7/2=", "3.5");
    EXPECT_RESULT("1/3=", "0.333333");
    EXPECT_RESULT("2/3=", "0.666667");       /* rounded */
    EXPECT_RESULT("0.1+0.2=", "0.3");        /* exact */
    EXPECT_RESULT("1.5*1.5=", "2.25");
    EXPECT_RESULT("100*0.07=", "7");
}

DMOD_TEST_STEP(operations_chain)
{
    EXPECT_RESULT("2+3+4=", "9");
    EXPECT_RESULT("2+3*4=", "20");           /* one at a time, as a pocket calculator does */
    EXPECT_RESULT("2+3+", "5");              /* the sum so far is shown at once */
    EXPECT_RESULT("2+*3=", "6");             /* the last operation counts */
    EXPECT_RESULT("5+=", "10");              /* "5 + =" is "5 + 5" */
    EXPECT_RESULT("2+3=*2=", "10");          /* a result goes on */
    EXPECT_RESULT("2+3=7", "7");             /* a number after a result is a new one */
}

DMOD_TEST_STEP(expression_line)
{
    calc_t c;
    run(&c, "12+");
    DMOD_TEST_EXPECT_TRUE(strcmp(c.expr, "12 +") == 0);
    run(&c, "12*");
    DMOD_TEST_EXPECT_TRUE(strcmp(c.expr, "12 x") == 0);
    run(&c, "12+3=");
    DMOD_TEST_EXPECT_TRUE(c.expr[0] == '\0');
}

DMOD_TEST_STEP(errors)
{
    EXPECT_RESULT("1/0=", "Error");
    EXPECT_RESULT("99999999999*99999999999=", "Error");
    EXPECT_RESULT("99999999999+1=", "Error");
    EXPECT_RESULT("1/0=5", "5");             /* a key after an error starts again */
    EXPECT_RESULT("1/0=c", "0");
}

DMOD_TEST_STEP(sign_percent_erase_clear)
{
    EXPECT_RESULT("5n", "-5");
    EXPECT_RESULT("5nn", "5");
    EXPECT_RESULT("0n", "0");
    EXPECT_RESULT("5n*3=", "-15");
    EXPECT_RESULT("2+3=n", "-5");
    EXPECT_RESULT("50%", "0.5");
    EXPECT_RESULT("200+10%=", "220");        /* 10 percent of 200 */
    EXPECT_RESULT("12b", "1");
    EXPECT_RESULT("1b", "0");
    EXPECT_RESULT("12+34c", "0");
}

DMOD_TEST_STEP(unknown_keys_change_nothing)
{
    calc_t c;
    run(&c, "12");
    DMOD_TEST_EXPECT_TRUE(!calc_key(&c, 'x'));
    DMOD_TEST_EXPECT_TRUE(calc_key(&c, '3'));
    DMOD_TEST_EXPECT_TRUE(strcmp(c.text, "123") == 0);
}
