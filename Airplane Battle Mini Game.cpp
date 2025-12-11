#include "raylib.h"
#include <stdlib.h>
#include <math.h>

#define MAX_BULLETS 200
#define MAX_ENEMIES 15
#define MAX_POWERUPS 5
#define MAX_ELITES 3
#define MAX_BOSSES 1
#define MAX_BOMBS 3
#define BOMB_RADIUS 300.0f
#define BOMB_DURATION 0.5f

typedef enum {
	MENU,
	PLAYING,
	PAUSED,
	GAME_OVER,
	TIME_UP,
	INSTRUCTIONS
} GameState;

typedef enum {
	TIMED_MODE,
	INFINITE_MODE
} GameMode;

typedef enum {
	SHOTGUN_POWERUP,
	HEALTH_POWERUP,
	BOMB_POWERUP
} PowerUpType;

typedef enum {
	NORMAL_ENEMY,
	ELITE_ENEMY,
	BOSS_ENEMY
} EnemyType;

typedef struct {
	Vector2 position;
	Vector2 speed;
	float radius;
	bool active;
	Color color;
	bool isPlayerBullet;
} Bullet;

typedef struct {
	Vector2 position;
	Vector2 speed;
	float radius;
	bool active;
	Color color;
	int health;
	int maxHealth;
	EnemyType type;
	float shootTimer;
	float shootInterval;
	int scoreValue;
} Enemy;

typedef struct {
	Vector2 position;
	Vector2 speed;
	float radius;
	bool active;
	PowerUpType type;
	float duration;
	Color color;
} PowerUp;

typedef struct {
	Vector2 position;
	Vector2 speed;
	float radius;
	Color color;
	bool hasShotgun;
	float shotgunTimer;
	int health;
	int maxHealth;
	int bombCount;
	int maxBombs;
	int bombDamage;
} Player;

typedef struct {
	Vector2 position;
	float radius;
	float timer;
	bool active;
} BombEffect;

typedef struct {
	int timedModeHighScore;
	int infiniteModeHighScore;
} HighScores;

typedef struct {
	bool pilotAchieved;
	bool hobbyistAchieved;
	int gamesPlayed;
} Achievements;

void DrawInstructionsScreen(int screenWidth, int screenHeight) {
	ClearBackground(BLACK);
	
	// Title
	DrawText("PLANE SHOOTER GAME - INSTRUCTIONS", 
			 screenWidth/2 - MeasureText("PLANE SHOOTER GAME - INSTRUCTIONS", 40)/2, 
			 50, 40, BLUE);
	
	// Controls Section
	DrawText("GAME CONTROLS:", 50, 120, 30, WHITE);
	DrawText("- ARROW KEYS: Move your plane", 70, 160, 25, WHITE);
	DrawText("- SPACE: Fire bullets", 70, 190, 25, WHITE);
	DrawText("- B: Drop bomb (damage all enemies in radius)", 70, 220, 25, WHITE);
	DrawText("- P: Pause game", 70, 250, 25, WHITE);
	DrawText("- R: Return to menu (in pause/game over)", 70, 280, 25, WHITE);
	
	// Game Modes
	DrawText("GAME MODES:", 50, 330, 30, WHITE);
	DrawText("- TIMED MODE: 60 seconds to get highest score", 70, 370, 25, WHITE);
	DrawText("- INFINITE MODE: Survive as long as possible", 70, 400, 25, WHITE);
	
	// Enemies
	DrawText("ENEMY TYPES:", 50, 450, 30, WHITE);
	DrawText("- RED: Normal enemies (10 points)", 70, 490, 25, WHITE);
	DrawText("- PURPLE: Elite enemies (25 points)", 70, 520, 25, WHITE);
	DrawText("- ORANGE: Boss enemies (100+ points)", 70, 550, 25, WHITE);
	
	// Powerups
	DrawText("POWERUPS:", screenWidth/2 + 50, 120, 30, WHITE);
	DrawText("- GREEN (S): Shotgun power (triple shot)", screenWidth/2 + 70, 160, 25, WHITE);
	DrawText("- BLUE (H): Health power (restore health)", screenWidth/2 + 70, 190, 25, WHITE);
	DrawText("- RED (B): Bomb power (+1 bomb)", screenWidth/2 + 70, 220, 25, WHITE);
	
	// Bomb System
	DrawText("BOMB SYSTEM:", screenWidth/2 + 50, 270, 30, WHITE);
	DrawText("- Initial bomb capacity: 3", screenWidth/2 + 70, 310, 25, WHITE);
	DrawText("- +1 bomb capacity every 5 levels", screenWidth/2 + 70, 340, 25, WHITE);
	DrawText("- Base bomb damage: 5", screenWidth/2 + 70, 370, 25, WHITE);
	DrawText("- +5 bomb damage every 5 levels", screenWidth/2 + 70, 400, 25, WHITE);
	DrawText("- Bomb radius: 300 pixels", screenWidth/2 + 70, 430, 25, WHITE);
	
	// Achievements
	DrawText("ACHIEVEMENTS:", screenWidth/2 + 50, 480, 30, WHITE);
	DrawText("- ACE PILOT: Score 3000+ in Infinite Mode", screenWidth/2 + 70, 520, 25, WHITE);
	DrawText("- FLIGHT ENTHUSIAST: Play 10+ games", screenWidth/2 + 70, 550, 25, WHITE);
	
	// Return prompt
	DrawText("PRESS ENTER TO RETURN", 
			 screenWidth/2 - MeasureText("PRESS ENTER TO RETURN", 25)/2, 
			 screenHeight - 60, 25, WHITE);
}

int main(void) {
	const int screenWidth = 1280;
	const int screenHeight = 768;
	
	InitWindow(screenWidth, screenHeight, "Plane Shooter Game");
	SetTargetFPS(60);
	
	GameState gameState = MENU;
	GameMode gameMode = TIMED_MODE;
	
	Player player = {
		.position = { screenWidth/2, screenHeight - 50 },
		.speed = { 10, 10 },
		.radius = 25,
		.color = BLUE,
		.hasShotgun = false,
		.shotgunTimer = 0.0f,
		.health = 3,
		.maxHealth = 5,
		.bombCount = 0,
		.maxBombs = MAX_BOMBS,
		.bombDamage = 5
	};
	
	Bullet bullets[MAX_BULLETS] = {0};
	Enemy enemies[MAX_ENEMIES] = {0};
	PowerUp powerups[MAX_POWERUPS] = {0};
	BombEffect bombEffect = {0};
	
	int score = 0;
	int level = 1;
	float enemySpawnTimer = 0;
	float enemySpawnInterval = 1.5f;
	float powerupSpawnTimer = 0;
	float powerupSpawnInterval = 10.0f;
	float eliteSpawnTimer = 0;
	float eliteSpawnInterval = 15.0f;
	float bossSpawnTimer = 0;
	float bossSpawnInterval = 30.0f;
	
	float gameTime = 60.0f;
	float timeElapsed = 0.0f;
	float minuteTimer = 0.0f;
	
	HighScores highScores = {0};
	Achievements achievements = {0};
	
	bool bossAlive = false;
	const Rectangle bossArea = { 
		screenWidth * 0.2f, 
		screenHeight * 0.1f, 
		screenWidth * 0.6f, 
		screenHeight * 0.6f 
	};
	
	while (!WindowShouldClose()) {
		switch (gameState) {
		case MENU:
			if (IsKeyPressed(KEY_ENTER)) {
				gameState = PLAYING;
				achievements.gamesPlayed++;
				
				if (achievements.gamesPlayed >= 10 && !achievements.hobbyistAchieved) {
					achievements.hobbyistAchieved = true;
				}
				
				score = 0;
				level = 1;
				player.health = 3;
				player.maxHealth = 5;
				gameTime = 60.0f;
				timeElapsed = 0.0f;
				minuteTimer = 0.0f;
				player.hasShotgun = false;
				player.shotgunTimer = 0.0f;
				player.bombCount = 0;
				player.maxBombs = MAX_BOMBS;
				player.bombDamage = 5;
				bossAlive = false;
				bombEffect.active = false;
				
				for (int i = 0; i < MAX_ENEMIES; i++) {
					enemies[i].active = false;
				}
				
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets[i].active = false;
				}
				
				for (int i = 0; i < MAX_POWERUPS; i++) {
					powerups[i].active = false;
				}
				
				player.position = (Vector2){ screenWidth/2, screenHeight - 50 };
			}
			if (IsKeyPressed(KEY_T)) {
				gameMode = TIMED_MODE;
			}
			if (IsKeyPressed(KEY_I)) {
				gameMode = INFINITE_MODE;
			}
			if (IsKeyPressed(KEY_H)) {
				gameState = INSTRUCTIONS;
			}
			break;
			
		case INSTRUCTIONS:
			if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
				gameState = MENU;
			}
			break;
			
		case PLAYING:
			if (IsKeyPressed(KEY_P)) {
				gameState = PAUSED;
			}
			
			if (gameMode == TIMED_MODE) {
				timeElapsed += GetFrameTime();
				gameTime = 60.0f - timeElapsed;
				if (gameTime <= 0) {
					gameTime = 0;
					if (score > highScores.timedModeHighScore) {
						highScores.timedModeHighScore = score;
					}
					gameState = TIME_UP;
				}
			} else {
				if (!bossAlive) {
					minuteTimer += GetFrameTime();
					if (minuteTimer >= 60.0f) {
						minuteTimer = 0.0f;
						level++;
						
						if (level % 5 == 0) {
							player.maxBombs++;
							player.bombDamage += 5;
						}
					}
				}
				
				if (score >= 3000 && !achievements.pilotAchieved) {
					achievements.pilotAchieved = true;
				}
			}
			
			if (player.hasShotgun) {
				player.shotgunTimer -= GetFrameTime();
				if (player.shotgunTimer <= 0) {
					player.hasShotgun = false;
				}
			}
			
			if (bombEffect.active) {
				bombEffect.timer -= GetFrameTime();
				if (bombEffect.timer <= 0) {
					bombEffect.active = false;
				}
			}
			
			if (IsKeyDown(KEY_RIGHT) && player.position.x < screenWidth - player.radius) {
				player.position.x += player.speed.x;
			}
			if (IsKeyDown(KEY_LEFT) && player.position.x > player.radius) {
				player.position.x -= player.speed.x;
			}
			if (IsKeyDown(KEY_UP) && player.position.y > player.radius) {
				player.position.y -= player.speed.y;
			}
			if (IsKeyDown(KEY_DOWN) && player.position.y < screenHeight - player.radius) {
				player.position.y += player.speed.y;
			}
			
			if (IsKeyPressed(KEY_SPACE)) {
				if (player.hasShotgun) {
					for (int i = 0; i < 3; i++) {
						for (int j = 0; j < MAX_BULLETS; j++) {
							if (!bullets[j].active) {
								float offsetX = (i - 1) * 15.0f;
								bullets[j].position = (Vector2){ 
									player.position.x + offsetX, 
									player.position.y - 30 
								};
								bullets[j].speed = (Vector2){ 0, -12 };
								bullets[j].radius = 6;
								bullets[j].active = true;
								bullets[j].color = YELLOW;
								bullets[j].isPlayerBullet = true;
								break;
							}
						}
					}
				} else {
					for (int i = 0; i < MAX_BULLETS; i++) {
						if (!bullets[i].active) {
							bullets[i].position = (Vector2){ player.position.x, player.position.y - 30 };
							bullets[i].speed = (Vector2){ 0, -12 };
							bullets[i].radius = 6;
							bullets[i].active = true;
							bullets[i].color = YELLOW;
							bullets[i].isPlayerBullet = true;
							break;
						}
					}
				}
			}
			
			if (IsKeyPressed(KEY_B) && player.bombCount > 0) {
				player.bombCount--;
				bombEffect.position = player.position;
				bombEffect.radius = BOMB_RADIUS;
				bombEffect.timer = BOMB_DURATION;
				bombEffect.active = true;
				
				for (int i = 0; i < MAX_ENEMIES; i++) {
					if (enemies[i].active) {
						float dx = enemies[i].position.x - bombEffect.position.x;
						float dy = enemies[i].position.y - bombEffect.position.y;
						float distance = sqrt(dx*dx + dy*dy);
						
						if (distance < bombEffect.radius) {
							enemies[i].health -= player.bombDamage;
							if (enemies[i].health <= 0) {
								enemies[i].active = false;
								score += enemies[i].scoreValue;
								if (enemies[i].type == BOSS_ENEMY) {
									bossAlive = false;
								}
							}
						}
					}
				}
			}
			
			enemySpawnTimer += GetFrameTime();
			if (enemySpawnTimer >= enemySpawnInterval) {
				enemySpawnTimer = 0;
				
				for (int i = 0; i < MAX_ENEMIES; i++) {
					if (!enemies[i].active && enemies[i].type == NORMAL_ENEMY) {
						enemies[i].position = (Vector2){ 
							GetRandomValue(50, screenWidth - 50), 
							-30 
						};
						enemies[i].speed = (Vector2){ 
							0, 
							GetRandomValue(3, 6) 
						};
						enemies[i].radius = 20;
						enemies[i].active = true;
						enemies[i].color = RED;
						enemies[i].type = NORMAL_ENEMY;
						enemies[i].health = (gameMode == INFINITE_MODE && !bossAlive) ? level : 1;
						enemies[i].maxHealth = (gameMode == INFINITE_MODE && !bossAlive) ? level : 1;
						enemies[i].shootTimer = 0.0f;
						enemies[i].shootInterval = 0.0f;
						enemies[i].scoreValue = 10;
						break;
					}
				}
			}
			
			if ((gameMode == TIMED_MODE && GetRandomValue(0, 100) < 5) || 
				(gameMode == INFINITE_MODE && level >= 2 && !bossAlive)) {
				
				eliteSpawnTimer += GetFrameTime();
				if (eliteSpawnTimer >= eliteSpawnInterval) {
					eliteSpawnTimer = 0;
					
					for (int i = 0; i < MAX_ENEMIES; i++) {
						if (!enemies[i].active && enemies[i].type != BOSS_ENEMY) {
							enemies[i].position = (Vector2){ 
								GetRandomValue(50, screenWidth - 50), 
								-30 
							};
							enemies[i].speed = (Vector2){ 
								0, 
								GetRandomValue(3, 5) 
							};
							enemies[i].radius = 22;
							enemies[i].active = true;
							enemies[i].color = PURPLE;
							enemies[i].type = ELITE_ENEMY;
							enemies[i].health = (gameMode == INFINITE_MODE && !bossAlive) ? (3 + level/2) : 2;
							enemies[i].maxHealth = (gameMode == INFINITE_MODE && !bossAlive) ? (3 + level/2) : 2;
							enemies[i].shootTimer = 0.0f;
							enemies[i].shootInterval = 2.0f;
							enemies[i].scoreValue = 25;
							break;
						}
					}
				}
			}
			
			if (gameMode == INFINITE_MODE && level % 5 == 0 && !bossAlive) {
				bossSpawnTimer += GetFrameTime();
				if (bossSpawnTimer >= bossSpawnInterval) {
					bossSpawnTimer = 0;
					bossAlive = true;
					
					for (int i = 0; i < MAX_ENEMIES; i++) {
						if (!enemies[i].active && enemies[i].type != ELITE_ENEMY) {
							enemies[i].position = (Vector2){ 
								bossArea.x + bossArea.width/2, 
								bossArea.y - 60 
							};
							enemies[i].speed = (Vector2){ 
								GetRandomValue(-3, 3), 
								1.5f 
							};
							enemies[i].radius = 35;
							enemies[i].active = true;
							enemies[i].color = ORANGE;
							enemies[i].type = BOSS_ENEMY;
							enemies[i].health = 10 + (level/5 - 1) * 10;
							enemies[i].maxHealth = 10 + (level/5 - 1) * 10;
							enemies[i].shootTimer = 0.0f;
							enemies[i].shootInterval = 1.5f;
							enemies[i].scoreValue = 100 + (level/5) * 50;
							break;
						}
					}
				}
			}
			
			if (gameMode == INFINITE_MODE && !bossAlive) {
				powerupSpawnTimer += GetFrameTime();
				if (powerupSpawnTimer >= powerupSpawnInterval) {
					powerupSpawnTimer = 0;
					
					for (int i = 0; i < MAX_POWERUPS; i++) {
						if (!powerups[i].active) {
							PowerUpType type = (PowerUpType)GetRandomValue(0, 2);
							powerups[i].position = (Vector2){ 
								GetRandomValue(50, screenWidth - 50), 
								-30 
							};
							powerups[i].speed = (Vector2){ 0, 4 };
							powerups[i].radius = 12;
							powerups[i].active = true;
							powerups[i].type = type;
							powerups[i].duration = 5.0f;
							
							switch (type) {
							case SHOTGUN_POWERUP:
								powerups[i].color = GREEN;
								break;
							case HEALTH_POWERUP:
								powerups[i].color = SKYBLUE;
								break;
							case BOMB_POWERUP:
								powerups[i].color = RED;
								break;
							}
							break;
						}
					}
				}
			}
			
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i].active) {
					bullets[i].position.y += bullets[i].speed.y;
					
					if (bullets[i].position.y < 0) {
						bullets[i].active = false;
					}
				}
			}
			
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (enemies[i].active) {
					enemies[i].position.y += enemies[i].speed.y;
					
					if (enemies[i].type == BOSS_ENEMY) {
						enemies[i].position.x += enemies[i].speed.x;
						if (enemies[i].position.x < bossArea.x || enemies[i].position.x > bossArea.x + bossArea.width) {
							enemies[i].speed.x *= -1;
						}
						if (enemies[i].position.y < bossArea.y || enemies[i].position.y > bossArea.y + bossArea.height) {
							enemies[i].speed.y *= -1;
						}
					}
					
					if (enemies[i].position.y > screenHeight + 60) {
						enemies[i].active = false;
						if (enemies[i].type == BOSS_ENEMY) {
							bossAlive = false;
						}
					}
					
					if (enemies[i].type == ELITE_ENEMY || enemies[i].type == BOSS_ENEMY) {
						enemies[i].shootTimer += GetFrameTime();
						if (enemies[i].shootTimer >= enemies[i].shootInterval) {
							enemies[i].shootTimer = 0.0f;
							
							if (enemies[i].type == ELITE_ENEMY) {
								for (int j = 0; j < MAX_BULLETS; j++) {
									if (!bullets[j].active) {
										bullets[j].position = (Vector2){ 
											enemies[i].position.x, 
											enemies[i].position.y + enemies[i].radius 
										};
										bullets[j].speed = (Vector2){ 0, 6 };
										bullets[j].radius = 5;
										bullets[j].active = true;
										bullets[j].color = PURPLE;
										bullets[j].isPlayerBullet = false;
										break;
									}
								}
							} else if (enemies[i].type == BOSS_ENEMY) {
								for (int k = 0; k < 3; k++) {
									for (int j = 0; j < MAX_BULLETS; j++) {
										if (!bullets[j].active) {
											float offsetX = (k - 1) * 20.0f;
											bullets[j].position = (Vector2){ 
												enemies[i].position.x + offsetX, 
												enemies[i].position.y + enemies[i].radius 
											};
											bullets[j].speed = (Vector2){ 0, 5 };
											bullets[j].radius = 7;
											bullets[j].active = true;
											bullets[j].color = ORANGE;
											bullets[j].isPlayerBullet = false;
											break;
										}
									}
								}
							}
						}
					}
				}
			}
			
			for (int i = 0; i < MAX_POWERUPS; i++) {
				if (powerups[i].active) {
					powerups[i].position.y += powerups[i].speed.y;
					
					if (powerups[i].position.y > screenHeight + 30) {
						powerups[i].active = false;
					}
				}
			}
			
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i].active && bullets[i].isPlayerBullet) {
					for (int j = 0; j < MAX_ENEMIES; j++) {
						if (enemies[j].active) {
							float dx = bullets[i].position.x - enemies[j].position.x;
							float dy = bullets[i].position.y - enemies[j].position.y;
							float distance = sqrt(dx*dx + dy*dy);
							
							if (distance < bullets[i].radius + enemies[j].radius) {
								bullets[i].active = false;
								enemies[j].health--;
								if (enemies[j].health <= 0) {
									enemies[j].active = false;
									score += enemies[j].scoreValue;
									if (enemies[j].type == BOSS_ENEMY) {
										bossAlive = false;
									}
								}
							}
						}
					}
				}
			}
			
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i].active && !bullets[i].isPlayerBullet) {
					float dx = player.position.x - bullets[i].position.x;
					float dy = player.position.y - bullets[i].position.y;
					float distance = sqrt(dx*dx + dy*dy);
					
					if (distance < player.radius + bullets[i].radius) {
						bullets[i].active = false;
						player.health--;
						
						if (player.health <= 0) {
							if (gameMode == INFINITE_MODE && score > highScores.infiniteModeHighScore) {
								highScores.infiniteModeHighScore = score;
							}
							gameState = GAME_OVER;
						}
					}
				}
			}
			
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (enemies[i].active) {
					float dx = player.position.x - enemies[i].position.x;
					float dy = player.position.y - enemies[i].position.y;
					float distance = sqrt(dx*dx + dy*dy);
					
					if (distance < player.radius + enemies[i].radius) {
						enemies[i].active = false;
						player.health--;
						
						if (player.health <= 0) {
							if (gameMode == INFINITE_MODE && score > highScores.infiniteModeHighScore) {
								highScores.infiniteModeHighScore = score;
							}
							gameState = GAME_OVER;
						}
					}
				}
			}
			
			for (int i = 0; i < MAX_POWERUPS; i++) {
				if (powerups[i].active) {
					float dx = player.position.x - powerups[i].position.x;
					float dy = player.position.y - powerups[i].position.y;
					float distance = sqrt(dx*dx + dy*dy);
					
					if (distance < player.radius + powerups[i].radius) {
						powerups[i].active = false;
						if (powerups[i].type == SHOTGUN_POWERUP) {
							player.hasShotgun = true;
							player.shotgunTimer = powerups[i].duration;
						} else if (powerups[i].type == HEALTH_POWERUP) {
							if (player.health < player.maxHealth) {
								player.health++;
							} else {
								player.maxHealth++;
								player.health = player.maxHealth;
							}
						} else if (powerups[i].type == BOMB_POWERUP) {
							if (player.bombCount < player.maxBombs) {
								player.bombCount++;
							}
						}
					}
				}
			}
			break;
			
		case PAUSED:
			if (IsKeyPressed(KEY_P)) {
				gameState = PLAYING;
			}
			if (IsKeyPressed(KEY_R)) {
				gameState = MENU;
			}
			break;
			
		case GAME_OVER:
			if (IsKeyPressed(KEY_R)) {
				gameState = MENU;
			}
			break;
			
		case TIME_UP:
			if (IsKeyPressed(KEY_R)) {
				gameState = MENU;
			}
			break;
		}
		
		BeginDrawing();
		ClearBackground(BLACK);
		
		if (gameState == PLAYING || gameState == PAUSED) {
			DrawCircleV(player.position, player.radius, player.color);
			
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i].active) {
					DrawCircleV(bullets[i].position, bullets[i].radius, bullets[i].color);
				}
			}
			
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (enemies[i].active) {
					Color enemyColor = enemies[i].color;
					if (enemies[i].type == ELITE_ENEMY) {
						enemyColor = PURPLE;
					} else if (enemies[i].type == BOSS_ENEMY) {
						enemyColor = ORANGE;
					}
					
					DrawCircleV(enemies[i].position, enemies[i].radius, enemyColor);
					
					if (enemies[i].type == ELITE_ENEMY) {
						DrawText("E", enemies[i].position.x - 8, enemies[i].position.y - 10, 20, WHITE);
					} else if (enemies[i].type == BOSS_ENEMY) {
						DrawText("B", enemies[i].position.x - 10, enemies[i].position.y - 12, 24, WHITE);
					}
					
					if (enemies[i].maxHealth > 1) {
						float healthBarWidth = enemies[i].radius * 2.5f;
						float healthRatio = (float)enemies[i].health / enemies[i].maxHealth;
						DrawRectangle(enemies[i].position.x - healthBarWidth/2, enemies[i].position.y - enemies[i].radius - 15, 
									  healthBarWidth, 8, GRAY);
						DrawRectangle(enemies[i].position.x - healthBarWidth/2, enemies[i].position.y - enemies[i].radius - 15, 
									  healthBarWidth * healthRatio, 8, GREEN);
					}
				}
			}
			
			for (int i = 0; i < MAX_POWERUPS; i++) {
				if (powerups[i].active) {
					DrawCircleV(powerups[i].position, powerups[i].radius, powerups[i].color);
					if (powerups[i].type == SHOTGUN_POWERUP) {
						DrawText("S", powerups[i].position.x - 6, powerups[i].position.y - 10, 20, BLACK);
					} else if (powerups[i].type == HEALTH_POWERUP) {
						DrawText("H", powerups[i].position.x - 6, powerups[i].position.y - 10, 20, BLACK);
					} else if (powerups[i].type == BOMB_POWERUP) {
						DrawText("B", powerups[i].position.x - 6, powerups[i].position.y - 10, 20, BLACK);
					}
				}
			}
			
			if (bombEffect.active) {
				DrawCircleLines(bombEffect.position.x, bombEffect.position.y, bombEffect.radius, Fade(RED, 0.5f));
			}
			
			DrawText(TextFormat("Score: %d", score), 50, 30, 24, WHITE);
			
			for (int i = 0; i < player.maxHealth; i++) {
				Color heartColor = (i < player.health) ? RED : GRAY;
				DrawCircle(80 + i * 50, 60, 12, heartColor);
			}
			
			DrawText(TextFormat("Bombs: %d/%d", player.bombCount, player.maxBombs), 
					 screenWidth - 250, 150, 24, RED);
			DrawText(TextFormat("Bomb Damage: %d", player.bombDamage), 
					 screenWidth - 250, 180, 24, RED);
			
			if (gameMode == TIMED_MODE) {
				DrawText(TextFormat("Time: %.1f", gameTime), screenWidth - 250, 30, 24, WHITE);
			} else {
				DrawText("INFINITE MODE", screenWidth - 250, 30, 24, GREEN);
				if (player.hasShotgun) {
					DrawText(TextFormat("Shotgun: %.1f", player.shotgunTimer), screenWidth - 250, 60, 24, GREEN);
				}
				DrawText(TextFormat("Time: %d:%02d", (int)(minuteTimer/60), (int)minuteTimer%60), 
						 screenWidth - 250, 90, 24, WHITE);
				DrawText(TextFormat("Level: %d", level), screenWidth - 250, 120, 24, WHITE);
				
				if (level % 5 == 0 && bossAlive) {
					DrawText("BOSS FIGHT!", screenWidth/2 - MeasureText("BOSS FIGHT!", 36)/2, 50, 36, ORANGE);
					DrawRectangleLinesEx(bossArea, 2.0f, Fade(ORANGE, 0.3f));
				}
			}
		}
		
		switch (gameState) {
		case MENU:
			DrawText("PLANE SHOOTER GAME", 
					 screenWidth/2 - MeasureText("PLANE SHOOTER GAME", 60)/2, 
					 150, 60, BLUE);
			DrawText("PRESS ENTER TO START", 
					 screenWidth/2 - MeasureText("PRESS ENTER TO START", 30)/2, 
					 250, 30, WHITE);
			DrawText("H: HOW TO PLAY", 
					 screenWidth/2 - MeasureText("H: HOW TO PLAY", 30)/2, 
					 300, 30, WHITE);
			DrawText("T: TIMED MODE (60 SECONDS)", 
					 screenWidth/2 - MeasureText("T: TIMED MODE (60 SECONDS)", 30)/2, 
					 350, 30, gameMode == TIMED_MODE ? GREEN : WHITE);
			DrawText("I: INFINITE MODE (WITH POWERUPS & BOSSES)", 
					 screenWidth/2 - MeasureText("I: INFINITE MODE (WITH POWERUPS & BOSSES)", 30)/2, 
					 400, 30, gameMode == INFINITE_MODE ? GREEN : WHITE);
			DrawText(TextFormat("TIMED HIGHSCORE: %d", highScores.timedModeHighScore), 
					 screenWidth/2 - MeasureText(TextFormat("TIMED HIGHSCORE: %d", highScores.timedModeHighScore), 30)/2, 
					 450, 30, YELLOW);
			DrawText(TextFormat("INFINITE HIGHSCORE: %d", highScores.infiniteModeHighScore), 
					 screenWidth/2 - MeasureText(TextFormat("INFINITE HIGHSCORE: %d", highScores.infiniteModeHighScore), 30)/2, 
					 500, 30, YELLOW);
			
			if (achievements.hobbyistAchieved) {
				DrawText("ACHIEVEMENT: FLIGHT ENTHUSIAST", 
						 screenWidth/2 - MeasureText("ACHIEVEMENT: FLIGHT ENTHUSIAST", 30)/2, 
						 550, 30, GOLD);
			}
			if (achievements.pilotAchieved) {
				DrawText("ACHIEVEMENT: ACE PILOT", 
						 screenWidth/2 - MeasureText("ACHIEVEMENT: ACE PILOT", 30)/2, 
						 580, 30, GOLD);
			}
			break;
			
		case INSTRUCTIONS:
			DrawInstructionsScreen(screenWidth, screenHeight);
			break;
			
		case PAUSED:
			DrawText("GAME PAUSED", 
					 screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, 
					 screenHeight/2 - 100, 40, BLUE);
			DrawText("PRESS P TO CONTINUE", 
					 screenWidth/2 - MeasureText("PRESS P TO CONTINUE", 30)/2, 
					 screenHeight/2 - 50, 30, WHITE);
			DrawText("PRESS R TO RETURN TO MENU", 
					 screenWidth/2 - MeasureText("PRESS R TO RETURN TO MENU", 30)/2, 
					 screenHeight/2 + 20, 30, WHITE);
			break;
			
		case GAME_OVER:
			DrawText("GAME OVER", 
					 screenWidth/2 - MeasureText("GAME OVER", 40)/2, 
					 screenHeight/2 - 100, 40, RED);
			DrawText(TextFormat("YOUR SCORE: %d", score), 
					 screenWidth/2 - MeasureText(TextFormat("YOUR SCORE: %d", score), 30)/2, 
					 screenHeight/2 - 50, 30, WHITE);
			if (gameMode == INFINITE_MODE) {
				DrawText(TextFormat("HIGHSCORE: %d", highScores.infiniteModeHighScore), 
						 screenWidth/2 - MeasureText(TextFormat("HIGHSCORE: %d", highScores.infiniteModeHighScore), 30)/2, 
						 screenHeight/2 - 10, 30, YELLOW);
				DrawText(TextFormat("LEVEL REACHED: %d", level), 
						 screenWidth/2 - MeasureText(TextFormat("LEVEL REACHED: %d", level), 25)/2, 
						 screenHeight/2 + 30, 25, WHITE);
				
				if (achievements.pilotAchieved) {
					DrawText("ACE PILOT ACHIEVED!", 
							 screenWidth/2 - MeasureText("ACE PILOT ACHIEVED!", 30)/2, 
							 screenHeight/2 + 60, 30, GOLD);
				}
			} else {
				DrawText(TextFormat("HIGHSCORE: %d", highScores.timedModeHighScore), 
						 screenWidth/2 - MeasureText(TextFormat("HIGHSCORE: %d", highScores.timedModeHighScore), 30)/2, 
						 screenHeight/2 - 10, 30, YELLOW);
			}
			DrawText("PRESS R TO RETURN TO MENU", 
					 screenWidth/2 - MeasureText("PRESS R TO RETURN TO MENU", 20)/2, 
					 screenHeight/2 + 100, 20, WHITE);
			break;
			
		case TIME_UP:
			DrawText("TIME'S UP!", 
					 screenWidth/2 - MeasureText("TIME'S UP!", 40)/2, 
					 screenHeight/2 - 100, 40, GREEN);
			DrawText(TextFormat("YOUR SCORE: %d", score), 
					 screenWidth/2 - MeasureText(TextFormat("YOUR SCORE: %d", score), 30)/2, 
					 screenHeight/2 - 50, 30, WHITE);
			DrawText(TextFormat("HIGHSCORE: %d", highScores.timedModeHighScore), 
					 screenWidth/2 - MeasureText(TextFormat("HIGHSCORE: %d", highScores.timedModeHighScore), 30)/2, 
					 screenHeight/2 - 10, 30, YELLOW);
			DrawText("PRESS R TO RETURN TO MENU", 
					 screenWidth/2 - MeasureText("PRESS R TO RETURN TO MENU", 20)/2, 
					 screenHeight/2 + 30, 20, WHITE);
			break;
		}
		EndDrawing();
	}
	
	CloseWindow();
	return 0;
}

