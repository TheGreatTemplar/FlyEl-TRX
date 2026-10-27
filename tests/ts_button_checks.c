/* Compile with the actual implementation and inspect optimized return values. */
#include "ts_button.c"
uint32_t HMI_LongPressTicks(void)
{
    return 2000U;
}


#define CHECK(expr)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            return __LINE__;                                                                                           \
    } while (0)
#define UPDATE(down, pressed, tick) step = TS_Button_Update(&button, down, pressed, tick, step)

int test_short_cycle(void)
{
    ts_button_t button;
    uint32_t step = 10U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0);
    CHECK(step == 10);
    UPDATE(0, 0, 200);
    CHECK(step == 100);
    UPDATE(1, 1, 400);
    UPDATE(0, 0, 500);
    CHECK(step == 1000);
    UPDATE(1, 1, 600);
    UPDATE(0, 0, 700);
    CHECK(step == 10);
    return 0;
}


int test_boundary_and_hold(void)
{
    ts_button_t button;
    uint32_t step = 100U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 100);
    UPDATE(1, 0, 2100);
    CHECK(step == 100);
    UPDATE(1, 0, 2101);
    CHECK(step == 10000 && button.restore_step_hz == 100);
    UPDATE(1, 0, 20000);
    CHECK(step == 10000);
    UPDATE(0, 0, 20100);
    CHECK(step == 10000);
    return 0;
}


int test_short_exit(void)
{
    ts_button_t button;
    uint32_t step = 1000U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0);
    UPDATE(1, 0, 2001);
    UPDATE(0, 0, 2100);
    CHECK(step == 10000);
    UPDATE(1, 1, 2200);
    CHECK(step == 1000);
    UPDATE(0, 0, 2300);
    CHECK(step == 1000);
    UPDATE(1, 1, 2400);
    UPDATE(0, 0, 2500);
    CHECK(step == 10);
    return 0;
}


int test_long_exit(void)
{
    ts_button_t button;
    uint32_t step = 10U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0);
    UPDATE(1, 0, 2001);
    UPDATE(0, 0, 2100);
    CHECK(step == 10000);
    UPDATE(1, 1, 2200);
    CHECK(step == 10);
    UPDATE(1, 0, 9000);
    CHECK(step == 10);
    UPDATE(0, 0, 9100);
    CHECK(step == 10);
    return 0;
}


int test_hold_release_without_intermediate_poll(void)
{
    ts_button_t button;
    uint32_t step = 100U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0);
    UPDATE(0, 0, 2001);
    CHECK(step == 10000);
    return 0;
}


int test_exactly_one_second_is_short(void)
{
    ts_button_t button;
    uint32_t step = 100U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0);
    UPDATE(0, 0, 2000);
    CHECK(step == 1000);
    return 0;
}


int test_tick_wrap(void)
{
    ts_button_t button;
    uint32_t step = 1000U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0xffffff00U);
    UPDATE(1, 0, 0x6d0U);
    CHECK(step == 1000);
    UPDATE(1, 0, 0x6d1U);
    CHECK(step == 10000);
    UPDATE(0, 0, 0x700U);
    UPDATE(1, 1, 0x800U);
    CHECK(step == 1000);
    return 0;
}


int test_repeated_ten_khz_entry(void)
{
    ts_button_t button;
    uint32_t step = 10U;
    TS_Button_Init(&button, step);
    UPDATE(1, 1, 0);
    UPDATE(0, 0, 100);
    CHECK(step == 100);
    UPDATE(1, 1, 200);
    UPDATE(1, 0, 2201);
    UPDATE(0, 0, 2300);
    CHECK(step == 10000);
    UPDATE(1, 1, 2400);
    UPDATE(0, 0, 2500);
    CHECK(step == 100);
    UPDATE(1, 1, 2600);
    UPDATE(0, 0, 2700);
    CHECK(step == 1000);
    UPDATE(1, 1, 2800);
    UPDATE(1, 0, 4801);
    UPDATE(0, 0, 4900);
    CHECK(step == 10000);
    UPDATE(1, 1, 5000);
    UPDATE(0, 0, 5100);
    CHECK(step == 1000);
    return 0;
}
