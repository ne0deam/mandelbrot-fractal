#include "types_core.hpp"
#include <cmath>
#include <gtest/gtest.h>

// Упрощённая версия логики зума для тестирования
class ZoomLogicTest {
public:
    static void ZoomToPoint(ViewPort &viewport, int pixel_x, int pixel_y, std::uint32_t screen_width,
                            std::uint32_t screen_height, bool zoom_in, double factor = 0.8) {
        const double target_x = viewport.x_min + (static_cast<double>(pixel_x) / screen_width) * viewport.width();
        const double target_y = viewport.y_min + (static_cast<double>(pixel_y) / screen_height) * viewport.height();

        const double zoom_factor = zoom_in ? factor : (1.0 / factor);
        const double new_width = viewport.width() * zoom_factor;
        const double new_height = viewport.height() * zoom_factor;

        viewport.x_min = target_x - new_width * (static_cast<double>(pixel_x) / screen_width);
        viewport.x_max = viewport.x_min + new_width;
        viewport.y_min = target_y - new_height * (static_cast<double>(pixel_y) / screen_height);
        viewport.y_max = viewport.y_min + new_height;
    }
};

// Тест зума в центр
TEST(EventsHandlerTest, ZoomInCenter) {
    ViewPort viewport = AppState::INITIAL_VIEWPORT;

    // Зум в центр экрана (400,300)
    ZoomLogicTest::ZoomToPoint(viewport, 400, 300, 800, 600, true, 0.8);

    // После зума в центр — центр вьюпорта должен остаться тем же (-0.5, 0.0)
    double center_x = (viewport.x_min + viewport.x_max) / 2.0;
    double center_y = (viewport.y_min + viewport.y_max) / 2.0;

    EXPECT_NEAR(center_x, -0.5, 1e-6);
    EXPECT_NEAR(center_y, 0.0, 1e-6);

    // Ширина и высота должны уменьшиться в 0.8 раза
    EXPECT_NEAR(viewport.width(), 4.0 * 0.8, 1e-6);
    EXPECT_NEAR(viewport.height(), 4.0 * 0.8, 1e-6);
}

// Тест зума в угол
TEST(EventsHandlerTest, ZoomInCorner) {
    ViewPort viewport = AppState::INITIAL_VIEWPORT;

    // Зум в верхний левый угол (0,0)
    ZoomLogicTest::ZoomToPoint(viewport, 0, 0, 800, 600, true, 0.5);

    // После зума в угол (0,0) — левый нижний угол вьюпорта должен остаться (-2.5, -2.0)
    EXPECT_NEAR(viewport.x_min, -2.5, 1e-6);
    EXPECT_NEAR(viewport.y_min, -2.0, 1e-6);

    // Ширина и высота уменьшились в 2 раза
    EXPECT_NEAR(viewport.width(), 4.0 * 0.5, 1e-6);
    EXPECT_NEAR(viewport.height(), 4.0 * 0.5, 1e-6);
}

// Тест сброса вьюпорта
TEST(EventsHandlerTest, ResetViewport) {
    ViewPort viewport{-1.0, 0.0, -0.5, 0.5};  // изменённый вьюпорт

    // Сброс к начальному виду
    viewport = AppState::INITIAL_VIEWPORT;

    EXPECT_NEAR(viewport.x_min, -2.5, 1e-6);
    EXPECT_NEAR(viewport.x_max, 1.5, 1e-6);
    EXPECT_NEAR(viewport.y_min, -2.0, 1e-6);
    EXPECT_NEAR(viewport.y_max, 2.0, 1e-6);
}

// Тест координат "морского конька"
TEST(EventsHandlerTest, SeahorseValleyCoordinates) {
    // Проверяем, что координаты находятся внутри начального вьюпорта
    EXPECT_GT(AppState::AUTO_ZOOM_TARGET_X, AppState::INITIAL_VIEWPORT.x_min);
    EXPECT_LT(AppState::AUTO_ZOOM_TARGET_X, AppState::INITIAL_VIEWPORT.x_max);
    EXPECT_GT(AppState::AUTO_ZOOM_TARGET_Y, AppState::INITIAL_VIEWPORT.y_min);
    EXPECT_LT(AppState::AUTO_ZOOM_TARGET_Y, AppState::INITIAL_VIEWPORT.y_max);

    // Проверяем точные значения (известные координаты "морского конька")
    EXPECT_NEAR(AppState::AUTO_ZOOM_TARGET_X, -0.7436438870371587, 1e-12);
    EXPECT_NEAR(AppState::AUTO_ZOOM_TARGET_Y, 0.1318259043124, 1e-12);
}

// Граничный случай: очень глубокий зум (маленький вьюпорт)
TEST(EventsHandlerTest, DeepZoomStability) {
    ViewPort viewport{-0.7436438870371587 - 1e-10, -0.7436438870371587 + 1e-10, 0.1318259043124 - 1e-10,
                      0.1318259043124 + 1e-10};

    // Должен корректно обработать зум даже при очень маленьком вьюпорте
    EXPECT_GT(viewport.width(), 0.0);
    EXPECT_GT(viewport.height(), 0.0);
    EXPECT_LT(viewport.width(), 1e-9);
    EXPECT_LT(viewport.height(), 1e-9);

    // Выполняем зум — не должно быть деления на ноль или переполнения
    ZoomLogicTest::ZoomToPoint(viewport, 400, 300, 800, 600, true, 0.9);
    EXPECT_GT(viewport.width(), 0.0);
    EXPECT_GT(viewport.height(), 0.0);

    // После зума ширина/высота должны уменьшиться
    EXPECT_LT(viewport.width(), 2e-10);
    EXPECT_LT(viewport.height(), 2e-10);
}

// Граничный случай: зум за пределы допустимых значений (защита от некорректных координат)
TEST(EventsHandlerTest, InvalidPixelCoordinates) {
    ViewPort viewport = AppState::INITIAL_VIEWPORT;

    // Координаты за пределами экрана — должны обрабатываться корректно
    ZoomLogicTest::ZoomToPoint(viewport, -100, -100, 800, 600, true, 0.8);
    EXPECT_TRUE(std::isfinite(viewport.x_min));
    EXPECT_TRUE(std::isfinite(viewport.x_max));
    EXPECT_TRUE(std::isfinite(viewport.y_min));
    EXPECT_TRUE(std::isfinite(viewport.y_max));

    // Координаты сильно за пределами — не должно вызывать переполнения
    ZoomLogicTest::ZoomToPoint(viewport, 10000, 10000, 800, 600, true, 0.8);
    EXPECT_TRUE(std::isfinite(viewport.x_min));
    EXPECT_TRUE(std::isfinite(viewport.x_max));
}