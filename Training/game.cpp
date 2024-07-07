#include "game.h"

int Game::width = 1000;
int Game::height = 1000;
const int ObjectId::MAX_ID = 1e9;

template <class T> void renderWrap(SDL_Renderer *renderer, const Vector &position, double radius, const T *object, void (T::*func)(SDL_Renderer*, Vector) const, bool offset_x, bool offset_y) {
    vector<int> horizontal = { 0 };
    vector<int> vertical = { 0 };
    if (position.x + radius >= Game::getWidth() && offset_x) {
        horizontal.push_back(-Game::getWidth());
    }
    if (position.x - radius <= 0 && offset_x) {
        horizontal.push_back(Game::getWidth());
    }
    if (position.y + radius >= Game::getHeight() && offset_y) {
        vertical.push_back(-Game::getHeight());
    }
    if (position.y - radius <= 0 && offset_y) {
        vertical.push_back(Game::getHeight());
    }
    for (int x : horizontal) {
        for (int y : vertical) {
            Vector offset(x, y);
            (object->*func)(renderer, offset);
        }
    }
}

void renderFilledPolygon(SDL_Renderer *renderer, Polygon shape, const Vector &offset, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    shape.translate(offset);
    Rect rect = shape.getRect();
    rect.top = ceil(rect.top);
    rect.bottom = floor(rect.bottom);
    for (int y = rect.top; y <= rect.bottom; y++) {
        vector<double> intersections;
        for (int i = 0; i < shape.points.size(); i++) {
            Vector p1 = shape.points[i];
            Vector p2 = shape.points[(i + 1) % shape.points.size()];
            if (p1.x == p2.x) {
                if (y >= min(p1.y, p2.y) && y <= max(p1.y, p2.y)) {
                    intersections.push_back(p1.x);
                }
            } else if (p1.y != p2.y) {
                double m = (p2.y - p1.y) / (p2.x - p1.x);
                double b = p1.y - m * p1.x;
                double intersect_x = (y - b) / m;
                if (intersect_x >= min(p1.x, p2.x) && intersect_x <= max(p1.x, p2.x)) {
                    intersections.push_back(intersect_x);
                }
            }
        }
        if (intersections.empty()) {
            continue;
        }
        sort(intersections.begin(), intersections.end());
        double fx = intersections[0];
        bool fill = true;
        for (int i = 1; i < intersections.size(); i++) {
            if (intersections[i] == intersections[i - 1]) {
                continue;
            }
            if (fill) {
                lineRGBA(renderer, fx, y, intersections[i], y, r, g, b, a);
            }
            fill = !fill;
            fx = intersections[i];
        }
    }
}

void renderArrow(SDL_Renderer *renderer, const Vector &u, const Vector &v, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    Vector diff = v - u;
    double angle = diff.angle();
    double len = diff.mag();
    Vector p1(u.x + len, u.y);
    Vector p2(u.x + len - 5, u.y - 5);
    Vector p3(u.x + len - 5, u.y + 5);
    p1.rotate(-angle, u);
    p2.rotate(-angle, u);
    p3.rotate(-angle, u);
    lineRGBA(renderer, u.x, u.y, p1.x, p1.y, r, g, b, a);
    lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, r, g, b, a);
    lineRGBA(renderer, p1.x, p1.y, p3.x, p3.y, r, g, b, a);
}

void renderText(SDL_Renderer *renderer, TTF_Font *font, const string &text, int x, int y, TextRenderType type, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), { r, g, b, a });
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    int text_width, text_height;
    SDL_QueryTexture(texture, NULL, NULL, &text_width, &text_height);
    SDL_FreeSurface(surface);
    SDL_Rect rect;
    switch(type) {
        case TEXT_CENTERED:
            rect = { x - text_width / 2, y - text_height / 2, text_width, text_height };
            break;
        case TEXT_LEFT:
            rect = { x, y, text_width, text_height };
            break;
        case TEXT_RIGHT:
            rect = { x - text_width, y, text_width, text_height };
            break;
    }
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
}

string trimDouble(double num) {
    int num_int = num * 100;
    string num_int_str = to_string(num_int);
    while (num_int_str.length() < 2) {
        num_int_str.insert(num_int_str.begin(), '0');
    }
    num_int_str.insert(num_int_str.begin() + num_int_str.length() - 2, '.');
    return num_int_str;
}

ObjectId::ObjectId() : id(0) { }

int ObjectId::get(ObjectWithId *obj) {
    if (obj->id == -1) {
        obj->id = this->id++;
        this->id %= ObjectId::MAX_ID;
    }
    return obj->id;
}

ObjectWithId::ObjectWithId() : id(-1) { }

Bullet::Bullet(const Json::Value &config, Vector position, Vector velocity, double life) : position(position), velocity(velocity), life(life), dead(false) { }

void Bullet::update(double delay) {
    this->position += this->velocity * delay;
    wrap(this->position, Game::getWidth(), Game::getHeight());
    this->life -= delay;
    this->dead |= (this->life <= 0);
}

void Bullet::renderBullet(SDL_Renderer *renderer, Vector offset) const {
    Vector new_position = this->position + offset;
    filledCircleRGBA(renderer, new_position.x, new_position.y, 1, 255, 255, 255, 255);
}

void Bullet::render(SDL_Renderer *renderer) const {
    renderWrap<Bullet>(renderer, this->position, 1, this, &(this->renderBullet));
}

bool Bullet::checkAsteroidCollision(const Json::Value &config, vector<Asteroid*> *split_asteroids, int wave, Asteroid *asteroid) {
    if (asteroid->invincibility > 0 || asteroid->dead || this->dead) {
        return false;
    }
    int horizontal[] = { 0, Game::getWidth(), -Game::getWidth() };
    int vertical[] = { 0, Game::getHeight(), -Game::getHeight() };
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vector offset(horizontal[i], vertical[j]);
            bool hit = asteroid->bounds.containsPoint(this->position + offset);
            if (hit) {
                this->dead = asteroid->dead = true;
                asteroid->destroy(config, split_asteroids, wave);
                return true;
            }
        }
    }
    return false;
}

bool Bullet::checkSaucerCollision(Saucer *saucer) {
    if (saucer->dead || this->dead) {
        return false;
    }
    int horizontal[] = { 0, Game::getWidth(), -Game::getWidth() };
    int vertical[] = { 0, Game::getHeight(), -Game::getHeight() };
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vector offset(horizontal[i], vertical[j]);
            bool hit = saucer->bounds.containsPoint(this->position + offset);
            if (hit) {
                this->dead = saucer->dead = true;
                return true;
            }
        }
    }
    return false;
}

void Asteroid::analyzeAsteroidConfigurations(Json::Value &config) {
    for (int i = 0; i < config["asteroid_shapes"].size(); i++) {
        Polygon bounds({});
        for (int j = 0; j < config["asteroid_shapes"][i].size(); j++) {
            bounds.points.emplace_back(config["asteroid_shapes"][i][j][0].asDouble(), config["asteroid_shapes"][i][j][1].asDouble());
        }
        Rect rect = bounds.getRect();
        Vector offset(-rect.left, -rect.top);
        bounds.translate(offset);
        for (int j = 0; j < config["asteroid_shapes"][i].size(); j++) {
            config["asteroid_shapes"][i][j][0] = bounds.points[j].x;
            config["asteroid_shapes"][i][j][1] = bounds.points[j].y;
        }
    }
}

Asteroid::Asteroid(const Json::Value &config, Vector position, int size, int wave, mt19937 &gen) : ObjectWithId(), position(position), size(size), bounds(vector<Vector>()), gen(gen()) {
    int max_size = config["asteroid_sizes"].size() - 1;
    int type = floor(randomInRange(this->gen, 0, config["asteroid_shapes"].size()));
    if (size == max_size) {
        this->invincibility = config["asteroid_invincibility_time"].asDouble();
    } else {
        this->invincibility = 0;
    }
    for (int i = 0; i < config["asteroid_shapes"][type].size(); i++) {
        this->bounds.points.emplace_back(config["asteroid_shapes"][type][i][0].asDouble(), config["asteroid_shapes"][type][i][1].asDouble());
    }
    this->bounds.scale(config["asteroid_sizes"][size].asDouble());
    Rect rect = this->bounds.getRect();
    Vector offset(-rect.width / 2, -rect.height / 2);
    this->bounds.translate(offset);
    this->bounds.translate(position);
    this->angle = randomInRange(this->gen, 0, M_PI * 2);
    this->bounds.rotate(this->angle, this->position);
    this->rotation_speed = randomInRange(this->gen, config["asteroid_rotation_speed_range"][0].asDouble(), config["asteroid_rotation_speed_range"][1].asDouble());
    if (floor(randomDouble(this->gen) * 2) == 1) {
        this->rotation_speed *= -1;
    }
    double velocity_angle = randomDouble(this->gen) * M_PI * 2;
    this->velocity.x = cos(velocity_angle);
    this->velocity.y = sin(velocity_angle);
    double speed = randomInRange(this->gen, Asteroid::generateAsteroidSpeed(wave - 1), Asteroid::generateAsteroidSpeed(wave));
    this->velocity *= config["asteroid_size_speed_scaling"][size].asDouble() * speed;
    this->dead = false;
}

void Asteroid::rotate(double delay) {
    double old_angle = this->angle;
    this->angle += this->rotation_speed * delay;
    this->bounds.rotate(this->angle - old_angle, this->position);
    while (this->angle < 0) {
        this->angle += 2 * M_PI;
    }
    while (this->angle >= 2 * M_PI) {
        this->angle -= 2 * M_PI;
    }
}

void Asteroid::move(double delay) {
    Vector old_position = this->position;
    this->position += this->velocity * delay;
    wrap(this->position, Game::getWidth(), Game::getHeight());
    this->bounds.translate(this->position - old_position);
}

void Asteroid::update(double delay) {
    this->rotate(delay);
    this->move(delay);
    if (this->invincibility > 0) {
        this->invincibility -= delay;
    }
}

void Asteroid::renderAsteroid(SDL_Renderer *renderer, Vector offset) const {
    double alpha = 255;
    if (this->invincibility > 0) {
        alpha *= 0.25;
    } else {
        renderFilledPolygon(renderer, this->bounds, offset, 200, 100, 100, 255 * 0.35);
    }
    for (int i = 0; i < this->bounds.points.size(); i++) {
        Vector p1 = this->bounds.points[i] + offset;
        Vector p2 = this->bounds.points[(i + 1) % this->bounds.points.size()] + offset;
        lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 255, 255, 255, alpha);
    }
    Vector vec = Vector::normalize(this->velocity) * this->velocity.mag() * 10 + this->position + offset;
    renderArrow(renderer, this->position + offset, vec, 250, 250, 100, 255);
    filledCircleRGBA(renderer, this->position.x + offset.x, this->position.y + offset.y, 1, 125, 250, 125, 255);
}

void Asteroid::render(SDL_Renderer *renderer) const {
    Rect rect = this->bounds.getRect();
    renderWrap<Asteroid>(renderer, this->position, max(rect.width, rect.height) / 2, this, &(this->renderAsteroid));
}

void Asteroid::destroy(const Json::Value &config, vector<Asteroid*> *split_asteroids, int wave) {
    this->dead = true;
    if (this->size == 0) {
        return;
    }
    split_asteroids->emplace_back(new Asteroid(config, this->position, this->size - 1, wave, this->gen));
    split_asteroids->emplace_back(new Asteroid(config, this->position, this->size - 1, wave, this->gen));
}

double Asteroid::generateAsteroidSpeed(int wave) {
    return max(1.0, 1 + 0.1 * log2(wave));
}

void Saucer::analyzeSaucerConfigurations(Json::Value &config) {
    Polygon bounds({});
    for (int i = 0; i < config["saucer_shape"].size(); i++) {
        bounds.points.emplace_back(config["saucer_shape"][i][0].asDouble(), config["saucer_shape"][i][1].asDouble());
    }
    Rect rect = bounds.getRect();
    Vector shift(-rect.left, -rect.top);
    bounds.translate(shift);
    for (int i = 0; i < config["saucer_shape"].size(); i++) {
        config["saucer_shape"][i][0] = bounds.points[i].x;
        config["saucer_shape"][i][1] = bounds.points[i].y;
    }
}

Saucer::Saucer(const Json::Value &config, int size, int wave, mt19937 &gen) : ObjectWithId(), size(size), bounds(vector<Vector>()), gen(gen()) {
    for (int i = 0; i < config["saucer_shape"].size(); i++) {
        this->bounds.points.emplace_back(config["saucer_shape"][i][0].asDouble(), config["saucer_shape"][i][1].asDouble());
    }
    this->bounds.scale(config["saucer_sizes"][size].asDouble());
    Rect rect = this->bounds.getRect();
    Vector offset(-rect.width / 2, -rect.height / 2);
    this->bounds.translate(offset);
    double py = randomInRange(this->gen, rect.height / 2, Game::getHeight() - rect.height / 2);
    double px;
    if (floor(randomInRange(this->gen, 0, 2)) == 0) {
        px = -rect.width / 2;
    } else {
        px = Game::getWidth() + rect.width / 2;
    }
    this->position = Vector(px, py);
    this->bounds.translate(this->position);
    this->velocity = Vector(randomInRange(this->gen, Saucer::generateSaucerSpeed(max(1, wave - 1)), Saucer::generateSaucerSpeed(wave)), 0);
    if (this->position.x > Game::getWidth()) {
        this->velocity *= -1;
    }
    this->direction_change_rate = Saucer::generateDirectionChangeRate(wave);
    this->direction_change_cooldown = 1;
    this->vertical_movement = 1;
    if (floor(randomInRange(this->gen, 0, 2)) == 0) {
        this->vertical_movement = -1;
    }
    this->entered_x = this->entered_y = false;
    this->bullet_life = config["saucer_bullet_life"].asDouble();
    this->fire_rate = randomInRange(this->gen, Saucer::generateFireRate(max(1, wave - 1)), Saucer::generateFireRate(wave));
    this->bullet_cooldown = 0;
    this->bullet_speed = randomInRange(this->gen, Saucer::generateBulletSpeed(max(1, wave - 1)), Saucer::generateBulletSpeed(wave));
    this->dead = false;
}

void Saucer::move(double delay) {
    if (this->direction_change_cooldown <= 0) {
        if (floor(randomInRange(this->gen, 0, 2)) == 0) {
            double direction = this->velocity.x / abs(this->velocity.x);
            if (this->velocity.y == 0) {
                Vector new_velocity(direction, this->vertical_movement);
                new_velocity.normalize();
                new_velocity *= this->velocity.mag();
                this->velocity = new_velocity;
            } else {
                direction = this->velocity.x / abs(this->velocity.x);
                this->velocity = Vector(this->velocity.mag() * direction, 0);
            }
        }
        this->direction_change_cooldown = 1;
    }
    this->direction_change_cooldown = max(0.0, this->direction_change_cooldown - this->direction_change_rate * delay);
    Vector old_position = this->position;
    this->position += this->velocity * delay;
    wrap(this->position, Game::getWidth(), Game::getHeight(), this->entered_x, this->entered_y);
    this->bounds.translate(this->position - old_position);
    Rect rect = this->bounds.getRect();
    this->entered_x |= (this->position.x <= Game::getWidth() - rect.width / 2 && this->position.x >= rect.width / 2);
    this->entered_y |= (this->position.y >= rect.height / 2 && this->position.y <= Game::getHeight() - rect.height / 2);
}

Vector Saucer::bestFireDirection(const Ship &ship) const {
    int horizontal[] = { 0, Game::getWidth(), -Game::getWidth() };
    int vertical[] = { 0, Game::getHeight(), -Game::getHeight() };
    Vector best = ship.position - this->position;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == 0 && j == 0) {
                continue;
            }
            Vector shifted_position = this->position + Vector(horizontal[i], vertical[i]);
            Vector choice = ship.position - shifted_position;
            if (choice.mag() < best.mag()) {
                best = choice;
            }
        }
    }
    return best;
}

void Saucer::fire(double delay, const Json::Value &config, const Ship &ship, vector<Bullet*> *saucer_bullets) {
    if (this->bullet_cooldown >= 1) {
        Vector bullet_velocity = this->bestFireDirection(ship);
        bullet_velocity.normalize();
        bullet_velocity *= this->bullet_speed;
        Vector bullet_position = this->position;
        bullet_position += bullet_velocity;
        Bullet *bullet = new Bullet(config, bullet_position, bullet_velocity, this->bullet_life);
        saucer_bullets->push_back(bullet);
        this->bullet_cooldown = 0;
    }
    this->bullet_cooldown = min(1.0, this->bullet_cooldown + this->fire_rate * delay);
}

void Saucer::update(double delay, const Json::Value &config, const Ship &ship, vector<Bullet*> *saucer_bullets) {
    this->move(delay);
    this->fire(delay, config, ship, saucer_bullets);
}

void Saucer::renderSaucer(SDL_Renderer *renderer, Vector offset) const {
    renderFilledPolygon(renderer, this->bounds, offset, 200, 100, 100, 255 * 0.35);
    vector<Vector> points = this->bounds.points;
    for (int i = 0; i < points.size(); i++) {
        Vector p1 = points[i] + offset;
        Vector p2 = points[(i + 1) % points.size()] + offset;
        lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 255, 255, 255, 255);
    }
    Vector p1 = points[1] + offset;
    Vector p2 = points[2] + offset;
    Vector pm2 = points[points.size() - 2] + offset;
    Vector pm3 = points[points.size() - 3] + offset;
    lineRGBA(renderer, p1.x, p1.y, pm2.x, pm2.y, 255, 255, 255, 255);
    lineRGBA(renderer, p2.x, p2.y, pm3.x, pm3.y, 255, 255, 255, 255);
    Vector vec = Vector::normalize(this->velocity) * this->velocity.mag() * 10 + this->position + offset;
    renderArrow(renderer, this->position + offset, vec, 250, 250, 100, 255);
    filledCircleRGBA(renderer, this->position.x + offset.x, this->position.y + offset.y, 1, 125, 250, 125, 255);
}

void Saucer::render(SDL_Renderer *renderer) const {
    Rect rect = this->bounds.getRect();
    renderWrap(renderer, this->position, max(rect.width, rect.height) / 2, this, &(this->renderSaucer), this->entered_x, this->entered_y);
}

double Saucer::generateSaucerSpeed(int wave) {
    return min(5.0, 3 + (double)wave / 5);
}

double Saucer::generateDirectionChangeRate(int wave) {
    return min(1e-2, 1 - 1e-2 / wave);
}

double Saucer::generateFireRate(int wave) {
    return min(0.02, (double)wave / 10 * 0.02);
}

double Saucer::generateBulletSpeed(int wave) {
    return min(6.0, 4 + (double)wave / 10 * 4);
}

Ship::Ship(const Json::Value &config) : position(), velocity(), bounds(vector<Vector>()) {
    this->position.x = Game::getWidth() / 2;
    this->position.y = Game::getHeight() / 2;
    this->width = config["ship_width"].asDouble();
    this->height = config["ship_height"].asDouble();
    this->bounds.points.emplace_back(-this->width / 2, -this->height / 2);
    this->bounds.points.emplace_back(-this->width / 2, this->height / 2);
    this->bounds.points.emplace_back(this->width / 2, 0);
    this->angle = M_PI / 2;
    Vector offset;
    this->bounds.rotate(this->angle, offset);
    this->rotation_speed = config["ship_rotation_speed"].asDouble();
    this->acceleration = config["ship_acceleration"].asDouble();
    this->drag_coefficient = config["ship_drag_coefficient"].asDouble();
    this->bullet_cooldown = 1;
    this->fire_rate = config["ship_fire_rate"].asDouble();
    this->bullet_speed = config["ship_bullet_speed"].asDouble();
    this->bullet_life = config["ship_bullet_life"].asDouble();
    this->thruster_status = 0;
    this->bounds.translate(this->position);
    this->lives = config["game_lives"].asInt();
    this->dead = false;
    this->invincibility = 0;
    this->invincibility_time = 100;
    this->invincibility_flash = 0;
    this->accelerating = false;
}

void Ship::reviveShip() {
    this->lives--;
    if (this->lives > 0) {
        this->invincibility = this->invincibility_time;
        this->dead = false;
        Vector old_position = this->position;
        this->position.x = Game::getWidth() / 2;
        this->position.y = Game::getHeight() / 2;
        this->bounds.translate(this->position - old_position);
        double old_angle = this->angle;
        this->angle = M_PI / 2;
        this->bounds.rotate(this->angle - old_angle, this->position);
        this->velocity.x = 0;
        this->velocity.y = 0;
        this->thruster_status = 0;
        this->bullet_cooldown = 1;
    }
}

void Ship::rotate(double delay) {
    double old_angle = this->angle;
    if (EventManager::left) {
        this->angle += delay * this->rotation_speed;
    }
    if (EventManager::right) {
        this->angle -= delay * this->rotation_speed;
    }
    this->bounds.rotate(this->angle - old_angle, this->position);
    while (this->angle >= M_PI * 2) {
        this->angle -= M_PI * 2;
    }
    while (this->angle < 0) {
        this->angle += M_PI * 2;
    }
}

void Ship::move(double delay) {
    Vector direction(cos(this->angle), -sin(this->angle));
    if (EventManager::forward) {
        direction *= this->acceleration;
        this->velocity += direction * delay;
        this->thruster_status += delay * 0.05;
        while (this->thruster_status >= 1) {
            this->thruster_status--;
        }
        this->accelerating = true;
    } else {
        this->thruster_status = 0;
        this->accelerating = false;
    }
    Vector initial_velocity = this->velocity;
    this->velocity *= 1 / exp(delay * this->drag_coefficient);
    this->position = (this->position * this->drag_coefficient + initial_velocity - this->velocity) / this->drag_coefficient;
}

void Ship::fire(double delay, const Json::Value &config, vector<Bullet*> *ship_bullets) {
    if (EventManager::fire && this->bullet_cooldown >= 1) {
        Vector direction(cos(this->angle), -sin(this->angle));
        direction *= this->width / 2 + 5;
        Vector bullet_position = direction + this->position;
        direction.normalize();
        Vector bullet_velocity = direction * this->bullet_speed;
        bullet_velocity += this->velocity;
        Bullet *bullet = new Bullet(config, bullet_position, bullet_velocity, this->bullet_life);
        ship_bullets->push_back(bullet);
        this->bullet_cooldown = 0;
    }
    this->bullet_cooldown = min(1.0, this->bullet_cooldown + this->fire_rate * delay);
}

void Ship::updateInvincibility(double delay) {
    if (this->invincibility > 0) {
        this->invincibility_flash += 0.1 * delay;
        while (this->invincibility_flash >= 1) {
            this->invincibility_flash--;
        }
    }
    this->invincibility = max(0.0, this->invincibility - delay);
}

void Ship::update(double delay, const Json::Value &config, vector<Bullet*> *ship_bullets) {
    if (this->dead && this->lives > 0) {
        this->reviveShip();
    }
    if (this->dead) {
        return;
    }
    this->rotate(delay);
    Vector old_position = this->position;
    this->move(delay);
    wrap(this->position, Game::getWidth(), Game::getHeight());
    this->bounds.translate(this->position - old_position);
    this->fire(delay, config, ship_bullets);
    this->updateInvincibility(delay);
}

void Ship::renderShip(SDL_Renderer *renderer, Vector offset) const {
    if (this->invincibility > 0 && this->invincibility_flash < 0.5) {
        return;
    }
    renderFilledPolygon(renderer, this->bounds, offset, 200, 100, 100, 255 * 0.35);
    Vector tp = this->position + offset;
    Vector p1(tp.x - this->width / 2, tp.y - this->height / 2);
    Vector p2(tp.x + this->width / 2, tp.y);
    Vector p3(tp.x - this->width / 2, tp.y + this->height / 2);
    Vector p4(tp.x - this->width / 2 + 6, tp.y - this->height / 2 + (this->height / this->width) * 6 - 1);
    Vector p5(tp.x - this->width / 2 + 6, tp.y + this->height / 2 - (this->height / this->width) * 6 + 1);
    p1.rotate(this->angle, tp);
    p2.rotate(this->angle, tp);
    p3.rotate(this->angle, tp);
    p4.rotate(this->angle, tp);
    p5.rotate(this->angle, tp);
    lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 255, 255, 255, 255);
    lineRGBA(renderer, p3.x, p3.y, p2.x, p2.y, 255, 255, 255, 255);
    lineRGBA(renderer, p4.x, p4.y, p5.x, p5.y, 255, 255, 255, 255);
    if (this->thruster_status >= 0.5) {
        Vector p6(tp.x - this->width / 2 + 6 - 8, tp.y);
        p6.rotate(this->angle, tp);
        lineRGBA(renderer, p4.x, p4.y, p6.x, p6.y, 255, 255, 255, 255);
        lineRGBA(renderer, p6.x, p6.y, p5.x, p5.y, 255, 255, 255, 255);
    }
    Vector vec = Vector::normalize(this->velocity) * this->velocity.mag() * 10 + this->position + offset;
    renderArrow(renderer, this->position + offset, vec, 250, 250, 100, 255);
    if (this->accelerating) {
        vec = Vector(cos(this->angle), -sin(this->angle)) * this->acceleration * 250 + this->position + offset;
        renderArrow(renderer, this->position + offset, vec, 167, 184, 252, 255);
    }
    filledCircleRGBA(renderer, tp.x, tp.y, 1, 125, 250, 125, 255);
}

void Ship::renderLife(SDL_Renderer *renderer, Vector position) const {
    Vector p1(position.x - this->width / 2, position.y - this->height / 2);
    Vector p2(position.x + this->width / 2, position.y);
    Vector p3(position.x - this->width / 2, position.y + this->height / 2);
    Vector p4(position.x - this->width / 2 + 6, position.y - this->height / 2 + (this->height / this->width) * 6 - 1);
    Vector p5(position.x - this->width / 2 + 6, position.y + this->height / 2 - (this->height / this->width) * 6 + 1);
    p1.rotate(M_PI / 2, position);
    p2.rotate(M_PI / 2, position);
    p3.rotate(M_PI / 2, position);
    p4.rotate(M_PI / 2, position);
    p5.rotate(M_PI / 2, position);
    p1 *= 0.75;
    p2 *= 0.75;
    p3 *= 0.75;
    p4 *= 0.75;
    p5 *= 0.75;
    lineRGBA(renderer, p1.x, p1.y, p2.x, p2.y, 255, 255, 255, 255);
    lineRGBA(renderer, p3.x, p3.y, p2.x, p2.y, 255, 255, 255, 255);
    lineRGBA(renderer, p4.x, p4.y, p5.x, p5.y, 255, 255, 255, 255);
}

void Ship::render(SDL_Renderer *renderer) const {
    if (this->dead) {
        return;
    }
    renderWrap(renderer, this->position, this->width / 2, this, &(this->renderShip));
}

void Ship::renderLives(SDL_Renderer *renderer) const {
    Vector base_position(29, 70);
    for (int i = 0; i < this->lives; i++) {
        Vector offset(this->height * 2 * i, 0);
        this->renderLife(renderer, base_position + offset);
    }
}

bool Ship::checkBulletCollision(Bullet *bullet) {
    if (bullet->dead || this->dead || this->invincibility > 0) {
        return false;
    }
    int horizontal[] = { 0, Game::getWidth(), -Game::getWidth() };
    int vertical[] = { 0, Game::getHeight(), -Game::getHeight() };
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vector offset(horizontal[i], vertical[j]);
            bool hit = this->bounds.containsPoint(bullet->position + offset);
            if (hit) {
                this->dead = bullet->dead = true;
                return true;
            }
        }
    }
    return false;
}

bool Ship::checkAsteroidCollision(const Json::Value &config, vector<Asteroid*> *split_asteroids, int wave, Asteroid *asteroid) {
    if (asteroid->invincibility > 0 || asteroid->dead || this->dead || this->invincibility > 0) {
        return false;
    }
    int horizontal[] = { 0, Game::getWidth(), -Game::getWidth() };
    int vertical[] = { 0, Game::getHeight(), -Game::getHeight() };
    Vector old_offset(0, 0);
    Polygon shifted_bounds = this->bounds;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vector offset(horizontal[i], vertical[j]);
            shifted_bounds.translate(offset - old_offset);
            old_offset = offset;
            bool hit = asteroid->bounds.intersectsPolygon(shifted_bounds);
            if (hit) {
                this->dead = asteroid->dead = true;
                asteroid->destroy(config, split_asteroids, wave);
            }
        }
    }
    return false;
}

bool Ship::checkSaucerCollision(Saucer *saucer) {
    if (saucer->dead || this->dead || this->invincibility > 0) {
        return false;
    }
    int horizontal[] = { 0, Game::getWidth(), -Game::getWidth() };
    int vertical[] = { 0, Game::getHeight(), -Game::getHeight() };
    Vector old_offset(0, 0);
    Polygon shifted_bounds = this->bounds;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if ((horizontal[i] != 0 && !saucer->entered_x) || (vertical[j] != 0 && !saucer->entered_y)) {
                continue;
            }
            Vector offset(horizontal[i], vertical[j]);
            shifted_bounds.translate(offset - old_offset);
            old_offset = offset;
            bool hit = saucer->bounds.intersectsPolygon(shifted_bounds);
            if (hit) {
                this->dead = saucer->dead = true;
                return true;
            }
        }
    }
    return false;
}

void Game::analyzeGameConfiguration(Json::Value &config) {
    Game::width = config["window_width"].asInt();
    Game::height = config["window_height"].asInt();
    Asteroid::analyzeAsteroidConfigurations(config);
    Saucer::analyzeSaucerConfigurations(config);
}

Game::Game(const Json::Value &config, int seed) : ship(config), wave(0), score(0), extra_lives(0), saucer_cooldown(0), gen1(seed), gen2(-seed), time(0) {
    this->font = TTF_OpenFont("font.ttf", 20);
    this->debug_font = TTF_OpenFont("font.ttf", 15);
    this->extra_life_point_value = config["game_extra_life_point_value"].asInt();
    this->asteroid_point_value = config["game_asteroid_point_value"].asInt();
    this->saucer_point_value = config["game_saucer_point_value"].asInt();
}

Game::~Game() {
    for (Asteroid *asteroid : this->asteroids) {
        delete asteroid;
    }
    for (Saucer *saucer : this->saucers) {
        delete saucer;
    }
    for (Bullet *bullet : this->ship_bullets) {
        delete bullet;
    }
    for (Bullet *bullet : this->saucer_bullets) {
        delete bullet;
    }
    TTF_CloseFont(this->font);
    TTF_CloseFont(this->debug_font);
}

void Game::makeAsteroids(const Json::Value &config) {
    int count = Game::generateAsteroidSpawnCount(this->wave);
    for (int i = 0; i < count; i++) {
        Vector position(randomInRange(this->gen1, 0, Game::getWidth()), randomInRange(this->gen1, 0, Game::getHeight()));
        this->asteroids.push_back(new Asteroid(config, position, 2, this->wave, this->gen1));
    }
}

void Game::makeSaucer(double delay, const Json::Value &config) {
    if (this->saucer_cooldown >= 1) {
        this->saucers.push_back(new Saucer(config, floor(randomInRange(this->gen2, 0, config["saucer_sizes"].size())), this->wave, this->gen2));
        this->saucer_cooldown = 0;
    }
    this->saucer_cooldown = min(1.0, this->saucer_cooldown + Game::generateSaucerSpawnRate(this->wave) * delay);
}

void Game::update(double delay, const Json::Value &config) {
    if (this->asteroids.empty()) {
        this->wave++;
        this->makeAsteroids(config);
    }
    if (this->saucers.empty()) {
        this->makeSaucer(delay, config);
    }
    if (this->score >= (1 + this->extra_lives) * this->extra_life_point_value && this->ship.lives != 0) {
        this->ship.lives++;
        this->extra_lives++;
    }
    this->ship.update(delay, config, &(this->ship_bullets));
    for (Bullet *bullet : this->ship_bullets) {
        bullet->update(delay);
    }
    for (Asteroid *asteroid : this->asteroids) {
        asteroid->update(delay);
    }
    for (Saucer *saucer : this->saucers) {
        saucer->update(delay, config, this->ship, &(this->saucer_bullets));
    }
    for (Bullet *bullet : this->saucer_bullets) {
        bullet->update(delay);
    }
    vector<Asteroid*> split_asteroids;
    for (Bullet *bullet : this->saucer_bullets) {
        this->ship.checkBulletCollision(bullet);
    }
    for (Saucer *saucer : this->saucers) {
        this->ship.checkSaucerCollision(saucer);
    }
    for (Asteroid *asteroid : this->asteroids) {
        this->ship.checkAsteroidCollision(config, &split_asteroids, this->wave, asteroid);
    }
    vector<Bullet*> new_ship_bullets;
    for (Bullet *bullet : this->ship_bullets) {
        for (Asteroid *asteroid : this->asteroids) {
            bool hit = bullet->checkAsteroidCollision(config, &split_asteroids, this->wave, asteroid);
            if (hit && this->ship.lives > 0) {
                this->score += this->asteroid_point_value;
            }
        }
        for (Saucer *saucer : this->saucers) {
            bool hit = bullet->checkSaucerCollision(saucer);
            if (hit && this->ship.lives > 0) {
                this->score += this->saucer_point_value;
            }
        }
        if (!bullet->dead) {
            new_ship_bullets.push_back(bullet);
        } else {
            delete bullet;
        }
    }
    this->ship_bullets = new_ship_bullets;
    vector<Saucer*> new_saucers;
    for (Saucer *saucer : this->saucers) {
        if (!saucer->dead) {
            new_saucers.push_back(saucer);
        } else {
            delete saucer;
        }
    }
    this->saucers = new_saucers;
    vector<Bullet*> new_saucer_bullets;
    for (Bullet *bullet : this->saucer_bullets) {
        if (!bullet->dead) {
            new_saucer_bullets.push_back(bullet);
        } else {
            delete bullet;
        }
    }
    this->saucer_bullets = new_saucer_bullets;
    for (Asteroid *asteroid : split_asteroids) {
        this->asteroids.push_back(asteroid);
    }
    vector<Asteroid*> new_asteroids;
    for (Asteroid *asteroid : this->asteroids) {
        if (!asteroid->dead) {
            new_asteroids.push_back(asteroid);
        } else {
            delete asteroid;
        }
    }
    this->asteroids = new_asteroids;
    this->time += delay / 60;
}

int Game::generateAsteroidSpawnCount(int wave) {
    return (wave * 2 + 2) * (double)(Game::getWidth() * Game::getHeight()) / 1e6;
}

double Game::generateSaucerSpawnRate(int wave) {
    return min(1.0, (double)wave / 2000);
}

void Game::renderGame(SDL_Renderer *renderer) const {
    this->ship.render(renderer);
    for (const Asteroid *asteroid : this->asteroids) {
        asteroid->render(renderer);
    }
    for (const Bullet *bullet : this->ship_bullets) {
        bullet->render(renderer);
    }
    for (const Saucer *saucer : this->saucers) {
        saucer->render(renderer);
    }
    for (const Bullet *bullet : this->saucer_bullets) {
        bullet->render(renderer);
    }
}

void Game::renderOverlay(SDL_Renderer *renderer, double fps) const {
    renderText(renderer, this->font, to_string(this->score), 15, 8, TEXT_LEFT, 255, 255, 255, 255);
    renderText(renderer, this->debug_font, "Wave: " + to_string(this->wave), Game::width - 10, 10, TEXT_RIGHT, 242, 86, 75, 255);
    renderText(renderer, this->debug_font, "Saucer Count: " + to_string(this->saucers.size()), Game::width - 10, 30, TEXT_RIGHT, 242, 86, 75, 255);
    int count[] = { 0, 0, 0 };
    for (const Asteroid *asteroid : this->asteroids) {
        count[asteroid->size]++;
    }
    renderText(renderer, this->debug_font, "Large Asteroid Count: " + to_string(count[2]), Game::width - 10, 50, TEXT_RIGHT, 242, 86, 75, 255);
    renderText(renderer, this->debug_font, "Medium Asteroid Count: " + to_string(count[1]), Game::width - 10, 70, TEXT_RIGHT, 242, 86, 75, 255);
    renderText(renderer, this->debug_font, "Small Asteroid Count: " + to_string(count[0]), Game::width - 10, 90, TEXT_RIGHT, 242, 86, 75, 255);
    renderText(renderer, this->debug_font, "FPS: " + trimDouble(fps), Game::width - 10, 110, TEXT_RIGHT, 242, 86, 75, 255);
    int time_int = this->time * 100;
    string time_str = to_string(time_int);
    while (time_str.length() < 2) {
        time_str.insert(time_str.begin(), '0');
    }
    time_str.insert(time_str.begin() + time_str.length() - 2, '.');
    renderText(renderer, this->debug_font, "Time Elapsed: " + time_str, Game::width - 10, 130, TEXT_RIGHT, 242, 86, 75, 255);
    this->ship.renderLives(renderer);
}

AIShipData Game::getAIShipData() const {
    return { this->ship.position, this->ship.velocity, this->ship.width, this->ship.acceleration, this->ship.bullet_cooldown, this->ship.bullet_speed, this->ship.bullet_life, this->ship.drag_coefficient, this->ship.angle, this->ship.rotation_speed, this->ship.lives };
}

vector<AIDangerData> Game::getAIAsteroidsData() {
    vector<AIDangerData> asteroids;
    for (Asteroid *asteroid : this->asteroids) {
        asteroids.push_back({ asteroid->position, asteroid->velocity, asteroid->size, this->object_id.get(asteroid), asteroid->invincibility, true, true });
    }
    return asteroids;
}

vector<AIDangerData> Game::getAISaucersData() {
    vector<AIDangerData> saucers;
    for (Saucer *saucer : this->saucers) {
        saucers.push_back({ saucer->position, saucer->velocity, saucer->size, this->object_id.get(saucer), 0, saucer->entered_x, saucer->entered_y });
    }
    return saucers;
}

vector<AIDangerData> Game::getAISaucerBulletsData() {
    vector<AIDangerData> saucer_bullets;
    for (Bullet *bullet : this->saucer_bullets) {
        saucer_bullets.push_back({ bullet->position, bullet->velocity, 0, -1, 0, true, true });
    }
    return saucer_bullets;
}

int Game::getWidth() {
    return Game::width;
}

int Game::getHeight() {
    return Game::height;
}
