#include "camera.h"

Round rd;
#define Search_Stop_Line 50
//加权控制
const uint8 Weight[MT9V03X_H]=
{
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //图像最远端00 ——09 行权重
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //图像最远端10 ——19 行权重
        1, 1, 1, 1, 1, 1, 1, 3, 4, 5,              //图像最远端20 ——29 行权重
        6, 7, 9,11,13,15,17,19,20,20,              //图像最远端30 ——39 行权重
       19,17,15,13,11, 9, 7, 5, 3, 1,              //图像最远端40 ——49 行权重
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //图像最远端50 ——59 行权重
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //图像最远端60 ——69 行权重
};
float Err_Sum(void)
{
    int i;
    float err=0;
    float weight_count=0;
    //常规误差
    for(i=MT9V03X_H-1;i>=MT9V03X_H-Search_Stop_Line-1;i--)//常规误差计算
    {
        err+=(MT9V03X_W/2-((l_border[i]+r_border[i])>>1))*Weight[i];//右移1位，等效除2
        weight_count+=Weight[i];
    }
    err=err/weight_count;
    return err;
}
//环岛相关参数的初始化
void Round_init()
{
    rd.Inflection_point_x=1;
    rd.Inflection_point_y=1;
    rd.Right_Lost_Time=0;
    rd.Left_Lost_Time=0;
    rd.Ring_Flag=0;
    rd.state=0;
    rd.Ring_time=0;
    rd.count=0;
    rd.Ring_Start=0;
    rd.Ring_Start_L=0;
    rd.Ring_Start_R=0;
    rd.No_Ring_Flag=0;
    rd.L_Edgepoint_x=0;
    rd.L_Edgepoint_y=0;
    rd.R_Edgepoint_x=0;
    rd.R_Edgepoint_y=0;

}
//获取左右边界的丢线次数
void GetLostTime()
{
    uint16 i;
    uint16 l_lost=0;
    uint16 r_lost=0;
    for(i=MT9V03X_H-2;i>10;i--)//从下往上
    {
        if(l_border[i]==border_min)
        {
            l_lost++;
        }
        else if(r_border[i]==border_max)
        {
            r_lost++;
        }
    }
    rd.Left_Lost_Time=l_lost;
    rd.Right_Lost_Time=r_lost;
}
//圆环状态判断，即1状态判断
int Ring_Start_Test()
{
    if(rd.Left_Lost_Time>rd.Right_Lost_Time&&
        abs(rd.Left_Lost_Time-rd.Right_Lost_Time)>20&&
        (rd.Left_Lost_Time+rd.Right_Lost_Time)<190&&
        rd.L_Edgepoint_y>114&&rd.R_Edgepoint_y>114&&
        Continuity_Change_Right(119,51)==0&&Continuity_Change_Left(119,51)!=0&&
        Monotonicity_Change_Left(80,20)!=0&&Monotonicity_Change_Right(80,20)==0&&
        Find_Left_Down_Point(115,60)
        )
    {
        return 1;
    }
    else if(rd.Left_Lost_Time<rd.Right_Lost_Time&&
            abs(rd.Left_Lost_Time-rd.Right_Lost_Time)>25&&
            (rd.Left_Lost_Time+rd.Right_Lost_Time)<190&&
            rd.L_Edgepoint_y>114&&rd.R_Edgepoint_y>114&&
            Continuity_Change_Right(119,51)!=0&&Continuity_Change_Left(119,51)==0&&
            Monotonicity_Change_Left(80,20)==0&&Monotonicity_Change_Right(80,20)!=0&&
            Find_Right_Down_Point(115,60)
            )
        {
            return 2;
        }
    else
        return 0;

}
//边界起始点的获取
void Get_Edge_Point()
{
    uint16 i;
    uint8 l_flag=0;
    uint8 r_flag=0;
    for(i=MT9V03X_H-1;i>10;i--)//从下往上
    {
        if (l_border[i]!=border_min&&l_flag==0) {
           l_flag=1;
           rd.L_Edgepoint_x=l_border[i];
           rd.L_Edgepoint_y=i;
        }
        if (r_border[i]!=border_max&&r_flag==0) {
                   r_flag=1;
                   rd.R_Edgepoint_x=r_border[i];
                   rd.R_Edgepoint_y=i;
                }
        if(l_flag==1&&r_flag==1)
            return;

    }
}
//圆环搜索
void Ring_Search()
{
    rd.Ring_Start=Ring_Start_Test();
    if(rd.Ring_Start==1&&rd.Ring_Start_R==0)
    {
        rd.Ring_Start_L++;
    }
    else if(rd.Ring_Start==2&&rd.Ring_Start_L==0)
    {
        rd.Ring_Start_R++;
    }
    else
    {
        if(rd.Ring_Start_L!=0)//当标志未连续识别到时，减少标志位，防止误判
            rd.Ring_Start_L--;
        if(rd.Ring_Start_R!=0)
            rd.Ring_Start_R--;
     }

     if(rd.Ring_Start_L>=3)//连续三幅图左环标志，进左环处理，阈值可调
           rd.Ring_Flag=1;
     else if(rd.Ring_Start_R>=3)//连续三幅图右环标志，进右环处理
           rd.Ring_Flag=2;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     右下角点检测
  @param     起始点，终止点
  @return    返回角点所在的行数，找不到返回0
  Sample     Find_Right_Down_Point(int start,int end);
  @note      角点检测阈值可根据实际值更改
-------------------------------------------------------------------------------------------------------------------*/
int Find_Right_Down_Point(int start,int end)//找四个角点，返回值是角点所在的行数
{
    int i,t;
    int right_down_line=0;
    if(rd.Right_Lost_Time>=0.9*MT9V03X_H)//大部分都丢线，没有拐点判断的意义
        return right_down_line;
    if(start<end)
    {
        t=start;
        start=end;
        end=t;
    }
    if(start>=MT9V03X_H-1-5)//下面5行数据不稳定，不能作为边界点来判断，舍弃
        start=MT9V03X_H-1-5;
    if(end<=MT9V03X_H-Search_Stop_Line)
        end=MT9V03X_H-Search_Stop_Line;
    if(end<=5)
       end=5;
    for(i=start;i>=end;i--)
    {
        if(right_down_line==0&&//只找第一个符合条件的点
           abs(r_border[i]-r_border[i+1])<=5&&//角点的阈值可以更改
           abs(r_border[i+1]-r_border[i+2])<=5&&
           abs(r_border[i+2]-r_border[i+3])<=5&&
              (r_border[i]-r_border[i-2])<=-5&&
              (r_border[i]-r_border[i-3])<=-10&&
              (r_border[i]-r_border[i-4])<=-10)
        {
            right_down_line=i;//获取行数即可
            break;
        }
    }
    return right_down_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     左下角点检测
  @param     起始点，终止点
  @return    返回角点所在的行数，找不到返回0
  Sample     Find_Left_Down_Point(int start,int end);
  @note      角点检测阈值可根据实际值更改
-------------------------------------------------------------------------------------------------------------------*/
int Find_Left_Down_Point(int start,int end)//找四个角点，返回值是角点所在的行数
{
    int i,t;
    int left_down_line=0;
    if(rd.Left_Lost_Time>=0.9*MT9V03X_H)//大部分都丢线，没有拐点判断的意义
        return left_down_line;
    if(start<end)
    {
        t=start;
        start=end;
        end=t;
    }
    if(start>=MT9V03X_H-1-5)//下面5行数据不稳定，不能作为边界点来判断，舍弃
        start=MT9V03X_H-1-5;
    if(end<=MT9V03X_H-Search_Stop_Line)
        end=MT9V03X_H-Search_Stop_Line;
    if(end<=5)
       end=5;
    for(i=start;i>=end;i--)
    {
        if(left_down_line==0&&//只找第一个符合条件的点
           abs(l_border[i]-l_border[i+1])<=5&&//角点的阈值可以更改
           abs(l_border[i+1]-l_border[i+2])<=5&&
           abs(l_border[i+2]-l_border[i+3])<=5&&
              (l_border[i]-l_border[i-2])<=-5&&
              (l_border[i]-l_border[i-3])<=-10&&
              (l_border[i]-l_border[i-4])<=-10)
        {
            left_down_line=i;//获取行数即可
            break;
        }
    }
    return left_down_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     右赛道连续性检测
  @param     起始点，终止点
  @return    连续返回0，不连续返回断线出行数
  Sample     continuity_change_flag=Continuity_Change_Right(int start,int end)
  @note      连续性的阈值设置为5，可更改
-------------------------------------------------------------------------------------------------------------------*/
int Continuity_Change_Right(int start,int end)
{
    int i;
    int t;
    int continuity_change_flag=0;
    if(rd.Right_Lost_Time>=0.9*MT9V03X_H)//大部分都丢线，没必要判断了
       return 1;
    if(start>=MT9V03X_H-5)//数组越界保护
        start=MT9V03X_H-5;
    if(end<=5)
       end=5;
    if(start<end)//都是从下往上计算的，反了就互换一下
    {
       t=start;
       start=end;
       end=t;
    }

    for(i=start;i>=end;i--)
    {
        if(abs(r_border[i]-r_border[i-1])>=5)//连续性阈值是5，可更改
       {
            continuity_change_flag=i;
            break;
       }
    }
    return continuity_change_flag;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     左赛道连续性检测
  @param     起始点，终止点
  @return    连续返回0，不连续返回断线出行数
  Sample     continuity_change_flag=Continuity_Change_Left(int start,int end)
  @note      连续性的阈值设置为5，可更改
-------------------------------------------------------------------------------------------------------------------*/
int Continuity_Change_Left(int start,int end)
{
    int i;
    int t;
    int continuity_change_flag=0;
    if(rd.Left_Lost_Time>=0.9*MT9V03X_H)//大部分都丢线，没必要判断了
       return 1;
    if(start>=MT9V03X_H-5)//数组越界保护
        start=MT9V03X_H-5;
    if(end<=5)
       end=5;
    if(start<end)//都是从下往上计算的，反了就互换一下
    {
       t=start;
       start=end;
       end=t;
    }

    for(i=start;i>=end;i--)
    {
        if(abs(l_border[i]-l_border[i-1])>=5)//连续性阈值是5，可更改
       {
            continuity_change_flag=i;
            break;
       }
    }
    return continuity_change_flag;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     单调性突变检测-右边
  @param     起始点，终止行
  @return    点所在的行数，找不到返回0
  Sample     Find_Right_Up_Point(int start,int end);
  @note      前5后5它最大（最小），那他就是角点
-------------------------------------------------------------------------------------------------------------------*/
int Monotonicity_Change_Right(int start,int end)//单调性改变，返回值是单调性改变点所在的行数
{
    int i;
    int monotonicity_change_line=0;

    if(rd.Right_Lost_Time>=0.9*MT9V03X_H)//大部分都丢线，没有单调性判断的意义
        return monotonicity_change_line;
    if(start>=MT9V03X_H-1-5)//数组越界保护
        start=MT9V03X_H-1-5;
     if(end<=5)
        end=5;
    if(start<=end)
        return monotonicity_change_line;
    for(i=start;i>=end;i--)//会读取前5后5数据，所以前面对输入范围有要求
    {
        if(r_border[i]==r_border[i+5]&&r_border[i]==r_border[i-5]&&
        r_border[i]==r_border[i+4]&&r_border[i]==r_border[i-4]&&
        r_border[i]==r_border[i+3]&&r_border[i]==r_border[i-3]&&
        r_border[i]==r_border[i+2]&&r_border[i]==r_border[i-2]&&
        r_border[i]==r_border[i+1]&&r_border[i]==r_border[i-1])
        {//一堆数据一样，显然不能作为单调转折点
            continue;
        }
        else if(r_border[i] <r_border[i+5]&&r_border[i] <r_border[i-5]&&
        r_border[i] <r_border[i+4]&&r_border[i] <r_border[i-4]&&
        r_border[i]<=r_border[i+3]&&r_border[i]<=r_border[i-3]&&
        r_border[i]<=r_border[i+2]&&r_border[i]<=r_border[i-2]&&
        r_border[i]<=r_border[i+1]&&r_border[i]<=r_border[i-1])
        {//就很暴力，这个数据是在前5，后5中最大的，那就是单调突变点
            monotonicity_change_line=i;
            break;
        }
    }
    return monotonicity_change_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     单调性突变检测-左边
  @param     起始点，终止行
  @return    点所在的行数，找不到返回0
  Sample     Find_Right_Up_Point(int start,int end);
  @note      前5后5它最大（最小），那他就是角点
-------------------------------------------------------------------------------------------------------------------*/
int Monotonicity_Change_Left(int start,int end)//单调性改变，返回值是单调性改变点所在的行数
{
    int i;
    int monotonicity_change_line=0;

    if(rd.Left_Lost_Time>=0.9*MT9V03X_H)//大部分都丢线，没有单调性判断的意义
        return monotonicity_change_line;
    if(start>=MT9V03X_H-1-5)//数组越界保护
        start=MT9V03X_H-1-5;
     if(end<=5)
        end=5;
    if(start<=end)
        return monotonicity_change_line;
    for(i=start;i>=end;i--)//会读取前5后5数据，所以前面对输入范围有要求
    {
        if(l_border[i]==l_border[i+5]&&l_border[i]==l_border[i-5]&&
        l_border[i]==l_border[i+4]&&l_border[i]==l_border[i-4]&&
        l_border[i]==l_border[i+3]&&l_border[i]==l_border[i-3]&&
        l_border[i]==l_border[i+2]&&l_border[i]==l_border[i-2]&&
        l_border[i]==l_border[i+1]&&l_border[i]==l_border[i-1])
        {//一堆数据一样，显然不能作为单调转折点
            continue;
        }
        else if(l_border[i] >l_border[i+5]&&l_border[i] >l_border[i-5]&&
        l_border[i] >l_border[i+4]&&l_border[i] >l_border[i-4]&&
        l_border[i]>=l_border[i+3]&&l_border[i]>=l_border[i-3]&&
        l_border[i]>=l_border[i+2]&&l_border[i]>=l_border[i-2]&&
        l_border[i]>=l_border[i+1]&&l_border[i]>=l_border[i-1])
        {//就很暴力，这个数据是在前5，后5中最大的，那就是单调突变点
            monotonicity_change_line=i;
            break;
        }
    }
    return monotonicity_change_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     通过斜率，定点补线--补左线
  @param     k       输入斜率
             startY  输入起始点纵坐标
             endY    结束点纵坐标
  @return    null
  Sample     K_Add_Boundry_Left(float k,int startY,int endY);
  @note      补得线直接贴在边线上
-------------------------------------------------------------------------------------------------------------------*/
void K_Add_Boundry_Left(float k,int startX,int startY,int endY)
{
    int i,t;
    if(startY>=MT9V03X_H-1)
        startY=MT9V03X_H-1;
    else if(startY<=0)
        startY=0;
    if(endY>=MT9V03X_H-1)
        endY=MT9V03X_H-1;
    else if(endY<=0)
        endY=0;
    if(startY<endY)//--操作，start需要大
    {
        t=startY;
        startY=endY;
        endY=t;
    }
//这里有bug，下方循环--循环，需要start要大，只进行y的互换，但是没有进行x的互换
//建议进行判断，如果start更小，那就进行++访问
//这里修改各位自行操作
    for(i=startY;i>=endY;i--)
    {
        l_border[i]=(int)((i-startY)/k+startX);//(y-y1)=k(x-x1)变形，x=(y-y1)/k+x1
        if(l_border[i]>=MT9V03X_W-1)
        {
            l_border[i]=MT9V03X_W-1;
        }
        else if(l_border[i]<=0)
        {
            l_border[i]=0;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------
  @brief     通过斜率，定点补线--补右线
  @param     k       输入斜率的相反数
             startY  输入起始点纵坐标
             endY    结束点纵坐标
  @return    null
  Sample     K_Add_Boundry_Left(float k,int startY,int endY);
  @note      补得线直接贴在边线上
-------------------------------------------------------------------------------------------------------------------*/
void K_Add_Boundry_Right(float k,int startX,int startY,int endY)
{
    int i,t;
    if(startY>=MT9V03X_H-1)
        startY=MT9V03X_H-1;
    else if(startY<=0)
        startY=0;
    if(endY>=MT9V03X_H-1)
        endY=MT9V03X_H-1;
    else if(endY<=0)
        endY=0;
    if(startY<endY)//--操作，start需要大
    {
        t=startY;
        startY=endY;
        endY=t;
    }
//这里有bug，下方循环--循环，需要start要大，只进行y的互换，但是没有进行x的互换
//建议进行判断，如果start更小，那就进行++访问
//这里修改各位自行操作
    for(i=startY;i>=endY;i--)
    {
        r_border[i]=(int)((startY-i)/k+startX);//(y-y1)=k(x-x1)变形，x=(y-y1)/k+x1
        if(r_border[i]>=MT9V03X_W-1)
        {
            r_border[i]=MT9V03X_W-1;
        }
        else if(r_border[i]<=0)
        {
            r_border[i]=0;
        }
    }
}
// 假设这些变量在某处已经定义
uint8_t Pixle[35][100]; // 实际的二维像素数组定义
uint8_t garageout_flag = 0; // 初始化标志

uint8_t region = 0;
uint8_t garage_count = 0;
uint8_t white_black = 0;
uint8_t black_white = 0;
void crosswalk(void) {

    for (uint8_t hang = 20; hang < 35; hang++) {
        garage_count = 0;
        black_white = !white_black; // 重置，以便每次新行开始时触发变化检测
        for (uint8_t lie = 10; lie < 100; lie++) {
            white_black = (bin_image[hang][lie] == white_pixel) ? 1 : 0;

            if (white_black != black_white) {
                black_white = white_black;
                garage_count++;
            }
            if (garage_count > 11) {
                region++;
                break; // 一旦在一行中检测到足够的变化，即跳出循环
            }
        }
        if (region > 2) {
            garageout_flag = 1;
            break; // 在足够多的行中检测到变化后退出
        }
    }
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     根据斜率划线
  @param     输入斜率，定点，画一条黑线
  @return    null
  Sample     K_Draw_Line(k, 20,MT9V03X_H-1 ,0)
  @note      补的就是一条线，需要重新扫线
-------------------------------------------------------------------------------------------------------------------*/
void K_Draw_Line(float k, int startX, int startY,int endY)
{
    int endX=0;

    if(startX>=MT9V03X_W-1)//限幅处理
        startX=MT9V03X_W-1;
    else if(startX<=0)
        startX=0;
    if(startY>=MT9V03X_H-1)
        startY=MT9V03X_H-1;
    else if(startY<=0)
        startY=0;
    if(endY>=MT9V03X_H-1)
        endY=MT9V03X_H-1;
    else if(endY<=0)
        endY=0;
    endX=(int)((endY-startY)/k+startX);//(y-y1)=k(x-x1)变形，x=(y-y1)/k+x1
    Draw_Line(startX,startY,endX,endY);
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     画线
  @param     输入起始点，终点坐标，补一条宽度为2的黑线
  @return    null
  Sample     Draw_Line(0, 0,MT9V03X_W-1,MT9V03X_H-1);
             Draw_Line(MT9V03X_W-1, 0,0,MT9V03X_H-1);
                                    画一个大×
  @note     补的就是一条线，需要重新扫线
-------------------------------------------------------------------------------------------------------------------*/
void Draw_Line(int startX, int startY, int endX, int endY)
{
    int i,x,y;
    int start=0,end=0;
    if(startX>=MT9V03X_W-1)//限幅处理
        startX=MT9V03X_W-1;
    else if(startX<=0)
        startX=0;
    if(startY>=MT9V03X_H-1)
        startY=MT9V03X_H-1;
    else if(startY<=0)
        startY=0;
    if(endX>=MT9V03X_W-1)
        endX=MT9V03X_W-1;
    else if(endX<=0)
        endX=0;
    if(endY>=MT9V03X_H-1)
        endY=MT9V03X_H-1;
    else if(endY<=0)
        endY=0;
    if(startX==endX)//一条竖线
    {
        if (startY > endY)//互换
        {
            start=endY;
            end=startY;
        }
        for (i = start; i <= end; i++)
        {
            if(i<=1)
                i=1;
            bin_image[i][startX]=black_pixel;
            bin_image[i-1][startX]=black_pixel;
        }
    }
    else if(startY == endY)//补一条横线
    {
        if (startX > endX)//互换
        {
            start=endX;
            end=startX;
        }
        for (i = start; i <= end; i++)
        {
            if(startY<=1)
                startY=1;
            bin_image[startY][i]=black_pixel;
            bin_image[startY-1][i]=black_pixel;
        }
    }
    else //上面两个是水平，竖直特殊情况，下面是常见情况
    {
        if(startY>endY)//起始点矫正
        {
            start=endY;
            end=startY;
        }
        else
        {
            start=startY;
            end=endY;
        }
        for (i = start; i <= end; i++)//纵向补线，保证每一行都有黑点
        {
            x =(int)(startX+(endX-startX)*(i-startY)/(endY-startY));//两点式变形
            if(x>=MT9V03X_W-1)
                x=MT9V03X_W-1;
            else if (x<=1)
                x=1;
            bin_image[i][x] = black_pixel;
            bin_image[i][x-1] = black_pixel;
        }
        if(startX>endX)
        {
            start=endX;
            end=startX;
        }
        else
        {
            start=startX;
            end=endX;
        }
        for (i = start; i <= end; i++)//横向补线，保证每一列都有黑点
        {

            y =(int)(startY+(endY-startY)*(i-startX)/(endX-startX));//两点式变形
            if(y>=MT9V03X_H-1)
                y=MT9V03X_H-1;
            else if (y<=0)
                y=0;
            bin_image[y][i] = black_pixel;
        }
    }
}
