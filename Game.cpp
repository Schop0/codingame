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
    int getDirection() {
        return direction;
    }
    int setDirection(int newDirection) {
        direction = normalise(newDirection);
        return direction;
    }
    int x() {
        return round(cos(direction) * magnitude);
    }
    int y() {
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
    int angle() {
        int angle = atan2 (y,x) * 180 / numbers::pi;
        if (angle < 0) angle += 360; // Positive angles
        return angle;
    }
    int distance(Point p) {
        p = p - *this;
        return sqrt(p.x*p.x + p.y*p.y);
    }
    int magnitude() {
        return distance(Point(0,0));
    }
    Vector toVector() {
        return Vector(magnitude(), angle());
    }
    string to_string() {
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
    int distance(Point p) {
        return position.distance(p);
    }
    int speed() {
        return velocity.magnitude();
    }
    int heading() {
        return velocity.angle();
    }
    // Final resting point if not accelerating
    Point coastOffset() {
        return velocity * (20.0f/3.0f);
    }
    Point coastDest() {
        return position + coastOffset();
    }
    Vector coastVect() {
        return Vector(coastOffset().toVector());
    }
    int coastDist() {
        return coastVect().magnitude;
    }
};

// Game context as initially provided
class Game {
public:
    unsigned int laps;
    unsigned int checkpoint_count;
    vector<Point> checkpoints;
    // getCP allows wraparound so the following is safe:
    // getCP(pod.nextCpId + 1)
    Point getCp(unsigned int index) {
        return checkpoints[index % checkpoint_count];
    }
    Point getCp(Pod pod) {
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
    static const int RADIUS = 600;
    int getId() {
        return id;
    }
    int setId(int newId) {
        id = wrap(newId);
        return id;
    }
    int nextId() {
        return wrap(id+1);
    }
    Point point() {
        return context.getCp(id);
    }
    Point next() {
        return context.getCp(nextId());
    }
    Point advance() {
        id = nextId();
        return point();
    }
private:
    Game &context;
    int id;
    int wrap(int id) {
        return id % context.checkpoint_count;
    }
};

// Game move consisting of target coördinates and speed
class Move {
public:
    Move(Point target)
    : target(target) {
    }
    Move()
    : Move(Point()) {}
    Move(Checkpoint cp)
    : Move(cp.point()) {}
    Point target;
    int speed = 100;
    bool boost = false;
    int setSpeed(int desiredSpeed) {
        speed = min(100,max(0,desiredSpeed));
        return speed;
    }
    string to_string() {
        return target.to_string() + " "s + (boost ? "BOOST" : std::to_string(speed));
    }
};

/*
 *  Context parsing functions 
 */

void read_game(Game &game) {
    cin >> game.laps; cin.ignore();
    cin >> game.checkpoint_count; cin.ignore();
    for (int i = 0; i < game.checkpoint_count; i++) {
        Point checkpoint;
        cin >> checkpoint.x >> checkpoint.y; cin.ignore();
        game.checkpoints.push_back(checkpoint);
    }
}

void read_context(Pod pod[2]) {
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
    read_game(game);

    while (1) {
        Pod player[2];
        read_context(player);
        Pod enemy[2];
        read_context(enemy);

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
    static const int SAFETY_MARGIN = Checkpoint::RADIUS;
    // Ignore direction for simplicity: this may be wrong
    return pod.coastDist() > (distToCp(game, pod) + SAFETY_MARGIN);
}

// Slowdown factor when not facing the target
int speedLimitAngle(Pod pod, Point target) {
    static const int ROTATION_SLOWDOWN_FACTOR = 2;
    Point relativeTarget = (target - pod.position);
    int rotationalError = angleDiff(pod.angle, relativeTarget.angle());
    return max(0,min(100,100-(rotationalError*ROTATION_SLOWDOWN_FACTOR)));
}

// Slowdown factor when close to the target
int speedLimitDistance(Pod pod, Point target) {
    static const int PROXIMITY_SLOWDOWN_FACTOR = 5;
    int targetDistance = pod.distance(target);
    return max(0,min(100,targetDistance/PROXIMITY_SLOWDOWN_FACTOR));
}

string play(Pod &pod, Game &game) {
    Checkpoint targetCp(game, pod.nextCpId);
    
    if (expectToHitCp(game, pod)) {
        targetCp.advance();
    }

    Move move(targetCp);

    move.setSpeed(min(
        speedLimitAngle   (pod, move.target),
        speedLimitDistance(pod, move.target)
        ));

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
