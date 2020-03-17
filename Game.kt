import java.lang.Math.toDegrees
import java.lang.Math.toRadians
import java.util.Scanner
import kotlin.math.*

fun main() {
    try {
        Game().play()
    } catch (e: NoSuchElementException) {
        println("No more input")
    }
}

class Game {
    private val player = WHacker()
    private val map = Map()
    private val stdin = Scanner(System.`in`)

    init {
        // Give the player access to the game map
        player.map = map
    }

    fun play() {
        while (true) {
            updateMap()
            println(player.getNextTurn())
        }
    }

    private fun updateMap() {
        map.playerPod.move(readPoint())
        map.nextCheckpoint = Checkpoint(readPoint())
        map.vectorNextCheckpoint = readVector()
        map.opponentPod.move(readPoint())
        System.err.println(map)
    }
    private fun readPoint() = Point(stdin.nextInt(), stdin.nextInt())
    private fun readVector() = Vector(stdin.nextDouble(), stdin.nextDouble())
}

data class Map(
    var playerPod: Pod = Pod(),
    var opponentPod: Pod = Pod()
) {
    lateinit var nextCheckpoint: Checkpoint
    lateinit var vectorNextCheckpoint: Vector
}

abstract class Player {
    lateinit var map: Map
    abstract fun getNextTurn(): Turn
}

data class Turn(
    var target: Point = Point(),
    var thrust: String = "0"
) {
    constructor(
        target: Point = Point(),
        thrust: Int = 0
    ): this(target, thrust.toString())

    override fun toString() = "$target $thrust"
}

class Pod(
    private val initialLocation: Point? = null,
    var velocity: Vector = Vector()
) {
    var coordinates = initialLocation
    var location: Point
        get() = coordinates ?: Point()
        set(newLocation) {
            coordinates = newLocation
        }
    var boostAvailable = true
    companion object {
        const val radius = 400
        const val dragFactor = 0.85
        const val maxThrust = 100
    }

    fun move(newLocation: Point): Pod {
        val oldLocation = coordinates ?: newLocation
        velocity = Vector((newLocation - oldLocation) * dragFactor)
        coordinates = newLocation
        return this
    }

    fun boostOrMaxThrust(): String {
        val action = if (boostAvailable) {
            "BOOST"
        } else {
            maxThrust.toString()
        }
        boostAvailable = false
        return action
    }

    override fun toString() = "at $coordinates heading $velocity"
}

open class Point(
    val x: Int = 0,
    val y: Int = 0
) {
    constructor(point: Point) : this(point.x, point.y)

    operator fun plus(pt: Point) = Point(x+pt.x, y+pt.y)
    operator fun minus(pt: Point) = Point(x-pt.x, y-pt.y)
    operator fun unaryMinus() = Point(-x, -y)
    operator fun times(n: Double) = Point((n*x).toInt(), (n*y).toInt())

    override fun toString() = "$x $y"
}

class Checkpoint(
    x: Int = 0,
    y: Int = 0
): Point(x, y) {
    constructor(point: Point) : this(point.x, point.y)
    companion object {
        const val radius = 600
    }
}

open class Complex(re: Int = 0, im: Int = 0) {
    val re get() = complexCoords.x
    val im get() = complexCoords.y

    private val complexCoords = Point(re, im)

    constructor(point: Point): this(point.x, point.y)

    fun mod() = sqrt((re*re + im*im).toDouble())
    fun arg() = atan2(im.toDouble(), re.toDouble())
    fun toPoint() = complexCoords

    override fun toString() = "${complexCoords.toString()}j"
}

class Vector(point: Point) {
    constructor(
        magnitude: Double = 0.0,
        angle: Double = 0.0
    ): this(
        Point(
            (cos(toRadians(angle)) * magnitude).toInt(),
            (sin(toRadians(angle)) * magnitude).toInt()
        )
    )

    val magnitude get() = complex.mod()
    val angle get() = toDegrees(complex.arg())

    private val complex = Complex(point.x, point.y)

    fun xComponent() = complex.re
    fun yComponent() = complex.im
    fun toPoint() = complex.toPoint()

    override fun toString() = "${magnitude.toInt()} ${angle.toInt()}°"
}

class WHacker: Player() {
    override fun getNextTurn() = Turn(target(), "BOOST")

    private fun target() = map.nextCheckpoint - projectedVelocity()

    private fun projectedVelocity() = map.playerPod.velocity.toPoint() * 3.0
}
