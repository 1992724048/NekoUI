#include "Button.hpp"

#include <random>

#include "../Component/Animation.hpp"
using namespace neko::widget;

auto Button::mouse_move(const Vec2<int> pos) -> void {
    Mouse::mouse_move(pos);
}

auto Button::mouse_button(const Vec2<int> pos, const std::bitset<7> state) -> void {
    Mouse::mouse_button(pos, state);
}

auto Button::mouse_wheel(const Vec2<int> pos, const int wheel) -> void {
    Mouse::mouse_wheel(pos, wheel);
}

auto Button::draw(std::shared_ptr<backend::Backend>& backend) -> void {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution dist(0, 255);

    static animation::EaseInOutSineAnimation<std::uint8_t> r_anim(0, 300);
    static animation::EaseInOutSineAnimation<std::uint8_t> g_anim(0, 300);
    static animation::EaseInOutSineAnimation<std::uint8_t> b_anim(0, 300);

    if (!r_anim.is_done()) {
        const int t = dist(gen);
        r_anim(t);
    }
    if (!g_anim.is_done()) {
        const int t = dist(gen);
        g_anim(t);
    }
    if (!b_anim.is_done()) {
        const int t = dist(gen);
        b_anim(t);
    }

    backend->draw_rect({0, 0, 0, 0}, {r_anim, g_anim, b_anim, 255}, 1);
}
