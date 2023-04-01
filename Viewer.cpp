#include "Viewer.h"
#include <LibCore/File.h>

namespace PDF {

PDFErrorOr<Viewer*> Viewer::create(SDL_Window* window)
{
    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
        return Error(Error::Type::Internal, TRY(String::formatted("SDL_CreateRenderer failed: {}", SDL_GetError())));

    auto* viewer = new (nothrow) Viewer(renderer);
    if (!viewer)
        return Error(Error::Type::Internal, "Failed to allocate viewer");

    SDL_GetWindowSize(window, &viewer->m_width, &viewer->m_height);

    return viewer;
}

Viewer::Viewer(SDL_Renderer* renderer)
    : m_renderer { renderer }
{
}

Viewer::~Viewer()
{
    if (m_texture)
        SDL_DestroyTexture(m_texture);
    SDL_DestroyRenderer(m_renderer);
}

PDFErrorOr<void> Viewer::load(String const& path)
{
    if (!m_document.is_null()) {
        m_document.clear();
        m_buffer.clear();
        m_page_index = 0;
    }

    auto file = TRY(Core::File::open(path, Core::File::OpenMode::Read));
    m_buffer = TRY(file->read_until_eof());

    m_document = TRY(Document::create(m_buffer));

    if (auto sh = m_document->security_handler(); sh && !sh->has_user_password()) {
        StringBuilder password;
        TODO();
        m_document->security_handler()->try_provide_user_password(password.to_deprecated_string());
    }

    TRY(m_document->initialize());
    TRY(handle_resize(m_width, m_height));

    return {};
}

PDFErrorOr<void> Viewer::handle_resize(int width, int height)
{
    m_width = width;
    m_height = height;

    if (m_document) {
        auto page = TRY(m_document->get_page(m_page_index));

        double page_ratio = page.media_box.width() / (double)page.media_box.height();
        double window_ratio = width / (double)height;

        int page_height = height;
        int page_width = width * page_ratio / window_ratio;

        if (!page_height || !page_width)
            return {};

        if (m_texture)
            SDL_DestroyTexture(m_texture);
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, page_width, page_height);
        if (!m_texture)
            return Error(Error::Type::Internal, TRY(String::formatted("SDL_CreateTexture failed: {}", SDL_GetError())));

        m_page_size = { page_width, page_height };
    }

    return {};
}

PDFErrorOr<void> Viewer::render()
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);

    if (m_document) {
        auto page_bitmap = TRY(rasterize_page(m_page_index));

        int pitch = 0;
        u32* pixels = nullptr;

        SDL_LockTexture(m_texture, NULL, (void**)&pixels, &pitch);
        memcpy(pixels, page_bitmap->scanline_u8(0), pitch * page_bitmap->height());
        SDL_UnlockTexture(m_texture);

        SDL_Rect rect;
        rect.x = (m_width - m_page_size.width()) / 2;
        rect.y = 0;
        rect.w = m_page_size.width();
        rect.h = m_page_size.height();

        SDL_RenderCopy(m_renderer, m_texture, NULL, &rect);
    }

    SDL_RenderPresent(m_renderer);

    return {};
}

PDFErrorOr<void> Viewer::next_page()
{
    if (!m_document)
        return {};

    if (m_page_index < m_document->get_page_count() - 1)
        m_page_index++;
    return {};
}

PDFErrorOr<void> Viewer::previous_page()
{
    if (m_page_index)
        m_page_index--;
    return {};
}

PDFErrorOr<RefPtr<Gfx::Bitmap>> Viewer::rasterize_page(u32 index)
{
    auto page = TRY(m_document->get_page(index));
    auto bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, m_page_size));

    RenderingPreferences preferences;
    preferences.show_clipping_paths = false;
    preferences.show_images = true;

    auto errors = Renderer::render(*m_document, page, bitmap, preferences);
    if (errors.is_error()) {
        for (auto error : errors.error().errors())
            dbgln("{}", error.message());
    }

    return bitmap;
}

}
