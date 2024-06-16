#include <ncurses.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <unistd.h>

// 맵 크기
const int MAP_HEIGHT = 20;
const int MAP_WIDTH = 40;

// 맵 요소 값
const int WALL = 1;
const int IMMUNE_WALL = 2;
const int SNAKE_HEAD = 3;
const int SNAKE_BODY = 4;
const int GROWTH_ITEM = 5;
const int POISON_ITEM = 6;
const int GATE = 7;
const int SPECIAL_ITEM = 8;  // 새로운 아이템

// Snake 방향
enum Direction { UP, DOWN, LEFT, RIGHT };

//속도 요소
int speed = 100000; // 기본 속도 (0.1초 대기)
int slowDownEndTime = 0; // 느린 속도가 끝나는 시간


// Snake 구조체
struct SnakeSegment {
    int y, x;
    bool operator==(const SnakeSegment& other) const {
        return y == other.y && x == other.x;
    }
};

// 아이템 생성 함수
void generateItem(std::vector<std::vector<int>> &map, int itemType) {
    int y, x;
    do {
        y = rand() % MAP_HEIGHT;
        x = rand() % MAP_WIDTH;
    } while (map[y][x] != 0);
    map[y][x] = itemType;
}

// 게이트 생성 함수
void generateGate(std::vector<std::vector<int>> &map, std::pair<int, int> &gate1, std::pair<int, int> &gate2) {
    int y1, x1, y2, x2;
    do {
        y1 = rand() % MAP_HEIGHT;
        x1 = rand() % MAP_WIDTH;
    } while (map[y1][x1] != WALL);

    do {
        y2 = rand() % MAP_HEIGHT;
        x2 = rand() % MAP_WIDTH;
    } while (map[y2][x2] != WALL || (y1 == y2 && x1 == x2));

    map[y1][x1] = GATE;
    map[y2][x2] = GATE;
    gate1 = {y1, x1};
    gate2 = {y2, x2};
}

void updateItemAndGatePositions(std::vector<std::vector<int>> &map, std::pair<int, int> &gate1, std::pair<int, int> &gate2) {
    // 기존 아이템 위치를 초기화
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            if (map[i][j] == GROWTH_ITEM || map[i][j] == POISON_ITEM) {
                map[i][j] = 0;
            } else if (map[i][j] == GATE) {
                map[i][j] = WALL;  // 게이트를 원래 벽으로 복구
            }
        }
    }

    // 새로운 위치에 아이템 생성
    generateItem(map, GROWTH_ITEM);
    generateItem(map, POISON_ITEM);

    // 새로운 위치에 게이트 생성
    generateGate(map, gate1, gate2);
}

// 초기화 함수
void initGame(WINDOW *win, std::vector<std::vector<int>> &map, std::vector<SnakeSegment> &snake, Direction &dir, int &score, std::pair<int, int> &gate1, std::pair<int, int> &gate2, int stage) {
    // 맵 초기화
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            map[i][j] = 0;
        }
    }
    // 벽 생성
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        map[i][0] = map[i][MAP_WIDTH - 1] = WALL;
    }
    for (int i = 0; i < MAP_WIDTH; ++i) {
        map[0][i] = map[MAP_HEIGHT - 1][i] = WALL;
    }
    
    // 스테이지별로 맵 지형 변경
    if (stage == 2) {
        for (int i = 5; i < 15; ++i) {
            map[5][i] = IMMUNE_WALL;
            map[15][i] = IMMUNE_WALL;
        }
        for (int i = 5; i < 15; ++i) {
            map[i][10] = IMMUNE_WALL;
            map[i][30] = IMMUNE_WALL;
        }
    } else if (stage == 3) {
        for (int i = 5; i < 15; ++i) {
            map[i][5] = IMMUNE_WALL;
            map[i][15] = IMMUNE_WALL;
            map[5][i + 10] = IMMUNE_WALL;
            map[15][i + 10] = IMMUNE_WALL;
        }
    } else if (stage == 4) {
        for (int i = 0; i < MAP_HEIGHT; ++i) {
            if (i % 2 == 0) {
                map[i][MAP_WIDTH / 2] = IMMUNE_WALL;
            }
        }
        for (int i = 0; i < MAP_WIDTH; ++i) {
            if (i % 4 == 0) {
                map[MAP_HEIGHT / 2][i] = IMMUNE_WALL;
            }
        }
    } else {
        for (int i = 5; i < 15; ++i) {
            map[10][i] = IMMUNE_WALL;
        }
    }
    
    // Snake 초기 위치 설정
    snake.clear();
    snake.push_back({MAP_HEIGHT / 2, MAP_WIDTH / 2});
    snake.push_back({MAP_HEIGHT / 2, MAP_WIDTH / 2 - 1});
    snake.push_back({MAP_HEIGHT / 2, MAP_WIDTH / 2 - 2});
    for (const auto &seg : snake) {
        map[seg.y][seg.x] = (seg == snake.front()) ? SNAKE_HEAD : SNAKE_BODY;
    }
    dir = RIGHT;
    score = 0;

    // 아이템 생성
    generateItem(map, GROWTH_ITEM);
    generateItem(map, POISON_ITEM);
    generateItem(map, SPECIAL_ITEM);  // 새로운 아이템 생성

    // 게이트 생성
    generateGate(map, gate1, gate2);
}

// 맵 출력 함수
void drawMap(WINDOW *win, const std::vector<std::vector<int>> &map, const std::vector<SnakeSegment> &snake, Direction dir) {
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            switch (map[i][j]) {
                case WALL:
                    mvwaddch(win, i, j, '#');
                    break;
                case IMMUNE_WALL:
                    mvwaddch(win, i, j, '#');
                    break;
                case SNAKE_HEAD:
                    if (dir == UP) {
                        mvwaddch(win, i, j, '^');
                    } else if (dir == DOWN) {
                        mvwaddch(win, i, j, 'v');
                    } else if (dir == LEFT) {
                        mvwaddch(win, i, j, '<');
                    } else if (dir == RIGHT) {
                        mvwaddch(win, i, j, '>');
                    }
                    break;
                case SNAKE_BODY:
                    mvwaddch(win, i, j, 'o');
                    break;
                case GROWTH_ITEM:
                    mvwaddch(win, i, j, '+');
                    break;
                case POISON_ITEM:
                    mvwaddch(win, i, j, '-');
                    break;
                case GATE:
                    mvwaddch(win, i, j, 'O');
                    break;
                case SPECIAL_ITEM:
                    mvwaddch(win, i, j, '*');
                    break;
                default:
                    mvwaddch(win, i, j, ' ');
                    break;
            }
        }
    }
    wrefresh(win);
}

// 점수판 출력 함수
void drawScoreBoard(WINDOW *win, int score, int timeElapsed, int growthItems, int poisonItems, int gateUses) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 1, 2, "* TOTAL *");
    mvwprintw(win, 2, 2, "Score   : %d", score);
    mvwprintw(win, 3, 2, "Time    : %d", timeElapsed);
    mvwprintw(win, 4, 2, "+       : %d", growthItems);
    mvwprintw(win, 5, 2, "-       : %d", poisonItems);
    mvwprintw(win, 6, 2, "G       : %d", gateUses);
    wrefresh(win);
}

// 미션 상태 출력 함수
void drawMissionBoard(WINDOW *win, int stage, int maxLength, int growthItems, int poisonItems, int gateUses) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 1, 2, "* STAGE %d *", stage);
    mvwprintw(win, 2, 2, "Length : %d / 10 %s", maxLength, (maxLength >= 10) ? "( O )" : "( - )");
    mvwprintw(win, 3, 2, "+      : %d / 5  %s", growthItems, (growthItems >= 5) ? "( O )" : "( - )");
    mvwprintw(win, 4, 2, "-      : %d / 2  %s", poisonItems, (poisonItems <= 2) ? "( O )" : "( - )");
    mvwprintw(win, 5, 2, "G      : %d / 1  %s", gateUses, (gateUses >= 1) ? "( O )" : "( - )");
    wrefresh(win);
}

// 게이트 통과 시 새로운 방향 계산 함수
Direction getNewDirection(Direction dir, std::pair<int, int> gate, std::vector<std::vector<int>> &map) {
    if (gate.first == 0) {
        return DOWN;
    } else if (gate.first == MAP_HEIGHT - 1) {
        return UP;
    } else if (gate.second == 0) {
        return RIGHT;
    } else if (gate.second == MAP_WIDTH - 1) {
        return LEFT;
    } else {
        if (dir == UP || dir == DOWN) {
            if (map[gate.first][gate.second - 1] != WALL && map[gate.first][gate.second - 1] != IMMUNE_WALL) {
                return LEFT;
            } else {
                return RIGHT;
            }
        } else {
            if (map[gate.first - 1][gate.second] != WALL && map[gate.first - 1][gate.second] != IMMUNE_WALL) {
                return UP;
            } else {
                return DOWN;
            }
        }
    }
}

// Snake 이동 함수
bool moveSnake(std::vector<std::vector<int>> &map, std::vector<SnakeSegment> &snake, Direction &dir, int &score, int &growthItems, int &poisonItems, int &gateUses, std::pair<int, int> &gate1, std::pair<int, int> &gate2) {
    int newY = snake.front().y;
    int newX = snake.front().x;
    switch (dir) {
        case UP: newY--; break;
        case DOWN: newY++; break;
        case LEFT: newX--; break;
        case RIGHT: newX++; break;
    }
    // 진행 방향의 반대 방향으로 이동하면 죽음
    if ((dir == UP && newY > snake.front().y) || 
        (dir == DOWN && newY < snake.front().y) || 
        (dir == LEFT && newX > snake.front().x) || 
        (dir == RIGHT && newX < snake.front().x)) {
        return false;
    }
    // 충돌 체크
    if (newY < 0 || newY >= MAP_HEIGHT || newX < 0 || newX >= MAP_WIDTH || map[newY][newX] == WALL || map[newY][newX] == SNAKE_BODY || map[newY][newX] == IMMUNE_WALL) {
        return false;
    }

    // 게이트 통과
    if (map[newY][newX] == GATE) {
        gateUses++;
        if (newY == gate1.first && newX == gate1.second) {
            newY = gate2.first;
            newX = gate2.second;
            dir = getNewDirection(dir, gate2, map);
        } else {
            newY = gate1.first;
            newX = gate1.second;
            dir = getNewDirection(dir, gate1, map);
        }
        switch (dir) {
            case UP: newY--; break;
            case DOWN: newY++; break;
            case LEFT: newX--; break;
            case RIGHT: newX++; break;
        }
    }

    // 아이템 상호작용
    if (map[newY][newX] == GROWTH_ITEM) {
        score += 2; // 추가 점수 부여
        growthItems++;
        snake.push_back(snake.back());
        generateItem(map, GROWTH_ITEM);
    } else if (map[newY][newX] == POISON_ITEM) {
        score--;
        poisonItems++;
        if (snake.size() > 1) {
            map[snake.back().y][snake.back().x] = 0;
            snake.pop_back();
        } else {
            return false; // Snake의 길이가 1 이하가 되면 게임 종료
        }
        generateItem(map, POISON_ITEM);
    } else if (map[newY][newX] == SPECIAL_ITEM) {
        score += 5; // 새로운 아이템을 먹으면 점수 추가
        generateItem(map, SPECIAL_ITEM);
        speed = 200000; // 1초 동안 속도 느리게 (0.2초 대기)
        slowDownEndTime = time(0) + 1; // 1초 후에 속도 원복
    }

    // Snake 머리 이동
    SnakeSegment newHead = {newY, newX};
    snake.insert(snake.begin(), newHead);
    map[newY][newX] = SNAKE_HEAD;

    // 꼬리 제거
    SnakeSegment tail = snake.back();
    map[tail.y][tail.x] = 0;
    snake.pop_back();

    // 몸통 갱신
    for (size_t i = 1; i < snake.size(); ++i) {
        map[snake[i].y][snake[i].x] = SNAKE_BODY;
    }

    return true;
}
bool isSnakePassingGate(const std::vector<SnakeSegment> &snake, const std::pair<int, int> &gate1, const std::pair<int, int> &gate2) {
    for (const auto &segment : snake) {
        if ((segment.y == gate1.first && segment.x == gate1.second) || (segment.y == gate2.first && segment.x == gate2.second)) {
            return true;
        }
    }
    return false;
}

// 게임 방법 화면 출력 함수
void showInstructions() {
    clear();  // 화면을 지워서 겹치는 것을 방지
    refresh();  // 화면을 새로 고침

    int height = 15;
    int width = 50;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW *instructionsWin = newwin(height, width, start_y, start_x);
    box(instructionsWin, 0, 0);
    mvwprintw(instructionsWin, 1, 2, "Game Instructions:");
    mvwprintw(instructionsWin, 3, 2, "Item descriptions:");
    mvwprintw(instructionsWin, 4, 2, "+ : Growth item (Increases length)");
    mvwprintw(instructionsWin, 5, 2, "- : Poison item (Decreases length)");
    mvwprintw(instructionsWin, 6, 2, "* : Gate (Teleports snake)");
    mvwprintw(instructionsWin, 7, 2, "* : Special item (Adds score)");
    mvwprintw(instructionsWin, 9, 2, "Example Missions:");
    mvwprintw(instructionsWin, 10, 2, "Length (10) : Reach length 10");
    mvwprintw(instructionsWin, 11, 2, "' + ' (5)  : Collect 5 growth items");
    mvwprintw(instructionsWin, 12, 2, "' - ' (2)  : Collect 2 or fewer poison items");
    mvwprintw(instructionsWin, 13, 2, "' G ' (1)  : Pass through gate at least once");
    mvwprintw(instructionsWin, 14, 2, "Press any key to return to menu");
    wrefresh(instructionsWin);
    nodelay(stdscr, FALSE);
    wgetch(instructionsWin);
    nodelay(stdscr, TRUE);
    delwin(instructionsWin);

    clear();  // 화면을 지워서 겹치는 것을 방지
    refresh();  // 화면을 새로 고침
}

// 옵션 출력 함수
int showOptions() {
    clear();  // 화면을 지워서 겹치는 것을 방지
    refresh();  // 화면을 새로 고침

    mvprintw(4, (COLS - 50) / 2, "1. Start Game");
    mvprintw(6, (COLS - 50) / 2, "2. Instructions");
    mvprintw(8, (COLS - 50) / 2, "3. Exit");
    refresh();
    int choice = getch();
    return choice;
}

// 스테이지 클리어 메시지 출력 함수
void showStageClear(WINDOW *win) {
    int height = 5;
    int width = 30;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW *clearWin = newwin(height, width, start_y, start_x);
    box(clearWin, 0, 0);
    mvwprintw(clearWin, 2, 3, "Stage Clear!");
    wrefresh(clearWin);
    sleep(1);
    for (int i = 3; i > 0; --i) {
        mvwprintw(clearWin, 3, 3, "Next stage in %d...", i);
        wrefresh(clearWin);
        sleep(1);
    }
    delwin(clearWin);
    touchwin(win);
    wrefresh(win);
}

// 축하 메시지 출력 함수
void showCongrats(WINDOW *win) {
    int height = 5;
    int width = 30;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW *congratsWin = newwin(height, width, start_y, start_x);
    box(congratsWin, 0, 0);
    mvwprintw(congratsWin, 2, 5, "Congratulations!");
    wrefresh(congratsWin);
    nodelay(stdscr, FALSE);
    wgetch(congratsWin);
    nodelay(stdscr, TRUE);
    delwin(congratsWin);
    touchwin(win);
    wrefresh(win);
}

// 게임 오버 메시지 출력 함수
void showGameOver(int score, int timeElapsed) {
    clear();  // 화면을 지워서 겹치는 것을 방지
    refresh();  // 화면을 새로 고침

    int height = 7;
    int width = 40;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW *gameOverWin = newwin(height, width, start_y, start_x);
    box(gameOverWin, 0, 0);
    mvwprintw(gameOverWin, 1, 2, "Mission Failed!");
    mvwprintw(gameOverWin, 2, 2, "Final Score : %d", score);
    mvwprintw(gameOverWin, 3, 2, "Time        : %d", timeElapsed);
    mvwprintw(gameOverWin, 5, 2, "Press Enter to restart...");
    wrefresh(gameOverWin);
    nodelay(stdscr, FALSE);
    while (getch() != '\n') { }  // 엔터 키 입력 대기
    nodelay(stdscr, TRUE);
    delwin(gameOverWin);

    clear();  // 화면을 지워서 겹치는 것을 방지
    refresh();  // 화면을 새로 고침
}

int main() {
    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_GREEN); // 벽 색상 설정
    init_pair(2, COLOR_BLACK, COLOR_RED); // Immune Wall 색상 설정
    init_pair(3, COLOR_WHITE, COLOR_BLUE); // Snake Head 색상 설정
    init_pair(4, COLOR_WHITE, COLOR_BLACK); // Snake Body 색상 설정
    init_pair(5, COLOR_RED, COLOR_BLACK); // Growth Item 색상 설정
    init_pair(6, COLOR_BLACK, COLOR_BLACK); // Poison Item 색상 설정
    init_pair(7, COLOR_YELLOW, COLOR_BLACK); // Gate 색상 설정
    init_pair(8, COLOR_CYAN, COLOR_BLACK); // Special Item 색상 설정

    noecho(); // 입력된 문자를 화면에 출력하지 않음
    curs_set(0); // 커서를 보이지 않게 설정
    keypad(stdscr, TRUE); // 특수키 입력 활성화
    nodelay(stdscr, FALSE); // 입력 대기 모드 비활성화
    srand(time(0)); // 난수 생성을 위한 시드 설정

    int start_y = (LINES - MAP_HEIGHT) / 2;
    int start_x_game = (COLS - MAP_WIDTH) / 4;  // 게임 창을 중앙에서 왼쪽으로 이동
    WINDOW *win = newwin(MAP_HEIGHT, MAP_WIDTH, start_y, start_x_game);
    WINDOW *scoreWin = newwin(10, 30, start_y, start_x_game + MAP_WIDTH + 1);
    WINDOW *missionWin = newwin(8, 30, start_y + 11, start_x_game + MAP_WIDTH + 1);

    int currentStage = 1;
    const int totalStages = 4;

    while (true) {
    int choice = showOptions();
    if (choice == '1') {
        nodelay(stdscr, TRUE);
        clear();
        refresh();

        while (true) {
            for (currentStage = 1; currentStage <= totalStages; ++currentStage) {
                std::vector<std::vector<int>> map(MAP_HEIGHT, std::vector<int>(MAP_WIDTH));
                std::vector<SnakeSegment> snake;
                Direction dir;
                int score, growthItems = 0, poisonItems = 0, gateUses = 0;
                std::pair<int, int> gate1, gate2;
                int maxLength = 3;
                int startTime = time(0);

                initGame(win, map, snake, dir, score, gate1, gate2, currentStage);
                drawMap(win, map, snake, dir);

                bool missionComplete = false;
                int lastUpdateTime = startTime;
                while (!missionComplete) {
                    int ch = getch();
                    switch (ch) {
                        case KEY_UP:
                            if (dir != DOWN) dir = UP;
                            break;
                        case KEY_DOWN:
                            if (dir != UP) dir = DOWN;
                            break;
                        case KEY_LEFT:
                            if (dir != RIGHT) dir = LEFT;
                            break;
                        case KEY_RIGHT:
                            if (dir != LEFT) dir = RIGHT;
                            break;
                        default:
                            break;
                    }
                    score++; // 움직일 때마다 점수 증가
                    if (!moveSnake(map, snake, dir, score, growthItems, poisonItems, gateUses, gate1, gate2)) {
                        int timeElapsed = time(0) - startTime;
                        showGameOver(score, timeElapsed);  // 게임 오버 메시지 출력
                        missionComplete = false;
                        break; // 충돌 발생 시 게임 종료
                    }
                    if (snake.size() > maxLength) {
                        maxLength = snake.size();
                    }
                    int timeElapsed = time(0) - startTime;
                    drawMap(win, map, snake, dir);
                    drawScoreBoard(scoreWin, score, timeElapsed, growthItems, poisonItems, gateUses);
                    drawMissionBoard(missionWin, currentStage, maxLength, growthItems, poisonItems, gateUses);

                    // 5초마다 아이템 위치 업데이트
                    if (time(0) - lastUpdateTime >= 5) {
                        if (!isSnakePassingGate(snake, gate1, gate2)) {
                            updateItemAndGatePositions(map, gate1, gate2);
                        }
                        lastUpdateTime = time(0);
                    }
                    if (slowDownEndTime > 0 && time(0) >= slowDownEndTime) {
                            speed = 100000; // 기본 속도로 원복
                            slowDownEndTime = 0; // 효과 시간 초기화
                    }
                    usleep(speed); // 0.1초 대기 (뱀의 속도를 빠르게)

                    // 미션 달성 여부 확인
                    if (maxLength >= 10 && growthItems >= 5 && poisonItems <= 2 && gateUses >= 1) {
                        missionComplete = true;
                        if (currentStage == totalStages) {
                            showCongrats(win);
                        } else {
                            showStageClear(win);
                        }
                    }
                }

                if (!missionComplete) {
                    break; // 게임 종료
                }
            }

            // 스테이지 1로 리셋
            currentStage = 1;
        }

        nodelay(stdscr, FALSE);
    } else if (choice == '2') {
        showInstructions();
    } else if (choice == '3') {
        break;
    }
}

    delwin(win);
    delwin(scoreWin);
    delwin(missionWin);
    endwin();
    return 0;
}
