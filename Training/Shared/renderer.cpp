#include "renderer.h"

Renderer::Renderer(const json &config) : manager(false) {
    // Initialize SDL2 video features (and fonts)
    SDL_Init(SDL_INIT_VIDEO);
    this->window = SDL_CreateWindow("Asteroids+ Trainer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config["window_width"], config["window_height"], SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    TTF_Init();
    this->font = TTF_OpenFont("font.ttf", 20);
    this->small_font = TTF_OpenFont("font.ttf", 15);
    this->tiny_font = TTF_OpenFont("font.ttf", 11);
    this->pointer_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    this->arrow_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    // Create the shared memory region and map it
    int queue_fd = shm_open(RENDERER_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(queue_fd, sizeof(RenderQueue));
    this->queue = static_cast<RenderQueue*>(mmap(0, sizeof(RenderQueue), PROT_READ | PROT_WRITE, MAP_SHARED, queue_fd, 0));
    close(queue_fd);
    // Create the lock for the rendering queue
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(this->queue->lock), &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
}

Renderer::~Renderer() {
    if (this->manager) {
        // Destroy all SDL2 components (only for the manager process to do)
        TTF_CloseFont(this->font);
        TTF_CloseFont(this->small_font);
        TTF_CloseFont(this->tiny_font);
        TTF_Quit();
        SDL_FreeCursor(this->pointer_cursor);
        SDL_FreeCursor(this->arrow_cursor);
        SDL_DestroyRenderer(this->renderer);
        SDL_DestroyWindow(this->window);
        SDL_Quit();
        // Destroy the locks (as all other processes have been terminated by now)
        pthread_mutex_destroy(&(this->queue->lock));
    }
    // Unmap the RenderQueue object from the current process's memory space
    munmap(this->queue, sizeof(RenderQueue));
    if (this->manager) {
        shm_unlink(RENDERER_SHARED_MEMORY_NAME);
    }
}

// Begins the processing of the rendering queue (if we cannot start the processing, we return false)
bool Renderer::beginProcessing() {
    bool succeeded = (this->manager && !(this->queue->done_processing) && pthread_mutex_trylock(&(this->queue->lock)) == 0);
    if (succeeded) {
        // Set the current cursor type
        SDL_SetCursor(this->arrow_cursor);
    }
    return succeeded;
}

// Process every request in the RenderQueue and end the processing
void Renderer::endProcessing(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // Set the background color
    SDL_SetRenderDrawColor(this->renderer, r, g, b, a);
    SDL_RenderClear(renderer);
    // Render each request in the queue (in the order that the requests entered the queue)
    for (int i = 0; i < this->queue->len; i++) {
        this->processRequest(&(this->queue->queue[i]));
    }
    SDL_RenderPresent(renderer);
    // "Clear" the queue and state that processing is complete
    this->queue->len = 0;
    this->queue->done_processing = true;
    pthread_mutex_unlock(&(this->queue->lock));
}

// Renders text
void Renderer::renderText(const RenderRequest *request) {
    // Picks the font
    TTF_Font *font;
    switch (request->font) {
        case REGULAR:
            font = this->font;
            break;
        case SMALL:
            font = this->small_font;
            break;
        case TINY:
            font = this->tiny_font;
            break;
    }
    // Creates the texture of the font + text
    SDL_Surface *surface = TTF_RenderText_Solid(font, request->text, { request->r, request->g, request->b, request->a });
    SDL_Texture *texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    SDL_FreeSurface(surface);
    int text_width, text_height;
    SDL_QueryTexture(texture, NULL, NULL, &text_width, &text_height);
    // Picks the alignment for the text
    SDL_Rect rect;
    switch(request->alignment) {
        case MIDDLE:
            rect = { request->x1 - text_width / 2, request->y1 - text_height / 2, text_width, text_height };
            break;
        case LEFT:
            rect = { request->x1, request->y1, text_width, text_height };
            break;
        case RIGHT:
            rect = { request->x1 - text_width, request->y1, text_width, text_height };
            break;
    }
    // Actually render the texture
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    // Destroy the texture (dynamically-allocated)
    SDL_DestroyTexture(texture);
}

// Renders a line (no anti-aliasing)
void Renderer::renderLine(const RenderRequest *request) {
    SDL_SetRenderDrawColor(this->renderer, request->r, request->g, request->b, request->a);
    SDL_RenderDrawLine(this->renderer, request->x1, request->y1, request->x2, request->y2);
}

// Renders a filled circle
void Renderer::renderFilledCircle(const RenderRequest *request) {
    SDL_SetRenderDrawColor(this->renderer, request->r, request->g, request->b, request->a);
    // Algorithm is just to iterate pixel-by-pixel through the y-axis and render a line to connect both x-values that generate our current y-value on the circle
    int top = ceil(request->y1 - request->radius);
    int bottom = floor(request->y1 + request->radius);
    for (int i = top; i <= bottom; i++) {
        double dy = i - request->y1;
        double diff = sqrt(request->radius * request->radius - dy * dy);
        SDL_RenderDrawLine(this->renderer, request->x1 - diff, i, request->x1 + diff, i);
    }
}

// Renders a standard circle
void Renderer::renderCircle(const RenderRequest *request) {
    SDL_SetRenderDrawColor(this->renderer, request->r, request->g, request->b, request->a);
    // Algorithm is just standard Midpoint Circle Algorithm (https://en.wikipedia.org/wiki/Midpoint_circle_algorithm)
    int x = request->radius - 1;
    int y = 0;
    int tx = 1;
    int ty = 1;
    int err = tx - request->radius * 2;
    while (x >= y) {
        SDL_RenderDrawPoint(this->renderer, request->x1 + x, request->y1 - y);
        SDL_RenderDrawPoint(this->renderer, request->x1 + x, request->y1 + y);
        SDL_RenderDrawPoint(this->renderer, request->x1 - x, request->y1 - y);
        SDL_RenderDrawPoint(this->renderer, request->x1 - x, request->y1 + y);
        SDL_RenderDrawPoint(this->renderer, request->x1 + y, request->y1 - x);
        SDL_RenderDrawPoint(this->renderer, request->x1 + y, request->y1 + x);
        SDL_RenderDrawPoint(this->renderer, request->x1 - y, request->y1 - x);
        SDL_RenderDrawPoint(this->renderer, request->x1 - y, request->y1 + x);
        if (err <= 0) {
            y++;
            err += ty;
            ty += 2;
        }
        if (err > 0) {
            x--;
            tx += 2;
            err += tx - request->radius * 2;
        }
    }
}

// Renders a rectangle
void Renderer::renderRectangle(const RenderRequest *request) {
    SDL_SetRenderDrawColor(this->renderer, request->r, request->g, request->b, request->a);
    SDL_Rect rect = { request->x1, request->y1, request->x2 - request->x1, request->y2 - request->y1 };
    SDL_RenderDrawRect(this->renderer, &rect);
}

// Renders a filled rectangle
void Renderer::renderFilledRectangle(const RenderRequest *request) {
    SDL_SetRenderDrawColor(this->renderer, request->r, request->g, request->b, request->a);
    SDL_Rect rect = { request->x1, request->y1, request->x2 - request->x1, request->y2 - request->y1 };
    SDL_RenderFillRect(this->renderer, &rect);
}

// Takes a request and processes it based on the type of object to render (ex. rectangle, text)
void Renderer::processRequest(const RenderRequest *request) {
    switch (request->type) {
        case TEXT:
            this->renderText(request);
            break;
        case FILLED_CIRCLE:
            this->renderFilledCircle(request);
            break;
        case CIRCLE:
            this->renderCircle(request);
            break;
        case LINE:
            this->renderLine(request);
            break;
        case RECTANGLE:
            this->renderRectangle(request);
            break;
        case FILLED_RECTANGLE:
            this->renderFilledRectangle(request);
            break;
    }
}

// For a non-manager thread: We attempt to begin a request (only one process at a time can "own" the requests on the queue)
bool Renderer::beginRequest(int process_num) {
    return (this->queue->owner == process_num && this->queue->done_processing && pthread_mutex_trylock(&(this->queue->lock)) == 0);
}

// Marks the queue of requests as "complete", thus allowing the manager process to process the queue
void Renderer::endRequest() {
    this->queue->done_processing = false;
    pthread_mutex_unlock(&(this->queue->lock));
}

// Creates a request for rendering text (for all these requeusts, we assume we aren't exceeding MAX_QUEUE_LENGTH and that the text length doesn't exceed MAX_TEXT_LENGTH, will error/have undefined behavior otherwise)
void Renderer::requestText(FontType font, const string &text, int x, int y, TextAlignment alignment, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int i = this->queue->len++;
    this->queue->queue[i].type = TEXT;
    this->queue->queue[i].font = font;
    this->queue->queue[i].alignment = alignment;
    strcpy(this->queue->queue[i].text, text.c_str());
    this->queue->queue[i].x1 = x;
    this->queue->queue[i].y1 = y;
    this->queue->queue[i].r = r;
    this->queue->queue[i].g = g;
    this->queue->queue[i].b = b;
    this->queue->queue[i].a = a;
}

// Creates a request for a filled circle
void Renderer::requestFilledCircle(int x1, int y1, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int i = this->queue->len++;
    this->queue->queue[i].type = FILLED_CIRCLE;
    this->queue->queue[i].x1 = x1;
    this->queue->queue[i].y1 = y1;
    this->queue->queue[i].radius = radius;
    this->queue->queue[i].r = r;
    this->queue->queue[i].g = g;
    this->queue->queue[i].b = b;
    this->queue->queue[i].a = a;
}

// Create a request for a standard circle
void Renderer::requestCircle(int x1, int y1, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int i = this->queue->len++;
    this->queue->queue[i].type = CIRCLE;
    this->queue->queue[i].x1 = x1;
    this->queue->queue[i].y1 = y1;
    this->queue->queue[i].radius = radius;
    this->queue->queue[i].r = r;
    this->queue->queue[i].g = g;
    this->queue->queue[i].b = b;
    this->queue->queue[i].a = a;
}

// Create a request for a line (no anti-aliasing)
void Renderer::requestLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int i = this->queue->len++;
    this->queue->queue[i].type = LINE;
    this->queue->queue[i].x1 = x1;
    this->queue->queue[i].y1 = y1;
    this->queue->queue[i].x2 = x2;
    this->queue->queue[i].y2 = y2;
    this->queue->queue[i].r = r;
    this->queue->queue[i].g = g;
    this->queue->queue[i].b = b;
    this->queue->queue[i].a = a;
}

// Create a request for a rectangle (not filled)
void Renderer::requestRectangle(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int i = this->queue->len++;
    this->queue->queue[i].type = RECTANGLE;
    this->queue->queue[i].x1 = x1;
    this->queue->queue[i].y1 = y1;
    this->queue->queue[i].x2 = x2;
    this->queue->queue[i].y2 = y2;
    this->queue->queue[i].r = r;
    this->queue->queue[i].g = g;
    this->queue->queue[i].b = b;
    this->queue->queue[i].a = a;
}

// Create a request for a rectangle (filled)
void Renderer::requestFilledRectangle(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int i = this->queue->len++;
    this->queue->queue[i].type = FILLED_RECTANGLE;
    this->queue->queue[i].x1 = x1;
    this->queue->queue[i].y1 = y1;
    this->queue->queue[i].x2 = x2;
    this->queue->queue[i].y2 = y2;
    this->queue->queue[i].r = r;
    this->queue->queue[i].g = g;
    this->queue->queue[i].b = b;
    this->queue->queue[i].a = a;
}

// Set the cursor type (currently only properly works for the manager process)
void Renderer::setCursor(CursorType cursor) {
    if (cursor == POINTER) {
        SDL_SetCursor(this->pointer_cursor);
    } else {
        SDL_SetCursor(this->arrow_cursor);
    }
}

// Sets the owner of the renderer (the process that can exclusively send requests)
void Renderer::setOwner(int process_num) {
    this->queue->owner = process_num;
}

// Gets the owner of the renderer (the process number of the process as defined in main.cpp, not the PID)
int Renderer::getOwner() const {
    return this->queue->owner;
}

// States that the current process is the manager
void Renderer::setManager() {
    this->manager = true;
}
