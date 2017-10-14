/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :	ui_games_sub.c
* Author    : 	hulj
* Project   :	GX6102_DAWorld
* Type      :	APP
******************************************************************************
* Purpose   :	ģ��ʵ���ļ�
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008/03/27        hulj           creation
   2.5		2009/07/07		  lishj			 modify
*****************************************************************************/
#include "app.h"
#include "app_wnd_system_setting_opt.h"
#if (GAME_ENABLE > 0)
#define WND_TETRIS "wnd_tetris"
#define CANVAS_TETRIS_MAIN "canvas_main_tetris"
#define CANVAS_TETRIS_LITTER "canvas_little_tetris"
#define TXT_TETRIS_LEVEL "txt_tetris_level_value"
#define TXT_TETRIS_SCORE "txt_tetris_score_value"
#define TXT_TETRIS_LINE "txt_tetris_line_value"
#define TXT_TETRIS_OK "txt_tetris_ok_info"
#define BOX_TETRIS_STATUS "box_tetris_info"
#define BTN_TETRIS_STATUS "btn_tetris_status"

#define IMG_TETRIS_DIAMOND_BULE "GX6102_G_T_BLUE.bmp"
#define IMG_TETRIS_DIAMOND_BROWN "GX6102_G_T_BROWN.bmp"
#define IMG_TETRIS_DIAMOND_GREEN "GX6102_G_T_GREEN.bmp"
#define IMG_TETRIS_DIAMOND_ORANGE "GX6102_G_T_ORANGE.bmp"
#define IMG_TETRIS_DIAMOND_PINK "GX6102_G_T_PINK.bmp"
#define IMG_TETRIS_DIAMOND_RED "GX6102_G_T_RED.bmp"
#define IMG_TETRIS_DIAMOND_YELLOW "GX6102_G_T_YELLOW.bmp"

#define CANVAS_WIDTH_MAIN 420 
#define CANVAS_HEIGHT_MAIN 460
#define STRLEN 20
#define INIT_TIMER_VALUE 500

#define OFFSET_X 20
#define OFFSET_Y 20


#define CANVAS_WIDTH_LITTLE 100
#define CANVAS_HEIGHT_LITTLE 100

/* Private Constants --------------------------------------------------------*/
#define CellSize	 20		//��С���鵥Ԫ��С
#define VCellNo		 23		//��ֱ���򷽿���
#define HCellNo		 23		//ˮƽ���򷽿���



typedef enum
{
	GAMEQUIT,
	GAMESTART,
	GAMEPAUSE,
	GAMEOVER
}TetrisStatus;

/* Private Variables (static)------------------------------------------------*/ 
static uint16_t RussionGrade=0;		//��ǰ����
static uint32_t totalScore = 0;

char Top;						//�붥���ľ��룬�߼�����
static uint16_t Sel;						//���7�ַ����е���һ��(����)
static uint16_t SelInMove;						//���7�ַ����е���һ�� (��ǰ)
static uint16_t Flag;						//��Ƿ������ת����
static uint16_t org[4][2];					//ԭʼ�������
static uint16_t block[4][2];				//��ǰ�������
static uint16_t blockN[4][2];				//�����������
uint16_t RussionCurScoreline = 0;			//��ǰ�÷�
static uint16_t g_ShareBuffer0[HCellNo][VCellNo];		// �߼�����ϵ����
static TetrisStatus gameStatus = GAMEQUIT;

static event_list* tetrisTimer = NULL;
static int app_game_tetris_timer_handle(void *userdata);


/*******************************************************\
*				��ʼ������								*
\*******************************************************/
static void app_game_russion_Datainit(void)
{
	uint32_t i,j;
	
	Top = VCellNo-1;
	RussionCurScoreline = 0;
	totalScore = 0;
	//RussionGrade = 0;
	
	/* ���߼�����ϵ�ĵ�һ�к����һ����1�����Ʒ��鲻������Ϸ���� */
	for(i=0; i<VCellNo; i++)
	{
		g_ShareBuffer0[0][i]=8;
		g_ShareBuffer0[HCellNo-1][i]=8;
	}
	
	/* ���߼�����ϵ�������һ����1�����Ʒ��鲻������Ϸ���� */
	for(i=0; i<HCellNo; i++)
		g_ShareBuffer0[i][VCellNo-1]=8;
	
	/* ���߼�����ϵ��������ȫ��0����Ϸ����ֻ���������ƶ� */
	for(i=1; i<HCellNo-1; i++)
	{
		for(j=0; j<VCellNo-1; j++)
			g_ShareBuffer0[i][j]=0;
	}

}



static void app_tetris_image_draw(const char *widget_name,char *path,uint32_t x,uint32_t y)
{
	GuiUpdateImage img = {0};
	
	img.x = x;
	img.y = y;
	img.name = path;
	
	GUI_SetProperty(widget_name, "image",(void*)&img);
}


static void app_draw_main_canvas_image(char *path,uint32_t x,uint32_t y)
{
	app_tetris_image_draw(CANVAS_TETRIS_MAIN,path,x-OFFSET_X,y+OFFSET_Y);
}


static void app_draw_little_canvas_image(char *path,uint32_t x,uint32_t y)
{
	app_tetris_image_draw(CANVAS_TETRIS_LITTER,path,x,y);
}

static  void app_game_tetris_number2string(char *numStr,uint32_t len,uint32_t number)
{
	if(NULL == numStr || 0 == len)
		return;
	
	memset(numStr,0,len);
	sprintf(numStr,"%d",number);
	numStr[len -1] = '\0';
}


static void app_game_tetris_score_calculate(void)
{
#define MAXLINE 9

	char str[STRLEN] = {0};
	if(RussionCurScoreline >MAXLINE)
	{
		RussionGrade++;
		app_game_tetris_number2string(str,STRLEN,RussionGrade);
		GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);
		RussionCurScoreline -= MAXLINE;
		app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
		GUI_SetProperty(TXT_TETRIS_LINE, "string",str);
	}
}

static void app_tetris_canvas_clear(const char *widget_name, uint16_t x, uint16_t y,uint16_t width, uint16_t height)
{
	GuiUpdateRect rect = {0};

	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	rect.color = "Snake_BG";
	
	GUI_SetProperty(widget_name, "rectangle",(void*)&rect);
}


/*******************************************************\
*		�����߼�����ϵ��������ֵ�ػ�������Ϸ����		*
\*******************************************************/
void app_game_russion_paint(void)
{
	uint16_t m = 0;
	uint16_t n = 0;
	
	for (m=Top; m<VCellNo-1; m++)
	{
		for(n=1; n<HCellNo-1; n++)
		{
			if(1==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_RED,n*CellSize, m*CellSize);
			if(2==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,n*CellSize, m*CellSize);
			if(3==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BULE,n*CellSize, m*CellSize);
			if(4==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_GREEN, n*CellSize, m*CellSize);
			if(5==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BROWN,n*CellSize, m*CellSize);
			if(6==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_PINK,n*CellSize,m*CellSize);
			if(7==g_ShareBuffer0[n][m]) 
				app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,n*CellSize,m*CellSize);
		}
	}
}

/*******************************************************\
*				�����µķ���							*
\*******************************************************/
void app_game_russion_new_block(void)
{
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t k = 0;
	uint16_t lines = 0;

	Flag = 0;	/*������ת����*/
	
	for(i=Top; i<VCellNo-1; i++)
	{
		lines =0;
		/* ����Ƿ���ĳһ��ȫ��������,����������ȫ��1 */
		for(j=1; j<HCellNo; j++)
		{
			if(! g_ShareBuffer0[j][i]) 
			{
				lines=1;
				break;
			}
		}
		
		/* �����б�����������һ�е����״̬���Ƶ����У��������� */
		/* ���Ӹ��п�ʼ�����е��߼����궼����һ�У��÷ּ�1��Top��1 */
		if(!lines)
		{
			char str[STRLEN] = {0};
			for(j=1;j<HCellNo; j++)
			{
				for(k=i; k>=Top; k--)
				{
					g_ShareBuffer0[j][k]=g_ShareBuffer0[j][k-1];
				}
			}
			RussionCurScoreline++;
			app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
			GUI_SetProperty(TXT_TETRIS_LINE, "string",str);
			//totalScore += RussionCurScoreline;
			totalScore ++;
			app_game_tetris_number2string(str,STRLEN,totalScore);
			GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);
			app_game_tetris_score_calculate();
				
			app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0, 0,CANVAS_WIDTH_MAIN, CANVAS_HEIGHT_MAIN);
			app_game_russion_paint();
			Top++;
		}
	}
	
	for(i=0;i<4;i++)
	{
		for(j=0;j<2;j++)
		{
			org[i][j]=block[i][j] =blockN[i][j];
		}
	}

	SelInMove = Sel;
}



void app_game_russion_next_block(void)
{
#define X_OFFSET 10
#define Y_OFFSET 0

	uint16_t  i = 0;

	/* �������飬ÿ���������״��4��CELL������ÿ��CELL��λ����block[i][0]x�����block[i][1]y������� */
	/* Ϊʹ��ʼ�����ڶ������룬block[i][0]=7/8/9/10��block[i][1]=0/1/2 */ 
	Sel = rand()%7;
        
	switch(Sel)
	{
		case 0:
			blockN[0][0]=6;	blockN[0][1] =0;
			blockN[1][0]=7;	blockN[1][1] =0;
			blockN[2][0]=6;	blockN[2][1] =1;
			blockN[3][0]=7;	blockN[3][1] =1;
			break;

		case 1:
			blockN[0][0] =5;	blockN[0][1] =0;
			blockN[1][0] =6;	blockN[1][1] =0;
			blockN[2][0] =7;	blockN[2][1] =0;
			blockN[3][0] =8;	blockN[3][1] =0;
			break;

		case 2:
			blockN[0][0] =6;	blockN[0][1] =0;
			blockN[1][0] =6;	blockN[1][1] =1;
			blockN[2][0] =7;	blockN[2][1] =1;
			blockN[3][0] =7;	blockN[3][1] =2;
			break;

		case 3:
			blockN[0][0] =7;	blockN[0][1] =0;
			blockN[1][0] =7;	blockN[1][1] =1;
			blockN[2][0] =6;	blockN[2][1] =1;
			blockN[3][0] =6;	blockN[3][1] =2;
			break;
		
		case 4:
			blockN[0][0] =6;	blockN[0][1] =0;
			blockN[1][0] =6;	blockN[1][1] =1;
			blockN[2][0] =6;	blockN[2][1] =2;
			blockN[3][0] =7;	blockN[3][1] =2;
			break;

		case 5:
			blockN[0][0] =7;	blockN[0][1] =0;
			blockN[1][0] =7;	blockN[1][1] =1;
			blockN[2][0] =7;	blockN[2][1] =2;
			blockN[3][0] =6;	blockN[3][1] =2;
			break;
		case 6:
			blockN[0][0] =6;	blockN[0][1] =0;
			blockN[1][0] =5;	blockN[1][1] =1;
			blockN[2][0] =6;	blockN[2][1] =1;
			blockN[3][0] =7;	blockN[3][1] =1;
			break;
		default:
			break;
	}
	
	app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_WIDTH_LITTLE);
	
	for(i=0;i<4;i++)
	{
		switch(Sel)
		{
			case 0:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_RED,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 1:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 2:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BULE,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 3:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_GREEN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 4:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BROWN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 5:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_PINK,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			case 6:
				app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,(blockN[i][0]-5)*CellSize + X_OFFSET+10,(blockN[i][1]+1)*CellSize + X_OFFSET);
				break;
			default:
				break;
		}
	}
}




/*******************************************************\
*				����һ����С�ķ��鵥Ԫ					*
\*******************************************************/
void app_game_russion_draw_cell(uint16_t left, uint16_t top)
{
 	switch(SelInMove)	{
		case 0:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_RED,left, top);
			break;

		case 1:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,left, top);
			break;

		case 2:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BULE,left, top);
			break;

		case 3:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_GREEN,left, top);
			break;

		case 4:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_BROWN,left, top);
			break;

		case 5:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_PINK,left, top);
			break;
			
		case 6:
			app_draw_main_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,left, top);
			break;
		default:
			break;
	}
	
}


/*******************************************************\
*				�����˶�����							*
*				ÿ��������4��CELL��ɣ���7����״		*
\*******************************************************/
void app_game_russion_draw_block(uint16_t block [4][2])
{
	unsigned char i;
	for(i=0; i<4; i++)
	{
		app_game_russion_draw_cell(block[i][0]*CellSize, block[i][1]*CellSize);
	}
}


/*******************************************************************\
*			���ԭλ�õķ���										*
*			��ԭ�����ÿ��CELL����һ����CELLͬ����С�ĺڿ�			*
\*******************************************************************/
void app_game_russion_cover (uint16_t org[4][2])
{
	uint16_t i;
	for(i=0; i<4; i++)
	{
		app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,org[i][0]*CellSize-OFFSET_X,org[i][1]*CellSize+OFFSET_Y,CellSize,CellSize);		
	}
}

/*******************************************************\
*		�����ǰ���飬�����µ�λ�����»��Ʒ���			*
\*******************************************************/
void app_game_russion_draw(void)
{
	uint16_t i = 0;
	uint16_t j = 0;

	//���ǰһ�εķ���
	app_game_russion_cover(org);
	for(i=0; i<4; i++)
	{
		for(j=0; j<2; j++)
			org[i][j]=block[i][j];		
	}


	//�ػ��µķ���
	app_game_russion_draw_block(block);
	return;
}


/*******************************************************\
*				������Ӧ���ƶ���������				*
\*******************************************************/
void app_game_russion_move(uint16_t key)
{
	uint16_t r = 0;
	uint16_t i = 0;
	uint16_t j = 0;
	
	switch(key)
	{
		case STBK_LEFT:
			for(i=0; i<4; i++)
				block[i][0]--;
			break;

		case STBK_RIGHT:
			for(i=0; i<4; i++)
				block[i][0]++;
			break;
			
		case STBK_DOWN:
			for(i=0; i<4; i++)
				block[i][1]++;
			break;

		case STBK_UP:
			r=1;
			Flag++;     //������ת�����ۼ�
			switch(SelInMove) 
			{
				case 0: 
					break;

				case 1: 
					Flag =Flag%2;
					for(i=0; i<4; i++)
					{
						block[i][(Flag+1)%2] =org[2][(Flag+1)%2];
						block[i][Flag] =org[2][Flag]-2+i;
					}
					break;

				case 2:
					Flag =Flag%2;
					if(Flag)
					{	
						block[0][1] +=2;	block[3][0] -=2;	
					}
					else
					{
						block[0][1] -=2;	block[3][0] +=2;	
					}
					break;

				case 3:
					Flag =Flag%2;
					if(Flag)
					{
						block[0][1] +=2;	block[3][0] +=2;	
					}
					else
					{
						block[0][1] -=2;	block[3][0] -=2;	
					}
					break;

				case 4:
					Flag=Flag%4;
					switch(Flag)
					{
						case 0:
							block[2][0] +=2;	block[3][0] +=2;
							block[2][1] +=1;	block[3][1] +=1;
							break;
						case 1:
							block[2][0] +=1;	block[3][0] +=1;
							block[2][1] -=2;	block[3][1] -=2;
							break;
						case 2:
							block[2][0] -=2;	block[3][0] -=2;
							block[2][1] -=1;	block[3][1] -=1;
							break;
						case 3:
							block[2][0] -=1;	block[3][0] -=1;
							block[2][1] +=2;	block[3][1] +=2;
							break;
					}
					break;

				case 5:
					Flag=Flag%4;
					switch(Flag)
					{
						case 0:
							block[2][0] +=1;	block[3][0] +=1;
							block[2][1] +=2;	block[3][1] +=2;
							break;
						case 1:
							block[2][0] +=2;	block[3][0] +=2;
							block[2][1] -=1;	block[3][1] -=1;
							break;
						case 2:
							block[2][0] -=1;	block[3][0] -=1;
							block[2][1] -=2;	block[3][1] -=2;
							break;
						case 3:
							block[2][0] -=2;	block[3][0] -=2;
							block[2][1] +=1;	block[3][1] +=1;
							break;
					}
					break;

				case 6:
					Flag =Flag%4;
					switch(Flag)
					{
						case 0:
							block[0][0]++; block[0][1]--;
							block[1][0]--; block[1][1]--;
							block[3][0]++; block[3][1]++;
							break;
						case 1:
							block[1][0]++; block[1][1]++; break;
						case 2:
							block[0][0]--; block[0][1]++; break;
						case 3:
							block[3][0]--; block[3][1]--; break;
					}
					break;
				}
				break;
			default:
				break;
	}
	
	/* �жϷ�����ת����λ���Ƿ���CELL�����У�����תȡ�� */
	for(i=0; i<4; i++)
	{
		if(g_ShareBuffer0[block[i][0]][ block[i][1]])
		{ 
			if(r)
			{
				Flag +=3;
			}	
			for(i=0; i<4; i++)
			{
				for(j=0; j<2; j++)
					block[i][j]=org[i][j];
			}
			return;
		}
	}
	app_game_russion_draw();
}


/*******************************************************\
*				���Ʒ��飬ʹ�����˶�					*
\*******************************************************/
int  app_game_russion_down_able(void)
{
	uint16_t i,Temp=0xFFFF;
	uint32_t X ,Y = 0;
	static  uint32_t FirstBlock = 0;

	if(FirstBlock == 0)
	{
		FirstBlock = 1;
		app_game_russion_draw_block(org);
	}

	for(i=0; i<4; i++)
		block[i][1]++;
	
	/* ��鷽�������Ƿ񱻵�ס�����ж����ƺ��������Ƿ���1 */
	for(i=0; i<4; i++)
	{
		if(g_ShareBuffer0[ block[i][0] ][ block[i][1] ])
		{ 
			for(i=0; i<4; i++) 
				g_ShareBuffer0[ org[i][0] ][ org[i][1] ]=SelInMove+1; 
			if(Top>org[0][1]-2)
				Top=org[0][1]-2;
			FirstBlock = 0;

			if(org[0][1]<=2)
				Temp = org[0][1]-1;
			
			app_game_russion_new_block();
			app_game_russion_next_block();

			if(Temp != 0xFFFF)
			{
				if(Temp == 1)//��ʣ����
				{
					if(org[3][1]==2)
					{
						for(i=4; i>0; i--)
						if(org[i-1][1]>=1)
						{
							X = org[i-1][0]*CellSize;
							Y = (org[i-1][1]-1)*CellSize;
							app_game_russion_draw_cell(X, Y);
							
						}
					}
					else if(org[3][1]==1)
					{
						for(i=4; i>0; i--)
						{
							X = org[i-1][0]*CellSize;
							Y = (org[i-1][1])*CellSize;
							app_game_russion_draw_cell(X, Y);
							
						}
					}
					else if(org[3][1]==0)
					{
						for(i=4; i>0; i--)
						{
							X = org[i-1][0]*CellSize;
							Y = (org[i-1][1]+1)*CellSize;
							app_game_russion_draw_cell(X, Y);
							
						}
						//ʣ�����У��Ҵ˿�ֻ��һ�У����ٳ���һ��
						app_game_russion_new_block();
						app_game_russion_next_block();
						for(i=4; i>0; i--)
						{
							if(org[i-1][1]==org[3][1])
							{
								X = org[i-1][0]*CellSize;
								Y = 0;
								app_game_russion_draw_cell(X, Y);
								
							}
						}
					}
				}
				else if(Temp==0)//��ʣһ��
				{
					for(i=4; i>0; i--)
					{
						if(org[i-1][1]==org[3][1])
						{
							X = org[i-1][0]*CellSize;
							Y = 0;
							app_game_russion_draw_cell(X, Y);
						}

					}
				}
				Temp = 0xFFFF;
			}
			return 0;
		}
	}
	return 1;
}
void app_game_russion_down(void)
{
	if(app_game_russion_down_able())
	{
		app_game_russion_draw();
	}
}

void app_game_russion_game_process(void)
{
	app_game_russion_draw();
}


static void app_game_tetris_over(void)
{
	gameStatus = GAMEQUIT;

	if(NULL != tetrisTimer)
	{
		timer_stop(tetrisTimer);
		remove_timer(tetrisTimer);
		tetrisTimer = NULL;
		gameStatus = GAMEQUIT;
	}
}

static int app_game_tetris_timer_handle(void *userdata)
{
	if(GAMEOVER == gameStatus)
	{
		GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_GAMEOVER);
		GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
		app_game_tetris_over();
	}
	else if(GAMESTART == gameStatus)
	{
		app_game_russion_game_process();
		app_game_russion_down();

		if ((Top<1)|(Top>VCellNo))
		{
			gameStatus = GAMEOVER;
		}
	}else{}
	
	return 0;
}

static void app_game_tetris_set_andvance(void)
{
	int interval_timer = INIT_TIMER_VALUE;
	interval_timer = interval_timer/(RussionGrade+1);
	if(NULL != tetrisTimer)
	{
		remove_timer(tetrisTimer);
		tetrisTimer = create_timer(app_game_tetris_timer_handle, interval_timer, NULL, TIMER_REPEAT);
	}
}

static void app_game_handle_grade(void)
{
	char str[STRLEN] = {0};
	app_game_tetris_set_andvance();
	app_game_tetris_number2string(str,STRLEN,RussionGrade);
	GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);
}

void app_game_tetris_create(void)
{
    GUI_CreateDialog(WND_TETRIS);
}

#endif

/* Export functions ------------------------------------------------------ */
SIGNAL_HANDLER int app_tetris_create(const char* widgetname, void *usrdata)
{
#if (GAME_ENABLE > 0)
	char str[STRLEN] = {0};

	gameStatus = GAMEQUIT;
	//GUI_SetProperty(BOX_TETRIS_STATUS, "state","disable");
	GUI_SetProperty(WND_TETRIS, "state","hide");
	GUI_SetInterface("flush",NULL);
	app_game_russion_Datainit();

	app_game_tetris_number2string(str,STRLEN,RussionGrade);
	GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);

	app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
	GUI_SetProperty(TXT_TETRIS_LINE, "string",str);

	app_game_tetris_number2string(str,STRLEN,totalScore);
	GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);

	GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_READY);
	GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);


	  GUI_SetFocusWidget("btn_tetris_status");
	GUI_SetProperty(WND_TETRIS, "state","show");

	GUI_SetInterface("flush",NULL);

	app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0,0,CANVAS_WIDTH_MAIN,CANVAS_HEIGHT_MAIN);
	app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_HEIGHT_LITTLE);

#endif

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_destroy(const char* widgetname, void *usrdata)
{
#if (GAME_ENABLE > 0)
	if (tetrisTimer != NULL) 
	{
		remove_timer(tetrisTimer);
		tetrisTimer = NULL;
		gameStatus = GAMEQUIT;
	}
#endif	
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_lost_focus(GuiWidget *widget, void *usrdata)
{
#if (GAME_ENABLE > 0)
	if(gameStatus != GAMEQUIT)
	{
		gameStatus = GAMEPAUSE;
		GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
		GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
		timer_stop(tetrisTimer);
	}
#endif
	return EVENT_TRANSFER_STOP;
}

int app_tetris_get_focus()
{
#if (GAME_ENABLE > 0)
	char str[STRLEN] = {0};
	int i=0;
	
		app_game_tetris_number2string(str,STRLEN,RussionGrade);
		GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);

		app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
		GUI_SetProperty(TXT_TETRIS_LINE, "string",str);

		app_game_tetris_number2string(str,STRLEN,totalScore);
		GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);
		
		GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
		GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
		GUI_SetInterface("flush",NULL);
		app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_HEIGHT_LITTLE);
		app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0,0,CANVAS_WIDTH_MAIN,CANVAS_HEIGHT_MAIN);
		app_game_russion_draw_block(block);
		for(i=0;i<4;i++)
		{
			switch(Sel)
			{
				case 0:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_RED,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 1:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_YELLOW,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 2:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BULE,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 3:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_GREEN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 4:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_BROWN,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 5:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_PINK,(blockN[i][0]-5)*CellSize + X_OFFSET,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				case 6:
					app_draw_little_canvas_image(IMG_TETRIS_DIAMOND_ORANGE,(blockN[i][0]-5)*CellSize + X_OFFSET+10,(blockN[i][1]+1)*CellSize + X_OFFSET);
					break;
				default:
					break;
				}
			}
		app_game_russion_paint();
#endif	
	return 0;
}

SIGNAL_HANDLER int app_tetris_keypress(const char* widgetname, void *usrdata)
{
#if (GAME_ENABLE > 0)
	GUI_Event *event = NULL;
	char str[STRLEN] = {0};

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
#if (DLNA_SUPPORT > 0)
            case VK_DLNA_TRIGGER:
                app_dlna_ui_exec(NULL, NULL);
                break;
#endif
			case VK_BOOK_TRIGGER:
				app_system_reset_unfocus_image();
				GUI_EndDialog("after wnd_full_screen");
				break;

			case STBK_MENU:
			case STBK_EXIT:
				gameStatus = GAMEQUIT;
				timer_stop(tetrisTimer);
				remove_timer(tetrisTimer);
				tetrisTimer = NULL;
				GUI_EndDialog(WND_TETRIS);
				break;
			/*case STBK_PAUSE:
				gameStatus = GAMEPAUSE;
				GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
				timer_stop(tetrisTimer);
				break;*/
			case STBK_UP:
			case STBK_DOWN:
			case STBK_LEFT:
			case STBK_RIGHT:
			{
				if(GAMESTART == gameStatus)
				{
					app_game_russion_move(event->key.sym);
				}
				break;
			}
			case STBK_OK:
			{
				if(GAMEQUIT == gameStatus)
				{
					int interval_timer = INIT_TIMER_VALUE;
					gameStatus = GAMESTART;

					app_game_russion_Datainit();

					app_game_tetris_number2string(str,STRLEN,RussionGrade);
					GUI_SetProperty(TXT_TETRIS_LEVEL, "string",str);

					app_game_tetris_number2string(str,STRLEN,RussionCurScoreline);
					GUI_SetProperty(TXT_TETRIS_LINE, "string",str);
	
					app_game_tetris_number2string(str,STRLEN,totalScore);
					GUI_SetProperty(TXT_TETRIS_SCORE, "string",str);
					
					app_tetris_canvas_clear(CANVAS_TETRIS_LITTER,0,0,CANVAS_WIDTH_LITTLE,CANVAS_HEIGHT_LITTLE);
					app_tetris_canvas_clear(CANVAS_TETRIS_MAIN,0,0,CANVAS_WIDTH_MAIN,CANVAS_HEIGHT_MAIN);
					app_game_russion_next_block();
					app_game_russion_new_block();
					GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_START);
					GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_PAUSE);

					interval_timer = interval_timer/(RussionGrade+1);
					tetrisTimer = create_timer(app_game_tetris_timer_handle, interval_timer, NULL, TIMER_REPEAT);			
				}
				else if(GAMEPAUSE == gameStatus)
				{
					gameStatus = GAMESTART;
					reset_timer(tetrisTimer);
					GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_START);
					GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_PAUSE);
				}
				else if(GAMESTART == gameStatus)
				{
					gameStatus = GAMEPAUSE;
					GUI_SetProperty(BTN_TETRIS_STATUS, "string",(void*)STR_ID_PAUSE);
					GUI_SetProperty(TXT_TETRIS_OK, "string",(void*)STR_ID_START);
					timer_stop(tetrisTimer);
				}
				break;
			}
			case STBK_0:
				RussionGrade = 0;
				app_game_handle_grade();
				break;
			case STBK_1:
				RussionGrade = 1;
				app_game_handle_grade();
				break;
			case STBK_2:
				RussionGrade = 2;
				app_game_handle_grade();
				break;
			case STBK_3:
				RussionGrade = 3;
				app_game_handle_grade();
				break;
			case STBK_4:
				RussionGrade = 4;
				app_game_handle_grade();
				break;
			case STBK_5:
				RussionGrade = 5;
				app_game_handle_grade();
				break;
			case STBK_6:
				RussionGrade = 6;
				app_game_handle_grade();
				break;
			case STBK_7:
				RussionGrade = 7;
				app_game_handle_grade();
				break;
			case STBK_8:
				RussionGrade = 8;
				app_game_handle_grade();
				break;
			case STBK_9:
				RussionGrade = 9;
				app_game_handle_grade();
				break;
			default:
				break;
		}		
	}
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_main_canvas_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tetris_little_canvas_create(const char* widgetname, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
//* ------------------------------- End of file ---------------------------- */
