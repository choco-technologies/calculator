#include "calc.h"
#include <string.h>

/* ---- Fixed point <-> text ---- */

static int64_t abs64(int64_t v)
{
    return v < 0 ? -v : v;
}

#define CALC_LIMIT      (100000000000LL * CALC_SCALE)   /* 1e11: an operand has CALC_DIGITS digits at most */

/* "-12.5" for -12500000: no zeros at the end of the fraction, no point for a whole number */
static void format(int64_t value, char* out, size_t size)
{
    char digits[40];
    size_t n = 0;
    bool negative = value < 0;
    uint64_t v = (uint64_t)abs64(value);
    uint64_t whole = v / (uint64_t)CALC_SCALE;
    uint32_t frac = (uint32_t)(v % (uint64_t)CALC_SCALE);

    char text[40];
    size_t pos = 0;
    if (negative)
        text[pos++] = '-';
    do { digits[n++] = (char)('0' + whole % 10U); whole /= 10U; } while (whole != 0);
    while (n > 0)
        text[pos++] = digits[--n];
    if (frac != 0)
    {
        char f[6];
        for (int i = 5; i >= 0; i--) { f[i] = (char)('0' + frac % 10U); frac /= 10U; }
        size_t last = 6;
        while (last > 0 && f[last - 1] == '0')
            last--;
        text[pos++] = '.';
        for (size_t i = 0; i < last; i++)
            text[pos++] = f[i];
    }
    text[pos] = '\0';
    if (pos >= size)
        pos = size - 1U;
    memcpy(out, text, pos);
    out[pos] = '\0';
}

/* The number of what is typed: "-12.5" */
static int64_t parse(const char* text)
{
    bool negative = text[0] == '-';
    int64_t whole = 0, frac = 0;
    unsigned decimals = 0;
    bool dot = false;
    for (const char* p = text + (negative ? 1 : 0); *p != '\0'; p++)
    {
        if (*p == '.')
            dot = true;
        else if (!dot)
            whole = whole * 10 + (*p - '0');
        else if (decimals < 6)
        {
            frac = frac * 10 + (*p - '0');
            decimals++;
        }
    }
    for (; decimals < 6; decimals++)
        frac *= 10;
    int64_t v = whole * CALC_SCALE + frac;
    return negative ? -v : v;
}

/* ---- Operations: false when the result does not fit ---- */

static bool fits(int64_t v)
{
    return abs64(v) < CALC_LIMIT;
}

static bool multiply(int64_t a, int64_t b, int64_t* out)
{
    bool negative = (a < 0) != (b < 0);
    uint64_t x = (uint64_t)abs64(a), y = (uint64_t)abs64(b);
    uint64_t xi = x / (uint64_t)CALC_SCALE, xf = x % (uint64_t)CALC_SCALE;
    uint64_t yi = y / (uint64_t)CALC_SCALE, yf = y % (uint64_t)CALC_SCALE;
    /* (xi + xf) * (yi + yf), the parts of the scale each: no step is wider than 64 bits */
    if (xi != 0 && yi > (uint64_t)(CALC_LIMIT / CALC_SCALE) / xi)
        return false;
    uint64_t r = xi * yi * (uint64_t)CALC_SCALE + xi * yf + xf * yi + (xf * yf + (uint64_t)CALC_SCALE / 2U) / (uint64_t)CALC_SCALE;
    if (r >= (uint64_t)CALC_LIMIT)
        return false;
    *out = negative ? -(int64_t)r : (int64_t)r;
    return true;
}

static bool divide(int64_t a, int64_t b, int64_t* out)
{
    if (b == 0)
        return false;
    bool negative = (a < 0) != (b < 0);
    uint64_t x = (uint64_t)abs64(a), y = (uint64_t)abs64(b);
    uint64_t q = x / y, r = x % y;
    if (q >= (uint64_t)(CALC_LIMIT / CALC_SCALE))
        return false;
    uint64_t result = q * (uint64_t)CALC_SCALE;
    uint64_t frac = 0;
    for (int i = 0; i < 6; i++)             /* the fraction, a digit at a time: r < y < 1e17, so r * 10 fits */
    {
        r *= 10U;
        frac = frac * 10U + r / y;
        r %= y;
    }
    if (2U * r >= y)                        /* rounded, half away from zero */
        frac++;
    result += frac;
    if (result >= (uint64_t)CALC_LIMIT)
        return false;
    *out = negative ? -(int64_t)result : (int64_t)result;
    return true;
}

static bool apply(char op, int64_t a, int64_t b, int64_t* out)
{
    switch (op)
    {
        case '+': *out = a + b; return fits(*out);
        case '-': *out = a - b; return fits(*out);
        case '*': return multiply(a, b, out);
        case '/': return divide(a, b, out);
        default:  *out = b; return true;
    }
}

/* ---- The keys ---- */

void calc_reset(calc_t* calc)
{
    memset(calc, 0, sizeof(*calc));
    calc->text[0] = '0';
}

static void show_error(calc_t* calc)
{
    calc_reset(calc);
    calc->error = true;
    strcpy(calc->text, "Error");
}

static void show_expr(calc_t* calc)
{
    calc->expr[0] = '\0';
    if (calc->op == 0 || !calc->fresh)
        return;
    char value[CALC_TEXT];
    format(calc->acc, value, sizeof(value));
    size_t n = strlen(value);
    memcpy(calc->expr, value, n);
    calc->expr[n] = ' ';
    calc->expr[n + 1] = calc->op == '*' ? 'x' : calc->op;   /* (the keys say x) */
    calc->expr[n + 2] = '\0';
}

/* The number on the display: what is typed, or the result */
static int64_t shown(const calc_t* calc)
{
    return calc->typing ? parse(calc->text) : calc->acc;
}

static void digit(calc_t* calc, char key)
{
    if (!calc->typing)
    {
        calc->typing = true;
        calc->text[0] = '\0';
    }
    size_t n = strlen(calc->text);
    unsigned digits = 0;
    for (size_t i = 0; i < n; i++)
        digits += (calc->text[i] >= '0' && calc->text[i] <= '9');
    bool has_dot = strchr(calc->text, '.') != NULL;
    if (key == '.')
    {
        if (has_dot)
            return;
        if (n == 0 || (n == 1 && calc->text[0] == '-'))
            calc->text[n++] = '0';
        calc->text[n++] = '.';
    }
    else
    {
        const char* point = strchr(calc->text, '.');
        if (digits >= CALC_DIGITS || (point != NULL && strlen(point + 1) >= 6))
            return;
        if (n == 1 && calc->text[0] == '0')
            n = 0;                                   /* no zeros in front */
        else if (n == 2 && calc->text[0] == '-' && calc->text[1] == '0')
            n = 1;
        calc->text[n++] = key;
    }
    calc->text[n] = '\0';
}

static void operate(calc_t* calc, char op)
{
    int64_t value = shown(calc);
    if (calc->fresh && calc->op != 0 && calc->typing)
    {
        int64_t result;
        if (!apply(calc->op, calc->acc, value, &result))
        {
            show_error(calc);
            return;
        }
        value = result;
    }
    calc->acc = value;
    calc->fresh = true;
    calc->typing = false;
    calc->op = op;
    format(calc->acc, calc->text, sizeof(calc->text));
    show_expr(calc);
}

static void equals(calc_t* calc)
{
    if (calc->op == 0)
    {
        if (calc->typing)
        {
            calc->acc = parse(calc->text);
            calc->typing = false;
            calc->fresh = true;
            format(calc->acc, calc->text, sizeof(calc->text));
        }
        return;
    }
    int64_t value = calc->typing ? parse(calc->text) : calc->acc;   /* "5 + =" is "5 + 5" */
    int64_t result;
    if (!apply(calc->op, calc->acc, value, &result))
    {
        show_error(calc);
        return;
    }
    calc->acc = result;
    calc->op = 0;
    calc->typing = false;
    calc->fresh = true;
    format(result, calc->text, sizeof(calc->text));
    calc->expr[0] = '\0';
}

bool calc_key(calc_t* calc, char key)
{
    calc_t before = *calc;
    if (key == 'c')
    {
        calc_reset(calc);
        return memcmp(&before, calc, sizeof(*calc)) != 0;
    }
    if (calc->error)
        calc_reset(calc);                   /* any key after the error starts again */

    if ((key >= '0' && key <= '9') || key == '.')
    {
        if (!calc->typing && calc->op == 0)
            calc->fresh = false;            /* a number after a result: a new one */
        digit(calc, key);
    }
    else if (key == '+' || key == '-' || key == '*' || key == '/')
        operate(calc, key);
    else if (key == '=')
        equals(calc);
    else if (key == 'n')
    {
        if (calc->typing)
        {
            if (calc->text[0] == '-')
                memmove(calc->text, calc->text + 1, strlen(calc->text));
            else if (strlen(calc->text) + 2U < sizeof(calc->text) && !(calc->text[0] == '0' && calc->text[1] == '\0'))
            {
                memmove(calc->text + 1, calc->text, strlen(calc->text) + 1U);
                calc->text[0] = '-';
            }
        }
        else if (calc->acc != 0)
        {
            calc->acc = -calc->acc;
            format(calc->acc, calc->text, sizeof(calc->text));
        }
    }
    else if (key == '%')
    {
        int64_t result;
        if (!divide(shown(calc), 100 * CALC_SCALE, &result))
        {
            show_error(calc);
            return true;
        }
        if (calc->typing)
            calc->typing = false;
        if (calc->op == 0)
        {
            calc->acc = result;
            calc->fresh = true;
            format(result, calc->text, sizeof(calc->text));
        }
        else
        {
            /* "200 + 10 %" is 10 percent of 200 */
            int64_t part;
            if (!multiply(calc->acc, result, &part))
            {
                show_error(calc);
                return true;
            }
            calc->typing = true;
            format(part, calc->text, sizeof(calc->text));
        }
    }
    else if (key == 'b')
    {
        if (calc->typing)
        {
            size_t n = strlen(calc->text);
            if (n > 0)
                calc->text[--n] = '\0';
            if (n == 0 || (n == 1 && calc->text[0] == '-'))
            {
                calc->text[0] = '0';
                calc->text[1] = '\0';
            }
        }
    }
    else
        return false;

    show_expr(calc);
    return memcmp(&before, calc, sizeof(*calc)) != 0;
}
