#include <ra/Engine.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace {

constexpr int mapWidth = 128;
constexpr int mapHeight = 96;
constexpr float tileSize = 8.0f;

struct Resident {
    std::string name;
    float x;
    float y;
    float targetX;
    float targetY;
    float hunger;
    float thirst;
    float energy;
};

class IslaVivaGame final : public ra::Game {
public:
    void onStart(ra::Engine& engine) override
    {
        engine.setWindowTitle("Isla Viva | Red Alert Framework");
        std::vector<int> terrain(mapWidth * mapHeight);
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                const float dx = (x - 65.0f) / 54.0f;
                const float dy = (y - 48.0f) / 39.0f;
                const float wobble = 0.06f * std::sin(x * 0.23f)
                    + 0.06f * std::sin(y * 0.31f)
                    + 0.03f * std::cos((x + y) * 0.47f);
                const bool land = dx * dx + dy * dy < 1.0f + wobble
                    || std::pow((x - 17.0f) / 9.0f, 2.0f) + std::pow((y - 30.0f) / 12.0f, 2.0f) < 1.0f
                    || std::pow((x - 108.0f) / 10.0f, 2.0f) + std::pow((y - 68.0f) / 11.0f, 2.0f) < 1.0f;
                const bool lake = std::pow((x - 70.0f) / 12.0f, 2.0f)
                        + std::pow((y - 37.0f) / 9.0f, 2.0f) < 1.0f + 0.12f * std::sin(x * 0.6f)
                    || std::pow((x - 45.0f) / 8.0f, 2.0f) + std::pow((y - 72.0f) / 5.0f, 2.0f) < 1.0f;
                terrain[y * mapWidth + x] = !land ? 0 : (lake ? 2 : 1);
            }
        }

        constexpr int pixelsPerTile = 8;
        constexpr int imageWidth = mapWidth * pixelsPerTile;
        constexpr int imageHeight = mapHeight * pixelsPerTile;
        std::vector<unsigned char> pixels(imageWidth * imageHeight * 4);
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                const int kind = terrain[y * mapWidth + x];
                const std::array<unsigned char, 3> color = kind == 0
                    ? std::array<unsigned char, 3>{37, 91, 112}
                    : kind == 2 ? std::array<unsigned char, 3>{67, 139, 157}
                    : std::array<unsigned char, 3>{91, 137, 91};
                for (int py = 0; py < pixelsPerTile; ++py) {
                    for (int px = 0; px < pixelsPerTile; ++px) {
                        const std::size_t offset = static_cast<std::size_t>(
                            ((y * pixelsPerTile + py) * imageWidth + x * pixelsPerTile + px) * 4);
                        pixels[offset] = color[0];
                        pixels[offset + 1] = color[1];
                        pixels[offset + 2] = color[2];
                        pixels[offset + 3] = 255;
                    }
                }
            }
        }
        terrainTexture_ = engine.createTextureRGBA(pixels.data(), imageWidth, imageHeight);

        constexpr std::array<const char*, 6> names{"Luna", "Bruno", "Alma", "Teo", "Vera", "Nico"};
        for (std::size_t i = 0; i < names.size(); ++i) {
            const float x = 36.0f + static_cast<float>(i % 3);
            const float y = 52.0f + static_cast<float>(i / 3);
            residents_.push_back({names[i], x, y, x + 8.0f, y + 5.0f,
                70.0f - static_cast<float>(i * 4), 78.0f - static_cast<float>(i * 3), 82.0f});
        }
    }

    void onStop(ra::Engine& engine) override
    {
        engine.destroyTexture(terrainTexture_);
    }

    void onUpdate(ra::Engine& engine, float delta) override
    {
        if (engine.keyPressed(ra::Key::Escape)) {
            engine.quit();
            return;
        }
        if (engine.keyPressed(ra::Key::Space)) paused_ = !paused_;
        if (paused_) return;

        accumulator_ += std::min(delta, 0.25f);
        while (accumulator_ >= 0.2f) {
            accumulator_ -= 0.2f;
            elapsed_ += 0.2f;
            for (std::size_t i = 0; i < residents_.size(); ++i) {
                auto& resident = residents_[i];
                resident.hunger = std::max(0.0f, resident.hunger - 0.0132f);
                resident.thirst = std::max(0.0f, resident.thirst - 0.017f);
                resident.energy = std::max(0.0f, resident.energy - 0.0076f);
                const float dx = resident.targetX - resident.x;
                const float dy = resident.targetY - resident.y;
                const float distance = std::sqrt(dx * dx + dy * dy);
                if (distance > 0.05f) {
                    const float step = std::min(distance, 0.14f);
                    resident.x += dx / distance * step;
                    resident.y += dy / distance * step;
                } else {
                    resident.targetX = 30.0f + static_cast<float>((i * 17 + static_cast<int>(elapsed_ / 5)) % 60);
                    resident.targetY = 24.0f + static_cast<float>((i * 13 + static_cast<int>(elapsed_ / 7)) % 48);
                }
            }
        }
    }

    void onRender(ra::Engine& engine) override
    {
        const float width = static_cast<float>(engine.windowWidth());
        const float height = static_cast<float>(engine.windowHeight());
        const float panelWidth = 300.0f;
        const float scale = std::min((width - panelWidth - 48.0f) / (mapWidth * tileSize),
            (height - 100.0f) / (mapHeight * tileSize));
        const float mapX = 24.0f;
        const float mapY = (height - mapHeight * tileSize * scale) * 0.5f;
        engine.clear({14, 25, 31, 255});

        engine.drawTexture(terrainTexture_, {mapX, mapY, mapWidth * tileSize * scale,
            mapHeight * tileSize * scale});

        const float homeX = mapX + 36.0f * tileSize * scale;
        const float homeY = mapY + 52.0f * tileSize * scale;
        engine.fillRect({homeX, homeY, 7.0f * scale, 7.0f * scale}, {196, 154, 94, 255});
        for (const auto& resident : residents_) {
            const float x = mapX + resident.x * tileSize * scale;
            const float y = mapY + resident.y * tileSize * scale;
            engine.fillRect({x, y, 5.0f * scale, 5.0f * scale}, {246, 225, 177, 255});
        }

        const float panelX = width - panelWidth - 20.0f;
        engine.fillRect({panelX, 20.0f, panelWidth, height - 40.0f}, {28, 42, 48, 255});
        engine.drawText(panelX + 20.0f, 44.0f, "ISLA VIVA", {238, 198, 135, 255});
        engine.drawText(panelX + 20.0f, 74.0f,
            "DIA " + std::to_string(static_cast<int>(elapsed_ / 300.0f) + 1), {184, 202, 197, 255});
        engine.drawText(panelX + 20.0f, 104.0f, paused_ ? "PAUSADO (ESPACIO)" : "EN MARCHA (ESPACIO)",
            {184, 202, 197, 255});
        for (std::size_t i = 0; i < residents_.size(); ++i) {
            const auto& resident = residents_[i];
            const float y = 154.0f + static_cast<float>(i) * 86.0f;
            engine.drawText(panelX + 20.0f, y, resident.name, {246, 225, 177, 255});
            engine.drawText(panelX + 20.0f, y + 24.0f,
                "COMIDA " + std::to_string(static_cast<int>(resident.hunger))
                    + "  AGUA " + std::to_string(static_cast<int>(resident.thirst)),
                {184, 202, 197, 255});
            engine.drawText(panelX + 20.0f, y + 48.0f,
                "ENERGIA " + std::to_string(static_cast<int>(resident.energy)), {184, 202, 197, 255});
        }
    }

private:
    ra::Texture terrainTexture_{};
    std::vector<Resident> residents_;
    float accumulator_{0.0f};
    float elapsed_{0.0f};
    bool paused_{false};
};

} // namespace

int main()
{
    ra::EngineConfig config;
    config.title = "Isla Viva";
    config.width = 1440;
    config.height = 900;
    config.resizable = true;
    ra::Engine engine(config);
    IslaVivaGame game;
    return engine.run(game);
}