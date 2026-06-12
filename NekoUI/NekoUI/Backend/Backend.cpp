#include "Backend.hpp"
using namespace neko::backend;
Backend::Backend() {}
Backend::~Backend() {}

auto Backend::get_handle() -> Handle {
    return nullptr;
}

auto Backend::resize(Handle window_handle, Vec2<int> new_size) -> void {}

auto Backend::attach(const std::shared_ptr<window::Window>& window) -> bool {
    return false;
}

auto Backend::deattach(const std::shared_ptr<window::Window>& window) -> bool {
    return false;
}

auto Backend::submit(Handle window_handle) -> void {}
auto Backend::draw_text(Handle window_handle) -> void {}
auto Backend::draw_line(Handle window_handle) -> void {}
auto Backend::draw_triangle(Handle window_handle) -> void {}
auto Backend::draw_rect(Handle window_handle, Vec4<int> range, Color rgba, int thickness) -> void {}
auto Backend::draw_rect_fill(Handle window_handle) -> void {}
auto Backend::draw_circle_fill(Handle window_handle) -> void {}
auto Backend::draw_image(Handle window_handle) -> void {}
