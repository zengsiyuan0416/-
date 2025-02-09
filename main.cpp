#include<stdio.h>
#include<time.h>
#include<graphics.h>
#include"tools.h"
#include"vector2.h"

#include<mmsystem.h>
#include<math.h>
#pragma comment(lib,"winmm.lib")
#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZM_MAX 10

enum { WAN_DOU, XIANG_RI_KUI, ZHI_WU_COUNT };
IMAGE imgBg;
IMAGE imgBar;
IMAGE imgCards[ZHI_WU_COUNT];
IMAGE* imgZhiWu[ZHI_WU_COUNT][20];

int curX, curY;
int curZhiWu;

enum{GOING,WIN,FAIL};
int killCount;
int zmCount;
int gameStatus;

struct zhiwu {
	int type;
	int frameIndex;//序列帧的序号
	bool catched;//是否被僵尸捕获
	int deadTime;//死亡倒计时

	int timer;
	int x, y;

	int shootTime;
};

struct zhiwu map[3][9];

enum{SUNSHINE_DOWN,SUNSHINE_GROUND,SUNSHINE_COLLECT,SUNSHINE_PRODUCT};

struct sunshineBall {
	int x;
	int y;
	int destY;
	int frameIndex;
	bool used;
	int timer;

	float t;//贝塞尔曲线时间点

	float xoff;
	float yoff;

	vector2 p1, p2, p3, p4;
	vector2 pCur;
	float speed;
	int status;
};

struct sunshineBall balls[10];
IMAGE imgSunshineBalls[29];
int sunshine;

struct zm {
	int x, y;
	int frameIndex;
	bool used;
	int speed;
	int row;
	int blood;
	bool dead;
	bool eating;//吃植物
};
struct zm zms[10];
IMAGE imgZM[22];
IMAGE imgZMDead[20];
IMAGE imgZMEat[21];

//子弹的数据类型
struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast;//是否发生爆炸
	int frameIndex;//帧序号
};

struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBallBlast[4];
IMAGE imgZmStand[11];

bool fileExist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

void gameInit()
{
	loadimage(&imgBg, "res1\\res\\bg.jpg");
	loadimage(&imgBar, "res1\\res\\bar4.png");

	memset(imgZhiWu, 0, sizeof(imgZhiWu));
	memset(map, 0, sizeof(map));

	killCount = 0;
	zmCount = 0;
	gameStatus = GOING;

	char name[64];
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		sprintf_s(name, sizeof(name), "res1\\res\\Cards\\card_%d.png", i + 1);
		loadimage(&imgCards[i], name);
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res1\\res\\zhiwu\\%d\\%d.png", i, j + 1);
			if (fileExist(name)) {
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j], name);
			}
			else {
				break;
			}
		}
	}



	curZhiWu = 0;
	sunshine = 50;

	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res1\\res\\sunshine\\%d.png", i + 1);
		loadimage(&imgSunshineBalls[i], name);
	}

	srand(time(NULL));

	//创建游戏窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);

	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);
	setcolor(BLACK);

	//初始化僵尸数据
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res1/res/zm/%d.png", i + 1);
		loadimage(&imgZM[i], name);
	}

	loadimage(&imgBulletNormal, "res1/res/bullets/PeaNormal/PeaNormal_0.png");
	memset(bullets, 0, sizeof(bullets));

	//初始化豌豆子弹的帧图片数组
	loadimage(&imgBallBlast[3], "res1/res/bullets/PeaNormalExplode/PeaNormalExplode_0.png");
	for (int i = 0; i < 3; i++)
	{
		float k = (i + 1) * 0.2;
		loadimage(&imgBallBlast[i], "res1/res/bullets/PeaNormalExplode/PeaNormalExplode_0.png", imgBallBlast[3].getwidth() * k, imgBallBlast[3].getheight() * k, true);
	}

	for (int i = 0; i < 20; i++)
	{
		sprintf_s(name, sizeof(name), "res1/res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}

	for (int i = 0; i < 21; i++)
	{
		sprintf_s(name, sizeof(name), "res1/res/zm_eat/%d.png", i + 1);
		loadimage(&imgZMEat[i], name);
	}

	for (int i = 0; i < 11; i++)
	{
		sprintf_s(name, sizeof(name), "res1\\res\\zm_stand\\%d.png", i + 1);
		loadimage(&imgZmStand[i], name);
	}
}


void drawZM() {
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++) {
		if(zms[i].used)
		{
			//IMAGE* img = &imgZM[zms[i].frameIndex];
			//IMAGE* img = (zms[i].dead) ? imgZMDead : imgZM;
			IMAGE* img = NULL;
			if (zms[i].dead) img = imgZMDead;
			else if (zms[i].eating) img = imgZMEat;
			else img = imgZM;

			img += zms[i].frameIndex;

			putimagePNG(zms[i].x,
				zms[i].y - img->getheight(),
				img);
		}

	}
}

void drawSunshines()
{
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		//if (balls[i].used || balls[i].xoff) {
		if(balls[i].used) {
			IMAGE* img = &imgSunshineBalls[balls[i].frameIndex];
			//putimagePNG(balls[i].x, balls[i].y, img);
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}

	}
}

void drawCards()
{
	putimage(0, 0, &imgBg);
	//putimage(250, 0, &imgBar);
	putimagePNG(250, 0, &imgBar);

	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 320 + i * 65;
		int y = -3;
		putimage(x, y, &imgCards[i]);
	}

}

void drawZhiWu()
{
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0)
			{
				int x = 256-80 + j * 81;
				int y = 179 + i * 102 + 14;
				int zhiwuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(x, y, imgZhiWu[zhiwuType][index]);
				//putimagePNG(map[i][j].x, map[i][j].y, imgZhiWu[zhiwuType][index]);
			}
		}

	}
	if (curZhiWu > 0) {
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];
		putimagePNG(curX - img->getwidth(), curY - img->getheight(), img);
	}
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(276, 58, scoreText);//输出分数
}

void drawBullets()
{
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++)
	{
		if (bullets[i].used)
		{
			if (bullets[i].blast)
			{
				IMAGE* img = &imgBallBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else
			{
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

void updateWindow() {
	BeginBatchDraw();

	putimage(-112, 0, &imgBg);
	putimagePNG(250, 0, &imgBar);

	drawCards();
	drawZhiWu();
	drawSunshines();

	

	drawZM();

	drawBullets();

	EndBatchDraw();
}

void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBalls[0].getwidth();
	int h = imgSunshineBalls[0].getheight();
	for (int i = 0; i < count; i++) {
		//int x = balls[i].x;
		//int y = balls[i].y;
		int x = balls[i].pCur.x;
		int y = balls[i].pCur.y;
		if (msg->x > x && msg->x<x + w && msg->y>y && msg->y < y + h) {
			//balls[i].used = false;
			balls[i].status = SUNSHINE_COLLECT;//qwq
			//sunshine += 25;
			//mciSendString("play res1/res/sunshine.mp3", 0, 0, 0);
			PlaySound(NULL , NULL, SND_FILENAME | SND_ASYNC);
			//设置阳光球的偏移量
			//float destY = 0;
			//float destX = 262;
			//float angle = atan((y - destY) / (x - destX));
			//balls[i].yoff = 4 * sin(angle);
			//balls[i].xoff = 4 * cos(angle);
			balls[i].p1 = balls[i].pCur;
			balls[i].p4 = vector2(262, 0);
			balls[i].t = 0;
			float distance = dis(balls[i].p1 - balls[i].p4);
			float off = 8;
			balls[i].speed = 1.0 / (distance/off);
			break;
		}
	}
}

void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x > 320 && msg.x < 320 + 65 * ZHI_WU_COUNT && msg.y<87) {
				int index = (msg.x - 320) / 65;
				status = 1;
				curZhiWu = index + 1;
			}
			else {
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status==1) {
			curX = msg.x;
			curY = msg.y;

		}
		else if (msg.message == WM_LBUTTONUP) {
			if (msg.x > 256-112 && msg.y > 179 && msg.y < 489) {
				int row = (msg.y - 179) / 102;
				int col = (msg.x - (256-112)) / 81;
				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;
					map[row][col].frameIndex = 0;
					map[row][col].shootTime = 0;

					map[row][col].x = 256-112 + col * 81;
					map[row][col].y = 179 + col * 102 + 14;
				}
			}
	
			curZhiWu = 0;
			status = 0;
		}
	}

}


void creatSunshine() {
	static int count = 0;
	static int fre = 400;
	count++;
	if (count >= fre) {
		fre = 200 + rand() % 200;
		count = 0;


		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax) return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		//balls[i].x = 260 + rand() % (900 - 260);
		//balls[i].y = 60;
		//balls[i].destY = 200 + (rand() % 4) * 90;
		balls[i].timer = 0;
		//balls[i].xoff = 0;
		//balls[i].yoff = 0;

		balls[i].status = SUNSHINE_DOWN;//qwq
		balls[i].t = 0;
		balls[i].p1 = vector2(260-112 + rand() % (900 - 260+112), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);
	}	

	//向日葵生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == XIANG_RI_KUI + 1)
			{
				map[i][j].timer++;
				if (map[i][j].timer > 200)
				{
					map[i][j].timer = 0;
					

					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) return;

					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgZhiWu[XIANG_RI_KUI][0]->getheight() -
						imgSunshineBalls[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCT;
				}
			}
		}
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1)
				{
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND)
			{
				balls[i].timer++;
				if (balls[i].timer > 100)
				{
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1)
				{
					sun->used = false;
					sunshine += 25;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1)
				{
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}


		}

	}
}

void creatZM() {
	if (zmCount >= ZM_MAX)
	{
		return;
	}


	static int zmFre = 200;
	static int count = 0;
	count++;
	if (count >= zmFre) {
		count = 0;
		zmFre = rand() % 200+300;

		int i;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMax && zms[i].used; i++);
			if (i < zmMax) {
				memset(&zms[i], 0, sizeof(zms[i]));
				zms[i].used = true;
				zms[i].x = WIN_WIDTH;
				zms[i].row = rand() % 3;//0..2
				zms[i].y = 172 + (zms[i].row + 1) * 100;
				zms[i].speed = 10;
				zms[i].blood = 100;
				zms[i].dead = false;
				zmCount++;
			}
		
	}
	
	
}

void updateZM() {
	int zmMax = sizeof(zms) / sizeof(zms[0]);

	static int count = 0;
	count++;
	if (count > 2*2) {
		count = 0;
		//更新僵尸的位置
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 56) {
					//printf("Game,Over\n");
					//MessageBox(NULL, "over", "over", 0);//待优化
					//exit(0);
					gameStatus = FAIL;
				}
			}
		}
	}
	static int count2 = 0;
	count2++;
	if(count2>4*2)
	{
		count2 = 0;
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				if (zms[i].dead)
				{
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20)
					{
						zms[i].used = false;
						killCount++;
						if (killCount == ZM_MAX)
						{
							gameStatus = WIN;
						}
					}
				}
				else if (zms[i].eating)
				{
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else
				zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
			}
		}
	}
}

void shoot()
{
	static int count = 0;
	if (++count < 2) return;
	count = 0;
	int lines[3] = { 0 };
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - imgZM[0].getwidth();
	for (int i = 0; i < zmCount; i++)
	{
		if (zms[i].used && zms[i].x < dangerX)
		{
			lines[zms[i].row] = 1;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == WAN_DOU + 1 && lines[i])
			{
				//static int count2 = 0;
				//count2++;
				map[i][j].shootTime++;
				if (map[i][j].shootTime > 20)
				{
					//count2 = 0;
					map[i][j].shootTime = 0;

					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax)
					{
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 4;

						bullets[k].blast = false;
						bullets[k].frameIndex = 0;

						int zwX = 256-112 + j * 81;
						int zwY = 179 + i * 102 + 14;
						bullets[k].x = zwX + imgZhiWu[map[i][j].type - 1][0]->getwidth()-10;
						bullets[k].y = zwY + 5;
					}
				}
			}
		}
	}
}


void updateBullets()
{
	int countMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < countMax; i++)
	{
		if (bullets[i].used)
		{
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH)
			{
				bullets[i].used = false;
			}

			//待实现子弹的碰撞检测
			if (bullets[i].blast)
			{
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= 4)
				{
					bullets[i].used = false;
				}
			}
		}
	}
}

void checkBullet2Zm()
{
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bCount; i++)
	{
		if (bullets[i].used == false || bullets[i].blast) continue;
		for (int k = 0; k < zCount; k++)
		{
			if (zms[k].used == false) continue;
			int x1 = zms[k].x + 80;
			int x2 = zms[k].x + 110;
			int x = bullets[i].x;
			if (zms[k].dead == false && bullets[i].row == zms[k].row && x > x1 && x < x2)
			{
				zms[k].blood -= 10;
				bullets[i].blast = true;
				bullets[i].speed = 0;

				if (zms[k].blood <= 0)
				{
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;

				}

				break;
			}
		}
	}
}

void checkZm2ZhiWu()
{
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zCount; i++)
	{
		if (zms[i].dead) continue;

		int row = zms[i].row;
		for (int k = 0; k < 9; k++)
		{
			if (map[row][k].type == 0) continue;

			int ZhiWuX = 256-112 + k * 81;
			int x1 = ZhiWuX + 10;
			int x2 = ZhiWuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2)
			{
				if (map[row][k].catched)
				{
					//zms[i].frameIndex++;
					map[row][k].deadTime++;
					//if (zms[i].frameIndex > 100)
					if(map[row][k].deadTime>100)
					{
						map[row][k].deadTime = 0;
						map[row][k].type = 0;
						zms[i].eating = false;
						zms[i].frameIndex = 0;
						zms[i].speed = 1;
					}
				}
				else
				{
					map[row][k].catched = true;
					map[row][k].deadTime = 0;
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
				}
			}
		}
	}
}

void collisionCheck()
{
	checkBullet2Zm();//子弹对僵尸的碰撞检测
	checkZm2ZhiWu();//僵尸对植物的碰撞检测
}

void updateZhiWu()
{
	static int count = 0;
	if (++count < 2) return;
	count = 0;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameIndex++;
				int zhiwuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgZhiWu[zhiwuType][index] == NULL) {
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}

void updateGame() {
	updateZhiWu();
	creatSunshine();
	updateSunshine();

	creatZM();
	updateZM();

	shoot();//发射子弹
	updateBullets();//更新豌豆子弹

	collisionCheck();
}
//开始界面的创建
void startUI() {
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res1\\res\\Screen\\MainMenu.png");
	loadimage(&imgMenu1, "res1\\res\\menu1.png");
	loadimage(&imgMenu2, "res1\\res\\menu2.png");
	int flag = 0;

	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);
		EndBatchDraw();

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN &&	
				msg.x>474 && msg.x<474+300 &&
				msg.y>75 && msg.y<75+140
				) {
				flag = 1;
			}
			if (msg.message == WM_LBUTTONUP && flag) {
				EndBatchDraw();
				return;
			}
		}
	}
	
}

void viewScene()
{
	int xMin = WIN_WIDTH - imgBg.getwidth();
	vector2 points[9] = {
		{550,80},{530,160},{630,170},{530,200},{515,270},{565,370},
		{605,340},{705,280},{690,340}
	};
	int index[9];
	for (int i = 0; i < 9; i++)
	{
		index[i] = rand() % 11;
	}
	int count = 0;
	for (int x = 0; x >= xMin; x -= 2)
	{
		BeginBatchDraw();
		putimage(x, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x-xMin+x,points[k].y,&imgZmStand[index[k]]);
			if (count >= 10)
			{
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10) count = 0;
		EndBatchDraw();
		Sleep(5);
	}

	//停留
	for (int i = 0; i < 100; i++)
	{
		BeginBatchDraw();

		putimage(xMin, 0, &imgBg);
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x, points[k].y, &imgZmStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}
		EndBatchDraw();
		Sleep(15);
	}

	for (int x = xMin; x <= -112; x += 2)
	{
		BeginBatchDraw();

		putimage(x, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 10)
			{
				index[k] = (index[k] + 1) % 11;
			}
			if (count >= 10) count = 0;
		}

		EndBatchDraw();
		Sleep(5);
	}
}

void BarsDown()
{
	int height = imgBar.getheight();
	for (int y = -height; y <= 0; y++)
	{
		BeginBatchDraw();

		putimage(-112, 0, &imgBg);
		putimage(250, y, &imgBar);

		for (int i = 0; i < ZHI_WU_COUNT; i++)
		{
			int x = 320 + i * 65;
			putimage(x, 6+y, &imgCards[i]);
		}

		EndBatchDraw();
		Sleep(10);
	}
}

bool checkOver()
{
	int ret = false;
	if (gameStatus == WIN)
	{
		Sleep(2000);
		loadimage(0, "res1\\res\\gameWin.png");
		ret = true;
	}
	else if (gameStatus == FAIL)
	{
		Sleep(2000);
		loadimage(0, "res1/res/gameFail.png");
		ret = true;
	}
	return ret;
}

int main(void)
{
	gameInit();

	startUI();

	viewScene();

	BarsDown();

	int timer = 0;
	bool flag = true;
	while (1) {
		userClick();//小游戏里常用的减速帧
		timer += getDelay();
		if (timer > 20) {
			flag = true;
			timer = 0;
		}

		if(flag)
		{
			flag = false;
			updateWindow();
			updateGame();
			if (checkOver()) break;
		}
	}

	system("pause");
	return 0; 
}