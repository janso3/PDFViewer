#include "Viewer.h"

#include <AK/Format.h>
#include <AK/String.h>
#include <LibCore/StandardPaths.h>
#include <LibCore/ResourceImplementationFile.h>
#include <LibGfx/Font/FontDatabase.h>
#include <LibMain/Main.h>

template<typename T>
static T handle_error(PDF::PDFErrorOr<T> maybe_error)
{
    if (maybe_error.is_error()) {
        auto error = maybe_error.release_error();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error.message().characters(), NULL);
    }
    return static_cast<T>(maybe_error.release_value());
}

ErrorOr<int> serenity_main(Main::Arguments args)
{
    char const* serenity_resource_root = "../../serenity/Base/";
    Core::ResourceImplementation::install(make<Core::ResourceImplementationFile>(MUST(String::formatted("{}/res", serenity_resource_root))));
    //Gfx::FontDatabase::set_default_fonts_lookup_path(DeprecatedString::formatted("{}/res/fonts", serenity_resource_root));
    for (auto const& path : Core::StandardPaths::font_directories().release_value_but_fixme_should_propagate_errors())
        Gfx::FontDatabase::the().load_all_fonts_from_uri(MUST(String::formatted("file://{}", path)));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        dbgln("SDL_Init failed: {}", SDL_GetError());
        return -1;
    }

    SDL_Window* window;

    if (!(window = SDL_CreateWindow("LibPDF viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 800, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE))) {
        dbgln("SDL_CreateWindow failed: {}", SDL_GetError());
        return -1;
    }

    auto* viewer = handle_error(PDF::Viewer::create(window));

    if (args.argc > 1) {
        auto path = args.strings[1];
        handle_error(viewer->load(TRY(String::from_utf8(path))));
    }

    (void)viewer->render();

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
                running = false;
            else if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
                switch (ev.key.keysym.sym) {
                case SDLK_RIGHT:
                    (void)viewer->next_page();
                    (void)viewer->render();
                    break;
                case SDLK_LEFT:
                    (void)viewer->previous_page();
                    (void)viewer->render();
                    break;
                case SDLK_o: {
                    if (ev.key.keysym.mod == KMOD_LCTRL) {
                        char path[1024];
                        auto* file = popen("zenity --file-selection", "r");
                        fgets(path, sizeof(path), file);
                        auto path_length = strlen(path);
                        if (path_length < 10)
                            break;
                        // HACK: Remove newline.
                        path[path_length - 1] = 0;
                        try {
                            handle_error(viewer->load(TRY(String::formatted("{}", path))));
                        } catch(...) {
                            dbgln("--- Exception ---");
                        }
                        (void)viewer->render();
                    }
                    break;
                }
                default:
                    break;
                }
            } else if (ev.type == SDL_MOUSEWHEEL) {
                if (ev.wheel.y > 0) {
                    (void)viewer->previous_page();
                } else {
                    (void)viewer->next_page();
                }
                (void)viewer->render();
            } else if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    auto new_width = ev.window.data1;
                    auto new_height = ev.window.data2;
                    handle_error(viewer->handle_resize(new_width, new_height));
                    (void)viewer->render();
                }
            }
        }

        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
