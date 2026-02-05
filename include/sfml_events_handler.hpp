#pragma once

#include <SFML/Graphics.hpp>
#include <stdexec/execution.hpp>

#include "types_core.hpp"

class SfmlEventHandler {
public:
    using sender_concept = stdexec::sender_t;

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;
        AppState &state_;

        static constexpr float ZOOM_INTERVAL_MS = 100.0f;

        template <typename R>
        explicit OperationState(R &&r, sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
            : receiver_{std::forward<R>(r)}, window_{window}, render_settings_{render_settings}, state_{state} {}

        void start() noexcept {
            try {
                HandleEvents();

                // Авто-зум после обработки пользовательских событий
                if (!state_.should_exit) {
                    HandleAutoZoom();
                }

                // Завершение сендера
                if (state_.should_exit) {
                    stdexec::set_stopped(std::move(receiver_));
                } else {
                    stdexec::set_value(std::move(receiver_));
                }
            } catch (...) {
                stdexec::set_error(std::move(receiver_), std::current_exception());
            }
        }

    private:
        void HandleEvents() {
            sf::Event event;
            while (window_.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                    state_.should_exit = true;
                    break;

                case sf::Event::KeyPressed:
                    HandleKeyPress(event.key);
                    break;

                case sf::Event::MouseButtonPressed:
                    HandleMousePress(event.mouseButton);
                    break;

                case sf::Event::MouseButtonReleased:
                    HandleMouseRelease(event.mouseButton);
                    break;

                default:
                    break;
                }
            }
        }

        void HandleKeyPress(const sf::Event::KeyEvent &key) {
            switch (key.code) {
            case sf::Keyboard::X:
                state_.auto_zoom_enabled = !state_.auto_zoom_enabled;
                state_.need_rerender = true;
                break;

            case sf::Keyboard::C:
                state_.viewport = AppState::INITIAL_VIEWPORT;
                state_.auto_zoom_enabled = false;
                state_.need_rerender = true;
                state_.zoom_clock.restart();
                break;

            default:
                break;
            }
        }

        void HandleMousePress(const sf::Event::MouseButtonEvent &mouse) {
            if (state_.zoom_clock.getElapsedTime().asMilliseconds() < ZOOM_INTERVAL_MS) {
                return;
            }

            if (mouse.button == sf::Mouse::Left) {
                state_.left_mouse_pressed = true;
                ZoomToPoint(mouse.x, mouse.y, true);  // Zoom in
            } else if (mouse.button == sf::Mouse::Right) {
                state_.right_mouse_pressed = true;
                ZoomToPoint(mouse.x, mouse.y, false);  // Zoom out
            }
        }

        void HandleMouseRelease(const sf::Event::MouseButtonEvent &mouse) {
            if (mouse.button == sf::Mouse::Left) {
                state_.left_mouse_pressed = false;
            } else if (mouse.button == sf::Mouse::Right) {
                state_.right_mouse_pressed = false;
            }
        }

        void HandleAutoZoom() {
            if (!state_.auto_zoom_enabled) {
                return;
            }

            if (state_.zoom_clock.getElapsedTime().asMilliseconds() < ZOOM_INTERVAL_MS) {
                return;
            }

            // Конвертируем координаты в пиксели текущего вьюпорта
            const double target_x = AppState::AUTO_ZOOM_TARGET_X;
            const double target_y = AppState::AUTO_ZOOM_TARGET_Y;

            const int pixel_x =
                static_cast<int>((target_x - state_.viewport.x_min) / state_.viewport.width() * render_settings_.width);
            const int pixel_y = static_cast<int>((target_y - state_.viewport.y_min) / state_.viewport.height() *
                                                 render_settings_.height);

            ZoomToPoint(pixel_x, pixel_y, true, 0.95);
        }

        void ZoomToPoint(int pixel_x, int pixel_y, bool zoom_in, double factor = 0.8) {
            const double target_x = state_.viewport.x_min +
                                    (static_cast<double>(pixel_x) / render_settings_.width) * state_.viewport.width();
            const double target_y = state_.viewport.y_min +
                                    (static_cast<double>(pixel_y) / render_settings_.height) * state_.viewport.height();

            const double zoom_factor = zoom_in ? factor : (1.0 / factor);
            const double new_width = state_.viewport.width() * zoom_factor;
            const double new_height = state_.viewport.height() * zoom_factor;

            state_.viewport.x_min = target_x - new_width * (static_cast<double>(pixel_x) / render_settings_.width);
            state_.viewport.x_max = state_.viewport.x_min + new_width;
            state_.viewport.y_min = target_y - new_height * (static_cast<double>(pixel_y) / render_settings_.height);
            state_.viewport.y_max = state_.viewport.y_min + new_height;

            state_.need_rerender = true;
            state_.zoom_clock.restart();
        }
    };

    sf::RenderWindow &window_;
    RenderSettings render_settings_;
    AppState &state_;

    SfmlEventHandler(sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
        : window_{window}, render_settings_{render_settings}, state_{state} {}

    template <stdexec::receiver Receiver>
    auto connect(Receiver &&r) const {
        return OperationState<std::decay_t<Receiver>>{std::forward<Receiver>(r), window_, render_settings_, state_};
    }

    template <typename Env>
    auto get_completion_signatures(Env &&) const {
        return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t(),
                                              stdexec::set_error_t(std::exception_ptr)>{};
    }
};