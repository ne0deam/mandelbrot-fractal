#include "types_core.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

// Тесты ViewPort
TEST(ViewPortTest, Dimensions) {
    ViewPort vp{-2.5, 1.5, -2.0, 2.0};
    EXPECT_NEAR(vp.width(), 4.0, 1e-6);
    EXPECT_NEAR(vp.height(), 4.0, 1e-6);

    // Асимметричный вьюпорт
    ViewPort vp2{-1.0, 0.5, -0.75, 0.75};
    EXPECT_NEAR(vp2.width(), 1.5, 1e-6);
    EXPECT_NEAR(vp2.height(), 1.5, 1e-6);

    // Граничный случай: вырожденный вьюпорт (нулевая ширина/высота)
    ViewPort degenerate{0.0, 0.0, 0.0, 0.0};
    EXPECT_EQ(degenerate.width(), 0.0);
    EXPECT_EQ(degenerate.height(), 0.0);
}

// Тесты FrameClock
TEST(FrameClockTest, BasicTiming) {
    FrameClock clock;

    // Первый замер — должно быть >0 после небольшой задержки
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = clock.GetFrameTime();
    EXPECT_GE(elapsed.count(), 10'000'000);  // >= 10ms в наносекундах

    // После сброса — время должно быть маленьким
    clock.Reset();
    elapsed = clock.GetFrameTime();
    EXPECT_LT(elapsed.count(), 1'000'000);  // < 1ms

    // Граничный случай: мгновенный замер после сброса
    clock.Reset();
    elapsed = clock.GetFrameTime();
    EXPECT_GE(elapsed.count(), 0);  // Не может быть отрицательным
}

// Тесты AvrTimeCounter
TEST(AvrTimeCounterTest, AverageCalculation) {
    AvrTimeCounter counter;

    // Три замера: 10ms, 20ms, 30ms
    counter.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    counter.End();

    counter.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    counter.End();

    counter.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    counter.End();

    EXPECT_EQ(counter.Count(), 3);
    EXPECT_GE(counter.GetAvr(), 18);  // Среднее ≈20ms (с небольшим оверхедом)
    EXPECT_LE(counter.GetAvr(), 25);

    // Сброс счётчика
    counter.Reset();
    EXPECT_EQ(counter.Count(), 0);
    EXPECT_EQ(counter.GetAvr(), 0);

    // Граничный случай: нулевые замеры
    counter.Start();
    counter.End();
    counter.Start();
    counter.End();
    EXPECT_EQ(counter.Count(), 2);
    EXPECT_GE(counter.GetAvr(), 0);  // Среднее не может быть отрицательным
}