#include "raylib.h"
#include <stdlib.h>
#include <math.h>

#define MAX_BULLETS 200
#define MAX_ENEMIES 15
#define MAX_POWERUPS 5
#define MAX_ELITES 3
#define MAX_BOSSES 1

typedef enum {
	MENU,
	PLAYING,
	PAUSED,
	GAME_OVER,
	TIME_UP
} GameState;

typedef enum {
	TIMED_MODE,
	INFINITE_MODE
} GameMode;

typedef enum {
	SHOTGUN_POWERUP,
	HEALTH_POWERUP
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
} Player;

typedef struct {
	int timedModeHighScore;
	int infiniteModeHighScore;
} HighScores;

typedef struct {
	bool pilotAchieved;      // 王牌飞行员成就
	bool hobbyistAchieved;   // 飞行爱好者成就
	int gamesPlayed;         // 游玩次数
} Achievements;

int main(void) {
	const int screenWidth = 800;
	const int screenHeight = 450;
	
	InitWindow(screenWidth, screenHeight, "Plane Shooter Game");
	SetTargetFPS(60);
	
	GameState gameState = MENU;
	GameMode gameMode = TIMED_MODE;
	
	Player player = {
		.position = { screenWidth/2, screenHeight - 30 },
		.speed = { 5, 5 },
		.radius = 20,
		.color = BLUE,
		.hasShotgun = false,
		.shotgunTimer = 0.0f,
		.health = 3,
		.maxHealth = 5
	};
	
	Bullet bullets[MAX_BULLETS] = {0};
	Enemy enemies[MAX_ENEMIES] = {0};
	PowerUp powerups[MAX_POWERUPS] = {0};
	
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
	
	// 计时系统变量
	float gameTime = 60.0f;
	float timeElapsed = 0.0f;
	float minuteTimer = 0.0f;
	
	HighScores highScores = {0};
	Achievements achievements = {0};
	
	while (!WindowShouldClose()) {
		// 根据游戏状态处理输入
		switch (gameState) {
		case MENU:
			if (IsKeyPressed(KEY_ENTER)) {
				gameState = PLAYING;
				achievements.gamesPlayed++;
				
				// 检查飞行爱好者成就
				if (achievements.gamesPlayed >= 10 && !achievements.hobbyistAchieved) {
					achievements.hobbyistAchieved = true;
				}
				
				// 重置游戏
				score = 0;
				level = 1;
				player.health = 3;
				player.maxHealth = 5;
				gameTime = 60.0f;
				timeElapsed = 0.0f;
				minuteTimer = 0.0f;
				player.hasShotgun = false;
				player.shotgunTimer = 0.0f;
				
				for (int i = 0; i < MAX_ENEMIES; i++) {
					enemies[i].active = false;
				}
				
				for (int i = 0; i < MAX_BULLETS; i++) {
					bullets[i].active = false;
				}
				
				for (int i = 0; i < MAX_POWERUPS; i++) {
					powerups[i].active = false;
				}
				
				player.position = (Vector2){ screenWidth/2, screenHeight - 30 };
			}
			if (IsKeyPressed(KEY_T)) {
				gameMode = TIMED_MODE;
			}
			if (IsKeyPressed(KEY_I)) {
				gameMode = INFINITE_MODE;
			}
			break;
			
		case PLAYING:
			if (IsKeyPressed(KEY_P)) {
				gameState = PAUSED;
			}
			
			// 更新计时器（仅在计时模式下）
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
				// 无限模式下每分钟增加难度
				minuteTimer += GetFrameTime();
				if (minuteTimer >= 60.0f) {
					minuteTimer = 0.0f;
					level++;
				}
				
				// 检查王牌飞行员成就（3000分触发）
				if (score >= 3000 && !achievements.pilotAchieved) {
					achievements.pilotAchieved = true;
				}
			}
			
			// 更新霰弹道具计时器
			if (player.hasShotgun) {
				player.shotgunTimer -= GetFrameTime();
				if (player.shotgunTimer <= 0) {
					player.hasShotgun = false;
				}
			}
			
			// 玩家控制
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
			
			// 发射子弹
			if (IsKeyPressed(KEY_SPACE)) {
				if (player.hasShotgun) {
					// 霰弹模式：发射三排子弹
					for (int i = 0; i < 3; i++) {
						for (int j = 0; j < MAX_BULLETS; j++) {
							if (!bullets[j].active) {
								float offsetX = (i - 1) * 15.0f;
								bullets[j].position = (Vector2){ 
									player.position.x + offsetX, 
									player.position.y - 20 
								};
								bullets[j].speed = (Vector2){ 0, -10 };
								bullets[j].radius = 5;
								bullets[j].active = true;
								bullets[j].color = YELLOW;
								bullets[j].isPlayerBullet = true;
								break;
							}
						}
					}
				} else {
					// 普通模式：发射单发子弹
					for (int i = 0; i < MAX_BULLETS; i++) {
						if (!bullets[i].active) {
							bullets[i].position = (Vector2){ player.position.x, player.position.y - 20 };
							bullets[i].speed = (Vector2){ 0, -10 };
							bullets[i].radius = 5;
							bullets[i].active = true;
							bullets[i].color = YELLOW;
							bullets[i].isPlayerBullet = true;
							break;
						}
					}
				}
			}
			
			// 生成普通敌机
			enemySpawnTimer += GetFrameTime();
			if (enemySpawnTimer >= enemySpawnInterval) {
				enemySpawnTimer = 0;
				
				for (int i = 0; i < MAX_ENEMIES; i++) {
					if (!enemies[i].active && enemies[i].type == NORMAL_ENEMY) {
						enemies[i].position = (Vector2){ 
							GetRandomValue(20, screenWidth - 20), 
							-20 
						};
						enemies[i].speed = (Vector2){ 
							0, 
							GetRandomValue(2, 5) 
						};
						enemies[i].radius = 15;
						enemies[i].active = true;
						enemies[i].color = RED;
						enemies[i].type = NORMAL_ENEMY;
						enemies[i].health = (gameMode == INFINITE_MODE) ? level : 1;
						enemies[i].maxHealth = (gameMode == INFINITE_MODE) ? level : 1;
						enemies[i].shootTimer = 0.0f;
						enemies[i].shootInterval = 0.0f;
						enemies[i].scoreValue = 10;
						break;
					}
				}
			}
			
			// 生成精英怪（计时模式随机刷新，无限模式level>=2刷新）
			if ((gameMode == TIMED_MODE && GetRandomValue(0, 100) < 5) || 
				(gameMode == INFINITE_MODE && level >= 2)) {
				
				eliteSpawnTimer += GetFrameTime();
				if (eliteSpawnTimer >= eliteSpawnInterval) {
					eliteSpawnTimer = 0;
					
					for (int i = 0; i < MAX_ENEMIES; i++) {
						if (!enemies[i].active && enemies[i].type != BOSS_ENEMY) {
							enemies[i].position = (Vector2){ 
								GetRandomValue(20, screenWidth - 20), 
								-20 
							};
							enemies[i].speed = (Vector2){ 
								0, 
								GetRandomValue(2, 4) 
							};
							enemies[i].radius = 18;
							enemies[i].active = true;
							enemies[i].color = PURPLE;
							enemies[i].type = ELITE_ENEMY;
							enemies[i].health = (gameMode == INFINITE_MODE) ? (3 + level/2) : 2;
							enemies[i].maxHealth = (gameMode == INFINITE_MODE) ? (3 + level/2) : 2;
							enemies[i].shootTimer = 0.0f;
							enemies[i].shootInterval = 2.0f;
							enemies[i].scoreValue = 25;
							break;
						}
					}
				}
			}
			
			// 生成Boss（无限模式每5级一个）
			if (gameMode == INFINITE_MODE && level % 5 == 0) {
				bossSpawnTimer += GetFrameTime();
				if (bossSpawnTimer >= bossSpawnInterval) {
					bossSpawnTimer = 0;
					
					for (int i = 0; i < MAX_ENEMIES; i++) {
						if (!enemies[i].active && enemies[i].type != ELITE_ENEMY) {
							enemies[i].position = (Vector2){ 
								screenWidth/2, 
								-50 
							};
							enemies[i].speed = (Vector2){ 
								GetRandomValue(-2, 2), 
								1 
							};
							enemies[i].radius = 30;
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
			
			// 生成道具（仅在无限模式下）
			if (gameMode == INFINITE_MODE) {
				powerupSpawnTimer += GetFrameTime();
				if (powerupSpawnTimer >= powerupSpawnInterval) {
					powerupSpawnTimer = 0;
					
					for (int i = 0; i < MAX_POWERUPS; i++) {
						if (!powerups[i].active) {
							powerups[i].position = (Vector2){ 
								GetRandomValue(20, screenWidth - 20), 
								-20 
							};
							powerups[i].speed = (Vector2){ 0, 3 };
							powerups[i].radius = 10;
							powerups[i].active = true;
							powerups[i].type = (GetRandomValue(0, 1) == 0) ? SHOTGUN_POWERUP : HEALTH_POWERUP;
							powerups[i].duration = 5.0f;
							powerups[i].color = (powerups[i].type == SHOTGUN_POWERUP) ? GREEN : SKYBLUE;
							break;
						}
					}
				}
			}
			
			// 更新子弹
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i].active) {
					bullets[i].position.y += bullets[i].speed.y;
					
					if (bullets[i].position.y < 0) {
						bullets[i].active = false;
					}
				}
			}
			
			// 更新敌机
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (enemies[i].active) {
					enemies[i].position.y += enemies[i].speed.y;
					
					if (enemies[i].type == BOSS_ENEMY) {
						enemies[i].position.x += enemies[i].speed.x;
						if (enemies[i].position.x < 50 || enemies[i].position.x > screenWidth - 50) {
							enemies[i].speed.x *= -1;
						}
					}
					
					if (enemies[i].position.y > screenHeight + 50) {
						enemies[i].active = false;
					}
					
					// 敌机射击
					if (enemies[i].type == ELITE_ENEMY || enemies[i].type == BOSS_ENEMY) {
						enemies[i].shootTimer += GetFrameTime();
						if (enemies[i].shootTimer >= enemies[i].shootInterval) {
							enemies[i].shootTimer = 0.0f;
							
							// 精英怪发射单发子弹
							if (enemies[i].type == ELITE_ENEMY) {
								for (int j = 0; j < MAX_BULLETS; j++) {
									if (!bullets[j].active) {
										bullets[j].position = (Vector2){ 
											enemies[i].position.x, 
											enemies[i].position.y + enemies[i].radius 
										};
										bullets[j].speed = (Vector2){ 0, 5 };
										bullets[j].radius = 4;
										bullets[j].active = true;
										bullets[j].color = PURPLE;
										bullets[j].isPlayerBullet = false;
										break;
									}
								}
							} 
							// Boss发射三排子弹
							else if (enemies[i].type == BOSS_ENEMY) {
								for (int k = 0; k < 3; k++) {
									for (int j = 0; j < MAX_BULLETS; j++) {
										if (!bullets[j].active) {
											float offsetX = (k - 1) * 15.0f;
											bullets[j].position = (Vector2){ 
												enemies[i].position.x + offsetX, 
												enemies[i].position.y + enemies[i].radius 
											};
											bullets[j].speed = (Vector2){ 0, 4 };
											bullets[j].radius = 6;
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
			
			// 更新道具
			for (int i = 0; i < MAX_POWERUPS; i++) {
				if (powerups[i].active) {
					powerups[i].position.y += powerups[i].speed.y;
					
					if (powerups[i].position.y > screenHeight + 20) {
						powerups[i].active = false;
					}
				}
			}
			
			// 碰撞检测：子弹和敌机
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
								}
							}
						}
					}
				}
			}
			
			// 碰撞检测：玩家和敌机子弹
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
			
			// 碰撞检测：玩家和敌机
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
			
			// 碰撞检测：玩家和道具
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
						}
					}
				}
			}
			break;
			
		case PAUSED:
			if (IsKeyPressed(KEY_P)) {
				gameState = PLAYING;
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
		
		// 绘制
		BeginDrawing();
		ClearBackground(BLACK);
		
		// 绘制玩家
		if (gameState == PLAYING || gameState == PAUSED) {
			DrawCircleV(player.position, player.radius, player.color);
			
			// 绘制子弹
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (bullets[i].active) {
					DrawCircleV(bullets[i].position, bullets[i].radius, bullets[i].color);
				}
			}
			
			// 绘制敌机
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (enemies[i].active) {
					Color enemyColor = enemies[i].color;
					if (enemies[i].type == ELITE_ENEMY) {
						enemyColor = PURPLE;
					} else if (enemies[i].type == BOSS_ENEMY) {
						enemyColor = ORANGE;
					}
					
					DrawCircleV(enemies[i].position, enemies[i].radius, enemyColor);
					
					// 绘制敌机类型标识
					if (enemies[i].type == ELITE_ENEMY) {
						DrawText("E", enemies[i].position.x - 5, enemies[i].position.y - 8, 16, WHITE);
					} else if (enemies[i].type == BOSS_ENEMY) {
						DrawText("B", enemies[i].position.x - 8, enemies[i].position.y - 10, 20, WHITE);
					}
					
					// 绘制敌机血条
					if (enemies[i].maxHealth > 1) {
						float healthBarWidth = enemies[i].radius * 2;
						float healthRatio = (float)enemies[i].health / enemies[i].maxHealth;
						DrawRectangle(enemies[i].position.x - healthBarWidth/2, enemies[i].position.y - enemies[i].radius - 10, 
									  healthBarWidth, 5, GRAY);
						DrawRectangle(enemies[i].position.x - healthBarWidth/2, enemies[i].position.y - enemies[i].radius - 10, 
									  healthBarWidth * healthRatio, 5, GREEN);
					}
				}
			}
			
			// 绘制道具
			for (int i = 0; i < MAX_POWERUPS; i++) {
				if (powerups[i].active) {
					DrawCircleV(powerups[i].position, powerups[i].radius, powerups[i].color);
					// 绘制道具图标
					if (powerups[i].type == SHOTGUN_POWERUP) {
						DrawText("S", powerups[i].position.x - 5, powerups[i].position.y - 8, 16, BLACK);
					} else {
						DrawText("H", powerups[i].position.x - 5, powerups[i].position.y - 8, 16, BLACK);
					}
				}
			}
			
			// 绘制分数和生命值
			DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
			
			// 绘制玩家血条
			for (int i = 0; i < player.maxHealth; i++) {
				Color heartColor = (i < player.health) ? RED : GRAY;
				DrawCircle(30 + i * 25, 40, 8, heartColor);
			}
			
			// 绘制倒计时（仅在计时模式下）
			if (gameMode == TIMED_MODE) {
				DrawText(TextFormat("Time: %.1f", gameTime), screenWidth - 150, 10, 20, WHITE);
			} else {
				DrawText("INFINITE MODE", screenWidth - 150, 10, 20, GREEN);
				// 绘制霰弹道具剩余时间
				if (player.hasShotgun) {
					DrawText(TextFormat("Shotgun: %.1f", player.shotgunTimer), screenWidth - 150, 40, 20, GREEN);
				}
				// 绘制游戏时间和等级
				DrawText(TextFormat("Time: %d:%02d", (int)(minuteTimer/60), (int)minuteTimer%60), 
						 screenWidth - 150, 70, 20, WHITE);
				DrawText(TextFormat("Level: %d", level), screenWidth - 150, 100, 20, WHITE);
				
				// 显示Boss信息
				if (level % 5 == 0) {
					DrawText("BOSS INCOMING!", screenWidth/2 - MeasureText("BOSS INCOMING!", 30)/2, 30, 30, ORANGE);
				}
			}
		}
		
		// 绘制菜单状态
		switch (gameState) {
		case MENU:
			DrawText("PLANE SHOOTER GAME", 
					 screenWidth/2 - MeasureText("PLANE SHOOTER GAME", 40)/2, 
					 100, 40, BLUE);
			DrawText("PRESS ENTER TO START", 
					 screenWidth/2 - MeasureText("PRESS ENTER TO START", 20)/2, 
					 200, 20, WHITE);
			DrawText("ARROW KEYS: MOVE, SPACE: SHOOT", 
					 screenWidth/2 - MeasureText("ARROW KEYS: MOVE, SPACE: SHOOT", 20)/2, 
					 250, 20, WHITE);
			DrawText("T: TIMED MODE (60 SECONDS)", 
					 screenWidth/2 - MeasureText("T: TIMED MODE (60 SECONDS)", 20)/2, 
					 300, 20, gameMode == TIMED_MODE ? GREEN : WHITE);
			DrawText("I: INFINITE MODE (WITH POWERUPS & BOSSES)", 
					 screenWidth/2 - MeasureText("I: INFINITE MODE (WITH POWERUPS & BOSSES)", 20)/2, 
					 330, 20, gameMode == INFINITE_MODE ? GREEN : WHITE);
			// 显示最高分
			DrawText(TextFormat("TIMED HIGHSCORE: %d", highScores.timedModeHighScore), 
					 screenWidth/2 - MeasureText(TextFormat("TIMED HIGHSCORE: %d", highScores.timedModeHighScore), 20)/2, 
					 370, 20, YELLOW);
			DrawText(TextFormat("INFINITE HIGHSCORE: %d", highScores.infiniteModeHighScore), 
					 screenWidth/2 - MeasureText(TextFormat("INFINITE HIGHSCORE: %d", highScores.infiniteModeHighScore), 20)/2, 
					 400, 20, YELLOW);
			
			// 显示飞行爱好者成就
			if (achievements.hobbyistAchieved) {
				DrawText("ACHIEVEMENT: FLIGHT ENTHUSIAST", 
						 screenWidth/2 - MeasureText("ACHIEVEMENT: FLIGHT ENTHUSIAST", 20)/2, 
						 430, 20, GOLD);
			}
			break;
			
		case PAUSED:
			DrawText("GAME PAUSED", 
					 screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, 
					 screenHeight/2 - 50, 40, BLUE);
			DrawText("PRESS P TO CONTINUE", 
					 screenWidth/2 - MeasureText("PRESS P TO CONTINUE", 20)/2, 
					 screenHeight/2 + 20, 20, WHITE);
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
				
				// 显示王牌飞行员成就
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
			
		default:
			break;
		}
		EndDrawing();
	}
	
	CloseWindow();
	return 0;
}

