# calculator API Reference

## Usage

```bash
calculator [--display NAME] [<view>]
```

| Argument | Meaning |
|----------|---------|
| `--display NAME` | the display to claim, or the rent of one (`<display>/<n>`, which the launcher passes); default: the first display |
| `<view>` | the view to show, default `/eviews/calculator/calculator.dmv` |

Exit code: 0 when the claim is gone (the rent was taken back), else the error of `libdmview_claim()` (`-ENODEV` when there is no such display
or rent after 10 seconds).

## The view and the application

The view has two properties, set by the application when they change, and emits one event:

| | Name | Value |
|--|------|-------|
| property | `text` | the number on the display (at most 23 characters), `Error` after an error |
| property | `expr` | the small line above it: the first operand and the operation waiting (`12 +`) |
| event | `press` | a key of the screen: its code (`0-9 . + - * / = %`, `n` the sign, `b` erase, `c` clear) |

The keyboard is read from the events `char` (a character) and `key` (Enter `0x28`, Backspace `0x2A`; Escape is the phone's).

## The arithmetic (`src/calc.h`)

`calc_key(calc, key)` takes one key at a time and keeps the state in a `calc_t`; numbers are fixed point (`CALC_SCALE` = 1e6) so no floating
point is used. See the tests for what every key does.
