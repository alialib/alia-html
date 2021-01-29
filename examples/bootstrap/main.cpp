#include <alia/html/bootstrap.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

using namespace alia;
using namespace html;

/// [namespace]
namespace bs = alia::html::bootstrap;
/// [namespace]

ALIA_DEFINE_TAGGED_TYPE(src_tag, apply_signal<std::string>&)

typedef extend_context_type_t<html::context, src_tag> demo_context;

void
section_heading(html::context ctx, char const* anchor, char const* label)
{
    element(ctx, "h2").classes("mt-5 mb-4").children([&] {
        element(ctx, "a")
            .attr("name", anchor)
            .attr(
                "style",
                "padding-top: 112px; margin-top: -112px; "
                "display: inline-block;")
            .text(label);
    });
}

void
subsection_heading(html::context ctx, char const* label)
{
    element(ctx, "h4").classes("mt-3 mb-3").text(label);
}

std::string
extract_code_snippet(std::string const& code, std::string const& tag)
{
    std::string marker = "/// [" + tag + "]";
    auto start = code.find(marker);
    if (start == std::string::npos)
        return "Whoops! The code snippet [" + tag + "] is missing!";

    start += marker.length() + 1;
    auto end = code.find(marker, start);
    if (end == std::string::npos)
        return "Whoops! The code snippet [" + tag + "] is missing!";

    // Determine the indentation. We assume the first line of the code snippet
    // is the least indented (or at least tied for that distinction).
    int indentation = 0;
    while (std::isspace(code[start + indentation]))
        ++indentation;

    std::ostringstream snippet;
    auto i = start;
    while (true)
    {
        // Skip the indentation.
        int x = 0;
        while (x < indentation && code[i + x] == ' ')
            ++x;
        i += x;

        if (i >= end)
            break;

        // Copy the actual code line.
        while (code[i] != '\n')
        {
            snippet << code[i];
            ++i;
        }
        snippet << '\n';
        ++i;
    }

    return snippet.str();
}

void
code_snippet(demo_context ctx, char const* tag)
{
    auto src = apply(ctx, extract_code_snippet, get<src_tag>(ctx), value(tag));
    auto code = element(ctx, "pre").class_("language-cpp").children([&] {
        element(ctx, "code").text(src);
    });
    on_value_gain(ctx, src, callback([&] {
                      EM_ASM(
                          { Prism.highlightElement(Module.nodes[$0]); },
                          code.asmdom_id());
                  }));
}

void
buttons_demo(demo_context ctx)
{
    section_heading(ctx, "buttons", "Buttons");

    subsection_heading(ctx, "Normal");
    div(ctx, "demo-panel", [&] {
        /// [normal-buttons]
        bs::primary_button(ctx, "Primary", actions::noop());
        bs::secondary_button(ctx, "Secondary", actions::noop());
        bs::success_button(ctx, "Success", actions::noop());
        bs::danger_button(ctx, "Danger", actions::noop());
        bs::warning_button(ctx, "Warning", actions::noop());
        bs::info_button(ctx, "Info", actions::noop());
        bs::light_button(ctx, "Light", actions::noop());
        bs::dark_button(ctx, "Dark", actions::noop());
        bs::link_button(ctx, "Link", actions::noop());
        /// [normal-buttons]
    });
    code_snippet(ctx, "normal-buttons");

    subsection_heading(ctx, "Outline");
    div(ctx, "demo-panel", [&] {
        /// [outline-buttons]
        bs::outline_primary_button(ctx, "Primary", actions::noop());
        bs::outline_secondary_button(ctx, "Secondary", actions::noop());
        bs::outline_success_button(ctx, "Success", actions::noop());
        bs::outline_danger_button(ctx, "Danger", actions::noop());
        bs::outline_warning_button(ctx, "Warning", actions::noop());
        bs::outline_info_button(ctx, "Info", actions::noop());
        bs::outline_light_button(ctx, "Light", actions::noop());
        bs::outline_dark_button(ctx, "Dark", actions::noop());
        /// [outline-buttons]
    });
    code_snippet(ctx, "outline-buttons");
}

void
modals_demo(demo_context ctx)
{
    section_heading(ctx, "modals", "Modals");

    subsection_heading(ctx, "Simple");
    {
        /// [simple-modal]
        auto my_modal = bs::modal(ctx, [&] {
            bs::standard_modal_header(ctx, "Simple Modal");
            bs::modal_body(ctx, [&] { p(ctx, "This is a simple modal."); });
            bs::modal_footer(ctx, [&] {
                bs::primary_button(
                    ctx, "Close", callback([&] { bs::close_modal(); }));
            });
        });
        bs::primary_button(
            ctx, "Activate", callback([&] { my_modal.activate(); }));
        /// [simple-modal]
    }
    code_snippet(ctx, "simple-modal");

    subsection_heading(ctx, "w/Shared State");
    {
        /// [shared-state-modal]
        auto my_state = get_state(ctx, "Edit me!");
        p(ctx, "Here's some state that will be visible to the modal:");
        input(ctx, my_state);
        auto my_modal = bs::modal(ctx, [&] {
            bs::standard_modal_header(ctx, "Shared State Modal");
            bs::modal_body(ctx, [&] {
                p(ctx,
                  "Since this modal lives inside the parent component, it can "
                  "see (and modify) the parent's state:");
                input(ctx, my_state);
            });
            bs::modal_footer(ctx, [&] {
                bs::primary_button(
                    ctx, "Close", callback([&] { bs::close_modal(); }));
            });
        });
        bs::primary_button(
            ctx, "Activate", callback([&] { my_modal.activate(); }));
        /// [shared-state-modal]
    }
    code_snippet(ctx, "shared-state-modal");
}

void
root_ui(html::context vanilla_ctx)
{
    auto src = fetch_text(vanilla_ctx, value("src/main.cpp"));

    auto ctx = extend_context<src_tag>(vanilla_ctx, src);

    placeholder_root(ctx, "demos", [&] {
        div(ctx, "container", [&] {
            div(ctx, "row", [&] {
                div(ctx, "col-12", [&] {
                    p(ctx,
                      "All code snippets assume the following namespace "
                      "alias is in effect:");
                    code_snippet(ctx, "namespace");

                    buttons_demo(ctx);
                    modals_demo(ctx);
                });
            });
        });
    });
}

int
main()
{
    static html::system the_sys;
    initialize(the_sys, root_ui);
};
