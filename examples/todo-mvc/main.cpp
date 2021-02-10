#include <alia/html.hpp>
#include <alia/html/routing.hpp>

#include <algorithm>

using namespace alia;

// MODEL

struct todo_item
{
    bool completed;
    std::string title;
    int id;

    // The following disables copying of this struct (but keeps the default
    // move constructor and move operator).
    // You wouldn't normally do this when writing an alia app. We do it here
    // just to demonstrate that alia
    todo_item(todo_item const&) = delete;
    todo_item&
    operator=(todo_item const&)
        = delete;
    todo_item(todo_item&&) = default;
    todo_item&
    operator=(todo_item&&)
        = default;
    todo_item() = default;
};

typedef std::vector<todo_item> todo_list;

struct app_state
{
    todo_list todos;
    int next_id = 0;
};

app_state
add_todo(app_state state, std::string title)
{
    state.todos.push_back(todo_item{false, title, state.next_id});
    ++state.next_id;
    return state;
}

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

enum class view_filter
{
    ALL,
    ACTIVE,
    COMPLETED
};

view_filter
hash_to_filter(std::string const& hash)
{
    if (hash == "#/active")
        return view_filter::ACTIVE;
    if (hash == "#/completed")
        return view_filter::COMPLETED;
    return view_filter::ALL;
}

bool
matches_filter(view_filter filter, todo_item const& item)
{
    return item.completed && filter != view_filter::ACTIVE
           || !item.completed && filter != view_filter::COMPLETED;
}

// UI

auto
get_alia_id(todo_item const& item)
{
    return make_id(item.id);
}

auto
mouse_inside(html::context ctx, html::element_handle element)
{
    bool* state;
    if (get_data(ctx, &state))
        *state = false;

    element.callback("mouseenter", [&](auto) { *state = true; });
    element.callback("mouseleave", [&](auto) { *state = false; });

    return *state;
}

void
todo_item_ui(html::context ctx, duplex<todo_list> todos)
{
}

void
todo_list_ui(html::context ctx, duplex<todo_list> todos)
{
    auto filter = alia::apply(ctx, hash_to_filter, get_location_hash(ctx));

    ul(ctx, "todo-list", [&] {
        for_each(ctx, todos, [&](size_t index, auto todo) {
            auto matches = apply(ctx, matches_filter, filter, todo);
            alia_if(matches)
            {
                auto editing = get_state(ctx, false);
                li(ctx)
                    .class_("completed", alia_field(todo, completed))
                    .class_("editing", editing)
                    .children([&] {
                        alia_if(editing)
                        {
                            auto new_title = get_transient_state(
                                ctx, alia_field(todo, title));
                            input(ctx, new_title)
                                .class_("edit")
                                .on_enter(
                                    (alia_field(todo, title) <<= new_title,
                                     editing <<= false));
                        }
                        alia_else
                        {
                            auto view = div(ctx, "view");
                            view.children([&] {
                                checkbox(ctx, alia_field(todo, completed))
                                    .class_("toggle");
                                label(ctx, alia_field(todo, title));
                                alia_if(mouse_inside(ctx, view))
                                {
                                    button(
                                        ctx,
                                        actions::erase_index(todos, index))
                                        .class_("destroy");
                                }
                                alia_end
                            });
                            view.on("dblclick", editing <<= true);
                        }
                        alia_end
                    });
            }
            alia_end
        });
    });
}

namespace alia { namespace actions {

template<class Function, class PrimaryState, class... Args>
auto
apply(Function&& f, PrimaryState state, Args... args)
{
    return state <<= lazy_apply(
               std::forward<Function>(f),
               alia::move(state),
               std::move(args)...);
}

}} // namespace alia::actions

void
app_ui(html::context ctx)
{
    auto app = get_state(ctx, lambda_constant([&] { return app_state(); }));

    auto todos = alia_field(app, todos);

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
                    // (actions::apply(
                    //      add_todo, app, alia::mask(trimmed, trimmed != "")),
                    (app <<= alia::lazy_apply(
                         add_todo,
                         alia::move(app),
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
                todo_list_ui(ctx, todos);
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
                    li(ctx).children(
                        [&] { link(ctx, "All", "#/").class_("selected"); });
                    li(ctx).children([&] {
                        link(ctx, "Active", "#/active").class_("selected");
                    });
                    li(ctx).children([&] {
                        link(ctx, "Completed", "#/completed")
                            .class_("selected");
                    });
                });

                button(
                    ctx,
                    "Clear completed",
                    todos <<= lazy_apply(clear_completed, alia::move(todos)))
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
