#include "camera.h"

Round rd;
#define Search_Stop_Line 50
//��Ȩ����
const uint8 Weight[MT9V03X_H]=
{
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //ͼ����Զ��00 ����09 ��Ȩ��
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //ͼ����Զ��10 ����19 ��Ȩ��
        1, 1, 1, 1, 1, 1, 1, 3, 4, 5,              //ͼ����Զ��20 ����29 ��Ȩ��
        6, 7, 9,11,13,15,17,19,20,20,              //ͼ����Զ��30 ����39 ��Ȩ��
       19,17,15,13,11, 9, 7, 5, 3, 1,              //ͼ����Զ��40 ����49 ��Ȩ��
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //ͼ����Զ��50 ����59 ��Ȩ��
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,              //ͼ����Զ��60 ����69 ��Ȩ��
};
float Err_Sum(void)
{
    int i;
    float err=0;
    float weight_count=0;
    //�������
    for(i=MT9V03X_H-1;i>=MT9V03X_H-Search_Stop_Line-1;i--)//����������
    {
        err+=(MT9V03X_W/2-((l_border[i]+r_border[i])>>1))*Weight[i];//����1λ����Ч��2
        weight_count+=Weight[i];
    }
    err=err/weight_count;
    return err;
}
//������ز����ĳ�ʼ��
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

//��ȡ���ұ߽�Ķ��ߴ���
void GetLostTime()
{
    uint16 i;
    uint16 l_lost=0;
    uint16 r_lost=0;
    for(i=MT9V03X_H-2;i>10;i--)//��������
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

//Բ��״̬�жϣ���1״̬�ж�
//ͼ��̫�����ˣ���ȷ����ͼ�񣬸���ͼ���һ������
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
//�߽���ʼ��Ļ�ȡ
void Get_Edge_Point()
{
    uint16 i;
    uint8 l_flag=0;
    uint8 r_flag=0;
    for(i=MT9V03X_H-1;i>10;i--)//��������
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
//Բ������-����������n��ͼ�����Բ���ͽ�Բ��״̬����n����ʵ�������
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
        if(rd.Ring_Start_L!=0)//����־δ����ʶ��ʱ�����ٱ�־λ����ֹ����
            rd.Ring_Start_L--;
        if(rd.Ring_Start_R!=0)
            rd.Ring_Start_R--;
     }

     if(rd.Ring_Start_L>=3)//��������ͼ�󻷱�־�����󻷴�����ֵ�ɵ�
           rd.Ring_Flag=1;
     else if(rd.Ring_Start_R>=3)//��������ͼ�һ���־�����һ�����
           rd.Ring_Flag=2;
}
//��״̬��
void Left_Ring()
{
    rd.state=1;
    int16 L1,L2;
    //L1=l_border[Continuity_Change_Left(80,30)];
    if(rd.state==1)
    {

        if(Find_Right_Down_Point(118,60)==0)
        {
            rd.state=2;
        }
    }

}
//�һ�״̬��
void Right_Ring()
{
    rd.state=1;
    float k=0;
    int16 L1;
    int16 L2;
    if(rd.state==1)
    {
        L1=Find_Right_Down_Point(119,70);//�ǵ�
        L2=Monotonicity_Change_Right(70,25);//����ͻ���
        Draw_Line(r_border[L1], L1, r_border[L2], L2);
        if(k==0)
        {
            k=(188-rd.R_Edgepoint_x)/(188-r_border[L2]);
        }
        image_process();
        if(Find_Right_Down_Point(118,60)==0)
            rd.state=2;
    }
    else if(rd.state==2)
    {
        L2=Monotonicity_Change_Right(80,25);
        L1=188-(k*(188-r_border[L2]));
        Draw_Line(r_border[L1], L1, r_border[L2], L2);
        image_process();
        if(rd.R_Edgepoint_y>117||Monotonicity_Change_Right(70,25)>50)
            rd.state=3;
    }
    else if(rd.state==3)
    {
        L1=Continuity_Change_Right(110,30);
        Draw_Line(rd.L_Edgepoint_x, rd.L_Edgepoint_y, r_border[L1], L1);
        image_process();
        if(Continuity_Change_Left(110,50)==0&&Continuity_Change_Right(110,50)==0)
            rd.state=4;
    }
    else if(rd.state==4)
    {
        if(Continuity_Change_Left(110,40)!=0)
            rd.state=5;
    }
    else if(rd.state==5)
    {

    }
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ���½ǵ���
  @param     ��ʼ�㣬��ֹ��
  @return    ���ؽǵ����ڵ��������Ҳ�������0
  Sample     Find_Right_Down_Point(int start,int end);
  @note      �ǵ�����ֵ�ɸ���ʵ��ֵ����
-------------------------------------------------------------------------------------------------------------------*/
int Find_Right_Down_Point(int start,int end)//���ĸ��ǵ㣬����ֵ�ǽǵ����ڵ�����
{
    int i,t;
    int right_down_line=0;
    if(rd.Right_Lost_Time>=0.9*MT9V03X_H)//�󲿷ֶ����ߣ�û�йյ��жϵ�����
        return right_down_line;
    if(start<end)
    {
        t=start;
        start=end;
        end=t;
    }
    if(start>=MT9V03X_H-1-5)//����5�����ݲ��ȶ���������Ϊ�߽�����жϣ�����
        start=MT9V03X_H-1-5;
    if(end<=MT9V03X_H-Search_Stop_Line)
        end=MT9V03X_H-Search_Stop_Line;
    if(end<=5)
       end=5;
    for(i=start;i>=end;i--)
    {
        if(right_down_line==0&&//ֻ�ҵ�һ�����������ĵ�
           abs(r_border[i]-r_border[i+1])<=5&&//�ǵ����ֵ���Ը���
           abs(r_border[i+1]-r_border[i+2])<=5&&
           abs(r_border[i+2]-r_border[i+3])<=5&&
              (r_border[i]-r_border[i-2])<=-5&&
              (r_border[i]-r_border[i-3])<=-10&&
              (r_border[i]-r_border[i-4])<=-10)
        {
            right_down_line=i;//��ȡ��������
            break;
        }
    }
    return right_down_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ���½ǵ���
  @param     ��ʼ�㣬��ֹ��
  @return    ���ؽǵ����ڵ��������Ҳ�������0
  Sample     Find_Left_Down_Point(int start,int end);
  @note      �ǵ�����ֵ�ɸ���ʵ��ֵ����
-------------------------------------------------------------------------------------------------------------------*/
int Find_Left_Down_Point(int start,int end)//���ĸ��ǵ㣬����ֵ�ǽǵ����ڵ�����
{
    int i,t;
    int left_down_line=0;
    if(rd.Left_Lost_Time>=0.9*MT9V03X_H)//�󲿷ֶ����ߣ�û�йյ��жϵ�����
        return left_down_line;
    if(start<end)
    {
        t=start;
        start=end;
        end=t;
    }
    if(start>=MT9V03X_H-1-5)//����5�����ݲ��ȶ���������Ϊ�߽�����жϣ�����
        start=MT9V03X_H-1-5;
    if(end<=MT9V03X_H-Search_Stop_Line)
        end=MT9V03X_H-Search_Stop_Line;
    if(end<=5)
       end=5;
    for(i=start;i>=end;i--)
    {
        if(left_down_line==0&&//ֻ�ҵ�һ�����������ĵ�
           abs(l_border[i]-l_border[i+1])<=5&&//�ǵ����ֵ���Ը���
           abs(l_border[i+1]-l_border[i+2])<=5&&
           abs(l_border[i+2]-l_border[i+3])<=5&&
              (l_border[i]-l_border[i-2])<=-5&&
              (l_border[i]-l_border[i-3])<=-10&&
              (l_border[i]-l_border[i-4])<=-10)
        {
            left_down_line=i;//��ȡ��������
            break;
        }
    }
    return left_down_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     �����������Լ��
  @param     ��ʼ�㣬��ֹ��
  @return    ��������0�����������ض��߳�����
  Sample     continuity_change_flag=Continuity_Change_Right(int start,int end)
  @note      �����Ե���ֵ����Ϊ5���ɸ���
-------------------------------------------------------------------------------------------------------------------*/
int Continuity_Change_Right(int start,int end)
{
    int i;
    int t;
    int continuity_change_flag=0;
    if(rd.Right_Lost_Time>=0.9*MT9V03X_H)//�󲿷ֶ����ߣ�û��Ҫ�ж���
       return 1;
    if(start>=MT9V03X_H-5)//����Խ�籣��
        start=MT9V03X_H-5;
    if(end<=5)
       end=5;
    if(start<end)//���Ǵ������ϼ���ģ����˾ͻ���һ��
    {
       t=start;
       start=end;
       end=t;
    }

    for(i=start;i>=end;i--)
    {
        if(abs(r_border[i]-r_border[i-1])>=5)//��������ֵ��5���ɸ���
       {
            continuity_change_flag=i;
            break;
       }
    }
    return continuity_change_flag;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     �����������Լ��
  @param     ��ʼ�㣬��ֹ��
  @return    ��������0�����������ض��߳�����
  Sample     continuity_change_flag=Continuity_Change_Left(int start,int end)
  @note      �����Ե���ֵ����Ϊ5���ɸ���
-------------------------------------------------------------------------------------------------------------------*/
int Continuity_Change_Left(int start,int end)
{
    int i;
    int t;
    int continuity_change_flag=0;
    if(rd.Left_Lost_Time>=0.9*MT9V03X_H)//�󲿷ֶ����ߣ�û��Ҫ�ж���
       return 1;
    if(start>=MT9V03X_H-5)//����Խ�籣��
        start=MT9V03X_H-5;
    if(end<=5)
       end=5;
    if(start<end)//���Ǵ������ϼ���ģ����˾ͻ���һ��
    {
       t=start;
       start=end;
       end=t;
    }

    for(i=start;i>=end;i--)
    {
        if(abs(l_border[i]-l_border[i-1])>=5)//��������ֵ��5���ɸ���
       {
            continuity_change_flag=i;
            break;
       }
    }
    return continuity_change_flag;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ������ͻ����-�ұ�
  @param     ��ʼ�㣬��ֹ��
  @return    �����ڵ��������Ҳ�������0
  Sample     Find_Right_Up_Point(int start,int end);
  @note      ǰ5��5�������С�����������ǽǵ�
-------------------------------------------------------------------------------------------------------------------*/
int Monotonicity_Change_Right(int start,int end)//�����Ըı䣬����ֵ�ǵ����Ըı�����ڵ�����
{
    int i;
    int monotonicity_change_line=0;

    if(rd.Right_Lost_Time>=0.9*MT9V03X_H)//�󲿷ֶ����ߣ�û�е������жϵ�����
        return monotonicity_change_line;
    if(start>=MT9V03X_H-1-5)//����Խ�籣��
        start=MT9V03X_H-1-5;
     if(end<=5)
        end=5;
    if(start<=end)
        return monotonicity_change_line;
    for(i=start;i>=end;i--)//���ȡǰ5��5���ݣ�����ǰ������뷶Χ��Ҫ��
    {
        if(r_border[i]==r_border[i+5]&&r_border[i]==r_border[i-5]&&
        r_border[i]==r_border[i+4]&&r_border[i]==r_border[i-4]&&
        r_border[i]==r_border[i+3]&&r_border[i]==r_border[i-3]&&
        r_border[i]==r_border[i+2]&&r_border[i]==r_border[i-2]&&
        r_border[i]==r_border[i+1]&&r_border[i]==r_border[i-1])
        {//һ������һ������Ȼ������Ϊ����ת�۵�
            continue;
        }
        else if(r_border[i] <r_border[i+5]&&r_border[i] <r_border[i-5]&&
        r_border[i] <r_border[i+4]&&r_border[i] <r_border[i-4]&&
        r_border[i]<=r_border[i+3]&&r_border[i]<=r_border[i-3]&&
        r_border[i]<=r_border[i+2]&&r_border[i]<=r_border[i-2]&&
        r_border[i]<=r_border[i+1]&&r_border[i]<=r_border[i-1])
        {//�ͺܱ����������������ǰ5����5�����ģ��Ǿ��ǵ���ͻ���
            monotonicity_change_line=i;
            break;
        }
    }
    return monotonicity_change_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ������ͻ����-���
  @param     ��ʼ�㣬��ֹ��
  @return    �����ڵ��������Ҳ�������0
  Sample     Find_Right_Up_Point(int start,int end);
  @note      ǰ5��5�������С�����������ǽǵ�
-------------------------------------------------------------------------------------------------------------------*/
int Monotonicity_Change_Left(int start,int end)//�����Ըı䣬����ֵ�ǵ����Ըı�����ڵ�����
{
    int i;
    int monotonicity_change_line=0;

    if(rd.Left_Lost_Time>=0.9*MT9V03X_H)//�󲿷ֶ����ߣ�û�е������жϵ�����
        return monotonicity_change_line;
    if(start>=MT9V03X_H-1-5)//����Խ�籣��
        start=MT9V03X_H-1-5;
     if(end<=5)
        end=5;
    if(start<=end)
        return monotonicity_change_line;
    for(i=start;i>=end;i--)//���ȡǰ5��5���ݣ�����ǰ������뷶Χ��Ҫ��
    {
        if(l_border[i]==l_border[i+5]&&l_border[i]==l_border[i-5]&&
        l_border[i]==l_border[i+4]&&l_border[i]==l_border[i-4]&&
        l_border[i]==l_border[i+3]&&l_border[i]==l_border[i-3]&&
        l_border[i]==l_border[i+2]&&l_border[i]==l_border[i-2]&&
        l_border[i]==l_border[i+1]&&l_border[i]==l_border[i-1])
        {//һ������һ������Ȼ������Ϊ����ת�۵�
            continue;
        }
        else if(l_border[i] >l_border[i+5]&&l_border[i] >l_border[i-5]&&
        l_border[i] >l_border[i+4]&&l_border[i] >l_border[i-4]&&
        l_border[i]>=l_border[i+3]&&l_border[i]>=l_border[i-3]&&
        l_border[i]>=l_border[i+2]&&l_border[i]>=l_border[i-2]&&
        l_border[i]>=l_border[i+1]&&l_border[i]>=l_border[i-1])
        {//�ͺܱ����������������ǰ5����5�����ģ��Ǿ��ǵ���ͻ���
            monotonicity_change_line=i;
            break;
        }
    }
    return monotonicity_change_line;
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ͨ��б�ʣ����㲹��--������
  @param     k       ����б��
             startY  ������ʼ��������
             endY    ������������
  @return    null
  Sample     K_Add_Boundry_Left(float k,int startY,int endY);
  @note      ������ֱ�����ڱ�����
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
    if(startY<endY)//--������start��Ҫ��
    {
        t=startY;
        startY=endY;
        endY=t;
    }
//������bug���·�ѭ��--ѭ������ҪstartҪ��ֻ����y�Ļ���������û�н���x�Ļ���
//��������жϣ����start��С���Ǿͽ���++����
//�����޸ĸ�λ���в���
    for(i=startY;i>=endY;i--)
    {
        l_border[i]=(int)((i-startY)/k+startX);//(y-y1)=k(x-x1)���Σ�x=(y-y1)/k+x1
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
  @brief     ͨ��б�ʣ����㲹��--������
  @param     k       ����б�ʵ��෴��
             startY  ������ʼ��������
             endY    ������������
  @return    null
  Sample     K_Add_Boundry_Left(float k,int startY,int endY);
  @note      ������ֱ�����ڱ�����
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
    if(startY<endY)//--������start��Ҫ��
    {
        t=startY;
        startY=endY;
        endY=t;
    }
//������bug���·�ѭ��--ѭ������ҪstartҪ��ֻ����y�Ļ���������û�н���x�Ļ���
//��������жϣ����start��С���Ǿͽ���++����
//�����޸ĸ�λ���в���
    for(i=startY;i>=endY;i--)
    {
        r_border[i]=(int)((startY-i)/k+startX);//(y-y1)=k(x-x1)���Σ�x=(y-y1)/k+x1
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
// ������Щ������ĳ���Ѿ�����
uint8_t Pixle[35][100]; // ʵ�ʵĶ�ά�������鶨��
uint8_t garageout_flag = 0; // ��ʼ����־

uint8_t region = 0;
uint8_t garage_count = 0;
uint8_t white_black = 0;
uint8_t black_white = 0;
void crosswalk(void) {

    for (uint8_t hang = 20; hang < 35; hang++) {
        garage_count = 0;
        black_white = !white_black; // ���ã��Ա�ÿ�����п�ʼʱ�����仯���
        for (uint8_t lie = 10; lie < 100; lie++) {
            white_black = (bin_image[hang][lie] == white_pixel) ? 1 : 0;

            if (white_black != black_white) {
                black_white = white_black;
                garage_count++;
            }
            if (garage_count > 11) {
                region++;
                break; // һ����һ���м�⵽�㹻�ı仯��������ѭ��
            }
        }
        if (region > 2) {
            garageout_flag = 1;
            break; // ���㹻������м�⵽�仯���˳�
        }
    }
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ����б�ʻ���
  @param     ����б�ʣ����㣬��һ������
  @return    null
  Sample     K_Draw_Line(k, 20,MT9V03X_H-1 ,0)
  @note      ���ľ���һ���ߣ���Ҫ����ɨ��
-------------------------------------------------------------------------------------------------------------------*/
void K_Draw_Line(float k, int startX, int startY,int endY)
{
    int endX=0;

    if(startX>=MT9V03X_W-1)//�޷�����
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
    endX=(int)((endY-startY)/k+startX);//(y-y1)=k(x-x1)���Σ�x=(y-y1)/k+x1
    Draw_Line(startX,startY,endX,endY);
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief     ����
  @param     ������ʼ�㣬�յ����꣬��һ�����Ϊ2�ĺ���
  @return    null
  Sample     Draw_Line(0, 0,MT9V03X_W-1,MT9V03X_H-1);
             Draw_Line(MT9V03X_W-1, 0,0,MT9V03X_H-1);
                                    ��һ�����
  @note     ���ľ���һ���ߣ���Ҫ����ɨ��
-------------------------------------------------------------------------------------------------------------------*/
void Draw_Line(int startX, int startY, int endX, int endY)
{
    int i,x,y;
    int start=0,end=0;
    if(startX>=MT9V03X_W-1)//�޷�����
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
    if(startX==endX)//һ������
    {
        if (startY > endY)//����
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
    else if(startY == endY)//��һ������
    {
        if (startX > endX)//����
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
    else //����������ˮƽ����ֱ��������������ǳ������
    {
        if(startY>endY)//��ʼ�����
        {
            start=endY;
            end=startY;
        }
        else
        {
            start=startY;
            end=endY;
        }
        for (i = start; i <= end; i++)//�����ߣ���֤ÿһ�ж��кڵ�
        {
            x =(int)(startX+(endX-startX)*(i-startY)/(endY-startY));//����ʽ����
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
        for (i = start; i <= end; i++)//�����ߣ���֤ÿһ�ж��кڵ�
        {

            y =(int)(startY+(endY-startY)*(i-startX)/(endX-startX));//����ʽ����
            if(y>=MT9V03X_H-1)
                y=MT9V03X_H-1;
            else if (y<=0)
                y=0;
            bin_image[y][i] = black_pixel;
        }
    }
}
