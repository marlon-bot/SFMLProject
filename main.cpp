#include <iostream>
#include <vector>
#include <optional>
#include <cmath>
#include <random>
#include <sstream>
#include <SFML/Graphics.hpp>

/* Da dieses Projekt nicht so gross ist, habe ich keine wirkliche Struktur mit Header, logging etc gemacht.
 * Erstes mal mit SFML, meiner meinung nach mit QT einer der besten Graphic Frameworks fuer cpp.
 * Gemacht von Marlon
 * https://github.com/marlon-bot
*/

// Das ist "Ball"-Datentyp.
// besteht aus:
// - shape: das sichtbare Kreis-Objekt
// - velocity: die Geschwindigkeit in X und Y Richtung
// Noch nicht viel ahnung von eigenen Datatypen deshalb musste ich Chatgpt um hilfe bitten

struct Ball {
    sf::CircleShape shape;
    sf::Vector2f velocity;
};

// Funktion berechnet die Länge von einem Vector.
// Bsp:
// Vector = {3, 4}
// Länge = 5
// Das braucht man für Kreis-Kollision wenn man Google vertraut.
float getLength(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

// Diese Funktion macht aus Vector normalisierten Vector.
// Normalisiert bedeutet: Der Vector zeigt noch in die gleiche Richtung aber seine Länge ist genau 1.
// Braucht man, um Bälle sauber auseinanderzuschieben, denn bei einem Collisionsvector ist die Entfernung komplett irrelevant.
sf::Vector2f normalize(sf::Vector2f v) {
    float length = getLength(v);

    if (length == 0.f) {
        return { 0.f, 0.f };
    }

    return { v.x / length, v.y / length };
}
// main loop
int main()
{
    // Fenster erstellen
    const int windowWidth = 800;
    const int windowHeight = 600;

    sf::RenderWindow window(
        sf::VideoMode({ windowWidth, windowHeight }),
        "Simple Physics Balls by Marlon",
        sf::Style::Titlebar | sf::Style::Close
    );


    // Font laden
    sf::Font font("arial.ttf");

    // Text für HUD erstellen
    sf::Text hudText(font);
    hudText.setCharacterSize(20);
    hudText.setFillColor(sf::Color::White);
    hudText.setPosition({ 10.f, 10.f });

    // Hier werden alle Bälle gespeichert in Vector Balls.
    std::vector<Ball> balls;

    // Clocks messen Zeit.
    // frameClock: wird für Physics benutzt
    // spawnClock: sorgt dafür, dass beim Halten nicht unendlich viele Bälle pro Frame gespawnt werden
    // idleClock: wenn 30 Sekunden nichts passiert, clearen wir automatisch
    //SFML hat eigenes Clock modul sf::Clock
    sf::Clock frameClock;
    sf::Clock spawnClock;
    sf::Clock idleClock;

    // Random Gen für zufällige seitliche Geschwindigkeit
    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_real_distribution<float> randomSideSpeed(-250.f, 250.f);

    // Physics Einstellungen
    const float ballRadius = 4.f;

    const float gravity = 1200.f;

    const float floorY = static_cast<float>(windowHeight);
    const float leftWall = 0.f;
    const float rightWall = static_cast<float>(windowWidth);

    const float bounce = 0.05f;
    const float friction = 0.96f;
    const float maxSpeed = 900.f;

    // Alle 0.01 Sekunden darf ein neuer Ball gespawnt werden.
    const float spawnDelay = 0.01f;

    // Nach 30 Sekunden ohne Klick / Reset wird automatisch gelöscht.
    const float idleClearTime = 30.f;

    // Haupt-Loop
    while (window.isOpen())
    {

        // Delta Time berechnen um FPS zu bestimmen, genau wie in Unity
        float dt = frameClock.restart().asSeconds();

        // Falls Spiel kurz hängt soll die Physics nicht explodieren. Spreche aus erfahrung.
        if (dt > 0.03f) {
            dt = 0.03f;
        }

        // Events abarbeiten, Z.b Fenster schließen.
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // Input
        bool leftMousePressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        bool rPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
        // Reset mit R
        if (rPressed) {
            balls.clear();
            idleClock.restart();
        }

        // Ball spawnen, wenn linke Maustaste gedrückt wird.
        // spawnClock verhindert, dass pro Sekunde hunderte Bälle entstehen.
        if (leftMousePressed && spawnClock.getElapsedTime().asSeconds() >= spawnDelay) {
            sf::Vector2i mousePosInt = sf::Mouse::getPosition(window);
            sf::Vector2f mousePosFloat = static_cast<sf::Vector2f>(mousePosInt);

            Ball newBall;

            // Kreis einstellen
            newBall.shape.setRadius(ballRadius); //Groesse
            newBall.shape.setOrigin({ ballRadius, ballRadius });
            newBall.shape.setPosition(mousePosFloat); //Position
            newBall.shape.setFillColor(sf::Color::Yellow); //Farbe

            // Zufällige Geschwindigkeit nach links oder rechts
            float sideSpeed = randomSideSpeed(rng);

            // Damit die Geschwindigkeit nicht zu klein ist
            if (std::abs(sideSpeed) < 80.f) {
                if (sideSpeed < 0.f) {
                    sideSpeed = -80.f;
                } else {
                    sideSpeed = 80.f;
                }
            }

            newBall.velocity = { sideSpeed, 0.f };

            balls.push_back(newBall);

            spawnClock.restart();
            idleClock.restart();
        }

        // Automatisch löschen, wenn 30 Sekunden nichts gemacht wurde
        if (!balls.empty() && idleClock.getElapsedTime().asSeconds() >= idleClearTime) {
            balls.clear();
            idleClock.restart();
        }

        // Physics Update für jeden Ball
        for (auto& ball : balls) {
            // Schwerkraft zieht nach unten
            ball.velocity.y += gravity * dt;

            // Reibung bremst X-Geschwindigkeit langsam ab
            ball.velocity.x *= friction;

            // Geschwindigkeit limitieren
            float speed = getLength(ball.velocity);

            if (speed > maxSpeed) {
                ball.velocity = normalize(ball.velocity) * maxSpeed;
            }

            // Ball bewegen
            ball.shape.move(ball.velocity * dt);

            // Position holen
            sf::Vector2f pos = ball.shape.getPosition();

            // Boden Collision
            if (pos.y + ballRadius > floorY) {
                pos.y = floorY - ballRadius;
                ball.shape.setPosition(pos);

                ball.velocity.y *= -bounce;

                if (std::abs(ball.velocity.y) < 60.f) {
                    ball.velocity.y = 0.f;
                }
            }

            // Linke Wand
            if (pos.x - ballRadius < leftWall) {
                pos.x = leftWall + ballRadius;
                ball.shape.setPosition(pos);

                ball.velocity.x *= -bounce;
            }

            // Rechte Wand
            if (pos.x + ballRadius > rightWall) {
                pos.x = rightWall - ballRadius;
                ball.shape.setPosition(pos);

                ball.velocity.x *= -bounce;
            }
        }

        // Ball gegen Ball Collision
        // jeden Ball mit jedem anderen Ball vergleichen.
        for (std::size_t i = 0; i < balls.size(); i++) {
            for (std::size_t j = i + 1; j < balls.size(); j++) {
                Ball& ballA = balls[i];
                Ball& ballB = balls[j];

                sf::Vector2f posA = ballA.shape.getPosition();
                sf::Vector2f posB = ballB.shape.getPosition();

                sf::Vector2f difference = posB - posA;

                float distance = getLength(difference);
                float minDistance = ballRadius * 2.f;

                // Wenn distance kleiner ist als minDistance,
                // dann überlappen sich die Bälle.
                if (distance < minDistance && distance > 0.f) {
                    sf::Vector2f normal = normalize(difference);

                    float overlap = minDistance - distance;

                    // Nicht zu aggressiv auseinanderdrücken,
                    // sonst explodiert der Ballhaufen.
                    // (Passiert irgendwie immer noch, aber noch lange nicht so schlimm wie davor)
                    const float correctionStrength = 0.35f;

                    ballA.shape.move(-normal * overlap * correctionStrength);
                    ballB.shape.move(normal * overlap * correctionStrength);

                    // Relative Geschwindigkeit zwischen beiden Bällen
                    float relativeVelocity =
                        (ballB.velocity.x - ballA.velocity.x) * normal.x +
                        (ballB.velocity.y - ballA.velocity.y) * normal.y;

                    // Nur reagieren, wenn sie aufeinander zufliegen
                    if (relativeVelocity < 0.f) {
                        float impulse = -(1.f + bounce) * relativeVelocity;

                        // Impuls extra schwach machen,
                        // damit nichts wild explodiert
                        impulse *= 0.20f;

                        sf::Vector2f impulseVector = normal * impulse;

                        ballA.velocity -= impulseVector;
                        ballB.velocity += impulseVector;
                    }
                }
            }
        }

        // FPS berechnen
        // FPS = 1 / dt
        int fps = 0;

        if (dt > 0.f) {
            fps = static_cast<int>(1.f / dt);
        }

        // HUD Text bauen
        // std::ostringstream sehr praktisch, um Zahlen in Text zu packen.
        std::ostringstream hud;

        hud << "Balls: " << balls.size() << "\n";
        hud << "FPS: " << fps << "\n";
        hud << "R to reset";

        hudText.setString(hud.str());

        // Zeichnen
        // Wichtig:
        // window.clear() löscht jedes Frame alles.
        // Deshalb danach alles neu zeichnen.
        window.clear();

        for (const auto& ball : balls) {
            window.draw(ball.shape);
        }

        window.draw(hudText);

        window.display();
    }

    return 0;
}