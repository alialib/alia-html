#include <alia/html.hpp>
#include <alia/html/routing.hpp>

#include "model.hpp"

using namespace alia;

// Define the objects in our app's context.
ALIA_DEFINE_TAGGED_TYPE(app_state_tag, duplex<app_state>)
ALIA_DEFINE_TAGGED_TYPE(view_filter_tag, readable<item_filter>)
typedef extend_context_type_t<html::context, app_state_tag, view_filter_tag>
    app_context;

// Tell alia that our TODO items can be stably identified by their 'id' member.
auto
get_alia_id(todo_item const& item)
{
    return make_id(item.id);
}

void
todo_item_ui(app_context ctx, size_t index, duplex<todo_item> todo)
{
}

// Translate location hash strings to our item_filter enum type.
item_filter
hash_to_filter(std::string const& hash)
{
    if (hash == "#/active")
        return item_filter::ACTIVE;
    if (hash == "#/completed")
        return item_filter::COMPLETED;
    return item_filter::ALL;
}

void
todo_list_ui(app_context ctx)
{
    auto todos = alia_field(get<app_state_tag>(ctx), todos);

    ul(ctx, "todo-list", [&] {
        for_each(ctx, todos, [&](size_t index, auto todo) {
            auto matches
                = apply(ctx, matches_filter, get<view_filter_tag>(ctx), todo);
            alia_if(matches)
            {
                auto editing = get_transient_state(ctx, false);
                li(ctx)
                    .class_("completed", alia_field(todo, completed))
                    .class_("editing", editing)
                    .children([&] {
                        alia_if(editing)
                        {
                            auto new_title = get_transient_state(
                                ctx, alia_field(todo, title));
                            auto save
                                = (alia_field(todo, title) <<= new_title,
                                   editing <<= false);
                            input(ctx, new_title)
                                .class_("edit")
                                .on_init([](auto& self) { focus(self); })
                                .on("blur", save)
                                .on_enter(save)
                                .on_escape(editing <<= false);
                        }
                        alia_else
                        {
                            auto view = div(ctx, "view");
                            view.children([&] {
                                checkbox(ctx, alia_field(todo, completed))
                                    .class_("toggle");
                                label(ctx, alia_field(todo, title))
                                    .on("dblclick", editing <<= true);
                                alia_if(mouse_inside(ctx, view))
                                {
                                    button(
                                        ctx,
                                        actions::erase_index(todos, index))
                                        .class_("destroy");
                                }
                                alia_end
                            });
                        }
                        alia_end
                    });
            }
            alia_end
        });
    });
}

// This component function is responsible for the UI for adding a new TODO
// item to the list.
void
new_todo_ui(app_context ctx)
{
    // Get some component-local state to store the title of the new TODO.
    auto new_todo = get_state(ctx, std::string());

    input(ctx, new_todo)
        .class_("new-todo")
        .placeholder("What needs to be done?")
        .autofocus()
        .on_enter(
            // In response to the enter key, we need to:
            // - Trim the new_todo string.
            // - Check that it's not empty. (Abort if it is.)
            // - Invoke 'add_todo' to add it to our app's state.
            // - Reset new_todo to an empty string.
            new_todo <<= "");
    // (actions::apply(
    //      add_todo,
    //      get<app_state_tag>(ctx),
    //      hide_if_empty(apply(ctx, trim, new_todo))),
    //  new_todo <<= ""));
}

void
app_ui(app_context ctx)
{
    auto todos = alia_field(get<app_state_tag>(ctx), todos);
    auto filter = get<view_filter_tag>(ctx);

    header(ctx, "header", [&] {
        h1(ctx, "todos");
        new_todo_ui(ctx);
    });

    alia_if(!is_empty(todos))
    {
        auto items_left = apply(ctx, incomplete_count, todos);
        auto all_complete = items_left == size(todos);

        section(ctx, "main", [&] {
            element(ctx, "input")
                .attr("id", "toggle-all")
                .attr("class", "toggle-all")
                .attr("type", "checkbox")
                .prop("checked", !all_complete)
                .on("change",
                    actions::apply(set_completed_flags, todos, !all_complete));
            label(ctx, "Mark all as complete").attr("for", "toggle-all");
            todo_list_ui(ctx);
        });

        footer(ctx, "footer", [&] {
            span(ctx, "todo-count", [&] {
                strong(ctx, as_text(ctx, items_left));
                text(
                    ctx,
                    conditional(items_left != 1, " items left", " item left"));
            });
            ul(ctx, "filters", [&] {
                li(ctx).children([&] {
                    link(ctx, "All", "#/")
                        .class_("selected", filter == item_filter::ALL);
                });
                li(ctx).children([&] {
                    link(ctx, "Active", "#/active")
                        .class_("selected", filter == item_filter::ACTIVE);
                });
                li(ctx).children([&] {
                    link(ctx, "Completed", "#/completed")
                        .class_("selected", filter == item_filter::COMPLETED);
                });
            });

            button(
                ctx, "Clear completed", actions::apply(clear_completed, todos))
                .class_("clear-completed");
        });
    }
    alia_end
}

void
root_ui(html::context ctx)
{
    // Construct the signal for our applications state...
    // First, create a binding to the raw, JSON state in local storage.
    auto json_state = get_local_state(ctx, "todos-alia");
    // Pass that through a two-way serializer/deserializer to convert it to our
    // native C++ representation.
    auto native_state
        = duplex_apply(ctx, json_to_app_state, app_state_to_json, json_state);
    // And finally, add a default value (of default-initialized state) for when
    // the raw JSON doesn't exist yet.
    // auto state = add_default(
    //     native_state, lambda_constant([&] { return app_state(); }));

    auto state = get_state(ctx, lambda_constant([&] { return app_state(); }));

    // Parse the location hash to determine the active filter.
    auto filter = apply(ctx, hash_to_filter, get_location_hash(ctx));

    // Add these two signals to the alia/HTML context to create our app
    // context.
    with_extended_context<app_state_tag>(ctx, state, [&](auto ctx) {
        with_extended_context<view_filter_tag>(ctx, filter, [&](auto ctx) {
            // Root the app UI in the HTML DOM tree.
            // Our app's UI will be placed at the placeholder HTML element
            // with the ID "app-content".
            placeholder_root(ctx, "app-content", [&] { app_ui(ctx); });
        });
    });
}

int
main()
{
    static html::system sys;
    initialize(sys, root_ui);
    enable_hash_monitoring(sys);
};
