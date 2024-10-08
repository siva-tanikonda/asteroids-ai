#include "game.h"

class AI;

const int C_LENGTH = 34;

template <class T> void renderWrap(Renderer *renderer, AI *ai, const Vector &position, double radius, const T *object, void (T::*func)(Renderer*, AI*, Vector) const, bool offset_x = true, bool offset_y = true);

class AIShip {
    public:
        Vector position, velocity;
        double angle, width, bullet_cooldown, bullet_speed, bullet_life, drag_coefficient, rotation_speed, acceleration, size, target_safety_radius, flee_values[4], nudge_values[3];
        int lives;
        AIShip(const AIShipData &ship, double target_safety_radius);
        void render(Renderer *renderer, AI *ai, Vector offset) const;
    private:
        void renderArrowMetric(Renderer *renderer, double metric, double angle, const Vector &p, AI *ai, uint8_t r, uint8_t g, uint8_t b, uint8_t a) const;
};

class AIDanger {
    public:
        Vector position, velocity;
        double size;
        vector<double> danger_levels;
        bool entered_x, entered_y;
        AIDanger(const AIDangerData &danger, int size_index, vector<double> danger_levels);
        void render(Renderer *renderer, AI *ai, Vector offset) const;
};

class AITarget {
    public:
        Vector position, velocity;
        int size_index, id;
        double size, pessimistic_size, invincibility;
        bool entered_x, entered_y;
        AITarget(const AIDangerData &target, int size_index, double size);
        void render(Renderer *renderer, AI *ai, Vector offset) const;
};

class AIMarker {
    public:
        Vector position;
        int id, size_index;
        double life;
        AIMarker(AITarget &target, double life);
        void render(Renderer *renderer, AI *ai, Vector offset) const;
};

class AICrosshair {
    public:
        Vector position;
        int id;
        double angle, life;
        AICrosshair(int id, double angle, double ship_rotation_speed, const Vector &position);
        void render(Renderer *renderer, AI *ai, Vector offset) const;
};

class AI {
    public:
        AI(double (&c)[C_LENGTH], AIShipData ship, int seed);
        ~AI();
        void update(double delay, const json &config, Game *game);
        void renderGame(Renderer *renderer, Game *game);
        void renderOverlay(Renderer *renderer) const;
        void applyControls(EventManager *event_manager) const;
        double getFleeTime() const;
        int getMisses() const;
        int getFires() const;
        static const double DANGER_RADIUS[];
        static const double PESSIMISTIC_RADIUS[], FLOATING_POINT_COMPENSATION, RANDOM_WALK_ROTATION_PROBABILITY, RANDOM_WALK_SPEED_LIMIT;
        static const int ROTATION_PRECISION;
    private:
        int size_groups[2], misses, fires;
        bool controls_left, controls_right, controls_forward, controls_fire;
        double (&c)[C_LENGTH], max_danger, flee_values[4], nudge_values[3], flee_time;
        vector<AIDanger> dangers;
        vector<AITarget> targets;
        vector<AIMarker*> markers;
        AICrosshair *crosshair;
        AIShip ship;
        mt19937 gen;
        vector<double> calculateDangerLevels(const AIDangerData &danger);
        void generateVirtualEntities(Game *game);
        void calculateFleeAndNudgeValues();
        void manageFleeing();
        double calculateCirclePointCollisionTime(const Vector &p1, const Vector &v1, double r1, const Vector &p2, const Vector &v2) const;
        pair<double, Vector> calculateBulletCollisionTime(const AITarget &target, bool pessimistic_size = false, bool aiming = false) const;
        bool predictCollateralDamage(const AITarget &target, double collision_time) const;
        bool isTargetMarked(int id) const;
        bool predictClutterViolation(const AITarget &target) const;
        tuple<AITarget*, double, Vector> generateFiringOpportunity(bool aiming = false);
        void manageFiring(double delay, const json &config);
        void updateMarkers(double delay);
        void predictEntityStates(double delay);
        AICrosshair* generateAimTarget(double delay, const json &config, Game *game);
        void manageAim(double delay, const json &config, Game *game);
        void resetControls();
};
