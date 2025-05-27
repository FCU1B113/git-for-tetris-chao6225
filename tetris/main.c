#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>

#define MAX_LEVEL 10
#define INITIAL_FALL_DELAY 500
#define MIN_FALL_DELAY 100

#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20

#define FALL_DELAY 500
#define RENDER_DELAY 100

// 鍵盤對照表
#define LEFT_KEY 0x25
#define RIGHT_KEY 0x27 
#define ROTATE_KEY 0x26 
#define DOWN_KEY 0x28 
#define FALL_KEY 0x20 
#define SWITCH_KEY 0x51 // Q 鍵

// 判斷按鍵是否有被按下的函式
#define LEFT_FUNC() GetAsyncKeyState(LEFT_KEY) & 0x8000
#define RIGHT_FUNC() GetAsyncKeyState(RIGHT_KEY) & 0x8000
#define ROTATE_FUNC() GetAsyncKeyState(ROTATE_KEY) & 0x8000
#define DOWN_FUNC() GetAsyncKeyState(DOWN_KEY) & 0x8000
#define FALL_FUNC() GetAsyncKeyState(FALL_KEY) & 0x8000
#define SWITCH_FUNC() GetAsyncKeyState(SWITCH_KEY) & 0x8000



typedef enum
{
    RED = 41,
    GREEN,
    YELLOW,
    BLUE,
    PURPLE,
    CYAN,
    WHITE,
    BLACK = 0,
} Color;

typedef enum
{
    EMPTY = -1,
    I,
    J,
    L,
    O,
    S,
    T,
    Z
} ShapeId;

typedef struct
{
    ShapeId shape;
    Color color;
    int size;
    char rotates[4][4][4];
} Shape;

typedef struct
{
    int x;
    int y;
    int line;
    int rotate;
    int fallTime;
    int level;
    int score;
    int combo;
    ShapeId queue[4];
    ShapeId switchShape;
    bool switched;
} State;

typedef struct
{
    Color color;
    ShapeId shape;
    bool current;
} Block;

Shape shapes[7] = {
    {.shape = I,
     .color = CYAN,
     .size = 4,
     .rotates =
         {
             {{0, 0, 0, 0},
              {1, 1, 1, 1},
              {0, 0, 0, 0},
              {0, 0, 0, 0}},
             {{0, 0, 1, 0},
              {0, 0, 1, 0},
              {0, 0, 1, 0},
              {0, 0, 1, 0}},
             {{0, 0, 0, 0},
              {0, 0, 0, 0},
              {1, 1, 1, 1},
              {0, 0, 0, 0}},
             {{0, 1, 0, 0},
              {0, 1, 0, 0},
              {0, 1, 0, 0},
              {0, 1, 0, 0}}}},
    {.shape = J,
     .color = BLUE,
     .size = 3,
     .rotates =
         {
             {{1, 0, 0},
              {1, 1, 1},
              {0, 0, 0}},
             {{0, 1, 1},
              {0, 1, 0},
              {0, 1, 0}},
             {{0, 0, 0},
              {1, 1, 1},
              {0, 0, 1}},
             {{0, 1, 0},
              {0, 1, 0},
              {1, 1, 0}}}},
    {.shape = L,
     .color = YELLOW,
     .size = 3,
     .rotates =
         {
             {{0, 0, 1},
              {1, 1, 1},
              {0, 0, 0}},
             {{0, 1, 0},
              {0, 1, 0},
              {0, 1, 1}},
             {{0, 0, 0},
              {1, 1, 1},
              {1, 0, 0}},
             {{1, 1, 0},
              {0, 1, 0},
              {0, 1, 0}}}},
    {.shape = O,
     .color = WHITE,
     .size = 2,
     .rotates =
         {
             {{1, 1},
              {1, 1}},
             {{1, 1},
              {1, 1}},
             {{1, 1},
              {1, 1}},
             {{1, 1},
              {1, 1}}}},
    {.shape = S,
     .color = GREEN,
     .size = 3,
     .rotates =
         {
             {{0, 1, 1},
              {1, 1, 0},
              {0, 0, 0}},
             {{0, 1, 0},
              {0, 1, 1},
              {0, 0, 1}},
             {{0, 0, 0},
              {0, 1, 1},
              {1, 1, 0}},
             {{1, 0, 0},
              {1, 1, 0},
              {0, 1, 0}}}},
    {.shape = T,
     .color = PURPLE,
     .size = 3,
     .rotates =
         {
             {{0, 1, 0},
              {1, 1, 1},
              {0, 0, 0}},

             {{0, 1, 0},
              {0, 1, 1},
              {0, 1, 0}},
             {{0, 0, 0},
              {1, 1, 1},
              {0, 1, 0}},
             {{0, 1, 0},
              {1, 1, 0},
              {0, 1, 0}}}},
    {.shape = Z,
     .color = RED,
     .size = 3,
     .rotates =
         {
             {{1, 1, 0},
              {0, 1, 1},
              {0, 0, 0}},
             {{0, 0, 1},
              {0, 1, 1},
              {0, 1, 0}},
             {{0, 0, 0},
              {1, 1, 0},
              {0, 1, 1}},
             {{0, 1, 0},
              {1, 1, 0},
              {1, 0, 0}}}},
};

void setBlock(Block* block, Color color, ShapeId shape, bool current)
{
    block->color = color;
    block->shape = shape;
    block->current = current;
}

void resetBlock(Block* block)
{
    block->color = BLACK;
    block->shape = EMPTY;
    block->current = false;
}

void printCanvas(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state)
{
    printf("\033[1;1H\n");
    for (int i = 0; i < CANVAS_HEIGHT; i++)
    {
        printf("|");
        for (int j = 0; j < CANVAS_WIDTH; j++)
        {
            printf("\033[%dm\u3000", canvas[i][j].color);
        }
        printf("\033[0m|\n");
    }

    // 顯示Switch:
    printf("\033[%d;%dHSwitch:", 1, CANVAS_WIDTH * 2 + 5);
    Shape switchData = shapes[state->switchShape];
    for (int j = 0; j < 4; j++)
    {
        printf("\033[%d;%dH", 1 + j, CANVAS_WIDTH * 2 + 15);
        for (int k = 0; k < 4; k++)
        {
            if (j < switchData.size && k < switchData.size && switchData.rotates[0][j][k])
            {
                printf("\x1b[%dm  ", switchData.color);
            }
            else
            {
                printf("\x1b[0m  ");
            }
        }
    }////



    // 輸出Next:
    printf("\033[%d;%dHNext:", 6, CANVAS_WIDTH * 2 + 5);
    // 輸出有甚麼方塊
    for (int i = 1; i <= 3; i++)
    {
        Shape shapeData = shapes[state->queue[i]];
        for (int j = 0; j < 4; j++)
        {
            printf("\033[%d;%dH", 6 + (i - 1) * 4 + j, CANVAS_WIDTH * 2 + 15);
            for (int k = 0; k < 4; k++)
            {
                if (j < shapeData.size && k < shapeData.size && shapeData.rotates[0][j][k])
                {
                    printf("\x1b[%dm  ", shapeData.color);
                }
                else
                {
                    printf("\x1b[0m  ");
                }
            }
        }
    }
    // 顯示分數與等級
    printf("\033[%d;%dHLine: %d", CANVAS_HEIGHT - 5, CANVAS_WIDTH * 2 + 5, state->line);
    printf("\033[%d;%dHLevel: %d", CANVAS_HEIGHT - 4, CANVAS_WIDTH * 2 + 5, state->level);
    printf("\033[%d;%dHScore: %d", CANVAS_HEIGHT - 3, CANVAS_WIDTH * 2 + 5, state->score);
    printf("\033[%d;%dHCombo: %d", CANVAS_HEIGHT - 2, CANVAS_WIDTH * 2 + 5, state->combo);

    return;
}

bool move(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], int originalX, int originalY, int originalRotate, int newX, int newY, int newRotate, ShapeId shapeId)
{
    Shape shapeData = shapes[shapeId];
    int size = shapeData.size;

    // 判斷方塊有沒有不符合條件
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (shapeData.rotates[newRotate][i][j])
            {
                // 判斷有沒有出去邊界
                if (newX + j < 0 || newX + j >= CANVAS_WIDTH || newY + i < 0 || newY + i >= CANVAS_HEIGHT)
                {
                    return false;
                }
                // 判斷有沒有碰到別的方塊
                if (!canvas[newY + i][newX + j].current && canvas[newY + i][newX + j].shape != EMPTY)
                {
                    return false;
                }
            }
        }
    }

    // 移除方塊舊的位置
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (shapeData.rotates[originalRotate][i][j])
            {
                resetBlock(&canvas[originalY + i][originalX + j]);
            }
        }
    }

    // 移動方塊至新的位置
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (shapeData.rotates[newRotate][i][j])
            {
                setBlock(&canvas[newY + i][newX + j], shapeData.color, shapeId, true);
            }
        }
    }

    return true;
}

int clearLine(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH])
{
    for (int i = 0; i < CANVAS_HEIGHT; i++)
    {
        for (int j = 0; j < CANVAS_WIDTH; j++)
        {
            if (canvas[i][j].current)
            {
                canvas[i][j].current = false;
            }
        }
    }

    int linesCleared = 0;
    for (int i = CANVAS_HEIGHT - 1; i >= 0; i--)
    {
        bool isFull = true;
        for (int j = 0; j < CANVAS_WIDTH; j++)
        {
            if (canvas[i][j].shape == EMPTY) {
                isFull = false;
                break;
            }
        }
        if (isFull) {
            linesCleared += 1;

            for (int j = i; j > 0; j--)
            {
                for (int k = 0; k < CANVAS_WIDTH; k++)
                {
                    setBlock(&canvas[j][k], canvas[j - 1][k].color, canvas[j - 1][k].shape, false);
                    resetBlock(&canvas[j - 1][k]);
                }
            }
            i++;
        }
    }
    return linesCleared;
}


void logic(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state)
{
    // 根據分數調整等級（每 5 行升級）
    state->level = state->line / 5;
    if (state->level > MAX_LEVEL) state->level = MAX_LEVEL;

    int currentFallDelay = INITIAL_FALL_DELAY - (state->level * 40);
    if (currentFallDelay < MIN_FALL_DELAY) currentFallDelay = MIN_FALL_DELAY;

    if (ROTATE_FUNC())
    {
        int newRotate = (state->rotate + 1) % 4;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y, newRotate, state->queue[0]))
        {
            state->rotate = newRotate;
        }
    }
    else if (LEFT_FUNC())
    {
        if (move(canvas, state->x, state->y, state->rotate, state->x - 1, state->y, state->rotate, state->queue[0]))
        {
            state->x -= 1;
        }
    }
    else if (RIGHT_FUNC())
    {
        if (move(canvas, state->x, state->y, state->rotate, state->x + 1, state->y, state->rotate, state->queue[0]))
        {
            state->x += 1;
        }
    }
    else if (DOWN_FUNC())
    {
        state->fallTime = FALL_DELAY;
    }
    else if (FALL_FUNC())
    {
        state->fallTime += currentFallDelay * CANVAS_HEIGHT;
    }
    else if (SWITCH_FUNC() && !state->switched)
    {
        // 清除目前的方塊
        Shape oldShape = shapes[state->queue[0]];
        for (int i = 0; i < oldShape.size; i++)
        {
            for (int j = 0; j < oldShape.size; j++)
            {
                if (oldShape.rotates[state->rotate][i][j])
                {
                    resetBlock(&canvas[state->y + i][state->x + j]);
                }
            }
        }

        // 更換成 switchShape
        state->queue[0] = state->switchShape;
        state->switchShape = rand() % 7;
        state->rotate = 0;
        state->switched = true;

        Shape newShape = shapes[state->queue[0]];
        for (int i = 0; i < newShape.size; i++)
        {
            for (int j = 0; j < newShape.size; j++)
            {
                if (newShape.rotates[0][i][j])
                {
                    setBlock(&canvas[state->y + i][state->x + j], newShape.color, state->queue[0], true);
                }
            }
        }
    }////


    state->fallTime += RENDER_DELAY;

    while (state->fallTime >= currentFallDelay)
    {
        state->fallTime -= currentFallDelay;

        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y + 1, state->rotate, state->queue[0]))
        {
            state->y++;
        }
        else
        {
            // 這裡方塊已經落地，固定它
            // 將當前方塊的 current 設為 false，表示不再是活動方塊
            Shape shapeData = shapes[state->queue[0]];
            for (int i = 0; i < shapeData.size; i++)
            {
                for (int j = 0; j < shapeData.size; j++)
                {
                    if (shapeData.rotates[state->rotate][i][j])
                    {
                        canvas[state->y + i][state->x + j].current = false;
                    }
                }
            }

            state->switched = false;
            state->x = CANVAS_WIDTH / 2;
            state->y = 0;
            state->rotate = 0;
            state->fallTime = 0;
            state->queue[0] = state->queue[1];
            state->queue[1] = state->queue[2];
            state->queue[2] = state->queue[3];
            state->queue[3] = rand() % 7;

            int lines = clearLine(canvas);
            state->line += lines;

            if (lines > 0) {
                // 加 combo 分數
                state->combo++;
                // 基礎得分
                switch (lines) {
                case 1:
                    state->score += 40 * (state->level + 1);
                    break;
                case 2:
                    state->score += 100 * (state->level + 1);
                    break;
                case 3:
                    state->score += 300 * (state->level + 1);
                    break;
                case 4:
                    state->score += 1200 * (state->level + 1);
                    break;
                }

                // combo 額外得分
                state->score += 50 * state->combo * (state->level + 1);

            }
            else {
                state->combo = 0; // 連擊中斷
            }



            if (!move(canvas, state->x, state->y, state->rotate, state->x, state->y, state->rotate, state->queue[0]))
            {
                printf("\033[%d;%dH\x1b[41m   GAME OVER   \x1b[0m", CANVAS_HEIGHT + 2, CANVAS_WIDTH * 2 + 5);

                exit(0);//結束遊戲
            }
        }

    }
    return;
}

int main()
{
    srand(time(NULL));
    State state = {
        .x = CANVAS_WIDTH / 2,
        .y = 0,
        .line = 0,
        .rotate = 0,
        .fallTime = 0,
        .level = 0,
        .combo = 0
    };

    state.switchShape = rand() % 7;
    state.switched = false;

    for (int i = 0; i < 4; i++)
    {
        state.queue[i] = rand() % 7;
    }

    Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
    for (int i = 0; i < CANVAS_HEIGHT; i++)
    {
        for (int j = 0; j < CANVAS_WIDTH; j++)
        {
            resetBlock(&canvas[i][j]);
        }
    }

    Shape shapeData = shapes[state.queue[0]];

    for (int i = 0; i < shapeData.size; i++)
    {
        for (int j = 0; j < shapeData.size; j++)
        {
            if (shapeData.rotates[0][i][j])
            {
                setBlock(&canvas[state.y + i][state.x + j], shapeData.color, state.queue[0], true);
            }
        }
    }

    while (1)
    {
        printCanvas(canvas, &state);
        logic(canvas, &state);
        Sleep(100);
    }

    return 0;
}
