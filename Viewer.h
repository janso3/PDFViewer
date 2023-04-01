#pragma once

#include <LibPDF/Document.h>
#include <LibPDF/Renderer.h>
#include <SDL2/SDL.h>

namespace PDF {

class Viewer {
public:
    static PDFErrorOr<Viewer*> create(SDL_Window*);
    ~Viewer();

    PDFErrorOr<void> load(String const& path);

    PDFErrorOr<void> handle_resize(int width, int height);
    PDFErrorOr<void> render();

    PDFErrorOr<void> next_page();
    PDFErrorOr<void> previous_page();

private:
    Viewer(SDL_Renderer*);

    PDFErrorOr<RefPtr<Gfx::Bitmap>> rasterize_page(u32 index);

    AK::ByteBuffer m_buffer;
    RefPtr<Document> m_document { nullptr };
    SDL_Texture* m_texture { nullptr };
    SDL_Renderer* m_renderer;
    u32 m_page_index = 0;
    Gfx::IntSize m_page_size;

    int m_width = 0;
    int m_height = 0;
};

}
