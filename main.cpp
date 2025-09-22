#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <random>
#include <ctime>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// 游戏常量
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int BLOCK_SIZE = 30;
const int WINDOW_WIDTH = BOARD_WIDTH * BLOCK_SIZE + 200;
const int WINDOW_HEIGHT = BOARD_HEIGHT * BLOCK_SIZE + 100;

// 颜色定义 (Direct2D使用D2D1::ColorF)
const D2D1::ColorF COLORS[] = {
    D2D1::ColorF::Black,        // 空白 - 黑色
    D2D1::ColorF::Red,          // I型 - 红色
    D2D1::ColorF::Lime,         // O型 - 绿色
    D2D1::ColorF::Blue,         // T型 - 蓝色
    D2D1::ColorF::Yellow,       // S型 - 黄色
    D2D1::ColorF::Magenta,      // Z型 - 紫色
    D2D1::ColorF::Cyan,         // J型 - 青色
    D2D1::ColorF::Orange        // L型 - 橙色
};

// 方块形状定义（4x4矩阵）
const int TETROMINO_SHAPES[7][4][4][4] = {
    // I型
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}
    },
    // O型
    {
        {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}
    },
    // T型
    {
        {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,1,0,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,1,0,0}},
        {{0,0,0,0},{0,1,0,0},{1,1,0,0},{0,1,0,0}}
    },
    // S型
    {
        {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,0,0,0},{0,1,1,0},{1,1,0,0}},
        {{0,0,0,0},{1,0,0,0},{1,1,0,0},{0,1,0,0}}
    },
    // Z型
    {
        {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,0,1,0},{0,1,1,0},{0,1,0,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,0,0},{0,1,1,0}},
        {{0,0,0,0},{0,1,0,0},{1,1,0,0},{1,0,0,0}}
    },
    // J型
    {
        {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{0,1,0,0},{0,1,0,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,1,0,0},{0,1,0,0},{1,1,0,0}}
    },
    // L型
    {
        {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,0,0},{0,1,0,0},{0,1,1,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,0},{1,0,0,0}},
        {{0,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,0,0}}
    }
};

// 游戏状态枚举
enum GameState {
    PLAYING,
    GAME_OVER,
    PAUSED
};

// 方块类
class Tetromino {
public:
    int type;
    int rotation;
    int x, y;
    
    Tetromino() : type(0), rotation(0), x(0), y(0) {}
    
    Tetromino(int t) : type(t), rotation(0), x(BOARD_WIDTH/2 - 2), y(0) {}
    
    bool getBlock(int dx, int dy) const {
        if (dx < 0 || dx >= 4 || dy < 0 || dy >= 4) return false;
        return TETROMINO_SHAPES[type][rotation][dy][dx] != 0;
    }
    
    void rotate() {
        rotation = (rotation + 1) % 4;
    }
};

// 游戏类
class TetrisGame {
private:
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    Tetromino currentPiece;
    Tetromino nextPiece;
    GameState state;
    int score;
    int level;
    int lines;
    std::mt19937 rng;
    
public:
    TetrisGame() : state(PLAYING), score(0), level(1), lines(0) {
        // 初始化游戏板
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[y][x] = 0;
            }
        }
        
        // 初始化随机数生成器
        rng.seed(static_cast<unsigned>(time(nullptr)));
        
        // 生成第一个方块
        spawnNewPiece();
        generateNextPiece();
    }
    
    void spawnNewPiece() {
        currentPiece = nextPiece;
        generateNextPiece();
        
        // 检查游戏结束
        if (!isValidPosition(currentPiece.x, currentPiece.y, currentPiece.rotation)) {
            state = GAME_OVER;
        }
    }
    
    void generateNextPiece() {
        std::uniform_int_distribution<int> dist(0, 6);
        nextPiece = Tetromino(dist(rng));
    }
    
    bool isValidPosition(int x, int y, int rotation) const {
        for (int dy = 0; dy < 4; dy++) {
            for (int dx = 0; dx < 4; dx++) {
                if (TETROMINO_SHAPES[currentPiece.type][rotation][dy][dx]) {
                    int newX = x + dx;
                    int newY = y + dy;
                    
                    // 检查边界
                    if (newX < 0 || newX >= BOARD_WIDTH || newY >= BOARD_HEIGHT) {
                        return false;
                    }
                    
                    // 检查是否与已有方块重叠
                    if (newY >= 0 && board[newY][newX] != 0) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    void lockPiece() {
        // 将当前方块锁定到游戏板上
        for (int dy = 0; dy < 4; dy++) {
            for (int dx = 0; dx < 4; dx++) {
                if (TETROMINO_SHAPES[currentPiece.type][currentPiece.rotation][dy][dx]) {
                    int x = currentPiece.x + dx;
                    int y = currentPiece.y + dy;
                    if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH) {
                        board[y][x] = currentPiece.type + 1;
                    }
                }
            }
        }
        
        // 检查并清除完整的行
        clearLines();
        
        // 生成新方块
        spawnNewPiece();
    }
    
    void clearLines() {
        int linesCleared = 0;
        
        for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
            bool fullLine = true;
            for (int x = 0; x < BOARD_WIDTH; x++) {
                if (board[y][x] == 0) {
                    fullLine = false;
                    break;
                }
            }
            
            if (fullLine) {
                // 移除这一行
                for (int moveY = y; moveY > 0; moveY--) {
                    for (int x = 0; x < BOARD_WIDTH; x++) {
                        board[moveY][x] = board[moveY - 1][x];
                    }
                }
                
                // 清空顶行
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    board[0][x] = 0;
                }
                
                linesCleared++;
                y++; // 重新检查这一行
            }
        }
        
        if (linesCleared > 0) {
            lines += linesCleared;
            score += linesCleared * 100 * level;
            level = lines / 10 + 1;
        }
    }
    
    bool moveLeft() {
        if (state != PLAYING) return false;
        if (isValidPosition(currentPiece.x - 1, currentPiece.y, currentPiece.rotation)) {
            currentPiece.x--;
            return true;
        }
        return false;
    }
    
    bool moveRight() {
        if (state != PLAYING) return false;
        if (isValidPosition(currentPiece.x + 1, currentPiece.y, currentPiece.rotation)) {
            currentPiece.x++;
            return true;
        }
        return false;
    }
    
    bool moveDown() {
        if (state != PLAYING) return false;
        if (isValidPosition(currentPiece.x, currentPiece.y + 1, currentPiece.rotation)) {
            currentPiece.y++;
            return true;
        } else {
            lockPiece();
            return false;
        }
    }
    
    bool rotate() {
        if (state != PLAYING) return false;
        int newRotation = (currentPiece.rotation + 1) % 4;
        if (isValidPosition(currentPiece.x, currentPiece.y, newRotation)) {
            currentPiece.rotation = newRotation;
            return true;
        }
        return false;
    }
    
    void drop() {
        if (state != PLAYING) return;
        while (moveDown()) {
            score += 2;
        }
    }
    
    // Getter方法
    int getBoard(int x, int y) const {
        if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) return 0;
        return board[y][x];
    }
    
    const Tetromino& getCurrentPiece() const { return currentPiece; }
    const Tetromino& getNextPiece() const { return nextPiece; }
    GameState getState() const { return state; }
    int getScore() const { return score; }
    int getLevel() const { return level; }
    int getLines() const { return lines; }
    
    void pause() {
        if (state == PLAYING) state = PAUSED;
        else if (state == PAUSED) state = PLAYING;
    }
    
    void restart() {
        // 重置游戏状态
        state = PLAYING;
        score = 0;
        level = 1;
        lines = 0;
        
        // 清空游戏板
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[y][x] = 0;
            }
        }
        
        // 重新生成方块
        spawnNewPiece();
        generateNextPiece();
    }
};

// Direct2D全局变量
ID2D1Factory* pD2DFactory = nullptr;
ID2D1HwndRenderTarget* pRenderTarget = nullptr;
ID2D1SolidColorBrush* pBrushes[8] = {nullptr};
IDWriteFactory* pDWriteFactory = nullptr;
IDWriteTextFormat* pTextFormat = nullptr;
IDWriteTextFormat* pTitleTextFormat = nullptr;

// 游戏全局变量
TetrisGame* game = nullptr;
HWND hWnd;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT InitializeD2D();
void CleanupD2D();
HRESULT CreateDeviceIndependentResources();
HRESULT CreateDeviceResources();
void DiscardDeviceResources();
void DrawGame();
void DrawBlock(float x, float y, int color, float alpha = 1.0f);
void DrawBoard();
void DrawCurrentPiece();
void DrawNextPiece();
void DrawUI();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    const char* CLASS_NAME = "TetrisWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);
    
    // 创建窗口
    hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "俄罗斯方块",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    
    if (hWnd == NULL) {
        return 0;
    }
    
    // 初始化Direct2D
    if (FAILED(InitializeD2D())) {
        return -1;
    }
    
    // 创建游戏实例
    game = new TetrisGame();
    
    // 设置计时器（游戏循环）
    SetTimer(hWnd, 1, 500, NULL); // 500ms间隔
    
    ShowWindow(hWnd, nCmdShow);
    
    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理资源
    delete game;
    CleanupD2D();
    
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            DrawGame();
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_SIZE: {
            if (pRenderTarget) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
                pRenderTarget->Resize(size);
            }
            return 0;
        }
        
        case WM_KEYDOWN:
            if (game) {
                switch (wParam) {
                    case VK_LEFT:
                        game->moveLeft();
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    case VK_RIGHT:
                        game->moveRight();
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    case VK_DOWN:
                        game->moveDown();
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    case VK_UP:
                        game->rotate();
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    case VK_SPACE:
                        game->drop();
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    case 'P':
                    case 'p':
                        game->pause();
                        InvalidateRect(hwnd, NULL, FALSE);
                        break;
                    case 'R':
                    case 'r':
                        if (game->getState() == GAME_OVER) {
                            game->restart();
                            InvalidateRect(hwnd, NULL, FALSE);
                        }
                        break;
                }
            }
            return 0;
            
        case WM_TIMER:
            if (game && game->getState() == PLAYING) {
                game->moveDown();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HRESULT InitializeD2D() {
    HRESULT hr = S_OK;
    
    // 创建设备无关资源
    hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr)) {
        // 创建设备相关资源
        hr = CreateDeviceResources();
    }
    
    return hr;
}

void CleanupD2D() {
    DiscardDeviceResources();
    
    if (pDWriteFactory) {
        pDWriteFactory->Release();
        pDWriteFactory = nullptr;
    }
    
    if (pD2DFactory) {
        pD2DFactory->Release();
        pD2DFactory = nullptr;
    }
}

HRESULT CreateDeviceIndependentResources() {
    HRESULT hr = S_OK;
    
    // 创建Direct2D工厂
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    
    if (SUCCEEDED(hr)) {
        // 创建DirectWrite工厂
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(pDWriteFactory),
            reinterpret_cast<IUnknown**>(&pDWriteFactory)
        );
    }
    
    if (SUCCEEDED(hr)) {
        // 创建文本格式
        hr = pDWriteFactory->CreateTextFormat(
            L"Microsoft YaHei",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            14.0f,
            L"zh-cn",
            &pTextFormat
        );
    }
    
    if (SUCCEEDED(hr)) {
        // 创建标题文本格式
        hr = pDWriteFactory->CreateTextFormat(
            L"Microsoft YaHei",
            nullptr,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            16.0f,
            L"zh-cn",
            &pTitleTextFormat
        );
    }
    
    return hr;
}

HRESULT CreateDeviceResources() {
    HRESULT hr = S_OK;
    
    if (!pRenderTarget) {
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        
        // 创建渲染目标
        hr = pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &pRenderTarget
        );
        
        if (SUCCEEDED(hr)) {
            // 创建颜色画刷
            for (int i = 0; i < 8; i++) {
                hr = pRenderTarget->CreateSolidColorBrush(COLORS[i], &pBrushes[i]);
                if (FAILED(hr)) break;
            }
        }
    }
    
    return hr;
}

void DiscardDeviceResources() {
    for (int i = 0; i < 8; i++) {
        if (pBrushes[i]) {
            pBrushes[i]->Release();
            pBrushes[i] = nullptr;
        }
    }
    
    if (pTitleTextFormat) {
        pTitleTextFormat->Release();
        pTitleTextFormat = nullptr;
    }
    
    if (pTextFormat) {
        pTextFormat->Release();
        pTextFormat = nullptr;
    }
    
    if (pRenderTarget) {
        pRenderTarget->Release();
        pRenderTarget = nullptr;
    }
}

void DrawGame() {
    HRESULT hr = CreateDeviceResources();
    
    if (SUCCEEDED(hr)) {
        pRenderTarget->BeginDraw();
        
        // 清除背景为深灰色
        pRenderTarget->Clear(D2D1::ColorF(0.1f, 0.1f, 0.15f));
        
        if (game) {
            // 绘制游戏板
            DrawBoard();
            
            // 绘制当前方块
            if (game->getState() == PLAYING || game->getState() == PAUSED) {
                DrawCurrentPiece();
            }
            
            // 绘制下一个方块
            DrawNextPiece();
            
            // 绘制UI信息
            DrawUI();
        }
        
        hr = pRenderTarget->EndDraw();
        
        if (hr == D2DERR_RECREATE_TARGET) {
            DiscardDeviceResources();
        }
    }
}

void DrawBlock(float x, float y, int color, float alpha) {
    if (color < 0 || color >= 8 || !pBrushes[color]) return;
    
    D2D1_RECT_F rect = D2D1::RectF(
        x * BLOCK_SIZE + 50,
        y * BLOCK_SIZE + 50,
        (x + 1) * BLOCK_SIZE + 50,
        (y + 1) * BLOCK_SIZE + 50
    );
    
    // 设置画刷透明度
    pBrushes[color]->SetOpacity(alpha);
    
    // 填充方块
    pRenderTarget->FillRectangle(rect, pBrushes[color]);
    
    // 绘制边框（如果不是空白方块）
    if (color != 0) {
        // 创建高光效果
        D2D1_RECT_F highlightRect = D2D1::RectF(
            rect.left + 2, rect.top + 2,
            rect.right - 2, rect.bottom - 2
        );
        
        // 使用稍微亮一点的颜色作为高光
        D2D1::ColorF highlightColor = COLORS[color];
        highlightColor.r = min(1.0f, highlightColor.r + 0.2f);
        highlightColor.g = min(1.0f, highlightColor.g + 0.2f);
        highlightColor.b = min(1.0f, highlightColor.b + 0.2f);
        highlightColor.a = alpha * 0.5f;
        
        ID2D1SolidColorBrush* pHighlightBrush = nullptr;
        pRenderTarget->CreateSolidColorBrush(highlightColor, &pHighlightBrush);
        if (pHighlightBrush) {
            pRenderTarget->DrawRectangle(highlightRect, pHighlightBrush, 1.0f);
            pHighlightBrush->Release();
        }
        
        // 绘制外边框
        pBrushes[0]->SetOpacity(0.8f * alpha);
        pRenderTarget->DrawRectangle(rect, pBrushes[0], 1.0f);
    }
    
    // 重置透明度
    pBrushes[color]->SetOpacity(1.0f);
}

void DrawBoard() {
    // 绘制游戏板背景
    D2D1_RECT_F boardRect = D2D1::RectF(
        48, 48,
        50 + BOARD_WIDTH * BLOCK_SIZE + 2,
        50 + BOARD_HEIGHT * BLOCK_SIZE + 2
    );
    
    // 创建背景渐变
    ID2D1LinearGradientBrush* pBackgroundBrush = nullptr;
    ID2D1GradientStopCollection* pGradientStops = nullptr;
    
    D2D1_GRADIENT_STOP gradientStops[2];
    gradientStops[0].color = D2D1::ColorF(0.05f, 0.05f, 0.1f, 1.0f);
    gradientStops[0].position = 0.0f;
    gradientStops[1].color = D2D1::ColorF(0.15f, 0.15f, 0.2f, 1.0f);
    gradientStops[1].position = 1.0f;
    
    pRenderTarget->CreateGradientStopCollection(
        gradientStops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);
    
    if (pGradientStops) {
        pRenderTarget->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(boardRect.left, boardRect.top),
                D2D1::Point2F(boardRect.right, boardRect.bottom)
            ),
            pGradientStops,
            &pBackgroundBrush
        );
        
        if (pBackgroundBrush) {
            pRenderTarget->FillRectangle(boardRect, pBackgroundBrush);
            pBackgroundBrush->Release();
        }
        pGradientStops->Release();
    }
    
    // 绘制游戏板边框
    pBrushes[7]->SetOpacity(0.8f); // 使用橙色作为边框
    pRenderTarget->DrawRectangle(boardRect, pBrushes[7], 2.0f);
    pBrushes[7]->SetOpacity(1.0f);
    
    // 绘制网格线
    pBrushes[0]->SetOpacity(0.1f);
    for (int x = 1; x < BOARD_WIDTH; x++) {
        float lineX = 50 + x * BLOCK_SIZE;
        pRenderTarget->DrawLine(
            D2D1::Point2F(lineX, 50),
            D2D1::Point2F(lineX, 50 + BOARD_HEIGHT * BLOCK_SIZE),
            pBrushes[0], 0.5f
        );
    }
    for (int y = 1; y < BOARD_HEIGHT; y++) {
        float lineY = 50 + y * BLOCK_SIZE;
        pRenderTarget->DrawLine(
            D2D1::Point2F(50, lineY),
            D2D1::Point2F(50 + BOARD_WIDTH * BLOCK_SIZE, lineY),
            pBrushes[0], 0.5f
        );
    }
    pBrushes[0]->SetOpacity(1.0f);
    
    // 绘制游戏板内容
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int color = game->getBoard(x, y);
            if (color != 0) {
                DrawBlock(static_cast<float>(x), static_cast<float>(y), color);
            }
        }
    }
}

void DrawCurrentPiece() {
    const Tetromino& piece = game->getCurrentPiece();
    
    // 绘制当前方块
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
            if (TETROMINO_SHAPES[piece.type][piece.rotation][dy][dx]) {
                int x = piece.x + dx;
                int y = piece.y + dy;
                if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT) {
                    DrawBlock(static_cast<float>(x), static_cast<float>(y), piece.type + 1);
                }
            }
        }
    }
    
    // 绘制阴影（预览方块落地位置）
    Tetromino shadowPiece = piece;
    while (game->isValidPosition(shadowPiece.x, shadowPiece.y + 1, shadowPiece.rotation)) {
        shadowPiece.y++;
    }
    
    // 只有当阴影位置与当前位置不同时才绘制阴影
    if (shadowPiece.y != piece.y) {
        for (int dy = 0; dy < 4; dy++) {
            for (int dx = 0; dx < 4; dx++) {
                if (TETROMINO_SHAPES[shadowPiece.type][shadowPiece.rotation][dy][dx]) {
                    int x = shadowPiece.x + dx;
                    int y = shadowPiece.y + dy;
                    if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT) {
                        // 绘制半透明的阴影方块
                        DrawBlock(static_cast<float>(x), static_cast<float>(y), shadowPiece.type + 1, 0.3f);
                    }
                }
            }
        }
    }
}

void DrawNextPiece() {
    // 绘制"下一个"标签背景
    D2D1_RECT_F labelRect = D2D1::RectF(370, 50, 480, 180);
    
    // 创建背景渐变
    ID2D1LinearGradientBrush* pBackgroundBrush = nullptr;
    ID2D1GradientStopCollection* pGradientStops = nullptr;
    
    D2D1_GRADIENT_STOP gradientStops[2];
    gradientStops[0].color = D2D1::ColorF(0.2f, 0.2f, 0.25f, 0.8f);
    gradientStops[0].position = 0.0f;
    gradientStops[1].color = D2D1::ColorF(0.1f, 0.1f, 0.15f, 0.8f);
    gradientStops[1].position = 1.0f;
    
    pRenderTarget->CreateGradientStopCollection(
        gradientStops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);
    
    if (pGradientStops) {
        pRenderTarget->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(labelRect.left, labelRect.top),
                D2D1::Point2F(labelRect.right, labelRect.bottom)
            ),
            pGradientStops,
            &pBackgroundBrush
        );
        
        if (pBackgroundBrush) {
            // 绘制圆角矩形背景
            D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(labelRect, 5.0f, 5.0f);
            pRenderTarget->FillRoundedRectangle(roundedRect, pBackgroundBrush);
            pBackgroundBrush->Release();
        }
        pGradientStops->Release();
    }
    
    // 绘制边框
    pBrushes[7]->SetOpacity(0.6f);
    D2D1_ROUNDED_RECT borderRect = D2D1::RoundedRect(labelRect, 5.0f, 5.0f);
    pRenderTarget->DrawRoundedRectangle(borderRect, pBrushes[7], 1.5f);
    pBrushes[7]->SetOpacity(1.0f);
    
    // 绘制"下一个"标签
    if (pTitleTextFormat) {
        D2D1_RECT_F textRect = D2D1::RectF(375, 55, 475, 80);
        
        // 创建白色画刷用于文本
        ID2D1SolidColorBrush* pWhiteBrush = nullptr;
        pRenderTarget->CreateSolidColorBrush(D2D1::ColorF::White, &pWhiteBrush);
        if (pWhiteBrush) {
            pRenderTarget->DrawText(
                L"下一个",
                3,
                pTitleTextFormat,
                textRect,
                pWhiteBrush
            );
            pWhiteBrush->Release();
        }
    }
    
    // 绘制下一个方块的预览
    const Tetromino& nextPiece = game->getNextPiece();
    
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
            if (TETROMINO_SHAPES[nextPiece.type][0][dy][dx]) {
                D2D1_RECT_F rect = D2D1::RectF(
                    380 + dx * 20,
                    90 + dy * 20,
                    380 + (dx + 1) * 20,
                    90 + (dy + 1) * 20
                );
                
                // 填充方块
                pRenderTarget->FillRectangle(rect, pBrushes[nextPiece.type + 1]);
                
                // 绘制高光
                D2D1_RECT_F highlightRect = D2D1::RectF(
                    rect.left + 2, rect.top + 2,
                    rect.right - 2, rect.bottom - 2
                );
                
                D2D1::ColorF highlightColor = COLORS[nextPiece.type + 1];
                highlightColor.r = min(1.0f, highlightColor.r + 0.3f);
                highlightColor.g = min(1.0f, highlightColor.g + 0.3f);
                highlightColor.b = min(1.0f, highlightColor.b + 0.3f);
                highlightColor.a = 0.5f;
                
                ID2D1SolidColorBrush* pHighlightBrush = nullptr;
                pRenderTarget->CreateSolidColorBrush(highlightColor, &pHighlightBrush);
                if (pHighlightBrush) {
                    pRenderTarget->DrawRectangle(highlightRect, pHighlightBrush, 1.0f);
                    pHighlightBrush->Release();
                }
                
                // 绘制边框
                pBrushes[0]->SetOpacity(0.8f);
                pRenderTarget->DrawRectangle(rect, pBrushes[0], 1.0f);
                pBrushes[0]->SetOpacity(1.0f);
            }
        }
    }
}

void DrawUI() {
    if (!pTextFormat) return;
    
    // 创建白色画刷用于文本
    ID2D1SolidColorBrush* pWhiteBrush = nullptr;
    ID2D1SolidColorBrush* pYellowBrush = nullptr;
    ID2D1SolidColorBrush* pRedBrush = nullptr;
    
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF::White, &pWhiteBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF::Yellow, &pYellowBrush);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF::Red, &pRedBrush);
    
    if (pWhiteBrush) {
        wchar_t buffer[256];
        
        // 绘制信息面板背景
        D2D1_RECT_F infoRect = D2D1::RectF(370, 200, 480, 300);
        
        // 创建信息面板背景渐变
        ID2D1LinearGradientBrush* pInfoBackgroundBrush = nullptr;
        ID2D1GradientStopCollection* pInfoGradientStops = nullptr;
        
        D2D1_GRADIENT_STOP infoGradientStops[2];
        infoGradientStops[0].color = D2D1::ColorF(0.15f, 0.15f, 0.2f, 0.9f);
        infoGradientStops[0].position = 0.0f;
        infoGradientStops[1].color = D2D1::ColorF(0.05f, 0.05f, 0.1f, 0.9f);
        infoGradientStops[1].position = 1.0f;
        
        pRenderTarget->CreateGradientStopCollection(
            infoGradientStops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pInfoGradientStops);
        
        if (pInfoGradientStops) {
            pRenderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(infoRect.left, infoRect.top),
                    D2D1::Point2F(infoRect.right, infoRect.bottom)
                ),
                pInfoGradientStops,
                &pInfoBackgroundBrush
            );
            
            if (pInfoBackgroundBrush) {
                D2D1_ROUNDED_RECT infoRoundedRect = D2D1::RoundedRect(infoRect, 5.0f, 5.0f);
                pRenderTarget->FillRoundedRectangle(infoRoundedRect, pInfoBackgroundBrush);
                pInfoBackgroundBrush->Release();
            }
            pInfoGradientStops->Release();
        }
        
        // 绘制边框
        pBrushes[6]->SetOpacity(0.6f); // 青色边框
        D2D1_ROUNDED_RECT infoBorderRect = D2D1::RoundedRect(infoRect, 5.0f, 5.0f);
        pRenderTarget->DrawRoundedRectangle(infoBorderRect, pBrushes[6], 1.5f);
        pBrushes[6]->SetOpacity(1.0f);
        
        // 绘制分数
        swprintf(buffer, 256, L"分数: %d", game->getScore());
        D2D1_RECT_F scoreRect = D2D1::RectF(375, 210, 475, 230);
        pRenderTarget->DrawText(buffer, wcslen(buffer), pTextFormat, scoreRect, pYellowBrush);
        
        // 绘制等级
        swprintf(buffer, 256, L"等级: %d", game->getLevel());
        D2D1_RECT_F levelRect = D2D1::RectF(375, 235, 475, 255);
        pRenderTarget->DrawText(buffer, wcslen(buffer), pTextFormat, levelRect, pWhiteBrush);
        
        // 绘制行数
        swprintf(buffer, 256, L"行数: %d", game->getLines());
        D2D1_RECT_F linesRect = D2D1::RectF(375, 260, 475, 280);
        pRenderTarget->DrawText(buffer, wcslen(buffer), pTextFormat, linesRect, pWhiteBrush);
        
        // 绘制控制说明背景
        D2D1_RECT_F controlRect = D2D1::RectF(370, 320, 480, 450);
        
        ID2D1LinearGradientBrush* pControlBackgroundBrush = nullptr;
        ID2D1GradientStopCollection* pControlGradientStops = nullptr;
        
        D2D1_GRADIENT_STOP controlGradientStops[2];
        controlGradientStops[0].color = D2D1::ColorF(0.1f, 0.1f, 0.15f, 0.8f);
        controlGradientStops[0].position = 0.0f;
        controlGradientStops[1].color = D2D1::ColorF(0.05f, 0.05f, 0.1f, 0.8f);
        controlGradientStops[1].position = 1.0f;
        
        pRenderTarget->CreateGradientStopCollection(
            controlGradientStops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pControlGradientStops);
        
        if (pControlGradientStops) {
            pRenderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(controlRect.left, controlRect.top),
                    D2D1::Point2F(controlRect.right, controlRect.bottom)
                ),
                pControlGradientStops,
                &pControlBackgroundBrush
            );
            
            if (pControlBackgroundBrush) {
                D2D1_ROUNDED_RECT controlRoundedRect = D2D1::RoundedRect(controlRect, 5.0f, 5.0f);
                pRenderTarget->FillRoundedRectangle(controlRoundedRect, pControlBackgroundBrush);
                pControlBackgroundBrush->Release();
            }
            pControlGradientStops->Release();
        }
        
        // 绘制控制说明
        pRenderTarget->DrawText(L"控制:", 3, pTitleTextFormat, 
            D2D1::RectF(375, 325, 475, 345), pYellowBrush);
        pRenderTarget->DrawText(L"←→ 移动", 6, pTextFormat, 
            D2D1::RectF(375, 350, 475, 370), pWhiteBrush);
        pRenderTarget->DrawText(L"↓ 加速下降", 7, pTextFormat, 
            D2D1::RectF(375, 370, 475, 390), pWhiteBrush);
        pRenderTarget->DrawText(L"↑ 旋转", 5, pTextFormat, 
            D2D1::RectF(375, 390, 475, 410), pWhiteBrush);
        pRenderTarget->DrawText(L"空格 快速下降", 8, pTextFormat, 
            D2D1::RectF(375, 410, 475, 430), pWhiteBrush);
        pRenderTarget->DrawText(L"P 暂停", 5, pTextFormat, 
            D2D1::RectF(375, 430, 475, 450), pWhiteBrush);
        
        // 游戏状态提示
        if (game->getState() == GAME_OVER) {
            // 绘制游戏结束背景
            D2D1_RECT_F gameOverRect = D2D1::RectF(100, 250, 350, 350);
            
            ID2D1SolidColorBrush* pGameOverBrush = nullptr;
            pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.0f, 0.0f, 0.9f), &pGameOverBrush);
            if (pGameOverBrush) {
                D2D1_ROUNDED_RECT gameOverRoundedRect = D2D1::RoundedRect(gameOverRect, 10.0f, 10.0f);
                pRenderTarget->FillRoundedRectangle(gameOverRoundedRect, pGameOverBrush);
                pGameOverBrush->Release();
            }
            
            // 绘制游戏结束文本
            if (pTitleTextFormat) {
                pRenderTarget->DrawText(L"游戏结束!", 5, pTitleTextFormat, 
                    D2D1::RectF(110, 270, 340, 300), pWhiteBrush);
                pRenderTarget->DrawText(L"按R重新开始", 7, pTextFormat, 
                    D2D1::RectF(110, 310, 340, 330), pWhiteBrush);
            }
        } else if (game->getState() == PAUSED) {
            // 绘制暂停提示
            pRenderTarget->DrawText(L"游戏暂停", 4, pTitleTextFormat, 
                D2D1::RectF(375, 470, 475, 490), pYellowBrush);
        }
    }
    
    // 清理画刷
    if (pWhiteBrush) pWhiteBrush->Release();
    if (pYellowBrush) pYellowBrush->Release();
    if (pRedBrush) pRedBrush->Release();
}