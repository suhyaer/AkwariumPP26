#include <unity.h>
#include "MovingAverage.h"

void setUp(void) {}
void tearDown(void) {}

void test_first_sample_returns_itself() {
    MovingAverage f(10);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, f.addSample(5.0f));
}

void test_partial_buffer_uses_sample_count() {
    // bufor jeszcze nie jest zapelniony: srednia z liczby probek, nie z N
    MovingAverage f(10);
    f.addSample(1.0f);
    f.addSample(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, f.addSample(3.0f));  // (1+2+3)/3
}

void test_window_rolls_over() {
    MovingAverage f(3);
    f.addSample(1.0f);
    f.addSample(2.0f);
    f.addSample(3.0f);                                           // (1+2+3)/3 = 2
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, f.addSample(4.0f));  // (2+3+4)/3 = 3
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, f.addSample(5.0f));  // (3+4+5)/3 = 4
}

void test_constant_input_gives_constant_output() {
    MovingAverage f(10);
    float r = 0;
    for (int i = 0; i < 50; i++) r = f.addSample(7.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 7.5f, r);
}

void test_spike_is_attenuated() {
    // jednorazowy skok 1000 na tle 100 powinien zmienic srednia tylko o 1/N skoku
    MovingAverage f(10);
    for (int i = 0; i < 10; i++) f.addSample(100.0f);
    float r = f.addSample(1000.0f);                              // (9*100 + 1000)/10 = 190
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 190.0f, r);
}

void test_reset_clears_buffer() {
    MovingAverage f(5);
    for (int i = 0; i < 5; i++) f.addSample(100.0f);
    f.reset();
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, f.addSample(10.0f));
}

void test_invalid_window_size_is_clamped() {
    MovingAverage f(0);                                          // nie moze dzielic przez 0
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, f.addSample(4.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, f.addSample(6.0f));  // okno = 1
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_first_sample_returns_itself);
    RUN_TEST(test_partial_buffer_uses_sample_count);
    RUN_TEST(test_window_rolls_over);
    RUN_TEST(test_constant_input_gives_constant_output);
    RUN_TEST(test_spike_is_attenuated);
    RUN_TEST(test_reset_clears_buffer);
    RUN_TEST(test_invalid_window_size_is_clamped);
    return UNITY_END();
}
