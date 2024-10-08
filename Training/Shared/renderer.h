#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <nlohmann/json.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "event_manager.h"

using json = nlohmann::json;

struct RenderQueue;

constexpr const int MAX_QUEUE_LENGTH = 100000;
constexpr const int MAX_TEXT_LENGTH = 200;
constexpr const char *RENDERER_SHARED_MEMORY_NAME = "/renderer_shared_memory";

enum RenderType { TEXT, FILLED_CIRCLE, CIRCLE, LINE, RECTANGLE, FILLED_RECTANGLE };
enum FontType { REGULAR, SMALL, TINY };
enum TextAlignment { LEFT, RIGHT, MIDDLE };
enum CursorType { POINTER, ARROW };

// The structure of a request from the renderer owner to the manager thread
struct RenderRequest {
    RenderType type;
    FontType font;
    TextAlignment alignment;
    char text[MAX_TEXT_LENGTH + 1];
    int x1, y1, x2, y2, radius;
    uint8_t r, g, b, a;
};

// The shared memory description for the render queue
struct RenderQueue {
    pthread_mutex_t lock;
    bool done_processing;
    int len, owner;
    RenderRequest queue[MAX_QUEUE_LENGTH];
};

class Renderer {
    public:
        Renderer(const json &config);
        ~Renderer();
        bool beginProcessing();
        void endProcessing(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        bool beginRequest(int process_num);
        void endRequest();
        void requestText(FontType font, const string &text, int x, int y, TextAlignment alignment, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void requestFilledCircle(int x1, int y1, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void requestCircle(int x1, int y1, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void requestLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void requestRectangle(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void requestFilledRectangle(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void setCursor(CursorType cursor);
        void setOwner(int process_num);
        int getOwner() const;
        void setManager();
    private:
        bool manager;
        SDL_Window *window;
        SDL_Renderer *renderer;
        TTF_Font *font, *small_font, *tiny_font;
        SDL_Cursor *pointer_cursor, *arrow_cursor;
        RenderQueue *queue;
        void renderText(const RenderRequest *request);
        void renderLine(const RenderRequest *request);
        void renderFilledCircle(const RenderRequest *request);
        void renderCircle(const RenderRequest *request);
        void renderRectangle(const RenderRequest *request);
        void renderFilledRectangle(const RenderRequest *request);
        void processRequest(const RenderRequest *request);
};
