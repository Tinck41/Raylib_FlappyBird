#include <limits>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "raylib.h"

constexpr int screenWidth = 800;
constexpr int screenHeight = 500;
constexpr float pipesGap = 75.f;
constexpr float pipesDistance = 100.f;
constexpr float pipesSpawnOffset = 1000.f;

const auto assetsDir = std::string(GetApplicationDirectory()) + "assets/";

std::random_device device;
std::mt19937 rng {device()};

float getRandomFloatInRang(float min, float max) {
	std::uniform_real_distribution<float> dist(min, max);
	return dist(rng);
}

struct Animation {
	int totalFrames;
	float accumulator;
	float speed;
};

struct Bird {
	float currentRotation;
	float targetRotation;
	float rotationDuration;
	float rotationSpeed;
	float fallTimer;
	Vector2 origin;
	Vector2 position;
	std::array<Texture2D, 3> textures;
	Animation animation;
};

void applyGravity(Bird& bird, float delta) {
	constexpr float gravity = 400.f;

	bird.position.y += gravity * delta;
}

void applyForce(Bird& bird, float delta, float force) {
	bird.position.y -= force * delta;
}

RenderTexture2D createBackground() {
	const auto bgDay = LoadTexture((assetsDir + "background-day.png").c_str());
	const auto numbBackground = std::ceil(static_cast<float>(screenWidth) / static_cast<float>(bgDay.width)) + 1;

	const auto background = LoadRenderTexture(bgDay.width * numbBackground, bgDay.height);

	BeginTextureMode(background);
	for (auto i = 0; i < numbBackground; ++i) {
		DrawTexture(bgDay, i * bgDay.width, 0, WHITE);
	}
	EndTextureMode();

	UnloadTexture(bgDay);

	return background;
}

RenderTexture2D createPipes() {
	const auto pipe = LoadTexture((assetsDir + "pipe-green.png").c_str());

	const auto pipes = LoadRenderTexture(pipe.width, pipe.height * 2.f + pipesGap);

	BeginTextureMode(pipes);
	DrawTexture(pipe, 0, pipesGap + pipe.height, WHITE);
	DrawTexturePro(
		pipe,
		{ 0, 0, static_cast<float>(pipe.width), static_cast<float>(pipe.height) },
		{ 0, static_cast<float>(pipe.height), static_cast<float>(pipe.width), static_cast<float>(pipe.height) },
		{ static_cast<float>(pipe.width), 0 },
		180,
		WHITE
	);
	EndTextureMode();

	UnloadTexture(pipe);

	return pipes;
}

void initPipes(const RenderTexture2D& pipes, std::vector<Vector2>& pipesArr, std::vector<Rectangle>& colliders) {
	const auto numPipes = std::ceil(static_cast<float>(screenWidth) / (static_cast<float>(pipes.texture.width) + pipesDistance)) + 1;

	pipesArr.reserve(numPipes);

	const auto pipeHeight = (pipes.texture.height - pipesGap) * 0.5f;
	const auto maxHeight = screenHeight * 0.5f - pipes.texture.height * 0.5f;
	const auto minHeight = maxHeight - 150.f;
	for (int i = 0; i < numPipes; i++) {
		const auto x = pipesSpawnOffset + i * (pipes.texture.width + pipesDistance);
		const auto y = getRandomFloatInRang(minHeight, maxHeight);

		pipesArr.emplace_back(x, y);
		colliders.emplace_back(x, y, pipes.texture.width, pipeHeight);
		colliders.emplace_back(x, y + pipeHeight + pipesGap, pipes.texture.width, pipeHeight);
	}
}

int main() {
	float speed = 100.f;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "FlappyBird");

	Bird bird = {
		.currentRotation = 0.f,
		.targetRotation = 0.f,
		.position = { screenWidth * 0.5, screenHeight * 0.5 },
		.textures = {
			LoadTexture((assetsDir + "yellowbird-downflap.png").c_str()),
			LoadTexture((assetsDir + "yellowbird-midflap.png").c_str()),
			LoadTexture((assetsDir + "yellowbird-upflap.png").c_str())
		},
		.animation = {
			.totalFrames = 3,
			.accumulator = 0.f,
			.speed = 5.f,
		}
	};

	bird.origin = { bird.textures[0].width * 0.5f, bird.textures[0].height * 0.5f };

	const auto background = createBackground();
	const auto pipes = createPipes();

	const auto base = LoadTexture((assetsDir + "base.png").c_str());

	SetTargetFPS(60);

	float force = 0;

	const auto numPlatform = std::ceil(static_cast<float>(screenWidth) / static_cast<float>(base.width)) + 1;

	std::vector<Vector2> platforms;
	std::vector<Vector2> pipesArr;

	platforms.reserve(numPlatform);

	for (int i = 0; i < numPlatform; i++) {
		platforms.emplace_back(i * base.width, screenHeight - base.height);
	}

	std::vector<Rectangle> pipeCollides;

	Rectangle platformCollier {
		.x = 0,
		.y = static_cast<float>(screenHeight - base.height),
		.width = screenWidth,
		.height = static_cast<float>(base.height),
	};

	bool drawColliders = true;
	bool gameOver = false;
	bool isRunning = false;
	bool useFixedDt = false;

	float checkPoint = 0;

	unsigned score = 0;

	constexpr auto fixedDt = 0.01f;

	while (!WindowShouldClose()) {
		const auto dt = useFixedDt ? fixedDt : GetFrameTime();

		if (IsKeyPressed(KEY_E)) {
			useFixedDt = !useFixedDt;
		}
		if (IsKeyPressed(KEY_R)) {
			pipesArr.clear();
			pipeCollides.clear();
			bird.position.x = screenWidth * 0.5f;
			bird.position.y = screenHeight * 0.5f;
			bird.targetRotation = 0.f;
			bird.currentRotation = 0.f;
			bird.rotationDuration = 0.f;
			gameOver = false;
			isRunning = false;
			speed = 100.f;
			force = 0;
			score = 0;
			checkPoint = 0;
		}
		if (IsKeyPressed(KEY_D)) {
			drawColliders = !drawColliders;
		}
		if (!gameOver && IsKeyPressed(KEY_W)) {
			isRunning = true;
			if (pipesArr.empty()) {
				initPipes(pipes, pipesArr, pipeCollides);
				checkPoint = pipesSpawnOffset;
			}
			force = 600.f;
			bird.targetRotation = -20.f;
			bird.rotationDuration = 0.1f;
			bird.rotationSpeed = (bird.targetRotation - bird.currentRotation) / bird.rotationDuration;
			bird.fallTimer = 0.5f;
		}

		if (bird.rotationDuration > 0) {
			bird.rotationDuration -= dt;
			bird.currentRotation +=  bird.rotationSpeed * dt;

			if (bird.rotationDuration <= 0) {
				bird.rotationDuration = 0.f;
				bird.currentRotation = bird.targetRotation;
			}
		}

		if (bird.fallTimer > 0) {
			bird.fallTimer -= dt;

			if (bird.fallTimer <= 0.f) {
				bird.fallTimer = 0.f;
				bird.targetRotation = 90.f;
				bird.rotationDuration = 0.5f;
				bird.rotationSpeed = (bird.targetRotation - bird.currentRotation) / bird.rotationDuration;
			}
		}

		if (isRunning) {
			applyGravity(bird, dt);
		}

		if (force > 0 && isRunning) {
			applyForce(bird, dt, force);
			force -= 700.f * dt;
		}

		for (const auto& collider : pipeCollides) {
			/*if (collider.x <= bird.position.x - bird.origin.x + bird.textures[0].width
				&& collider.x + collider.width >= bird.position.x - bird.origin.x
				&& collider.y <= bird.position.y - bird.origin.y  + bird.textures[0].height
				&& collider.y + collider.height >= bird.position.y - bird.origin.y) {*/
			if (CheckCollisionRecs({ bird.position.x - bird.origin.x, bird.position.y - bird.origin.y, float(bird.textures[0].width), float(bird.textures[0].height) }, collider)) {
				gameOver = true;
				speed = 0;
				break;
			}
		}

		if (CheckCollisionRecs({  bird.position.x - bird.origin.x, bird.position.y - bird.origin.y, float(bird.textures[0].width), float(bird.textures[0].height)  }, platformCollier)) {
			gameOver = true;
			speed = 0;
			bird.position.y = platformCollier.y;
		}

		if (isRunning && checkPoint <= bird.position.x) {
			score++;
			checkPoint += pipesDistance + pipes.texture.width;
		}

		if (!pipesArr.empty() && pipesArr.front().x + pipes.texture.width < 0) {
			auto pipe = *pipesArr.erase(pipesArr.begin());
			const auto maxHeight = screenHeight * 0.5f - pipes.texture.height * 0.5f;
			const auto minHeight = maxHeight - 150.f;
			pipe.x = pipesArr.back().x + pipes.texture.width + pipesDistance;
			pipe.y = minHeight + rand() % static_cast<int>(maxHeight);

			auto topCollider = *pipeCollides.erase(pipeCollides.begin());
			auto botCollider = *pipeCollides.erase(pipeCollides.begin());

			topCollider.x = pipe.x;
			botCollider.x = pipe.x;

			topCollider.y = pipe.y;
			botCollider.y = pipe.y + botCollider.height + pipesGap;

			pipesArr.push_back(pipe);
			pipeCollides.push_back(topCollider);
			pipeCollides.push_back(botCollider);
		}

		if (!gameOver) {
			bird.animation.accumulator += bird.animation.speed * dt;
		}

		checkPoint -= speed * dt;

		BeginDrawing();

		ClearBackground(SKYBLUE);

		DrawTextureRec(background.texture, (Rectangle) { 0, 0, (float)background.texture.width, -(float)background.texture.height }, (Vector2) { 0, static_cast<float>(screenHeight - background.texture.height) }, WHITE);

		for (auto& pos : pipesArr) {
			pos.x -= speed * dt;
			DrawTextureRec(pipes.texture, (Rectangle) { 0, 0, (float)pipes.texture.width, -(float)pipes.texture.height }, (Vector2) pos, WHITE);
		}

		for (auto& collider : pipeCollides) {
			collider.x -= speed * dt;
			if (drawColliders) {
				DrawRectangleRec(collider, RED);
			}
		}

		if (drawColliders) {
			DrawRectangleV({ bird.position.x - bird.origin.x, bird.position.y - bird.origin.y }, { float(bird.textures[0].width), float(bird.textures[0].height) }, RED);
		}

		if (platforms.front().x + base.width < 0) {
			auto platform = *platforms.erase(platforms.begin());
			platform.x = platforms.back().x + base.width;
			platforms.push_back(platform);
		}

		const auto index = static_cast<int>(bird.animation.accumulator) % bird.animation.totalFrames;
		//DrawTextureEx(bird.textures[index], bird.position, bird.currentRotation, 1.f, WHITE);
		DrawTexturePro(
			bird.textures[index],
			{ 0, 0, static_cast<float>(bird.textures[index].width), static_cast<float>(bird.textures[index].height) },
			{ bird.position.x, bird.position.y, static_cast<float>(bird.textures[index].width), static_cast<float>(bird.textures[index].height) },
			bird.origin,
			bird.currentRotation,
			WHITE
		);

		for (auto& pos : platforms) {
			pos.x -= speed * dt;
			DrawTexture(base, pos.x, pos.y, WHITE);
		}

		if (drawColliders) {
			DrawLine(checkPoint, 0, checkPoint, screenHeight, GREEN);
		}
		DrawText(std::to_string(score).c_str(), screenWidth * 0.5f, screenHeight * 0.1f, 46, WHITE);

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
