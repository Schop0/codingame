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
    int getDirection() const {
        return direction;
    }
    int setDirection(int newDirection) {
        direction = normalise(newDirection);
        return direction;
    }
    int x() const {
        return round(cos(direction) * magnitude);
    }
    int y() const {
        return round(sin(direction) * magnitude);
    }
    int magnitude;
private:
    int direction;
    static int normalise(int direction) {
        return direction % 360;
    }
};

class Point {
public:
    Point(int x, int y) : x(x)  , y(y)   {}
    Point()             : x(0)  , y(0)   {}
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
        return sqrt(p.x*p.x + p.y*p.y);
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
    friend Point operator*(Point lhs, float rhs) {
        lhs.x = (int)(0.5f + lhs.x * rhs);
        lhs.y = (int)(0.5f + lhs.y * rhs);
        return lhs;
    }
};

class Pod {
public:
    Point position;
    Point velocity;
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
        return velocity * (20.0f/3.0f);
    }
    Point coastDest() const {
        return position + coastOffset();
    }
    Vector coastVect() const {
        return Vector(coastOffset().toVector());
    }
    int coastDist() const {
        return coastVect().magnitude;
    }
};

// Game context as initially provided
class Game {
public:
    unsigned int laps;
    unsigned int checkpointCount;
    vector<Point> checkpoints;
    // getCP allows wraparound so the following is safe:
    // getCP(pod.nextCpId + 1)
    Point getCp(unsigned int index) const {
        return checkpoints[index % checkpointCount];
    }
    Point getCp(Pod pod) const {
        return getCp(pod.nextCpId);
    }
};

class Checkpoint {
public:
    Checkpoint(Game &context, int id) :
        context(context),
        id(wrap(id)) {}
    Checkpoint(Game &context)
    : Checkpoint(context, 0) {}
    static const int radius = 600;
    int getId() const {
        return id;
    }
    int setId(int newId) {
        id = wrap(newId);
        return id;
    }
    int nextId() const {
        return wrap(id+1);
    }
    Point point() const {
        return context.getCp(id);
    }
    Point next() const {
        return context.getCp(nextId());
    }
    Point advance() {
        id = nextId();
        return point();
    }
    operator Point() const {
        return point();
    }
private:
    Game &context;
    int id;
    int wrap(int id) const {
        return id % context.checkpointCount;
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
 *  Context parsing functions 
 */

void readGame(Game &game) {
    cin >> game.laps; cin.ignore();
    cin >> game.checkpointCount; cin.ignore();
    for (int i = 0; i < game.checkpointCount; i++) {
        Point checkpoint;
        cin >> checkpoint.x >> checkpoint.y; cin.ignore();
        game.checkpoints.push_back(checkpoint);
    }
}

void readContext(Pod pod[2]) {
    for (int i = 0; i < 2; i++) {
        cin >> pod[i].position.x >> pod[i].position.y >> pod[i].velocity.x >> pod[i].velocity.y >> pod[i].angle >> pod[i].nextCpId; cin.ignore();
    }
}

/*
 *  Game loop
 */

string play(Pod &pod, Game &game);

int main() {
    Game game;
    readGame(game);

    while (1) {
        Pod player[2];
        readContext(player);
        Pod enemy[2];
        readContext(enemy);

        cout << play(player[0], game) << endl;
        cout << play(player[1], game) << endl;
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

int distToCp(Game game, Pod pod) {
    return pod.distance(game.getCp(pod));
}

/*
 *  Gameplay logic
 */

// Guess if pod is likely to hit it's CP without accelerating
bool expectToHitCp(Game game, Pod pod) {
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
float speedFactorAngle(Pod pod, Point target) {
    static const float rotationSlowdownFactor = 0.02f;
    Point relativeTarget = (target - pod.position);
    int rotationalError = angleDiff(pod.angle, relativeTarget.angle());
    return max(0.0f, min(1.0f, 1.0f - (rotationalError * rotationSlowdownFactor)));
}

// Slowdown factor when close to the target
float speedFactorDistance(Pod pod, Point target) {
    static const float proximitySlowdownFactor = 0.002f;
    int targetDistance = pod.distance(target);
    return max(0.0f, min(1.0f, targetDistance * proximitySlowdownFactor));
}

string play(Pod &pod, Game &game) {
    Checkpoint targetCp(game, pod.nextCpId);
    float desiredSpeed = 100;

    if (expectToHitCp(game, pod)) {
        targetCp.advance();
        desiredSpeed = 0;  // Prevent acceleration until current checkpoint is reached
    }
    desiredSpeed *= speedFactorAngle   (pod, targetCp);
    desiredSpeed *= speedFactorDistance(pod, targetCp);

    Move move(targetCp, desiredSpeed);

    // Start game with a boost
    if (pod.angle == -1) move.boost = true;

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
