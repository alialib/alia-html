#include <alia/html/widgets.hpp>

namespace alia { namespace html {

namespace detail {

struct input_data
{
    captured_id value_id;
    std::string value;
    signal_validation_data validation;
    unsigned version = 0;
};

element_handle<html::context>
input(html::context ctx, duplex<std::string> value_)
{
    input_data* data;
    get_cached_data(ctx, &data);

    auto value = enforce_validity(ctx, value_, data->validation);

    on_refresh(ctx, [&](auto ctx) {
        if (!value.is_invalidated())
        {
            refresh_signal_view(
                data->value_id,
                value,
                [&](std::string new_value) {
                    data->value = std::move(new_value);
                    ++data->version;
                },
                [&]() {
                    data->value.clear();
                    ++data->version;
                });
        }
    });

    return element(ctx, "input")
        .prop("value", data->value)
        .callback("input", [=](emscripten::val& e) {
            auto new_value = e["target"]["value"].as<std::string>();
            write_signal(value, new_value);
            data->value = new_value;
            ++data->version;
        });
}

element_handle<html::context>
link(html::context ctx, readable<std::string> text, action<> on_click)
{
    return element(ctx, "a")
        .attr("href", "javascript: void(0);")
        .attr("disabled", on_click.is_ready() ? "false" : "true")
        .children([&](auto ctx) { text_node(ctx, text); })
        .callback("click", [&](emscripten::val) { perform_action(on_click); });
}

} // namespace detail

element_handle<html::context>
button(html::context ctx, action<> on_click)
{
    return element(ctx, "button")
        .attr("type", "button")
        .attr("disabled", !on_click.is_ready())
        .callback("click", [&](auto& e) { perform_action(on_click); });
}

element_handle<html::context>
button(html::context ctx, readable<std::string> text, action<> on_click)
{
    return button(ctx, on_click).text(text);
}

element_handle<html::context>
button(html::context ctx, char const* text, action<> on_click)
{
    return button(ctx, on_click).text(value(text));
}

}} // namespace alia::html
