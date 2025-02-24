#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <cassert>

using namespace std;

class Vector {
public:
    Vector(int magnitude = 1, int direction = 0) :
        magnitude(magnitude),
        direction(normalise(direction)) {
    }
    int getMagnitude() const {
        return magnitude;
    }
    int getDirection() const {
        return direction;
    }
    int setDirection(int newDirection) {
        direction = normalise(newDirection);
        return direction;
    }
    float getRadians() const {
        return direction * numbers::pi / 180.0f;
    }
    int x() const {
        return round(magnitude * cos(getRadians()));
    }
    int y() const {
        return round(magnitude * sin(getRadians()));
    }
private:
    int magnitude;
    int direction;
    static int normalise(int direction) {
        int normalised = direction % 360;
        if (normalised < 0) normalised += 360;
        assert(normalised >=   0);
        assert(normalised <  360);
        return normalised;
    }
};

struct Point {
    Point(int x=0, int y=0) : x(x)  , y(y)   {}
    Point(Vector v) : x(v.x()), y(v.y()) {}
    int x;
    int y;
    int angle() const {
        int angle = atan2 (y,x) * 180 / numbers::pi;
        if (angle < 0) angle += 360; // Positive angles
        return angle;
    }
    int distance(Point p) const {
        p = p - *this;
        return round(sqrt(p.x*p.x + p.y*p.y));
    }
    int magnitude() const {
        return distance(Point(0,0));
    }
    Vector toVector() const {
        return Vector(magnitude(), angle());
    }
    string to_string() const {
        return std::to_string(x) + " "s + std::to_string(y);
    }
    friend Point operator+(Point lhs, const Point &rhs) {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
        return lhs;
    }
    friend Point operator-(Point lhs, const Point &rhs) {
        lhs.x -= rhs.x;
        lhs.y -= rhs.y;
        return lhs;
    }
    friend Point operator*(Point lhs, int rhs) {
        lhs.x *= rhs;
        lhs.y *= rhs;
        return lhs;
    }
    friend Point operator*(Point lhs, float rhs) {
        lhs.x = round(lhs.x * rhs);
        lhs.y = round(lhs.y * rhs);
        return lhs;
    }
};

struct Pod {
    Pod(int x=0, int y=0, int vx=0, int vy=0, int a=0, int id=0) :
        position(x, y), velocity(vx, vy), angle(a), nextCpId(id) {
    }
    Point position;
    Point velocity;
    static const int maxSpeed = 100;
    int angle;
    int nextCpId;
    int distance(Point p) const {
        return position.distance(p);
    }
    int speed() const {
        return velocity.magnitude();
    }
    int heading() const {
        return velocity.angle();
    }
    // Final resting point if not accelerating
    Point coastOffset() const {
        static const float coastFactor = 20.0f/3.0f; // sum of 0.85^n from 0 to infinity
        return velocity * coastFactor;
    }
    Point coastDest() const {
        return position + coastOffset();
    }
    Vector coastVect() const {
        return Vector(coastOffset().toVector());
    }
    int coastDist() const {
        return coastVect().getMagnitude();
    }
};

// Game context as initially provided
struct Game {
    static const unsigned int podCount = 2;
    unsigned int turn = 0;
    unsigned int laps;
    unsigned int checkpointCount;
    vector<Point> checkpoints;
    vector<Pod> player;
    vector<Pod> enemy;
    void init(istream &in) {
        in >> laps; in.ignore();
        in >> checkpointCount; in.ignore();
        for (int i = 0; i < checkpointCount; i++) {
            Point cp;
            in >> cp.x >> cp.y; in.ignore();
            checkpoints.push_back(cp);
        }
    }
    static Pod readPod(istream &in) {
        int x, y, vx, vy, angle, nextCpId;
        in >> x >> y >> vx >> vy >> angle >> nextCpId; in.ignore();
        return Pod(x, y, vx, vy, angle, nextCpId);
    }
    static vector<Pod> readPlayer(istream &in) {
        vector<Pod> pods;
        for (int i = 0; i < podCount; i++) {
            pods.push_back(readPod(in));
        }
        return pods;
    }
    void prepareNextTurn(istream &in) {
        player = readPlayer(in);
        enemy = readPlayer(in);
        turn++;
    }
    // getCP allows wraparound so the following is safe:
    // getCP(pod.nextCpId + 1)
    Point getCp(unsigned int index) const {
        return checkpoints[index % checkpointCount];
    }
    Point getCp(Pod pod) const {
        return getCp(pod.nextCpId);
    }
    bool isFirstTurn() const {
        return 1 == turn;
    }
};

class Checkpoint {
public:
    Checkpoint(const Game &context, int id=0) :
        context(&context),
        id(wrap(id)) {}
    static const int radius = 600;
    int getId() const {
        return id;
    }
    int nextId() const {
        return wrap(id+1);
    }
    Point point() const {
        return context->getCp(id);
    }
    Checkpoint next() {
        return Checkpoint(*context, nextId());
    }
    operator Point() const {
        return point();
    }
private:
    const Game *context;
    int id;
    int wrap(int id) const {
        return id % context->checkpointCount;
    }
};

// Game move consisting of target coördinates and speed
class Move {
public:
    Move(Point target = Point(), int speed = 100) :
        target(target),
        speed(speed) {}
    Point target;
    bool boost = false;
    int setSpeed(int desiredSpeed) {
        speed = min(100,max(0,desiredSpeed));
        return speed;
    }
    string to_string() const {
        return target.to_string() + " "s + (boost ? "BOOST" : std::to_string(speed));
    }
private:
    int speed;
};

/*
 *  Game loop
 */

string play(const Pod &pod, const Game &game);

int main() {
    Game game;
    game.init(cin);

    while (1) {
        game.prepareNextTurn(cin);

        for (Pod pod : game.player) {
            cout << play(pod, game) << endl;
        }
    }
}

/*
 *  Helper functions 
 */

// Inputs: 0-360 (or -1 at the start of the game)
// Output: 0-180
int angleDiff(int a1, int a2) {
    if (a1 == -1 || a2 == -1) {
        return 0;
    } else {
        assert(a1 >= 0);
        assert(a1 <= 360);
        assert(a2 >= 0);
        assert(a2 <= 360);
        int ret = 180 - abs(abs(a1 - a2) - 180);
        assert(ret >= 0);
        assert(ret <= 180);
        return ret;
    }
}

int distToCp(const Game &game, const Pod &pod) {
    return pod.distance(game.getCp(pod));
}

/*
 *  Gameplay logic
 */

// Guess if pod is likely to hit it's CP without accelerating
bool expectToHitCp(const Game &game, const Pod &pod) {
    Point cp = game.getCp(pod);

    static const int safetyMarginSpeed = Checkpoint::radius * 0;
    // Does our speed allow us to coast to the checkpoint with margin to spare?
    bool enoughSpeed = pod.coastDist() >= (pod.distance(cp) + safetyMarginSpeed);

    static const int safetyMarginAngle = Checkpoint::radius * 0;
    Vector velocityNormalisedToCpDistance(pod.distance(cp), pod.velocity.angle());
    Point pointClosestToCp = pod.position + Point(velocityNormalisedToCpDistance);
    int distanceClosestToCp = cp.distance(pointClosestToCp);
    // Does our direction project through the radius of the checkpoint with margin to spare?
    bool enoughAccuracy = distanceClosestToCp <= (Checkpoint::radius - safetyMarginAngle);

    return enoughSpeed && enoughAccuracy;
}

// Slowdown factor when not facing the target
float speedFactorAngle(const Pod &pod, Point target) {
    static const float rotationSlowdownFactor = 0.02f;
    Point relativeTarget = (target - pod.position);
    int rotationalError = angleDiff(pod.angle, relativeTarget.angle());
    return max(0.0f, min(1.0f, 1.0f - (rotationalError * rotationSlowdownFactor)));
}

// Slowdown factor when close to the target
float speedFactorDistance(const Pod &pod, Point target) {
    static const float proximitySlowdownFactor = 0.002f;
    int targetDistance = pod.distance(target);
    return max(0.0f, min(1.0f, targetDistance * proximitySlowdownFactor));
}

string play(const Pod &pod, const Game &game) {
    Checkpoint targetCp(game, pod.nextCpId);
    float desiredSpeed = Pod::maxSpeed;

    if (expectToHitCp(game, pod)) {
        targetCp = targetCp.next();
        desiredSpeed = 0;  // Prevent acceleration until current checkpoint is reached
    }
    desiredSpeed *= speedFactorAngle   (pod, targetCp);
    desiredSpeed *= speedFactorDistance(pod, targetCp);

    Move move(targetCp, desiredSpeed);

    // Start game with a boost
    if (game.isFirstTurn()) move.boost = true;

    // Debug
    cerr
        << "moving from " << pod.position.to_string()
        << " to (" << targetCp.getId()
        << ") Distance: " << pod.distance(move.target)
        << " (" << (move.target - pod.position).angle()
        << "°) from us. Pointing " << pod.angle
        << "° Velocity " << pod.velocity.angle()
        << "°" << endl;
    
    // Render result
    return move.to_string();
}
