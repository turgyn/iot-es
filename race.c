#include <LiquidCrystal.h>
#include <IRremote.h>
#define UP 20655            // up
#define DOWN 4335           // down
#define FIRE 36975          // VOL-
#define START 41055         // start/resume 
#define PLAYER 0
#define ENEMY 1
#define OBSTACLE 2
#define DAMAGED_OBSTACLE 3

byte OBSTACLE_BYTES[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};
byte DAMAGED_OBSTACLE_BYTES[8] = {
    B11111,
    B10111,
    B11101,
    B00111,
    B11001,
    B11110,
    B11111,
    B11111
};
byte PLAYER_BYTES[8] = {
    B00000,
    B00000,
    B10100,
    B11110,
    B11111,
    B11110,
    B10100,
    B00000
};
byte ENEMY_BYTES[8] = {
    B00000,
    B00000,
    B00000,
    B10100,
    B11110,
    B10100,
    B00000,
    B00000
};

const int recvPin = 6;
const int updateFrequency = 200;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
IRrecv irrecv(recvPin);

long prevTime = 0;
decode_results results;

class Game {
public:
    int playerPos;
    bool gameOn;
private:
    const char OBSTACLE_CHAR = 'O';
    const char ENEMY_CHAR = '<';
    const char BULLET_CHAR = '-';
    const char DAMAGED_OBSTACLE_CHAR= 'x';
    int bulletPos[2];
    bool bulletFlies;
    char field[2][16];
    String infoMessage = "Press Start";

public:
    void drawGameField() {
        lcd.clear();
        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 16; c++) {
                if (field[r][c] != ' ') {
                    lcd.setCursor(c, r);
                    if (field[r][c] == ENEMY_CHAR) {
                        lcd.write(ENEMY);
                    } else if (field[r][c] == OBSTACLE_CHAR) {
                        lcd.write(OBSTACLE);
                    } else if (field[r][c] == DAMAGED_OBSTACLE_CHAR) {
                        lcd.write(DAMAGED_OBSTACLE);
                    }
                }
            }
        }
        delay(10);
    }

    void drawSpecials() {
        lcd.setCursor(0, playerPos);
        lcd.write((uint8_t) PLAYER);
        if (bulletFlies) {
            lcd.setCursor(bulletPos[1], bulletPos[0]);
            lcd.print('-');
        }
    }

    void drawMessage(String message) {
        Serial.println(message);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(message);
        lcd.setCursor(0, 1);
        lcd.print(infoMessage);
        delay(10);
    }

    void playerCollision() {
        for (int r = 0; r < 2; r++) {
            if (field[r][0] != ' ' && playerPos == r) {
                Serial.print("Died: ");
                Serial.println(field[r][0]);
                drawMessage("You died");
                gameOn = false;
                return;
            }
        }
    }

    void bulletCollision() {
        if (bulletFlies) {
            int r = bulletPos[0];
            int c = bulletPos[1];
            if (field[r][c] != ' ') {
                bulletFlies = false;
                if (field[r][c] == OBSTACLE_CHAR) {
                    field[r][c] = DAMAGED_OBSTACLE_CHAR;

                } else {
                    field[r][c] = ' ';
                }
            }
        }
    }

    // += bulletCollision
    void bulletMove() {
        if (bulletFlies) {
            bulletPos[1] = bulletPos[1] + 1;
            if (bulletPos[1] == 16) {
                bulletFlies = false;
            }
        }
    }

    void updateField() {
        
        // Move filed to the right
        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 15; c++) {
                field[r][c] = field[r][c + 1];
            }
        }

        // New column
        for (int r = 0; r < 2; r++) {
            int num = random(10);
            switch (num) {
                case 0:
                    field[r][15] = OBSTACLE_CHAR;
                    break;
                case 1:
                    field[r][15] = ENEMY_CHAR;
                    break;
                default:
                    field[r][15] = ' ';
            }
        }
    }

    void reset() {
        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 16; c++) {
                field[r][c] = ' ';
            }
        }
        playerPos = 1;
        bulletFlies = false;
        gameOn = false;
    }

    void fire() {
        if (bulletFlies) {
            return;
        }
        bulletFlies = true;
        bulletPos[0] = playerPos;
        bulletPos[1] = 0;
    }
} game;


void setup() {
    lcd.begin(16, 2);
    lcd.createChar(PLAYER, PLAYER_BYTES);
    lcd.createChar(ENEMY, ENEMY_BYTES);
    lcd.createChar(OBSTACLE, OBSTACLE_BYTES);
    lcd.createChar(DAMAGED_OBSTACLE, DAMAGED_OBSTACLE_BYTES);
    irrecv.enableIRIn();
    Serial.begin(9600);
    game.reset();
    game.drawMessage("Race Game");
}

void loop() {
    bool gameStateChanged = false;

    if (irrecv.decode(&results)) {
        gameStateChanged = true;
        unsigned int value = results.value;

        if (value == START) {
            if (game.gameOn) {
                game.drawMessage("RESETed");
                game.reset();
            }
            game.gameOn = !game.gameOn;
        }

        if (game.gameOn) {
            if (value == UP) {
                Serial.println("UP pressed");
                game.playerPos = 0;
            }
            if (value == DOWN) {
                game.playerPos = 1;
            }
            if (value == FIRE) {
                game.fire();
            }
        }
        irrecv.resume();
    }

    if (game.gameOn) {
        long curTime = millis();
        if (curTime - prevTime >= updateFrequency) {
            game.updateField();
            game.playerCollision();
            game.bulletCollision();
            game.bulletMove();
            game.bulletCollision();
            gameStateChanged = true;
            prevTime = millis();
        }
    }
    if (game.gameOn && gameStateChanged) {
        game.drawGameField();
        game.drawSpecials();                    // player car and bullet
    }
    
}