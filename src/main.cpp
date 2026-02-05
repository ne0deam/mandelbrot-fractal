#include <chrono>
#include <memory>
#include <print>
#include <thread>
#include <utility>

#include <SFML/Graphics.hpp>

#include <exec/any_sender_of.hpp>
#include <exec/repeat_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "sfml_display_sender.hpp"
#include "sfml_events_handler.hpp"
#include "types_sfml.hpp"

using namespace std::chrono_literals;
namespace ex = stdexec;

class WaitForFPS {
public:
    explicit WaitForFPS(FrameClock &frame_clock, unsigned int target_fps = 60)
        : frame_clock_(frame_clock),
          frame_time_(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1) / target_fps)) {}

    void operator()() {
        auto elapsed = frame_clock_.GetFrameTime();
        if (elapsed < frame_time_) {
            std::this_thread::sleep_for(frame_time_ - elapsed);
        }
        frame_clock_.Reset();
    }

private:
    FrameClock &frame_clock_;
    const std::chrono::milliseconds frame_time_;
};

class MandelbrotApp {
public:
    MandelbrotApp() : compute_pool_{std::max(1u, std::thread::hardware_concurrency())}, sfml_thread_{1} {
        std::println("hardware_concurrency: {}\n", std::thread::hardware_concurrency());
    }

    void Run() {
        auto compute_sched = compute_pool_.get_scheduler();
        auto sfml_sched = sfml_thread_.get_scheduler();

        // Инициализация SFML-состояния в выделенном потоке
        auto initialize =
            ex::on(sfml_sched, ex::just() | ex::then([this]() {
                                   state_ = std::make_unique<SfmlState>(RenderSettings{
                                       .width = 800, .height = 600, .max_iterations = 100, .escape_radius = 2.0});
                               }));
        ex::sync_wait(std::move(initialize));

        // Основной пайплайн обработки одного кадра
        auto process_frame =
            ex::on(sfml_sched, SfmlEventHandler{state_->window, state_->render_settings, state_->app_state}) |
            ex::let_value([this, compute_sched, sfml_sched]() {
                bool need_rerender = state_->app_state.need_rerender;
                state_->app_state.need_rerender = false;

                auto compute =
                    ex::just(&state_->fb) |
                    mandelbrot::MakeComputeSender(state_->render_settings, state_->app_state.viewport, need_rerender);
                auto display = render::MakeSfmlDisplaySender(*state_);

                return ex::on(compute_sched, std::move(compute)) | ex::on(sfml_sched, std::move(display));
            }) |
            ex::then([this]() { WaitForFPS{state_->frame_clock, 60}(); });

        // Бесконечный цикл выполнения пайплайна до выхода
        auto repeated_pipeline = std::move(process_frame) | ex::then([this] { return state_->app_state.should_exit; }) |
                                 exec::repeat_until();
        ex::sync_wait(std::move(repeated_pipeline));
    }

private:
    std::unique_ptr<SfmlState> state_;
    exec::static_thread_pool compute_pool_;
    exec::static_thread_pool sfml_thread_;
};

int main() {
    std::println("=== Mandelbrot Fractal Renderer ===\n");
    std::println("Controls:");
    std::println("  Left Mouse Button  - Zoom In");
    std::println("  Right Mouse Button - Zoom Out");
    std::println("  X                  - Toggle Auto Zoom (infinite zoom to 'Seahorse Valley' point)");
    std::println("  C                  - Reset to Initial View");
    std::println("  Close Window       - Exit\n");

    try {
        MandelbrotApp app;
        app.Run();
    } catch (const std::exception &e) {
        std::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}