#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types_sfml.hpp"

#include <print>
#include <stdexec/execution.hpp>

using namespace std::chrono_literals;
namespace ex = stdexec;

namespace mandelbrot {

static auto MakeComputeSender(RenderSettings settings, ViewPort viewport, bool need_rerender) {
    static AvrTimeCounter time_counter;

    constexpr std::uint32_t kStatsPrintEveryNFrames = 10;

    return ex::then([settings, viewport, need_rerender](FrameBuffer *fb) {
        if (!need_rerender) {
            return fb;
        }

        // Запуск таймера
        time_counter.Start();

        const std::uint32_t width = fb->width;
        const std::uint32_t height = fb->height;

        // Вычисление цвета для каждого пикселя
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                // Преобразуем пиксель в комплексное число
                const auto c = Pixel2DToComplex(x, y, viewport, width, height);

                // Вычисляем количество итераций
                const auto iterations = CalculateIterationsForPoint(c, settings.max_iterations, settings.escape_radius);

                // Преобразуем итерации в цвет
                const auto color = IterationsToColor(iterations, settings.max_iterations);

                // Записываем цвет в буфер (формат RGBA: 4 байта на пиксель)
                const size_t idx = ((y * width) + x) * 4;
                fb->rgba[idx + 0] = color.r;  // Red
                fb->rgba[idx + 1] = color.g;  // Green
                fb->rgba[idx + 2] = color.b;  // Blue
                fb->rgba[idx + 3] = 255;      // Alpha (полностью непрозрачный)
            }
        }

        // Остановка таймера и вывод статистики
        time_counter.End();
        if (time_counter.Count() % kStatsPrintEveryNFrames == 0) {
            std::println("\nAverage compute time: {} ms over {} frames", time_counter.GetAvr(), time_counter.Count());
        }

        return fb;
    });
}

}  // namespace mandelbrot