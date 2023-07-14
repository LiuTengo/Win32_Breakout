// PracticeF.cpp : 定义应用程序的入口点。
//

#include<Windows.h>
#include<gdiplus.h> //使用GDI+
#include<math.h>
#include<ctime>

#include "framework.h"
#include "PracticeF.h"

using namespace Gdiplus;

#define MAX_LOADSTRING 100

// 链接 GDI+ 库
#pragma comment (lib,"Gdiplus.lib")
#pragma comment(lib,"Msimg32.lib")
#pragma comment(lib,"Winmm.lib")

//方向
enum Direction 
{
    LEFT,RIGHT,NONE
};

//State of Game
enum GameState
{
    Start,Play,Serve,GameOver,Win
};

//物体信息
struct GameObject 
{
    HBITMAP ObjImg;//物体图像
    float x;//物体位置--x
    float y;//物体位置--y
    float vx,vy;//物体速度
    int w, h;//物体的宽、高
    Direction dir; //玩家方向
};

typedef struct E 
{
    int effectNum;
    HBITMAP Img;
    int x, y;
    int vy;
    int w, h;
    struct E* next;
}Effect;

//砖块节点数据结构体
typedef struct B
{
    int TextureNum;
    HBITMAP ObjImg;//物体图像
    float x, y;
    int w, h;
    int containEffect = 0;//包含特殊效果，默认为0
    int score;
    struct B* next;
} Block;

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
ULONG_PTR m_gdiplusToken;

HBITMAP background, blocks[11], paddles[4],balls[4],effectImg[4],hearts[2];

Direction dir = NONE;
int SCREEN_WIDTH, SCREEN_HEIGHT;//屏幕宽、高
int blockColor[4], blockTier[3];//随机生成地图的参数。color--砖块颜色，tier--砖块的等级
int timeStep = 20;//每执行一次Update函数的时间间隔
int score = 0;//分数
int highScore;//最高分数
int level = 1;//关卡数
int dsX = 10;//砖块列间距
int dsY = 20;//砖块行间距
int playerHp;
int playerIndex;

GameObject player,ball,effects[4];//玩家
GameState gameState = Play;//游戏状态
Block* brickList;
Effect* effectList;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


int Min(int a, int b)
{
    int result = (a <= b) ? a : b;
    return result;
}

int GetImgWidth(HBITMAP bitmap,int LargeScale = 1)
{
    BITMAP bm;
    GetObject(bitmap, sizeof(bm), &bm);

    return LargeScale*bm.bmWidth;
}
int GetImgHeight(HBITMAP bitmap,int LargeScale =1)
{
    BITMAP bm;
    GetObject(bitmap, sizeof(bm), &bm);

    return LargeScale * bm.bmHeight;
}

//判断小球是否与平板碰撞
int IsCollide(GameObject currentObject, GameObject otherObject)
{
    if (currentObject.x > otherObject.x + otherObject.w|| currentObject.x +currentObject.w < otherObject.x)
        return 0;
    if (currentObject.y > otherObject.y + otherObject.h|| currentObject.y + currentObject.h< otherObject.y)
        return 0;

    return 1;
}

int IsCollide(GameObject currentObject,int otherX,int otherY,int otherW,int otherH)
{
    if (currentObject.x > otherX + otherW || currentObject.x + currentObject.w < otherX)
        return 0;
    if (currentObject.y > otherY + otherH || currentObject.y + currentObject.h < otherY)
        return 0;

    return 1;
}

void AddNewBrick(Block* head, Block* newPlayer)
{
    newPlayer->next = head->next;
    head->next = newPlayer;
}
void AddNewEffect(Effect* head, Effect* newPlayer)
{
    newPlayer->next = head->next;
    head->next = newPlayer;
}
void InsertBlocks(int textureNum,int c,int r,int i,int j,int hasEffect,int score)
{
    //节点初始化
    Block* newNode = new Block;
    newNode->TextureNum = textureNum;
    newNode->ObjImg = blocks[textureNum];
    newNode->w = GetImgWidth(blocks[textureNum], 2);
    newNode->h = GetImgHeight(blocks[textureNum], 2);
    newNode->x = (SCREEN_WIDTH - (c * (newNode->w + dsX) - dsX)) / 2 + j * (newNode->w + dsX);
    newNode->y = (2 * dsY + i * (newNode->h + dsY));
    newNode->containEffect = hasEffect;
    newNode->score = score;
    newNode->next = nullptr;

    if (brickList == nullptr)
    {
        brickList = newNode;
    }
    else
    {
        AddNewBrick(brickList, newNode);
    }
}

void InsertEffect(int x,int y) 
{
    Effect* newEffect = new Effect;
    newEffect->effectNum = rand() % 4;
    newEffect->Img = effectImg[newEffect->effectNum];
    newEffect->x = x;
    newEffect->y = y;
    newEffect->vy = 1;
    newEffect->w = GetImgWidth(newEffect->Img,2);
    newEffect->h = GetImgHeight(newEffect->Img, 2);
    newEffect->next = nullptr;

    if (effectList == nullptr)
    {
        effectList = newEffect;
    }
    else
    {
        AddNewEffect(effectList, newEffect);
    }
}

void RemoveBlock(Block* head, Block* player)
{
    Block* p = head->next;
    if (p == player)	// 第一个节点是待删除节点
    {
        head->next = p->next;
        free(p);
        return;
    }
    while (p->next != player && p->next != NULL)
        p = p->next;
    if (p->next == player)	// 删除节点，防止断链
    {
        Block* tmp = p->next->next;
        free(p->next);
        p->next = tmp;
    }
}

void RemoveEffect(Effect* head, Effect* player)
{
    Effect* p = head;
    if (p == player)	// 第一个节点是待删除节点
    {
        head->next = p->next;
        free(p);
        return;
    }
    while ( p->next != player && p->next != NULL)
        p = p->next;
    if (p->next == player)	// 删除节点，防止断链
    {
        Effect* tmp = p->next->next;
        free(p->next);
        p->next = tmp;
    }
}

void CreatMap(int level)//参数： 关卡数
{
    int MapType = rand() % 2;//0--General,1--Triangle

    int r = rand() % 3 + 2;
    int c = rand() % 4 + 6+level;
    c = (c % 2 == 0) ? (c+1): c;//保证c为奇数，

    int highTier = Min(2,floor(level/5));
    int highColor = Min(3,level%5);

    if (MapType == 0) 
    {
        for (int i = 0; i < r; i++) 
    {
        int skipPattern = (rand() % 2 == 0) ? (1) : (0); //是否跳过该行
        int skipFlag = (rand() % 2 == 0) ? (1) : (0); //是否跳过该方块

        int alternatePattern = (rand() % 2 == 0) ? (1) : (0);//是否在该行转换颜色
        int alternateFlag = (rand() % 2 == 0) ? (1) : (0);

        int alternateColor1 = rand() % highColor;
        int alternateColor2 = rand() % highColor;
        int alternateTier1 = rand() % (highTier+1);
        int alternateTier2 = rand() % (highTier+1);

        for (int j = 0; j < c; j++) 
        {
            int color = 0;
            int tier =0;
            if (skipFlag && skipPattern) 
            {
                skipFlag = 0;
                continue;
            }
            else 
            {
                skipFlag = 1;
            }
            int hasEffect = (rand() % 3 == 0)?(1):(0);

            if (alternatePattern && alternateFlag) 
            {
                color = alternateColor1;
                tier = alternateTier2;
                alternateFlag = 0;
                InsertBlocks(color*3+tier*2, c, r, i, j, hasEffect, color * 3*10 + tier * 2*5);
                continue;
            }
            else 
            {
                color = alternateColor2;
                tier = alternateTier1;
                alternateFlag = 1;
                InsertBlocks(color * 3 + tier * 2, c, r, i, j, hasEffect, color * 3 * 10 + tier * 2 * 5);
                continue;
            }
        }
    }

    }
    else if(MapType == 1)
    {
        c = rand() % 3 + 9 + level;
        c = (c % 2 == 0) ? (c + 1) : c;//保证c为奇数，

        for (int i = 0; i < r; i++) 
        {
            int hasEffect = (rand() % 3 == 0) ? (1) : (0);

            int alternatePattern = (rand() % 2 == 0) ? (1) : (0);//是否在该行转换颜色
            int alternateFlag = (rand() % 2 == 0) ? (1) : (0);

            int alternateColor1 = rand() % highColor;
            int alternateColor2 = rand() % highColor;
            int alternateTier1 = rand() % (highTier + 1);
            int alternateTier2 = rand() % (highTier + 1);

            for (int j = 0; j<c; j++)
            {
                int color = 0;
                int tier = 0;
                if (j < i*1||j>c-i-1) 
                {
                    continue;
                }
                else 
                {

                    if (alternatePattern && alternateFlag)
                    {
                        color = alternateColor1;
                        tier = alternateTier2;
                        alternateFlag = 0;
                        InsertBlocks(color * 3 + tier * 2, c, r, i, j, hasEffect, color * 3 * 10 + tier * 2 * 5);
                        continue;
                    }
                    else
                    {
                        color = alternateColor2;
                        tier = alternateTier1;
                        alternateFlag = 1;
                        InsertBlocks(color * 3 + tier * 2, c, r, i, j, hasEffect, color * 3 * 10 + tier * 2 * 5);
                        continue;
                    }

                }
                
            }


        }
    }
}

//加载PNG图片,并返回Bitmap句柄
HBITMAP LoadPNGImage(const WCHAR* filename)
{
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    Bitmap bitmap(filename);
    HBITMAP hBitmap = nullptr;

    bitmap.GetHBITMAP(Color::White,&hBitmap);

    GdiplusShutdown(gdiplusToken);

    return hBitmap;
}


//创建HBitMap
void GetHBITMAP()
{
    background = LoadPNGImage(L"Resources\\graphics\\background.png"); //载入背景图片

    paddles[0] = LoadPNGImage(L"Resources\\graphics\\Paddles\\Paddle1.png");//载入玩家平板图片
    paddles[1] = LoadPNGImage(L"Resources\\graphics\\Paddles\\Paddle2.png");
    paddles[2] = LoadPNGImage(L"Resources\\graphics\\Paddles\\Paddle3.png");
    paddles[3] = LoadPNGImage(L"Resources\\graphics\\Paddles\\Paddle4.png");

    balls[0] = LoadPNGImage(L"Resources\\graphics\\Balls\\Ball1.png");//载入小球图片
    balls[1] = LoadPNGImage(L"Resources\\graphics\\Balls\\Ball2.png");
    balls[2] = LoadPNGImage(L"Resources\\graphics\\Balls\\Ball3.png");
    balls[3] = LoadPNGImage(L"Resources\\graphics\\Balls\\Ball4.png");

    blocks[0] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block1.png"); //载入方块图片
    blocks[1] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block2.png"); 
    blocks[2] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block3.png");
    blocks[3] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block4.png");
    blocks[4] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block5.png");
    blocks[5] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block6.png");
    blocks[6] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block7.png");
    blocks[7] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block8.png");
    blocks[8] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block9.png");
    blocks[9] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block10.png");
    blocks[10] = LoadPNGImage(L"Resources\\graphics\\Blocks\\block11.png");

    effectImg[0] = LoadPNGImage(L"Resources\\graphics\\Effectcs\\effect1.png");
    effectImg[1] = LoadPNGImage(L"Resources\\graphics\\Effectcs\\effect2.png");
    effectImg[2] = LoadPNGImage(L"Resources\\graphics\\Effectcs\\effect3.png");
    effectImg[3] = LoadPNGImage(L"Resources\\graphics\\Effectcs\\effect4.png");

    hearts[0] = LoadPNGImage(L"Resources\\graphics\\Hp\\heart1.png");
    hearts[1] = LoadPNGImage(L"Resources\\graphics\\Hp\\heart2.png");
}

//初始化玩家
void InitPlayer() 
{
    player.ObjImg = paddles[playerIndex];
    player.w = GetImgWidth(player.ObjImg,2);
    player.h = GetImgHeight(player.ObjImg,2);
    player.x = (SCREEN_WIDTH-player.w)/2;
    player.y = SCREEN_HEIGHT-100;
    player.vx = 0;
    player.vy = 0;
    player.dir = NONE;
}

void InitBall() 
{
    ball.ObjImg = balls[0];
    ball.w = GetImgWidth(ball.ObjImg,2);
    ball.h = GetImgHeight(ball.ObjImg,2);
    ball.vx = 8;
    ball.vy = 0;
    ball.x = 0;
    ball.y = 0;
    ball.dir = NONE;
}

void Reset() 
{
    InitPlayer();
    InitBall();
    brickList = (Block*)malloc(sizeof(Block));
    if (brickList != nullptr)
    {
        brickList->next = NULL;
        brickList->ObjImg = blocks[0];
        brickList->w = GetImgWidth(brickList->ObjImg, 2);
        brickList->h = GetImgHeight(brickList->ObjImg, 2);
        brickList->x = -70;
        brickList->y = -70;
    }
    CreatMap(level);

    effectList = (Effect*)malloc(sizeof(Effect));
    if (effectList != nullptr)
    {
        effectList->y = -20;
        effectList->vy = 0;
        effectList->next = NULL;
    }
}

void ResetInfo() 
{
    score = 0;
    playerHp = 3;
    playerIndex = 1;
    level = 1;
}

// 初始化
void Initialize() 
{   
    score = 0;
    highScore = 0;
    level = 1;
    playerHp = 3;
    playerIndex = 1;
    gameState = Start;
    GetHBITMAP();
    InitPlayer();
    InitBall();

    brickList = (Block*)malloc(sizeof(Block));
    if (brickList != nullptr)
    {
        brickList->next = NULL;
        brickList->ObjImg = blocks[0];
        brickList->w = GetImgWidth(brickList->ObjImg,2);
        brickList->h = GetImgHeight(brickList->ObjImg, 2);
        brickList->x = -70;
        brickList ->y = -70;
    }
    CreatMap(level);

    effectList = (Effect*)malloc(sizeof(Effect));
    if (effectList != nullptr)
    {
        effectList->y = -20;
        effectList->vy = 0;
        effectList->next = NULL;
    }
}

//发球
void ServeBall() 
{
    ball.vx = ((rand()%2==1)?( - 1 ): 1) * (rand() % 3 + 2);
    ball.vy = -1*(rand() % 4+3); 
}

//绘图方法
void DrawGameObject(HDC hdc,GameObject obj,float x,float y)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP bmp = obj.ObjImg;
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, bmp);	// 选定绘制的位图
    BITMAP bm;
    GetObject(bmp, sizeof(bm), &bm);

    SetStretchBltMode(hdc, STRETCH_HALFTONE);				// 设定位图缩放模式为STRETCH_HALFTONE,提高缩放质量
    TransparentBlt(hdc,x, y, obj.w , obj.h, hdcMem,0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}

void DrawGameObject(HDC hdc, HBITMAP img,int w,int h, float x, float y)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP bmp = img;
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, bmp);	// 选定绘制的位图
    BITMAP bm;
    GetObject(bmp, sizeof(bm), &bm);

    SetStretchBltMode(hdc, STRETCH_HALFTONE);				// 设定位图缩放模式为STRETCH_HALFTONE,提高缩放质量
    TransparentBlt(hdc, x, y, w, h, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, RGB(255, 255, 255));
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}


void DrawMap(HDC hdc)
{
    Block* p = brickList;
    //遍历链表，绘制方块
    while (p != NULL) 
    {
       DrawGameObject(hdc,p->ObjImg,p->w,p->h,p->x,p->y);
       p = p->next;
    }
}

void DrawEffect(HDC hdc) 
{
    Effect* p = effectList;
    while (p != NULL)
    {
        DrawGameObject(hdc, p->Img, p->w, p->h, p->x, p->y);
        p = p->next;
    }
}


//链表方法
void EffectCollide()
{
    Effect* p = effectList;
    //遍历链表，检测是否有碰撞
    while (p != NULL)
    {
        if (IsCollide(player, p->x, p->y, p->w, p->h))
        {
            switch (p->effectNum)
            {
            case 0:
                if (playerIndex < 3)
                    playerIndex++;
                player.ObjImg = paddles[playerIndex];
                player.w = GetImgWidth(player.ObjImg, 2);
                player.h = GetImgHeight(player.ObjImg, 2);
                break;
            case 1:
                if (playerIndex > 0)
                    playerIndex--;
                player.ObjImg = paddles[playerIndex];
                player.w = GetImgWidth(player.ObjImg, 2);
                player.h = GetImgHeight(player.ObjImg, 2);
                break;
            case 2:
                if (playerHp < 3)
                    playerHp++;
                break;
            case 3:
                playerHp--;
                PlaySound(TEXT("Resources\\sounds\\hurt.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放失败音效
                if (playerHp == 0) 
                {
                    gameState = GameOver;
                    ResetInfo();
                    Reset();
                }
                break;
            default:
                break;
            }
            RemoveEffect(effectList, p);
            break;
        }
        p = p->next;
    }
}

void BrickCollide()
{
    Block* p = brickList;
    //遍历链表，检测是否有碰撞
    while (p != NULL)
    {
        if (IsCollide(ball, p->x, p->y, p->w, p->h))
        {
            if (p->TextureNum % 3 == 0 && p->TextureNum!=0)
            {
                score += 10;
            }
            else
            {
                score += 5;
            }

            //碰撞
            if (ball.x + 4 < p->x && ball.vx > 0)//collide from left  
            {
                ball.vx = -ball.vx;
                ball.x = p->x - ball.w;
            }
            else if (ball.x + 8 > p->x + p->w && ball.vx < 0) //collide from right
            {
                ball.vx = -ball.vx;
                ball.x = p->x + p->w;
            }
            else if (ball.y + 8 < p->y) //collide from top
            {
                ball.vy = -ball.vy;
                ball.y = p->y - ball.h;
            }
            else//collide from bottom
            {
                ball.vy = -ball.vy;
                ball.y = p->y + p->h;
            }
            

            if (p->TextureNum == 0)
            {
                PlaySound(TEXT("Resources\\sounds\\brick-hit-1.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放撞击1音效

                if (p->containEffect)
                {
                    InsertEffect(p->x + p->w / 2 - 16, p->y + p->h / 2 - 16);
                }
                RemoveBlock(brickList, p);

                if (brickList->next == nullptr) 
                {
                    gameState = Win;
                    PlaySound(TEXT("Resources\\sounds\\victory.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放按键音效
                    level++;
                    if (highScore <= score)
                        highScore = score;
                    Reset();
                }

            }
            else
            {
                p->TextureNum--;
                p->ObjImg = blocks[p->TextureNum];
                PlaySound(TEXT("Resources\\sounds\\brick-hit-2.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放撞击1音效
            }

            break;
        }
        p = p->next;
    }
}

void DestroyMap(Block* head)
{
    Block* p = head->next;
    Block* q = p->next;
    while (q != NULL)
    {
        q = p->next;
        free(p);
        p = q;
    }
}
void DestroyEffect(Effect* head)
{
    Effect* p = head->next;
    Effect* q = p->next;
    while (q != NULL)
    {
        q = p->next;
        free(p);
        p = q;
    }
}


void UpdateHealth(HDC hdc,int playerHp) 
{
    int w = GetImgWidth(hearts[0], 2);
    int h = GetImgHeight(hearts[0], 2);
    for (int i = 0; i < 3; i++) 
    {
        int index = ((playerHp -1) >= i) ? (0) : (1);
        DrawGameObject(hdc,hearts[index],w,h, 8+i*(4+w), 45);
    }
}

void UpdateEffectPosition(int dt) 
{
    Effect* p = effectList;
    while (p != NULL) 
    {     
        p->y += p->vy * dt;  
       /* if (p->y > SCREEN_HEIGHT)
            RemoveEffect(effectList,p);*/
        p = p->next;
    }
}

//绘制游戏信息
void DrawScene(HDC hdc) 
{ 
    // 绘制背景位图
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, background);
    BITMAP bm;
    GetObject(background, sizeof(bm), &bm);
    SetStretchBltMode(hdc, STRETCH_HALFTONE);
    StretchBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcMem, 0, 0, bm.bmWidth,bm.bmHeight,SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);

    //创建字体
    HFONT font;
    long lfHeight;
    lfHeight = -MulDiv(16, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    font = CreateFont(lfHeight * 2, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"04b03");
    HFONT hfOld = (HFONT)SelectObject(hdc, font);

    //FIXME:改为透明背景
    SetBkMode(hdc,TRANSPARENT);
    SetTextColor(hdc,RGB(255,255,255));

    //存储文本信息的char数组
    WCHAR Title[64];
    WCHAR Score[16];
    WCHAR InfoTxt[64];
    

    switch(gameState) 
    {
    //绘制开始界面
    case Start:
        wsprintf(Title,L"Breakout!");        //打印Title
        wsprintf(InfoTxt,L"Press 'Space' To Start");        //打印提示

        TextOut(hdc,SCREEN_WIDTH/2-100,SCREEN_HEIGHT/4,Title,wcslen(Title));
        TextOut(hdc, SCREEN_WIDTH / 2-200, SCREEN_HEIGHT -200, InfoTxt, wcslen(InfoTxt));

        break;
    //绘制发球界面
    case Serve:
        DrawMap(hdc);
        DrawGameObject(hdc, ball,ball.x,ball.y);
        DrawGameObject(hdc,player,player.x,player.y);

        wsprintf(InfoTxt, L"Press 'Space' To Serve");
        TextOut(hdc, SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 175, InfoTxt, wcslen(InfoTxt));//打印提示
        wsprintf(InfoTxt, L"Level:%d", level);
        TextOut(hdc, SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 , InfoTxt, wcslen(InfoTxt));//打印提示
        break;
    //绘制游戏界面
    case Play:
        DrawEffect(hdc);
        DrawMap(hdc);
        DrawGameObject(hdc, ball, ball.x, ball.y);
        DrawGameObject(hdc, player, player.x, player.y);

        UpdateHealth(hdc,playerHp);
        wsprintf(Score, L"Score:%d", score);
        TextOut(hdc, 0, 0, Score, wcslen(Score));
        break;
    case GameOver:
        wsprintf(InfoTxt, L"Sorry, You Fail");
        TextOut(hdc, SCREEN_WIDTH / 2-160, SCREEN_HEIGHT / 2-80, InfoTxt, wcslen(InfoTxt));//打印提示
        wsprintf(InfoTxt, L"Press 'Space' To Restart");
        TextOut(hdc, SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2+80, InfoTxt, wcslen(InfoTxt));//打印提示
        break;
    case Win:
        wsprintf(InfoTxt, L"!!!Congratulation!!!");
        TextOut(hdc, SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT/2-100, InfoTxt, wcslen(InfoTxt));//打印提示
        wsprintf(InfoTxt, L"Your Highest Score is %d",highScore);
        TextOut(hdc, SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT /2, InfoTxt, wcslen(InfoTxt));//打印提示
        wsprintf(InfoTxt, L"Press 'Space' To Next Level");
        TextOut(hdc, SCREEN_WIDTH / 2 - 280, SCREEN_HEIGHT- 150, InfoTxt, wcslen(InfoTxt));//打印提示
        break;
    default:
        break;
    }

    SelectObject(hdc, hfOld);
    DeleteObject(font);
}


void Update(int dt) 
{

    if (gameState == Play) 
    {
        BrickCollide();
        EffectCollide();
        UpdateEffectPosition(dt);
    }

    if (gameState == Play || gameState == Serve)
    {
        mciSendString(L"open Resources\\sounds\\music.mp3", NULL, 0, NULL);//音乐
        mciSendString(L"play Resources\\sounds\\music.mp3 repeat", NULL, 0, NULL);
        switch (player.dir)
        {
        case LEFT:
            player.x = (player.x <= 0) ? 0 : player.x + player.vx * dt;
            ball.x += ball.vx * dt;
            ball.y += ball.vy * dt;
            if (gameState == Serve) 
            {
                ball.x = player.x + (player.w - ball.w) / 2;
                ball.y = player.y - ball.h;
            }
            break;
        case RIGHT:
            player.x = (player.x >= SCREEN_WIDTH - player.w) ? (SCREEN_WIDTH - player.w) : player.x + player.vx * dt;
            ball.x += ball.vx * dt;
            ball.y += ball.vy * dt;
            if (gameState == Serve)
            {
                ball.x = player.x + (player.w - ball.w) / 2;
                ball.y = player.y - ball.h;
            }
            break;
        case NONE:
            player.x += player.vx * 0;
            ball.x += ball.vx * dt;
            ball.y += ball.vy * dt;
            if (gameState == Serve)
            {
                ball.x = player.x + (player.w - ball.w) / 2;
                ball.y = player.y - ball.h;

            }
            break;
        }
    }
    else
    {
        mciSendString(L"stop Resources\\sounds\\music.mp3", NULL, 0, NULL);//音乐
        mciSendString(L"close Resources\\sounds\\music.mp3", NULL, 0, NULL);//音乐
    }
    //Wall Collide
    if (ball.x + ball.w >= SCREEN_WIDTH ) 
    {
        ball.vx = -ball.vx;
        ball.x = SCREEN_WIDTH - ball.w - 4;
        if(gameState == Play)
            PlaySound(TEXT("Resources\\sounds\\wall_hit.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放撞墙音效
    }
    else if (ball.x <= 0)
    {
        ball.vx = -ball.vx;
        ball.x = 4;
        if (gameState == Play)
            PlaySound(TEXT("Resources\\sounds\\wall_hit.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放撞墙音效

    }
    if (ball.y <= 0) 
    {
        ball.vy = -ball.vy;
        ball.y = 4;
        if (gameState == Play)
            PlaySound(TEXT("Resources\\sounds\\wall_hit.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放撞墙音效
    }
    //死亡
    if(ball.y+ball.h>=SCREEN_HEIGHT)
    {
        PlaySound(TEXT("Resources\\sounds\\hurt.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放失败音效

        playerHp--;
        if (playerHp > 0) 
        {            
            gameState = Serve;
        }
        else
        {
            if (highScore <= score)
                highScore = score;

            gameState = GameOver;
            ResetInfo();
            Reset();
        }
    }

    if (IsCollide(ball, player)) 
    {
        ball.y = player.y - ball.h;
        ball.vy = -ball.vy;

        if (ball.x < (player.x + player.w / 2) && player.vx < 0) 
        {
            ball.vx = -0.1*((player.x + player.w / 2) - ball.x);
            if (gameState == Play)
                PlaySound(TEXT("Resources\\sounds\\paddle_hit.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放球拍撞击音效

        }
        else if (ball.x > (player.x + player.w / 2) && player.vx > 0) 
        {
            ball.vx = 0.1 * fabs((player.x + player.w / 2) - ball.x);
            if (gameState == Play)
                PlaySound(TEXT("Resources\\sounds\\paddle_hit.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放球拍撞击音效
        }
    }

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 初始化全局字符串
    
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PRACTICEF, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRACTICEF));

    Initialize();


    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRACTICEF));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PRACTICEF);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindow(szWindowClass, TEXT("Breakout Developed By 22游技LDX"), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
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
        SCREEN_HEIGHT = HIWORD(lParam);
        SCREEN_WIDTH = LOWORD(lParam);
        break;
    case WM_CREATE: 
        {            
            //设置计时器
            SetTimer(hWnd,1,timeStep,NULL);

            srand((unsigned)time(NULL));
            break;
        }

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...

            //创建内存HDC
            HDC memHDC = CreateCompatibleDC(hdc);

            //获取客户区大小
            RECT rectClient;
            GetClientRect(hWnd, &rectClient);

            //创建位图
            HBITMAP bmpBuff = CreateCompatibleBitmap(hdc, SCREEN_WIDTH,SCREEN_HEIGHT);
            HBITMAP pOldBMP = (HBITMAP)SelectObject(memHDC, bmpBuff);
            PatBlt(memHDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITENESS);	// 设置背景为白色


            // 进行真正的绘制
            DrawScene(memHDC);


            //拷贝内存HDC内容到实际HDC
            BOOL tt = BitBlt(hdc, rectClient.left, rectClient.top, SCREEN_WIDTH, SCREEN_HEIGHT,
                 memHDC, rectClient.left, rectClient.top, SRCCOPY);

            //内存回收
            SelectObject(memHDC, pOldBMP);
            DeleteObject(bmpBuff);
            DeleteDC(memHDC);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_TIMER: 
        if (wParam == 1)	// 对游戏进行更新
        {
            Update(timeStep / 10);
            InvalidateRect(hWnd, NULL, TRUE);	// 让窗口变为无效,从而触发重绘消息
        }
        break;
    case WM_ERASEBKGND:		// 不擦除背景,避免闪烁
        break;
    case WM_KEYDOWN:
        InvalidateRect(hWnd,NULL,TRUE);
        switch (wParam) 
        {
        case VK_LEFT:
            player.dir = LEFT;
            player.vx = -8;
            break;
        case VK_RIGHT:
            player.dir = RIGHT;
            player.vx = 8;
            break;
        case VK_SPACE:
            if (gameState == Start)
            {
                PlaySound(TEXT("Resources\\sounds\\confirm.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放按键音效
                gameState = Serve;
            }
            else if (gameState == Serve)
            {
                gameState = Play;
                PlaySound(TEXT("Resources\\sounds\\confirm.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放按键音效
                ServeBall();
            }
            else if (gameState == GameOver) 
            {
                PlaySound(TEXT("Resources\\sounds\\confirm.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放按键音效
                gameState = Start;
            }
            else if (gameState == Win) 
            {
                PlaySound(TEXT("Resources\\sounds\\confirm.wav"), NULL, SND_FILENAME | SND_ASYNC); //播放按键音效
                gameState = Serve;
            }
            break;
        default:
            break;
        }
        break;
    case WM_KEYUP:
        InvalidateRect(hWnd,NULL,TRUE);
        switch (wParam) 
        {
        case VK_LEFT:
            player.dir = NONE;
            player.vx = 0;
            break;
        case VK_RIGHT:
            player.dir = NONE;
            player.vx = 0;
            break;
        default:
            break;
        }
        break;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
