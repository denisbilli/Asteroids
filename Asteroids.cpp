// Asteroids.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Asteroids.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <chrono> 
#include <string>
#include <fstream>
#include <sstream>

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")
ULONG_PTR m_gdiplusToken;

#define MAX_LOADSTRING 100
#define PI 3.14159

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND mainWindowHwnd;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


// ---------------------------------------------------------------------------------- STRUTTURE DATI

const int GAME_TIMER = 20;
const int MAX_ASTEROIDS = 500;
const int MAX_POINTS = 5;
const int MAX_PROJECTILES = 500;
const float PROJECTILE_SPEED = 10.0f;
const int PROJECTILE_LIFESPAN = 40;
const int STARTING_ASTEROIDS = 3;
const int MAX_DEBRIS = 10;
const int EXPLOSION_FRAMES = 20;
const int MAX_EXPLOSIONS = 500;
const int MAX_TIME_OUTOFTHESCREEN = 1;
const int MAX_DIFFICULTY_LEVEL = 100;
const int MAX_LEVEL_FRAMES = 60;
const int MAX_GAMEOVER_FRAMES = 180;
const int LEADERBOARD_ENTRIES = 10;


struct Debris {
	int posX;
	int posY;
	int movX;
	int movY;
};

struct Explosion {
	Debris debris[MAX_DEBRIS];
	bool active;
	int frame;
};

struct Projectile {
	float posX;
	float posY;
	float movX;
	float movY;
	bool active;
	int frame;
};

enum AsteroidSize {
	SMALL = 10,
	MEDIUM = 20,
	BIG = 30
};

enum AsteroidMaterial {
	ROCK = 1,
	ICE = 2,
	IRON = 4,
	// add as many materials as you want
};

struct Asteroid {
	float movX;
	float movY;
	AsteroidSize size;
	AsteroidMaterial material;
	Gdiplus::PointF points[MAX_POINTS]; // Each asteroid will have an array of points
	bool active = false;
	int hitPoints;
	float centroidX;
	float centroidY;
	std::chrono::steady_clock::time_point lastSeen;
};

struct Spaceship {
	bool active;
	int posX;
	int posY;
	float movX; // Movement direction in X
	float movY; // Movement direction in Y
	float speed;  // Speed of the spaceship
	float acceleration;  // Acceleration of the spaceship
	float rotation;  // Rotation of the spaceship in degrees
	Gdiplus::PointF points[3]; // Spaceship will always be a triangle
	Gdiplus::PointF originalPoints[3]; // Original unrotated points
	float respawnTimer;  // in seconds
};

struct LevelDisplay {
	int frame;
	int maxFrames;
	int level;
	bool active;
};

struct GameOverDisplay {
	bool active;
	int frame;
	int maxFrames;
};

struct LeaderboardEntry {
	std::string name;
	int score;
};

int screenWidth = 0;
int screenHeight = 0;
int gameScore = 0;
int difficultyLevel = 0;
int activeAsteroids = 0;
int lives = 3;
bool DEBUG = false;

bool isGameActive = false;
bool isGamePaused = false;

Asteroid asteroids[MAX_ASTEROIDS];
Projectile projectiles[MAX_PROJECTILES] = { 0 };
Explosion explosions[MAX_EXPLOSIONS];
Spaceship spaceship;
LevelDisplay levelDisplay;
GameOverDisplay gameOverDisplay;
std::vector<LeaderboardEntry> leaderboard;
int asteroidMaterialHitPoints[] = {1, 8, 16};
int asteroidMaterialColor[] = { Gdiplus::Color::SaddleBrown, Gdiplus::Color::LightBlue, Gdiplus::Color::Gray };

std::vector<std::vector<Gdiplus::Point>> asteroidModels = {
	{ {-20, -10}, {-10, -25}, {10, -20}, {20, -10}, {20, 10}, {10, 20}, {-10, 20} }, // Heptagon 1
	{ {-10, -20}, {10, -20}, {25, 0}, {10, 20}, {-10, 20}, {-25, 0} }, // Hexagon
	{ {-20, -10}, {-10, -20}, {10, -20}, {20, -10}, {20, 10}, {10, 20}, {0, 25}, {-10, 20}, {-20, 10} }, // Nonagon
	{ {-20, -10}, {-10, -20}, {10, -20}, {20, 0}, {10, 20}, {0, 25}, {-10, 20}, {-20, 10} } // Octagon
};

// -------------------------------------------------------------------------------------------------

int numPointsFromSize(AsteroidSize size) {
	int numPoints = 3;
	switch(size) {
	case SMALL:
		numPoints = 3;
		break;
	case MEDIUM:
		numPoints = 4;
		break;
	case BIG:
		numPoints = 5;
		break;
	default:
		numPoints = 3;
	}
	return numPoints;
}

// -------------------------------------------------------------------------------- INIZIALIZZAZIONE

void createAsteroid(Asteroid& asteroid, int screenWidth, int screenHeight)
{
	// Calculate max asteroid speed based on difficulty
	int maxAsteroidSpeed = 2 + difficultyLevel * 1;

	int edge = rand() % 4; // choose a random edge: 0 = top, 1 = right, 2 = bottom, 3 = left
	int randomPosX, randomPosY;

	switch(edge) {
	case 0: // Top edge
		randomPosX = rand() % (screenWidth + 1);
		randomPosY = 0;
		break;
	case 1: // Right edge
		randomPosX = screenWidth;
		randomPosY = rand() % (screenHeight + 1);
		break;
	case 2: // Bottom edge
		randomPosX = rand() % (screenWidth + 1);
		randomPosY = screenHeight;
		break;
	case 3: // Left edge
		randomPosX = 0;
		randomPosY = rand() % (screenHeight + 1);
		break;
	}

	float randomMovX = ((float)(rand() % 401) - 200) / 100;
	if(randomMovX == 0) {
		randomMovX = (rand() % 2) ? 0.1 : -0.1;
	}
	float randomMovY = ((float)(rand() % 401) - 200) / 100;
	if(randomMovY == 0) {
		randomMovY = (rand() % 2) ? 0.1 : -0.1;
	}

	asteroid.movX = randomMovX; 
	asteroid.movY = randomMovY;

	// Initialize the points for each asteroid
	int numPoints = numPointsFromSize(asteroid.size);
	float radius = (float)((int)asteroid.size + 20); // Using the size as the radius

	// Perturbation factor - you may want to adjust this
	float perturbation = (float)(((int)asteroid.size) / 4.0f);

	for(int j = 0; j < numPoints; j++) {
		// Distribute points evenly around the circle, adding a bit of random perturbation
		float angle = (float)(((float)j / numPoints) * 2 * PI);
		float randomRadius = radius + static_cast<float>(rand()) / RAND_MAX * perturbation - perturbation / 2;

		asteroid.points[j].X = (int)(randomPosX + randomRadius * cos(angle));
		asteroid.points[j].Y = (int)(randomPosY + randomRadius * sin(angle));
	}

	float sumX = 0, sumY = 0;
	for(int j = 0; j < numPoints; ++j) {
		sumX += asteroid.points[j].X;
		sumY += asteroid.points[j].Y;
	}
	asteroid.centroidX = sumX / numPoints;
	asteroid.centroidY = sumY / numPoints;

	// Determine asteroid type based on the difficultyLevel
	if(difficultyLevel < 15) {
		asteroid.material = ROCK;
	}
	else if(difficultyLevel < 35) {
		// Generate a random number between 0 and 1
		float random = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		if(random < 0.5) {  // Half the time it should be ROCK
			asteroid.material = ROCK;
		}
		else {  // The other half it should be ICE
			asteroid.material = ICE;
		}
	}
	else {
		// Generate a random number between 0 and 1
		float random = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float ironThreshold = static_cast <float> (difficultyLevel - 35) / 65.0f;
		if(random < ironThreshold) {
			asteroid.material = IRON;
		}
		else if(random < (ironThreshold + (1.0f - ironThreshold) / 2)) {
			asteroid.material = ICE;
		}
		else {
			asteroid.material = ROCK;
		}
	}
	asteroid.hitPoints = asteroidMaterialHitPoints[asteroid.material];

	asteroid.active = true;
}

//Initialize asteroids
void createAsteroids(Asteroid asteroids[], int screenWidth, int screenHeight, int difficultyLevel) {
	// Calculate number of asteroids based on difficulty
	int numAsteroids = std::fminf(STARTING_ASTEROIDS + difficultyLevel * 5, 150);

	for(int i = 0; i < numAsteroids; i++) {
		// Size is inversely proportional to difficulty
		AsteroidSize randomSize = static_cast<AsteroidSize>((rand() % 3 + 1) * 10);
		asteroids[i].size = randomSize;
		createAsteroid(asteroids[i], screenWidth, screenHeight);
	}

	activeAsteroids = numAsteroids;
}

void createSpaceship(Spaceship& spaceship, int screenWidth, int screenHeight) {
	spaceship.movX = 0;
	spaceship.movY = 0;
	spaceship.posX = (int)(screenWidth / 2);
	spaceship.posY = (int)(screenHeight / 2);
	spaceship.speed = 0.0f;  // speed
	spaceship.acceleration = 0.0f;  // acceleration
	spaceship.rotation = 0.0f;  // rotation

	spaceship.points[0] = Gdiplus::PointF(screenWidth / 2, screenHeight / 2 - 20);
	spaceship.points[1] = Gdiplus::PointF(screenWidth / 2 - 10, screenHeight / 2 + 10);
	spaceship.points[2] = Gdiplus::PointF(screenWidth / 2 + 10, screenHeight / 2 + 10);

	spaceship.originalPoints[0] = spaceship.points[0];
	spaceship.originalPoints[1] = spaceship.points[1];
	spaceship.originalPoints[2] = spaceship.points[2];

	spaceship.active = true;
}

void createExplosion(int centerX, int centerY) {
	// Find the first inactive explosion
	for(int i = 0; i < MAX_EXPLOSIONS; i++) {
		if(!explosions[i].active) {
			explosions[i].active = true;
			explosions[i].frame = 0;

			// Generate debris
			for(int j = 0; j < MAX_DEBRIS; j++) {
				explosions[i].debris[j].posX = centerX;
				explosions[i].debris[j].posY = centerY;
				explosions[i].debris[j].movX = rand() % 21 - 10; // random movement between -10 and 10
				explosions[i].debris[j].movY = rand() % 21 - 10;
			}

			return;
		}
	}
}

void createProjectile(Projectile projectiles[], Spaceship& spaceship) {
	// Find an inactive projectile slot
	for(int i = 0; i < MAX_PROJECTILES; ++i) {
		if(!projectiles[i].active) {
			// Set the new projectile's position to match the spaceship's
			projectiles[i].posX = (float)spaceship.points[0].X;
			projectiles[i].posY = (float)spaceship.points[0].Y;
			// Compute the spaceship's direction vector
			float radian = (float)((spaceship.rotation - 90) * PI / 180.0);
			projectiles[i].movX = std::cos(radian);
			projectiles[i].movY = std::sin(radian);
			// Mark the projectile as active
			projectiles[i].active = true;
			projectiles[i].frame = 0;
			break;
		}
	}
}

void createLevelPassed(LevelDisplay& levelDisplay) {
	levelDisplay.active = true;
	levelDisplay.frame = 0;
	levelDisplay.level = difficultyLevel;
	levelDisplay.maxFrames = MAX_LEVEL_FRAMES;
}

void createGameOver(GameOverDisplay& gameOverDisplay) {
	if(!gameOverDisplay.active) {
		gameOverDisplay.active = true;
		gameOverDisplay.frame = 0;
		gameOverDisplay.maxFrames = MAX_GAMEOVER_FRAMES;
	}
}

// -------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------- FUNZIONI AUSILIARIE

int countActiveAsteroids(Asteroid asteroids[]) {
	int count = 0;
	for(int i = 0; i < MAX_ASTEROIDS; i++) {
		if(asteroids[i].active) {
			count++;
		}
	}
	return count;
}

float angleBetweenVectors(float x1, float y1, float x2, float y2) {
	float dot_product = x1 * x2 + y1 * y2;
	float magnitude_a = std::sqrt(x1 * x1 + y1 * y1);
	float magnitude_b = std::sqrt(x2 * x2 + y2 * y2);
	return (float)(std::acos(dot_product / (magnitude_a * magnitude_b)) * 180.0f / PI);
}

Gdiplus::PointF getCenter(const Spaceship& spaceship) {
	float centerX = (spaceship.points[0].X + spaceship.points[1].X + spaceship.points[2].X) / 3;
	float centerY = (spaceship.points[0].Y + spaceship.points[1].Y + spaceship.points[2].Y) / 3;
	return Gdiplus::PointF(centerX, centerY);
}

void rotatePoint(Gdiplus::PointF& point, Gdiplus::PointF origin, double angle) {
	// Convert angle in degrees to radians
	double rad = angle * PI / 180.0;
	double sin = std::sin(rad);
	double cos = std::cos(rad);

	// Translate point back to origin:
	point.X -= origin.X;
	point.Y -= origin.Y;

	// Rotate point
	double newX = point.X * cos - point.Y * sin;
	double newY = point.X * sin + point.Y * cos;

	// Translate point back:
	point.X = newX + origin.X;
	point.Y = newY + origin.Y;
}

// Function to update the position XY of the asteroid
void updateAsteroid(Asteroid& asteroid, int screenWidth, int screenHeight) {
	// If the asteroid is not active, do not proceed
	if(!asteroid.active) {
		return;
	}

	// Variables to hold the minimum and maximum X and Y of the asteroid points
	int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
	float sumX = 0, sumY = 0;
	auto numPoints = numPointsFromSize(asteroid.size);

	// Update the position XY of each point of the asteroid and calculate the min/max
	for(int i = 0; i < numPoints; i++) {
		asteroid.points[i].X += asteroid.movX;
		asteroid.points[i].Y += asteroid.movY;

		// Update min/max
		if(asteroid.points[i].X < minX) minX = asteroid.points[i].X;
		if(asteroid.points[i].X > maxX) maxX = asteroid.points[i].X;
		if(asteroid.points[i].Y < minY) minY = asteroid.points[i].Y;
		if(asteroid.points[i].Y > maxY) maxY = asteroid.points[i].Y;

		sumX += asteroid.points[i].X;
		sumY += asteroid.points[i].Y;
	}

	// Aggiorno la posizione del centroide dell'asteroide
	asteroid.centroidX = sumX / numPoints;
	asteroid.centroidY = sumY / numPoints;

	// Check if the asteroid is completely outside the screen
	if(asteroid.centroidX > screenWidth || asteroid.centroidX < 0 || asteroid.centroidY > screenHeight || asteroid.centroidY < 0) {
		// If the asteroid has been off-screen for more than a given threshold (here MAX_TIME_OUTOFTHESCREEN seconds), 
		// reposition it to a random position at the edge of the screen
		if(std::chrono::steady_clock::now() - asteroid.lastSeen > std::chrono::seconds(MAX_TIME_OUTOFTHESCREEN)) {
			createAsteroid(asteroid, screenWidth, screenHeight);
		}
	}
	else {
		// Update the timestamp when the asteroid is on the screen
		asteroid.lastSeen = std::chrono::steady_clock::now();
	}
}

// Function to update the position XY of the spaceship
void updateDirectionVector(Spaceship& spaceship) {
	float rotation = (spaceship.rotation - 90);

	// Compute new direction vector.
	float newMovX = (float)std::cos(rotation * PI / 180.0);
	float newMovY = (float)std::sin(rotation * PI / 180.0);

	// Now, adjust the spaceship's motion vector to be the sum of the current motion vector and the new direction vector.
	spaceship.movX += newMovX;
	spaceship.movY += newMovY;

	// Normalize the resultant vector to keep the magnitude consistent.
	float magnitude = (float)std::sqrt(spaceship.movX * spaceship.movX + spaceship.movY * spaceship.movY);
	spaceship.movX /= magnitude;
	spaceship.movY /= magnitude;
}

void updateSpaceship(Spaceship& spaceship, int screenWidth, int screenHeight) {
	// Apply acceleration to speed
	spaceship.speed += spaceship.acceleration;

	// Calculate movement vector based on speed and direction
	spaceship.posX += (int)(spaceship.speed * spaceship.movX);
	spaceship.posY += (int)(spaceship.speed * spaceship.movY);

	// Check screen boundaries
	if(spaceship.posX < 0) spaceship.posX += screenWidth;
	else if(spaceship.posX > screenWidth) spaceship.posX -= screenWidth;
	if(spaceship.posY < 0) spaceship.posY += screenHeight;
	else if(spaceship.posY > screenHeight) spaceship.posY -= screenHeight;

	// Update the spaceship points from the originalPoints
	for(int i = 0; i < 3; i++) {
		spaceship.points[i] = spaceship.originalPoints[i];
		spaceship.points[i].X += spaceship.posX - screenWidth / 2;
		spaceship.points[i].Y += spaceship.posY - screenHeight / 2;

		// Rotate points around the spaceship center
		rotatePoint(spaceship.points[i], Gdiplus::PointF(spaceship.posX, spaceship.posY), spaceship.rotation);
	}
}

// Function to update the explosions
void updateExplosions() {
	for(int i = 0; i < MAX_EXPLOSIONS; i++) {
		if(explosions[i].active) {
			if(++explosions[i].frame > EXPLOSION_FRAMES) {
				explosions[i].active = false; // explosion is over, deactivate it
			}
			else {
				for(int j = 0; j < MAX_DEBRIS; j++) {
					// Update debris position
					explosions[i].debris[j].posX += explosions[i].debris[j].movX;
					explosions[i].debris[j].posY += explosions[i].debris[j].movY;
				}
			}
		}
	}
}

// Function to update the position XY of the projectiles
void updateProjectiles(Projectile projectiles[]) {
	for(int i = 0; i < MAX_PROJECTILES; ++i) {
		if(projectiles[i].active) {
			if(++projectiles[i].frame > PROJECTILE_LIFESPAN) {
				projectiles[i].active = false; // explosion is over, deactivate it
				continue;
			}
			// Move the projectile
			projectiles[i].posX += PROJECTILE_SPEED * projectiles[i].movX;
			projectiles[i].posY += PROJECTILE_SPEED * projectiles[i].movY;
			// If the projectile is off-screen, deactivate it
			if(projectiles[i].posX < 0 || projectiles[i].posX > screenWidth || projectiles[i].posY < 0 || projectiles[i].posY > screenHeight) {
				projectiles[i].active = false;
			}
		}
	}
}


// -------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------- LEADERBOARD

void saveLeaderboard(const std::string& filename) {
	std::ofstream file(filename);

	for(const LeaderboardEntry& entry : leaderboard) {
		file << entry.name << " " << entry.score << "\n";
	}

	file.close();
}

void loadLeaderboard(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;

	while(std::getline(file, line)) {
		std::istringstream iss(line);
		std::string name;
		int score;

		if(!(iss >> name >> score)) {
			break;  // error
		}

		leaderboard.push_back({ name, score });
	}

	file.close();
}

void addScoreToLeaderboard(const std::string& name, int score) {
	leaderboard.push_back({ name, score });

	// Sort the leaderboard in descending order by score
	std::sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
		return a.score > b.score;
	});

	// Keep only the top 10 scores
	if(leaderboard.size() > 20) {
		leaderboard.resize(20);
	}
}

// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------- COLLISIONI

bool hasCollision(Projectile& projectile, Asteroid& asteroid) {
	int numPoints = numPointsFromSize(asteroid.size);
	int intersections = 0;
	for(size_t i = 0; i < numPoints; i++) {
		Gdiplus::PointF p1 = asteroid.points[i];
		Gdiplus::PointF p2 = asteroid.points[(i + 1) % numPoints];
		if(projectile.posY > std::fmin(p1.Y, p2.Y) && projectile.posY <= std::fmax(p1.Y, p2.Y) && projectile.posX < std::fmax(p1.X, p2.X)) {
			if(p1.Y != p2.Y) {
				double x = (projectile.posY - p1.Y) * (p2.X - p1.X) / (p2.Y - p1.Y) + p1.X;
				if(x > projectile.posX) {
					intersections++;
				}
			}
		}
	}
	return (intersections % 2 != 0);
}

float DotProduct(const Gdiplus::PointF& a, const Gdiplus::PointF& b) {
	return (a.X * b.X) + (a.Y * b.Y);
}

float Magnitude(const Gdiplus::PointF& vector) {
	return std::sqrt((vector.X * vector.X) + (vector.Y * vector.Y));
}

Gdiplus::PointF GetNormal(const Gdiplus::PointF& edge) {
	return Gdiplus::PointF(-edge.Y, edge.X);
}

bool CheckCollision(const Gdiplus::PointF* polygon1, int count1, const Gdiplus::PointF* polygon2, int count2) {
	// Iterate through the edges of both polygons
	for(int i = 0; i < count1 + count2; ++i) {
		const Gdiplus::PointF* polygon = (i < count1) ? polygon1 : polygon2;
		int count = (i < count1) ? count1 : count2;

		// Get the current edge
		Gdiplus::PointF currentEdge = polygon[i % count] - polygon[(i + 1) % count];

		// Get the normal vector of the current edge
		Gdiplus::PointF axis = GetNormal(currentEdge);

		// Normalize the axis
		float axisLength = Magnitude(axis);
		axis.X /= axisLength;
		axis.Y /= axisLength;

		// Project the vertices of both polygons onto the axis
		float minProjection1 = DotProduct(axis, polygon1[0]);
		float maxProjection1 = minProjection1;
		float minProjection2 = DotProduct(axis, polygon2[0]);
		float maxProjection2 = minProjection2;

		for(int j = 1; j < count1; ++j) {
			float projection = DotProduct(axis, polygon1[j]);
			if(projection < minProjection1) {
				minProjection1 = projection;
			}
			else if(projection > maxProjection1) {
				maxProjection1 = projection;
			}
		}

		for(int j = 1; j < count2; ++j) {
			float projection = DotProduct(axis, polygon2[j]);
			if(projection < minProjection2) {
				minProjection2 = projection;
			}
			else if(projection > maxProjection2) {
				maxProjection2 = projection;
			}
		}

		// Check for overlap
		if(maxProjection1 < minProjection2 || maxProjection2 < minProjection1) {
			// No overlap, polygons do not collide
			return false;
		}
	}

	// All axes had overlap, polygons collide
	return true;
}

bool hasCollision(Spaceship& spaceship, Asteroid& asteroid) {
	return CheckCollision(spaceship.points, 3, asteroid.points, numPointsFromSize(asteroid.size));
}

void handleCollisions(Projectile projectiles[], Asteroid asteroids[], Spaceship& spaceship) {
	for(int projIdx = 0; projIdx < MAX_PROJECTILES; ++projIdx) {
		if(projectiles[projIdx].active) {
			for(int astIdx = 0; astIdx < MAX_ASTEROIDS; ++astIdx) {
				if(asteroids[astIdx].active) {
					if(hasCollision(projectiles[projIdx], asteroids[astIdx])) {
						// The projectile hit the asteroid
						projectiles[projIdx].active = false;  // Deactivate the projectile

						createExplosion((int)projectiles[projIdx].posX, (int)projectiles[projIdx].posY);

						// Decrease the hit points of the asteroid
						asteroids[astIdx].hitPoints--;

						// Check if the asteroid still has hit points
						if(asteroids[astIdx].hitPoints > 0) {
							continue; // The asteroid was not destroyed, so return
						}

						// Handle the asteroid depending on its size
						switch(asteroids[astIdx].size) {
						case BIG:
							// Split the big asteroid into two small ones
							for(int k = 0; k < MAX_ASTEROIDS; ++k) {
								if(!asteroids[k].active) {
									asteroids[k] = asteroids[astIdx];  // Copy the current asteroid
									asteroids[k].movX *= -1;  // Change the direction of the new asteroid
									asteroids[k].movY *= -1;
									asteroids[k].size = SMALL;
									asteroids[k].active = true;
									break;
								}
							}
							asteroids[astIdx].movX *= -1;
							asteroids[astIdx].movY *= 1;
							asteroids[astIdx].size = MEDIUM;
							asteroids[astIdx].active = true;
							gameScore += 2 * asteroids[astIdx].material;
							break;
						case MEDIUM:
							// Split the big asteroid into two small ones
							asteroids[astIdx].movX *= -1;
							asteroids[astIdx].movY *= -1;
							asteroids[astIdx].size = SMALL;
							asteroids[astIdx].active = true;
							gameScore += 3 * asteroids[astIdx].material;
							break;
						case SMALL:
							// The small asteroid is destroyed
							asteroids[astIdx].active = false;
							gameScore += 5 * asteroids[astIdx].material;
							break;
						}

						break;  // Break out of the asteroid loop
					}
				}
			}
		}
	}

	// Checking collisions between spaceship and asteroids
	for(int astIdx = 0; astIdx < MAX_ASTEROIDS; ++astIdx) {
		if(asteroids[astIdx].active) {
			if(hasCollision(spaceship, asteroids[astIdx])) {
				// The spaceship hit the asteroid

				// Handle the collision as you see fit, such as deactivating the spaceship and creating an explosion
				createExplosion((int)spaceship.posX, (int)spaceship.posY);
				if(spaceship.active) {
					spaceship.active = false;
					lives--;

					// Start the respawn timer
					spaceship.respawnTimer = 3.0f;  // 3 seconds respawn time
				}
			}
		}
	}
}


// ----------------------------------------------------------------------------------------- DISEGNO

void drawExplosions(HDC hdc) {
	for(int i = 0; i < MAX_EXPLOSIONS; i++) {
		if(explosions[i].active) {
			for(int j = 0; j < MAX_DEBRIS; j++) {
				Gdiplus::Graphics graphics(hdc);
				// Calculate remaining life as a ratio
				float lifeRemaining = (float)(EXPLOSION_FRAMES - explosions[i].frame) / EXPLOSION_FRAMES;

				// Create an alpha value based on remaining life
				int alpha = (int)(255 * std::fminf(lifeRemaining / 0.1f, 1.0f));

				// Set brush color with alpha transparency
				Gdiplus::SolidBrush brush(Gdiplus::Color(alpha, 255, 255, 0)); // Yellow color with alpha transparency

				graphics.FillEllipse(&brush, explosions[i].debris[j].posX, explosions[i].debris[j].posY, 3, 3);
			}

			explosions[i].frame++;
			if(explosions[i].frame >= EXPLOSION_FRAMES) {
				explosions[i].active = false;
			}
		}
	}
}

void drawProjectiles(HDC hdc, Projectile projectiles[]) {
	if(!isGameActive)
		return;

	// Draw the projectile as a small circle
	Gdiplus::Graphics graphics(hdc);

	for(int i = 0; i < MAX_PROJECTILES; ++i) {
		if(projectiles[i].active) {
			// Calculate how much of the projectile's life remains
			float lifeRemaining = (float)(PROJECTILE_LIFESPAN - projectiles[i].frame) / PROJECTILE_LIFESPAN;
			// Fade the projectile to black as it gets close to the end of its life
			int alpha = (int)(255 * std::fminf(lifeRemaining / 0.1f, 1.0f));  // Transparency
			Gdiplus::SolidBrush brush(Gdiplus::Color(alpha, 255, 255, 255));  // White color with alpha transparency

			graphics.FillEllipse(&brush, (int)(projectiles[i].posX - 2), (int)(projectiles[i].posY - 2), 4, 4);
		}
	}
}

void drawLeaderboard(HDC hdc, float startX, float startY, float width, float lineHeight) {
	Gdiplus::Graphics graphics(hdc);
	Gdiplus::FontFamily fontFamily(L"Courier New");
	Gdiplus::Font font(&fontFamily, 18, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 255));

	for(int i = 0; i < leaderboard.size(); ++i) {
		std::wstring name = std::wstring(leaderboard[i].name.begin(), leaderboard[i].name.end());
		std::wstring text = std::to_wstring(i + 1) + L". " + name + L": " + std::to_wstring(leaderboard[i].score);

		Gdiplus::PointF pointF(startX + 10, (startY + 10) + i * 20);
		graphics.DrawString(text.c_str(), -1, &font, pointF, &brush);
	}
}

void drawRestartGame(HDC hdc) {
	if(!isGameActive && !gameOverDisplay.active) {
		Gdiplus::Graphics graphics(hdc);

		// Create a font 
		Gdiplus::FontFamily  fontFamily(L"Courier New");
		Gdiplus::Font        font(&fontFamily, 28, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		// Define the y-offset from the top of the window
		float textTopOffset = screenHeight * 0.2f; // Adjust this as needed
		Gdiplus::RectF leaderboardRectF(screenWidth * 0.2f, textTopOffset, screenWidth * 0.6f, screenHeight * 0.6f);
		Gdiplus::RectF rectF(0, leaderboardRectF.Height / 2 + 20, screenWidth, screenHeight);

		// Create a StringFormat object
		Gdiplus::StringFormat stringFormat;
		stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		// Create a SolidBrush object
		Gdiplus::SolidBrush solidBrush(Gdiplus::Color::White);  // White color with alpha transparency

		// Draw the string
		std::wstring levelText = L"PRESS SPACE TO START A NEW GAME";
		graphics.DrawString(levelText.c_str(), -1, &font, rectF, &stringFormat, &solidBrush);

		// Create a rectangle for the leaderboard
		Gdiplus::Pen rectPen = Gdiplus::Pen(Gdiplus::Color::White, 3);
		graphics.DrawRectangle(&rectPen, leaderboardRectF);

		// Draw the leaderboard title
		Gdiplus::Font fontLeaderboard(&fontFamily, 18, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		std::wstring leaderboardTitle = L"LEADERBOARD";
		graphics.DrawString(leaderboardTitle.c_str(), -1, &fontLeaderboard, Gdiplus::PointF(screenWidth * 0.5f, textTopOffset - 20), &stringFormat, &solidBrush);

		drawLeaderboard(hdc, leaderboardRectF.X, leaderboardRectF.Y, leaderboardRectF.Width, leaderboardRectF.Height);
	}
}

void drawLevelDisplay(HDC hdc, LevelDisplay& levelDisplay) {
	if(levelDisplay.active) {
		Gdiplus::Graphics graphics(hdc);

		// Calculate how much to scale the text based on the current frame
		float scale = 1.0f + (levelDisplay.frame / (float)levelDisplay.maxFrames) * 2.0f;

		// Create a font 
		Gdiplus::FontFamily  fontFamily(L"Courier New");
		Gdiplus::Font        font(&fontFamily, 24 * scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		// Create a rectangle for the string
		Gdiplus::RectF rectF(0, 0, screenWidth, screenHeight);

		// Create a StringFormat object
		Gdiplus::StringFormat stringFormat;
		stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		// Calculate remaining life as a ratio
		float lifeRemaining = (float)(levelDisplay.maxFrames - levelDisplay.frame) / levelDisplay.maxFrames;
		// Create an alpha value based on remaining life
		int alpha = (int)(255 * std::fminf(lifeRemaining / 0.1f, 1.0f));

		// Create a SolidBrush object
		Gdiplus::SolidBrush solidBrush(Gdiplus::Color(alpha, 255, 255, 255));  // White color with alpha transparency

		// Draw the string
		std::wstring levelText = L"LEVEL " + std::to_wstring(levelDisplay.level) + L" CLEARED";
		graphics.DrawString(levelText.c_str(), -1, &font, rectF, &stringFormat, &solidBrush);

		levelDisplay.frame++;
		if(levelDisplay.frame >= levelDisplay.maxFrames) {
			levelDisplay.active = false;
		}
	}
}

void drawGameOverDisplay(HDC hdc, GameOverDisplay& gameOverDisplay) {
	if(gameOverDisplay.active) {
		Gdiplus::Graphics graphics(hdc);

		// Calculate how much to scale the text based on the current frame
		float scale = 1.0f + (gameOverDisplay.frame / (float)gameOverDisplay.maxFrames) * 2.0f;

		// Create a font 
		Gdiplus::FontFamily  fontFamily(L"Courier New");
		Gdiplus::Font        font(&fontFamily, 24 * scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		// Create a rectangle for the string
		Gdiplus::RectF rectF(0, 0, screenWidth, screenHeight);

		// Create a StringFormat object
		Gdiplus::StringFormat stringFormat;
		stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		// Calculate remaining life as a ratio
		float lifeRemaining = (float)(gameOverDisplay.maxFrames - gameOverDisplay.frame) / gameOverDisplay.maxFrames;
		// Create an alpha value based on remaining life
		int alpha = (int)(255 * std::fminf(lifeRemaining / 0.1f, 1.0f));

		// Create a SolidBrush object
		Gdiplus::SolidBrush solidBrush(Gdiplus::Color(alpha, 255, 0, 0));  // White color with alpha transparency

		// Draw the string
		std::wstring gameOverText = L"GAME OVER";
		graphics.DrawString(gameOverText.c_str(), -1, &font, rectF, &stringFormat, &solidBrush);

		gameOverDisplay.frame++;
		if(gameOverDisplay.frame >= gameOverDisplay.maxFrames) {
			gameOverDisplay.active = false;
		}
	}
}

void drawPausedGame(HDC hdc) {
	if(isGamePaused) {
		Gdiplus::Graphics graphics(hdc);

		// Create a font 
		Gdiplus::FontFamily  fontFamily(L"Courier New");
		Gdiplus::Font        font(&fontFamily, 28, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		// Define the y-offset from the top of the window
		Gdiplus::RectF rectF(0, 0, screenWidth, screenHeight);

		// Create a StringFormat object
		Gdiplus::StringFormat stringFormat;
		stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		// Create a SolidBrush object
		Gdiplus::SolidBrush solidBrush(Gdiplus::Color::White);  // White color with alpha transparency

		// Draw the string
		std::wstring levelText = L"PAUSED";
		graphics.DrawString(levelText.c_str(), -1, &font, rectF, &stringFormat, &solidBrush);
	}
}

void Paint(HDC hdc) {
	Gdiplus::Graphics graphics(hdc);
	Gdiplus::Pen penSpaceship(Gdiplus::Color::White);

	// Start drawing your debug information
	Gdiplus::Font font(L"Courier New", 14);
	Gdiplus::Font debugInfoFont(L"Courier New", 8);
	Gdiplus::PointF pointF(10.0f, 10.0f);
	Gdiplus::PointF debugInfoPointF(screenWidth - 300, 10.0f);
	Gdiplus::SolidBrush solidBrush(Gdiplus::Color::LimeGreen);
	Gdiplus::SolidBrush debugInfoSolidBrush(Gdiplus::Color::Red);

	for(int i = 0; i < MAX_ASTEROIDS; i++) {
		if(!asteroids[i].active) {
			continue;
		}
		int numPoints = numPointsFromSize(asteroids[i].size);

		if(DEBUG) {
			std::wostringstream stream;
			stream << i << std::endl;
			stream << asteroids[i].hitPoints << std::endl;
			std::wstring str = stream.str();
			Gdiplus::PointF pointF(asteroids[i].centroidX, asteroids[i].centroidY);

			graphics.DrawString(str.c_str(), -1, &debugInfoFont, pointF, &debugInfoSolidBrush);
		}

		Gdiplus::Pen pen(asteroidMaterialColor[asteroids[i].material]);
		graphics.DrawPolygon(&pen, asteroids[i].points, numPoints);
	}

	// Drawing spaceship
	if(spaceship.active) {
		graphics.DrawPolygon(&penSpaceship, spaceship.points, 3);
	}

	std::wostringstream stream;
	stream << L"LEVEL: " << (difficultyLevel + 1) << std::endl;
	stream << L"ACTIVE ASTEROIDS: " << activeAsteroids << std::endl;
	stream << L"GAME SCORE: " << gameScore << std::endl;
	stream << L"LIVES: " << lives << std::endl;
	std::wstring str = stream.str();

	graphics.DrawString(str.c_str(), -1, &font, pointF, &solidBrush);


	// DEBUG INFO
	if(DEBUG) {
		std::wostringstream debugStream;
		debugStream << L"PosX: " << spaceship.posX << L" PosY: " << spaceship.posY << std::endl;
		debugStream << L"Speed: " << spaceship.speed << L" Acc: " << spaceship.acceleration << std::endl;
		debugStream << L"MovX: " << spaceship.movX << L" MovY: " << spaceship.movY << std::endl;
		debugStream << L"Rotation: " << spaceship.rotation << std::endl;
		debugStream << L"screenW: " << screenWidth << L" screenH: " << screenHeight << std::endl;

		for(size_t i = 0; i < MAX_ASTEROIDS; i++)
		{
			if(asteroids[i].active) {
				debugStream << L"[ASTEROID: " << i << "]" << std::endl;
				debugStream << L" PosX: " << asteroids[i].centroidX << L" PosY: " << asteroids[i].centroidY << std::endl;
				debugStream << L" MovX: " << asteroids[i].movX << L" MovY: " << asteroids[i].movY << std::endl;
				auto lastSeen = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - asteroids[i].lastSeen).count();
				debugStream << L" LastSeen: " << lastSeen << std::endl;
			}
		}

		std::wstring debugStr = debugStream.str();

		graphics.DrawString(debugStr.c_str(), -1, &debugInfoFont, debugInfoPointF, &debugInfoSolidBrush);
	}
	// END DEBUG


	drawProjectiles(hdc, projectiles);
	drawExplosions(hdc);

	drawGameOverDisplay(hdc, gameOverDisplay);
	drawRestartGame(hdc);
	drawLevelDisplay(hdc, levelDisplay);
	drawPausedGame(hdc);
}

// -------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------- STRUTTURA

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	srand(static_cast<unsigned>(time(0)));

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_ASTEROIDS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if(!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ASTEROIDS));

	MSG msg;
	loadLeaderboard("leaderboard.dat");

	// Main message loop:
	while(true)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if(msg.message == WM_QUIT)
			{
				break;
			}
		}
		else
		{
			// Se il gioco non è in pausa, posso aggiornare la dinamica di gioco
			if(!isGamePaused) {
				// Check key states
				if(GetAsyncKeyState(VK_LEFT)) {
					spaceship.rotation -= 5;
					spaceship.rotation = (float)fmod(spaceship.rotation, 360);
					if(spaceship.rotation < 0) spaceship.rotation += 360; // ensure it's always positive
				}
				if(GetAsyncKeyState(VK_RIGHT)) {
					spaceship.rotation += 5;
					spaceship.rotation = (float)fmod(spaceship.rotation, 360);
				}
				if(GetAsyncKeyState(VK_UP)) {
					spaceship.speed += 0.1f;
					updateDirectionVector(spaceship);
				}
				if(GetAsyncKeyState(VK_DOWN)) {
					// Calculate current direction vector
					float rotation = (spaceship.rotation - 90);
					float dirX = (float)std::cos(rotation * PI / 180.0);
					float dirY = (float)std::sin(rotation * PI / 180.0);

					// Calculate the angle between the movement direction and the current direction
					float angle = angleBetweenVectors(spaceship.movX, spaceship.movY, dirX, dirY);

					// If the angle is large (greater than 90 degrees), apply a larger deceleration
					if(angle > 90.0f)
						spaceship.speed -= 0.2f;  // Increase the deceleration
					else
						spaceship.speed -= 0.1f;  // Normal deceleration
				}
				if(GetAsyncKeyState(VK_SPACE)) {
					if(isGameActive) {
						createProjectile(projectiles, spaceship);
					}
					else {
						if(!gameOverDisplay.active) {
							if(!spaceship.active) {
								// Respawn the spaceship
								lives = 3;
								difficultyLevel = 0;
								createAsteroids(asteroids, screenWidth, screenHeight, difficultyLevel);
								createSpaceship(spaceship, screenWidth, screenHeight);
							}
							isGameActive = true;
						}
					}
				}

				// Update game state
				for(int i = 0; i < MAX_ASTEROIDS; i++) {
					updateAsteroid(asteroids[i], screenWidth, screenHeight);
				}
				updateSpaceship(spaceship, screenWidth, screenHeight);
			}

			if(isGameActive && !isGamePaused) {
				updateProjectiles(projectiles);
				updateExplosions();

				activeAsteroids = countActiveAsteroids(asteroids);

				handleCollisions(projectiles, asteroids, spaceship);

				if(lives == 0) {
					addScoreToLeaderboard("PLAYER", gameScore);
					isGameActive = false;
					createGameOver(gameOverDisplay);
				}
				else {
					// Check for spaceship collision and respawn
					if(!spaceship.active && spaceship.respawnTimer > 0) {
						spaceship.respawnTimer -= (float)(GAME_TIMER / 100.0);  // Decrease the timer (you need to calculate deltaTime)
						if(spaceship.respawnTimer <= 0) {
							// Respawn the spaceship
							createSpaceship(spaceship, screenWidth, screenHeight);
						}
					}
				}

				if(activeAsteroids == 0) {
					difficultyLevel++;
					createLevelPassed(levelDisplay);
					createAsteroids(asteroids, screenWidth, screenHeight, difficultyLevel);
				}
			}

			// Redraw window
			InvalidateRect(mainWindowHwnd, NULL, TRUE);
			UpdateWindow(mainWindowHwnd);
			// Sleep for a bit to keep the game running at a reasonable speed
			Sleep(GAME_TIMER);
		}
	}

	Gdiplus::GdiplusShutdown(m_gdiplusToken);
	saveLeaderboard("leaderboard.dat");

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ASTEROIDS));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ASTEROIDS);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	mainWindowHwnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);


	if(!mainWindowHwnd)
	{
		return FALSE;
	}

	ShowWindow(mainWindowHwnd, nCmdShow);
	UpdateWindow(mainWindowHwnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch(wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_SIZE:
		screenWidth = LOWORD(lParam);   // low-order word of lParam
		screenHeight = HIWORD(lParam);  // high-order word of lParam
		difficultyLevel = 0;
		createAsteroids(asteroids, screenWidth, screenHeight, difficultyLevel);
		//createSpaceship(spaceship, screenWidth, screenHeight);
		return 0;
		//case WM_PAINT:
		//{
		//	PAINTSTRUCT ps;
		//	HDC hdc = BeginPaint(hWnd, &ps);
		//	drawProjectiles(hdc, projectiles);
		//	OnPaint(hdc, asteroids);
		//	EndPaint(hWnd, &ps);
		//}
	case WM_ERASEBKGND:
		return true;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		// Create an off-screen DC for double-buffering
		HDC hdcBuffer = CreateCompatibleDC(hdc);
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		HBITMAP hbmBuffer = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
		HBITMAP hbmOldBuffer = (HBITMAP)SelectObject(hdcBuffer, hbmBuffer);

		Paint(hdcBuffer);

		// Transfer the off-screen DC to the screen
		BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcBuffer, 0, 0, SRCCOPY);

		// Clean up
		SelectObject(hdcBuffer, hbmOldBuffer);
		DeleteObject(hbmBuffer);
		DeleteDC(hdcBuffer);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_KEYDOWN: {
		// Check if F11 key is pressed
		if(wParam == VK_F11) {
			DEBUG = !DEBUG;
		}
		else if(wParam == 'P') {
			isGamePaused = !isGamePaused;
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch(message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
