#include "mandelbrot_fractal_utils.hpp"
#include <gtest/gtest.h>

using namespace mandelbrot;

// Тесты вычисления итераций для точек
TEST(MandelbrotUtilsTest, CalculateIterationsForPoint) {
    // Точка (0,0) всегда внутри множества → достигает max_iterations
    EXPECT_EQ(CalculateIterationsForPoint(Complex{0.0, 0.0}, 100, 2.0), 100);

    // Точка (2,0) выходит на 2-й итерации
    EXPECT_EQ(CalculateIterationsForPoint(Complex{2.0, 0.0}, 100, 2.0), 2);

    // Точка (-1,0) — известная точка множества
    EXPECT_EQ(CalculateIterationsForPoint(Complex{-1.0, 0.0}, 100, 2.0), 100);

    // Точка (1.0, 1.0) выходит быстро (за 2 итерации)
    EXPECT_EQ(CalculateIterationsForPoint(Complex{1.0, 1.0}, 100, 2.0), 2);

    // Точка (0.5, 0.5) выходит за 5 итераций
    auto iters = CalculateIterationsForPoint(Complex{0.5, 0.5}, 100, 2.0);
    EXPECT_GE(iters, 4);
    EXPECT_LE(iters, 6);
}

// Тесты преобразования пикселя в комплексное число
TEST(MandelbrotUtilsTest, Pixel2DToComplex) {
    ViewPort viewport{-2.5, 1.5, -2.0, 2.0};  // width=4.0, height=4.0

    // Центр экрана (400,300) → центр вьюпорта (-0.5, 0.0)
    auto c1 = Pixel2DToComplex(400, 300, viewport, 800, 600);
    EXPECT_NEAR(c1.real(), -0.5, 1e-6);
    EXPECT_NEAR(c1.imag(), 0.0, 1e-6);

    // Верхний левый угол (0,0) → левый нижний угол вьюпорта (-2.5, -2.0)
    auto c2 = Pixel2DToComplex(0, 0, viewport, 800, 600);
    EXPECT_NEAR(c2.real(), -2.5, 1e-6);
    EXPECT_NEAR(c2.imag(), -2.0, 1e-6);

    // Нижний правый угол (799,599) → близко к правому верхнему углу (1.5, 2.0)
    // Из-за дискретизации последний пиксель не достигает крайних координат
    auto c3 = Pixel2DToComplex(799, 599, viewport, 800, 600);
    EXPECT_NEAR(c3.real(), 1.495, 1e-6);     // 799/800 * 4.0 - 2.5 = 1.495
    EXPECT_NEAR(c3.imag(), 1.993333, 1e-6);  // 599/600 * 4.0 - 2.0 ≈ 1.993333

    // Граничный случай: очень маленький вьюпорт (глубокий зум)
    ViewPort deep_zoom{-0.7437, -0.7435, 0.1317, 0.1319};
    auto c4 = Pixel2DToComplex(400, 300, deep_zoom, 800, 600);
    EXPECT_NEAR(c4.real(), -0.7436, 1e-4);
    EXPECT_NEAR(c4.imag(), 0.1318, 1e-4);
}

// Тесты раскраски итераций
TEST(MandelbrotUtilsTest, IterationsToColor) {
    // Точка внутри множества → чёрный
    auto black = IterationsToColor(100, 100);
    EXPECT_EQ(black.r, 0);
    EXPECT_EQ(black.g, 0);
    EXPECT_EQ(black.b, 0);

    // Точка с 0 итераций → красный (hue=0)
    auto red = IterationsToColor(0, 100);
    EXPECT_EQ(red.r, 255);
    EXPECT_EQ(red.g, 0);
    EXPECT_EQ(red.b, 0);

    // Точка с 33 итерациями → зелёный оттенок (hue≈120)
    auto greenish = IterationsToColor(33, 100);
    EXPECT_GT(greenish.g, 100);  // Зелёная компонента доминирует

    // Точка с 66 итерациями → синий оттенок (hue≈240)
    auto blueish = IterationsToColor(66, 100);
    EXPECT_GT(blueish.b, 100);  // Синяя компонента доминирует

    // Граничный случай: 1 итерация
    auto almost_red = IterationsToColor(1, 100);
    EXPECT_GE(almost_red.r, 200);  // Близко к красному
}