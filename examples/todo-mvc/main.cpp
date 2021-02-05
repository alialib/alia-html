#include <alia/html.hpp>
#include <alia/html/routing.hpp>

#include <algorithm>

using namespace alia;

// MODEL

struct todo_item
{
    bool completed;
    std::string description;
};

typedef std::vector<todo_item> todo_list;

size_t
incomplete_count(todo_list const& todos)
{
    return std::count_if(todos.begin(), todos.end(), [](auto const& item) {
        return !item.completed;
    });
}

todo_list
clear_completed(todo_list todos)
{
    todos.erase(
        std::remove_if(
            todos.begin(),
            todos.end(),
            [](todo_item const& item) { return item.completed; }),
        todos.end());
    return todos;
}

// How is it that we still don't have string trimming in the C++ standard
// library!?
std::string
trim(std::string const& str)
{
    auto const begin = str.find_first_not_of(" ");
    if (begin == std::string::npos)
        return "";

    auto const end = str.find_last_not_of(" ");

    return str.substr(begin, end - begin + 1);
}

// UI

void
app_ui(html::context ctx)
{
    auto todos = get_state(ctx, lambda_constant([&] { return todo_list(); }));

    placeholder_root(ctx, "app-content", [&] {
        header(ctx, "header", [&] {
            h1(ctx, "todos");
            auto new_todo = get_state(ctx, std::string());
            auto trimmed = apply(ctx, trim, new_todo);
            input(ctx, new_todo)
                .class_("new-todo")
                .placeholder("What needs to be done?")
                .autofocus()
                .on_enter(
                    (alia::actions::push_back(todos) << alia::apply(
                         ctx,
                         [](auto description) {
                             return todo_item{false, description};
                         },
                         alia::mask(trimmed, trimmed != "")),
                     new_todo <<= ""));
        });

        alia_if(!is_empty(todos))
        {
            section(ctx, "main", [&] {
                element(ctx, "input")
                    .attr("id", "toggle-all")
                    .attr("class", "toggle-all")
                    .attr("type", "checkbox");
                label(ctx, "Mark all as complete").attr("for", "toggle-all");
                ul(ctx, "todo-list", [&] {
                    for_each(ctx, todos, [&](auto ctx, auto todo) {
                        li(ctx)
                            .class_(
                                mask("completed", alia_field(todo, completed)))
                            //.class(mask("editing", editing))
                            .children([&] {
                                div(ctx, "view", [&] {
                                    checkbox(ctx, alia_field(todo, completed))
                                        .class_("toggle");
                                    label(ctx, alia_field(todo, description));
                                    //     <button class="destroy"></button>
                                    // </div>
                                    // <input class="edit"
                                    //  value="Create a TodoMVC template">
                                });
                            });
                    });
                });
            });

            footer(ctx, "footer", [&] {
                span(ctx, "todo-count", [&] {
                    auto items_left = apply(ctx, incomplete_count, todos);
                    strong(ctx, as_text(ctx, items_left));
                    text(
                        ctx,
                        conditional(
                            items_left != 1, " items left", " item left"));
                });
                ul(ctx, "filters", [&] {
                    li(ctx).children([&] {
                        internal_link(ctx, "All", "/").class_("selected");
                    });
                    li(ctx).children([&] {
                        internal_link(ctx, "Active", "/active")
                            .class_("selected");
                    });
                    li(ctx).children([&] {
                        internal_link(ctx, "Completed", "/completed")
                            .class_("selected");
                    });
                });

                button(ctx, "Clear completed", actions::noop())
                    .class_("clear-completed");
            });
        }
        alia_end
    });
}

int
main()
{
    static html::system sys;
    initialize(sys, app_ui);
    enable_hash_monitoring(sys);
};
