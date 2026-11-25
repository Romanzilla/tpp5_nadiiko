#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

constexpr float PI = 3.14159265358979323846f;

// Стан фізичної моделі обертального руху тіла (Варіант 18)
struct SimulationState {
    // Керовані параметри моделі
    float torque = 6.0f;           // Зовнішній момент сили M (Н*м)
    float momentOfInertia = 2.0f;  // Момент інерції тіла I (кг*м^2)

    // Динамічні змінні стану
    float angularAccel = 3.0f;     // Кутове прискорення eps = M / I (рад/с^2)
    float angularVelocity = 0.0f;  // Кутова швидкість omega (рад/с)
    float angleRad = 0.0f;         // Кут повороту phi (рад)
    float elapsedTime = 0.0f;      // Час моделювання t (с)

    // Управління симуляцією
    bool isPaused = false;

    // Історія траєкторії мітки на ободі тіла
    std::vector<sf::Vector2f> trail;
};

// Інтерактивні кнопки на екрані
struct UiButtons {
    sf::RectangleShape btnPause;
    sf::RectangleShape btnReset;
    sf::RectangleShape btnTorquePlus;
    sf::RectangleShape btnTorqueMinus;
    sf::RectangleShape btnInertiaPlus;
    sf::RectangleShape btnInertiaMinus;
};

UiButtons createButtons() {
    UiButtons ui;
    auto setup = [](sf::RectangleShape& rect, sf::Vector2f pos, sf::Vector2f size, sf::Color color) {
        rect.setPosition(pos);
        rect.setSize(size);
        rect.setFillColor(color);
        rect.setOutlineThickness(1.5f);
        rect.setOutlineColor(sf::Color(200, 210, 225));
    };

    setup(ui.btnPause,        {680.f, 20.f},  {125.f, 36.f}, sf::Color(45, 110, 185));
    setup(ui.btnReset,        {815.f, 20.f},  {125.f, 36.f}, sf::Color(175, 65, 65));
    setup(ui.btnTorqueMinus,  {680.f, 70.f},  {125.f, 34.f}, sf::Color(55, 65, 80));
    setup(ui.btnTorquePlus,   {815.f, 70.f},  {125.f, 34.f}, sf::Color(55, 65, 80));
    setup(ui.btnInertiaMinus, {680.f, 114.f}, {125.f, 34.f}, sf::Color(55, 65, 80));
    setup(ui.btnInertiaPlus,  {815.f, 114.f}, {125.f, 34.f}, sf::Color(55, 65, 80));
    return ui;
}

void resetSimulation(SimulationState& state) {
    state.angularVelocity = 0.0f;
    state.angleRad = 0.0f;
    state.elapsedTime = 0.0f;
    state.angularAccel = state.torque / state.momentOfInertia;
    state.trail.clear();
}

// 1. Оброблення взаємодії з користувачем (клавіатура та миша)
void handleInput(sf::RenderWindow& window, SimulationState& state, const UiButtons& ui) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }

        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Up) {
                state.torque += 2.0f;
            } else if (key->code == sf::Keyboard::Key::Down) {
                state.torque -= 2.0f;
            } else if (key->code == sf::Keyboard::Key::Right) {
                state.momentOfInertia += 0.5f;
            } else if (key->code == sf::Keyboard::Key::Left) {
                state.momentOfInertia -= 0.5f;
            } else if (key->code == sf::Keyboard::Key::Num0 || key->code == sf::Keyboard::Key::Numpad0) {
                state.torque = 0.0f;
            } else if (key->code == sf::Keyboard::Key::P) {
                state.isPaused = !state.isPaused;
            } else if (key->code == sf::Keyboard::Key::R || key->code == sf::Keyboard::Key::Space) {
                resetSimulation(state);
            }
        }

        if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouse->button == sf::Mouse::Button::Left) {
                sf::Vector2f mPos(static_cast<float>(mouse->position.x),
                                  static_cast<float>(mouse->position.y));
                if (ui.btnPause.getGlobalBounds().contains(mPos)) {
                    state.isPaused = !state.isPaused;
                } else if (ui.btnReset.getGlobalBounds().contains(mPos)) {
                    resetSimulation(state);
                } else if (ui.btnTorquePlus.getGlobalBounds().contains(mPos)) {
                    state.torque += 2.0f;
                } else if (ui.btnTorqueMinus.getGlobalBounds().contains(mPos)) {
                    state.torque -= 2.0f;
                } else if (ui.btnInertiaPlus.getGlobalBounds().contains(mPos)) {
                    state.momentOfInertia += 0.5f;
                } else if (ui.btnInertiaMinus.getGlobalBounds().contains(mPos)) {
                    state.momentOfInertia -= 0.5f;
                }
            }
        }

        state.torque = std::clamp(state.torque, -30.0f, 30.0f);
        state.momentOfInertia = std::clamp(state.momentOfInertia, 0.5f, 15.0f);
        state.angularAccel = state.torque / state.momentOfInertia;
    }
}

// 2. Оновлення фізичної моделі обертального руху з кроком часу dt
void updatePhysics(SimulationState& state, float dt, sf::Vector2f center, float visualRadius) {
    state.angularAccel = state.torque / state.momentOfInertia;

    if (state.isPaused) {
        return;
    }

    // Обмеження кроку dt для збереження стійкості інтегрування
    if (dt > 0.05f) {
        dt = 0.05f;
    }

    // Основний закон динаміки обертання: eps = M / I
    // Метод напівнеявного Ейлера (Euler-Cromer):
    state.angularVelocity += state.angularAccel * dt;
    state.angleRad += state.angularVelocity * dt;
    state.elapsedTime += dt;

    // Координати контрольної мітки на ободі тіла для побудови траєкторії
    sf::Vector2f markerPos{
        center.x + visualRadius * std::cos(state.angleRad),
        center.y + visualRadius * std::sin(state.angleRad)
    };
    state.trail.push_back(markerPos);
    if (state.trail.size() > 240) {
        state.trail.erase(state.trail.begin());
    }
}

// 3. Відображення графічної сцени та телеметрії
void drawScene(sf::RenderWindow& window,
               const SimulationState& state,
               const UiButtons& ui,
               const sf::Font& font,
               sf::Vector2f center,
               float visualRadius) {
    window.clear(sf::Color(18, 22, 30));

    // Опорна координатна сітка навколо осі обертання
    sf::RectangleShape axisX({380.f, 1.f});
    axisX.setOrigin({190.f, 0.5f});
    axisX.setPosition(center);
    axisX.setFillColor(sf::Color(55, 65, 85));
    window.draw(axisX);

    sf::RectangleShape axisY({1.f, 380.f});
    axisY.setOrigin({0.5f, 190.f});
    axisY.setPosition(center);
    axisY.setFillColor(sf::Color(55, 65, 85));
    window.draw(axisY);

    // Траєкторія руху контрольної точки обода
    if (state.trail.size() > 1) {
        sf::VertexArray trailLine(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < state.trail.size(); ++i) {
            std::uint8_t alpha = static_cast<std::uint8_t>(40 + (200 * i) / state.trail.size());
            trailLine.append(sf::Vertex{state.trail[i], sf::Color(80, 190, 255, alpha)});
        }
        window.draw(trailLine);
    }

    // Тіло обертання (маховик), радіус якого масштабується відповідно до моменту інерції I
    float angleDeg = state.angleRad * 180.0f / PI;

    sf::CircleShape outerDisk(visualRadius);
    outerDisk.setOrigin({visualRadius, visualRadius});
    outerDisk.setPosition(center);
    outerDisk.setFillColor(sf::Color(42, 54, 74));
    outerDisk.setOutlineThickness(4.f);
    outerDisk.setOutlineColor(sf::Color(120, 175, 240));
    outerDisk.setRotation(sf::degrees(angleDeg));
    window.draw(outerDisk);

    // 4 радіальні спиці для наочного спостереження кута повороту та швидкості
    for (int k = 0; k < 4; ++k) {
        sf::RectangleShape spoke({visualRadius * 2.f - 8.f, 3.f});
        spoke.setOrigin({visualRadius - 4.f, 1.5f});
        spoke.setPosition(center);
        spoke.setFillColor(sf::Color(150, 175, 210));
        spoke.setRotation(sf::degrees(angleDeg + k * 45.0f));
        window.draw(spoke);
    }

    // Центральна вісь обертання
    sf::CircleShape hub(12.f);
    hub.setOrigin({12.f, 12.f});
    hub.setPosition(center);
    hub.setFillColor(sf::Color(220, 225, 235));
    window.draw(hub);

    // Контрольна червона мітка на ободі
    sf::Vector2f markerPos{
        center.x + (visualRadius - 12.f) * std::cos(state.angleRad),
        center.y + (visualRadius - 12.f) * std::sin(state.angleRad)
    };
    sf::CircleShape marker(9.f);
    marker.setOrigin({9.f, 9.f});
    marker.setPosition(markerPos);
    marker.setFillColor(sf::Color(245, 75, 75));
    window.draw(marker);

    // Візуалізація вектора зовнішнього моменту сили M у верхній точці обода
    if (std::abs(state.torque) > 0.01f) {
        float dir = (state.torque > 0.0f) ? 1.0f : -1.0f;
        float arrowLen = 25.f + std::abs(state.torque) * 3.2f;
        sf::Vector2f topPoint{center.x, center.y - visualRadius - 14.f};

        sf::RectangleShape shaft({arrowLen, 4.f});
        shaft.setPosition({dir > 0 ? topPoint.x : topPoint.x - arrowLen, topPoint.y - 2.f});
        shaft.setFillColor(sf::Color(255, 200, 60));
        window.draw(shaft);

        sf::ConvexShape head;
        head.setPointCount(3);
        if (dir > 0) {
            head.setPoint(0, {topPoint.x + arrowLen + 12.f, topPoint.y});
            head.setPoint(1, {topPoint.x + arrowLen, topPoint.y - 7.f});
            head.setPoint(2, {topPoint.x + arrowLen, topPoint.y + 7.f});
        } else {
            head.setPoint(0, {topPoint.x - arrowLen - 12.f, topPoint.y});
            head.setPoint(1, {topPoint.x - arrowLen, topPoint.y - 7.f});
            head.setPoint(2, {topPoint.x - arrowLen, topPoint.y + 7.f});
        }
        head.setFillColor(sf::Color(255, 200, 60));
        window.draw(head);
    }

    // Панель телеметрії (зліва вгорі)
    float revolutions = std::abs(state.angleRad) / (2.0f * PI);
    float rpm = state.angularVelocity * 60.0f / (2.0f * PI);
    float kineticEnergy = 0.5f * state.momentOfInertia * state.angularVelocity * state.angularVelocity;

    char buf[512];
    std::snprintf(
        buf, sizeof(buf),
        "Варіант 18: Моделювання обертання тіла\n"
        "Статус: %s   |   Час t = %.2f с\n"
        "----------------------------------------------\n"
        "Момент сили (M):          %+.2f Н*м\n"
        "Момент інерції (I):        %.2f кг*м^2\n"
        "Кутове прискорення (eps): %+.3f рад/с^2\n"
        "Кутова швидкість (omega): %+.3f рад/с (%.1f об/хв)\n"
        "Кут повороту (phi):       %.1f град (%.2f об.)\n"
        "Кінетична енергія (Ek):    %.2f Дж",
        state.isPaused ? "ПАУЗА (P)" : "МОДЕЛЮВАННЯ",
        state.elapsedTime,
        state.torque,
        state.momentOfInertia,
        state.angularAccel,
        state.angularVelocity, rpm,
        angleDeg, revolutions,
        kineticEnergy
    );

    // Конвертуємо UTF-8 рядок у sf::String для коректного відображення кирилиці
    sf::String telemetryStr = sf::String::fromUtf8(buf, buf + std::strlen(buf));
    sf::Text telemetryText(font, telemetryStr, 17);
    telemetryText.setPosition({20.f, 18.f});
    telemetryText.setFillColor(sf::Color(235, 240, 248));
    window.draw(telemetryText);

    // Відображення інтерактивних кнопок
    window.draw(ui.btnPause);
    window.draw(ui.btnReset);
    window.draw(ui.btnTorqueMinus);
    window.draw(ui.btnTorquePlus);
    window.draw(ui.btnInertiaMinus);
    window.draw(ui.btnInertiaPlus);

    auto drawBtnLabel = [&](const sf::RectangleShape& btn, const char* labelUtf8) {
        sf::String s = sf::String::fromUtf8(labelUtf8, labelUtf8 + std::strlen(labelUtf8));
        sf::Text t(font, s, 15);
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition({
            btn.getPosition().x + (btn.getSize().x - b.size.x) * 0.5f - b.position.x,
            btn.getPosition().y + (btn.getSize().y - b.size.y) * 0.5f - b.position.y
        });
        t.setFillColor(sf::Color::White);
        window.draw(t);
    };

    drawBtnLabel(ui.btnPause, state.isPaused ? "Старт (P)" : "Пауза (P)");
    drawBtnLabel(ui.btnReset, "Скинути (R)");
    drawBtnLabel(ui.btnTorqueMinus, "M -2 (Down)");
    drawBtnLabel(ui.btnTorquePlus,  "M +2 (Up)");
    drawBtnLabel(ui.btnInertiaMinus, "I -0.5 (Left)");
    drawBtnLabel(ui.btnInertiaPlus,  "I +0.5 (Right)");

    // Підказка керування внизу вікна
    const char* helpUtf8 =
        "Керування: [Up/Down] Момент сили M  |  [Left/Right] Момент інерції I  |  "
        "[0] M = 0 (інерція)  |  [P] Пауза/Старт  |  [R / Space] Перезапуск";
    sf::String helpStr = sf::String::fromUtf8(helpUtf8, helpUtf8 + std::strlen(helpUtf8));
    sf::Text helpText(font, helpStr, 14);
    helpText.setPosition({20.f, 608.f});
    helpText.setFillColor(sf::Color(165, 180, 200));
    window.draw(helpText);

    window.display();
}

int main() {
    sf::RenderWindow window(
        sf::VideoMode({960, 640}),
        sf::String::fromUtf8(
            "Лабораторна робота №5 | Варіант 18: Моделювання обертання тіла (Надійко Роман)",
            "Лабораторна робота №5 | Варіант 18: Моделювання обертання тіла (Надійко Роман)" + 140
        )
    );
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded =
        font.openFromFile("resources/fonts/arial.ttf") ||
        font.openFromFile("../resources/fonts/arial.ttf") ||
        font.openFromFile("../../resources/fonts/arial.ttf") ||
        font.openFromFile("C:/Windows/Fonts/arial.ttf");

    if (!fontLoaded) {
        return 1;
    }

    SimulationState state;
    UiButtons ui = createButtons();
    sf::Clock clock;

    const sf::Vector2f center{500.f, 355.f};

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // Візуальний радіус диска відображає зміну моменту інерції I
        float visualRadius = 85.f + state.momentOfInertia * 6.5f;

        handleInput(window, state, ui);
        updatePhysics(state, dt, center, visualRadius);
        drawScene(window, state, ui, font, center, visualRadius);
    }

    return 0;
}
