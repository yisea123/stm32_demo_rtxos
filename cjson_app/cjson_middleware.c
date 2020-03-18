#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include <stdio.h>
#include <time.h>

#include "project_config.h"
#include "cjson_middleware.h"
#include "mqtt_app.h"
#include "st_adc.h"
#include "st_rtc.h"
#include "cJSON.h"
#include "dianchuan.h"
#include "app4g.h"
#include "app_rs485_broker.h"

#include "ta6932.h"
#include "ds18b20.h"

#define CHJ_DEBUG
#ifdef CHJ_DEBUG
#define PRINT_DEBUG(format, args...)   printf(format, ##args)
#else
#define PRINT_DEBUG(format, args...)
#endif



//char device_uuid[20]; //充电站id编号，字符串uuid
//char json_buff[512];
//char client_id[11];

//Cut_wire_info cut_wire_info;
Charge_Info charge_Info= {0};

//#define ZERO_ERR_TM     600

//uint32_t null_sock_tm = ZERO_ERR_TM;

//uint32_t false_pull_tm= ZERO_ERR_TM;


//时间测试，将unix timestamp 和本地时间进行转换//0x5a603bfe   北京时间 2018/1/18 14:17:34
/* 下面打印是的格林威治标准时间年:118 月:0 日:18 时:6 分:17 秒:34格林威治标准时间比北京时间晚8个小时*/
void time_test(void)
{
    //将unix timestamp 转为本地时间

    struct tm *gm_date;
    time_t seconds=1502202265;//0x5a603bfe;
    //unix timestamp
    gm_date = localtime(&seconds);
    printf("年:%d \r\n",gm_date->tm_year+1900);
    printf("月:%d \r\n",gm_date->tm_mon);
    printf("日:%d \r\n",gm_date->tm_mday);
    printf("时:%d \r\n",gm_date->tm_hour);
    printf("分:%d \r\n",gm_date->tm_min);
    printf("秒:%d \r\n",gm_date->tm_sec);

    //将本地时间转为unix timestamp
    gm_date->tm_year=119;//2018年,+1900就是现在的年
    gm_date->tm_mon=3;//4//--月
    gm_date->tm_mday=11;

    gm_date->tm_hour=10;
    gm_date->tm_min=8;
    gm_date->tm_sec=30;
    seconds=mktime(gm_date);
    printf("unix timestamp:%08x \r\n",seconds);
}
//unix timestamp:5acdde9e 	//北京时间：	2018/4/11 18:8:30

//原文：https://blog.csdn.net/oshan2012/article/details/79892659
#if 1
char updata_path[200]= {0};

void json_parse_set_update2(cJSON *root)
{
    cJSON *url_j = NULL;

    char *p_data;
    char *skip_header=NULL;

    url_j = cJSON_GetObjectItem(root, "url");
    if(url_j == NULL)
    {
        printf("json_parse_set_update2 url_j == NULL!!!\r\n");
        return;
    }
    memset(updata_path, 0, sizeof(updata_path));
    memcpy(updata_path, url_j->valuestring, strlen(url_j->valuestring));

    if(memcmp(updata_path, "http://", strlen("http://") - 1) == 0)
    {
        skip_header = "http://";
    }
    else if(memcmp(updata_path, "https://", strlen("https://") - 1) == 0)
    {
        skip_header = "https://";
    }
    printf("skip_header=%s\r\n",skip_header);

    p_data= strrchr(updata_path,'/');
    printf("p_data=%s\r\n",p_data);

    PRINT_DEBUG("file_path:%s\r\n",p_data+strlen("/"));
    PRINT_DEBUG("updata_path:%s\r\n", updata_path);
    //notify mqtt_pause

}
#else
void json_parse_set_update2(cJSON *root)
{
    cJSON *url_j = NULL;
    char updata_path[200];
    char *p_data;

    PRINT_DEBUG("json_parse_set_update2\r\n");

    url_j = cJSON_GetObjectItem(root, "url");
    if(url_j == NULL)
    {
        printf("json_parse_set_update2 url_j == NULL!!!\r\n");
        return;
    }

    memset(updata_path, 0, sizeof(updata_path));
    memcpy(updata_path, url_j->valuestring, strlen(url_j->valuestring));

    if(memcmp(updata_path, "http://", sizeof("http://") - 1) == 0)
    {
        uint8_t server_ip[4], i;
        uint16_t server_port;
        char file_path[100];
        char server_path[100];

        p_data = (char *)(updata_path + sizeof("http://") - 1);
        if(*p_data >= '0' && *p_data <= '9' )
        {
            /*ip*/
            char num[6];
            uint8_t j;

            for( i = 0; i < 5; i++)
            {
                memset(num, 0, sizeof(num));
                if( i < 4)
                {
                    for( j = 0; (*p_data >= '0' && *p_data <= '9' ) ; j++)
                    {
                        num[j] = *p_data;
                        p_data++;
                    }
                    server_ip[i] = atoi(num);
                    if(i == 3 && *p_data != ':')
                    {
                        server_port = 80;
                        break;
                    }
                    p_data++;
                }

                if( i == 4)
                {
                    for( j = 0; (*p_data >= '0' && *p_data <= '9' ) ; j++)
                    {
                        num[j] = *p_data;
                        p_data++;
                    }
                    server_port = atoi(num);
                }
            }

            PRINT_DEBUG("server ip: %d.%d.%d.%d:%d ",server_ip[0], server_ip[1],server_ip[2],server_ip[3],server_port);

            p_data = (char *)(updata_path + sizeof("http://") - 1);
            while(1)
            {
                if(*p_data++ == '/')
                    break;
            }

            memset(file_path, 0, sizeof(file_path));
            file_path[0] = '/';
            strcpy( file_path+1,p_data);

            PRINT_DEBUG("file_path:%s\r\n", file_path);
            PRINT_DEBUG("updata_path:%s\r\n", updata_path);

            PRINT_DEBUG("recv update msg, reboot\r\n");
            /*
            iap_flag_set(updata_path);
            __set_FAULTMASK(1); 	 // 关闭所有中端
            NVIC_SystemReset();// 复位
            */
        }
        else
        {
            /*net string*/
            p_data = (char *)(updata_path + sizeof("http://") - 1);
            memset(server_path, 0, sizeof(server_path));
            i = 0;
            while(1)
            {
                if(*p_data == '/')
                    break;

                server_path[i] = *p_data;
                i++;
                p_data++;
            }
            p_data++;
            memset(file_path, 0, sizeof(file_path));
            file_path[0] = '/';
            strcpy( file_path+1,p_data);
            PRINT_DEBUG("server: %s,file_path:%s\r\n", server_path, file_path);
            PRINT_DEBUG("updata_path:%s\r\n", updata_path);
        }

        PRINT_DEBUG("recv update msg, reboot\r\n");

        /*
        iap_flag_set(updata_path);
        __set_FAULTMASK(1); 	 // 关闭所有中端
        NVIC_SystemReset();// 复位
        */
    }

    return;
}
#endif
void json_parse_set_update_test(char *url)
{
    //  cJSON *url_j = NULL;
    char updata_path[200];
    char *p_data;
    char *skip_header=NULL;

    PRINT_DEBUG("json_parse_set_update_test=%d %d\r\n",sizeof("http://"),sizeof("https://"));
    PRINT_DEBUG("json_parse_set_update_test=%d %d\r\n",strlen("http://"),strlen("https://"));

    memset(updata_path, 0, sizeof(updata_path));

    memcpy(updata_path, url, strlen(url));
    if(memcmp(updata_path, "http://", strlen("http://") - 1) == 0)
    {
        skip_header = "http://";
    }
    else if(memcmp(updata_path, "https://", strlen("https://") - 1) == 0)
    {
        skip_header = "https://";
    }
    printf("skip_header=%s\r\n",skip_header);

    p_data= strrchr(updata_path,'/');
    printf("p_data=%s\r\n",p_data);

    PRINT_DEBUG("file_path:%s\r\n",p_data+strlen("/"));
    PRINT_DEBUG("updata_path:%s\r\n", updata_path);


    return;
}

void json_parse_set_parm(cJSON *root)
{
    cJSON *tm = NULL,*heart_tm = NULL,*sta_up_tm = NULL, *temp_j = NULL, *wt_j = NULL, *ct_j=NULL;
    uint32_t sec, heart_sec, updata_sec,wait_sec;
    float warn_temp;

    struct tm *tm_now;
    tm = cJSON_GetObjectItem(root, "t");
    if(tm == NULL)
    {
        printf("json_parse_set_parm tm == NULL!!!\r\n");
        return;
    }
#if 0
    //tm_now =119-8-16 3:23:10
    tm_now->tm_year=119;/* 年份，其值等于实际年份减去1900 */
    tm_now->tm_mon=8;//4///* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
    tm_now->tm_mday=16;

    tm_now->tm_hour=3;
    tm_now->tm_min=23;
    tm_now->tm_sec=10;
    sec=mktime(tm_now);
    printf("unix timestamp:%d \r\n",sec);
#endif

    sec = tm->valueint;
    printf("%s sec=%d\r\n",__func__,sec);
    rtc_set_cnt(sec);

    tm_now = localtime(&sec);

    printf("tm_now =%d-%d-%d %d:%d:%d\r\n",	   \
           tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday,  \
           tm_now->tm_hour+8, tm_now->tm_min, tm_now->tm_sec);



    heart_tm = cJSON_GetObjectItem(root, "ht");
    if(heart_tm== NULL)
    {
        printf("json_parse_set_parm heart_tm == NULL!!!\r\n");
        return;
    }
    heart_sec = heart_tm->valueint;
    printf("%s heart_sec=%d\r\n",__func__,heart_sec);

    sta_up_tm = cJSON_GetObjectItem(root, "ut");
    if(sta_up_tm== NULL)
    {
        printf("json_parse_set_parm heart_tm == NULL!!!\r\n");
        return;
    }
    updata_sec = sta_up_tm->valueint;
    printf("%s updata_sec=%d\r\n",__func__,updata_sec);

    wt_j = cJSON_GetObjectItem(root, "wt");
    if(wt_j== NULL)
    {
        printf("json_parse_set_parm wt == NULL!!!\r\n");
        return;
    }
    wait_sec = wt_j->valueint;
//   null_sock_tm = wt_j->valueint;
    printf("%s wt=%d\r\n",__func__,wait_sec);

    ct_j = cJSON_GetObjectItem(root, "ct");
    if(ct_j== NULL)
    {
        printf("json_parse_set_parm ct == NULL!!!\r\n");
        return;
    }
   // false_pull_tm = ct_j->valueint;
   // printf("%s ct=%d\r\n",__func__,false_pull_tm);
//
    PRINT_DEBUG("heart:%d sec, updata:%d sec\r\n",heart_sec, updata_sec);

    temp_j = cJSON_GetObjectItem(root, "temp");
    if(temp_j== NULL)
    {
        printf("json_parse_set_parm temp == NULL!!!\r\n");
        return;
    }

    warn_temp = temp_j->valuedouble;
    printf("%s warn_temp %d=%f\r\n",__func__,temp_j->type,warn_temp);



    if((heart_sec < 10) ||(updata_sec < 10))
    {
        PRINT_DEBUG("parm err\r\n");
        return;
    }
    charge_mg_set_parm(heart_sec,updata_sec,wait_sec,warn_temp);

    //cut_wire_run_set_parm(heart_sec, updata_sec, warn_temp);
    //charge_mg_set_parm(heart_sec, updata_sec, warn_temp);
}

//int json_parse_set_sw(cJSON *root, uint8_t *get_ch, uint8_t *get_sta)
	
int json_parse_set_sw(cJSON *root, uint8_t *get_ch, Report_type *get_sta)
{
    cJSON *state = NULL;
    cJSON *state_item = NULL;
    cJSON *did = NULL;
    char *pdid = NULL;
    cJSON *channel = NULL, *sw_state = NULL, *open_tm = NULL,
           *max_w = NULL, *min_w = NULL, *tck_tm_j = NULL,*tm = NULL;
    int i, sw_cnt, ch, sw, op_tm, max_watter, min_watter, tck_tm;

    if(root == NULL)
    {
        printf("sw root NULL!!!\r\n");
        return 0;
    }

    printf("json_parse_set_sw\r\n");
    did = cJSON_GetObjectItem(root, "did");
    if(did == NULL)
    {
        printf("json_parse_set_sw did == NULL!!!\r\n");
        return 0;
    }
    else
    {
        pdid = did->valuestring;
        printf("did type=%d =%s\r\n",did->type,pdid);
    }

    tm = cJSON_GetObjectItem(root, "t");
    if(tm == NULL)
    {
        printf("json_parse_set_sw tm == NULL!!!\r\n");
        return 0;
    }
    else
    {
        printf("t type=%d,%d\r\n",tm->type,tm->valueint);
    }

    state = cJSON_GetObjectItem(root, "st");
    if(state == NULL)
    {
        printf("set sw state == NULL!!!\r\n");
        return 0;
    }
    else
    {
        printf("st type=%d,%d\r\n",state->type,state->valueint);
    }
    sw_cnt = cJSON_GetArraySize(state);
    for(i = 0; i < sw_cnt; i++)
    {
        state_item = cJSON_GetArrayItem(state, i);
        if(state_item == NULL)
        {
            printf("state_item == NULL!!!\r\n");
            return 0;
        }

        channel = cJSON_GetObjectItem(state_item, "cn");
        if(channel == NULL)
        {
            printf("channel == NULL!!!\r\n");
            return 0;
        }
        else
        {
            printf("cn type=%d,%d\r\n",channel->type,channel->valueint);
        }

        ch = channel->valueint;

        sw_state = cJSON_GetObjectItem(state_item, "sst");
        if(sw_state == NULL)
        {
            printf("sw_state == NULL!!!\r\n");
            return 0;
        }
        else
        {
            printf("sst type=%d,%d\r\n",sw_state->type,sw_state->valueint);
        }

        sw = sw_state->valueint;

        open_tm = cJSON_GetObjectItem(state_item, "opt");
        if(open_tm == NULL)
        {
            printf("open_tm == NULL!!!\r\n");
            return 0;
        }
        else
        {
            printf("opt type=%d,%d\r\n",open_tm->type,open_tm->valueint);
        }

        op_tm = open_tm->valueint;

        max_w = cJSON_GetObjectItem(state_item, "apow");
        if(max_w == NULL)
        {
            printf("max_w == NULL!!!\r\n");
            return 0;
        }
        else
        {
            printf("apow type=%d,%d\r\n",max_w->type,max_w->valueint);
        }

        max_watter = max_w->valueint;

        min_w = cJSON_GetObjectItem(state_item, "ipow");
        if(min_w == NULL)
        {
            printf("min_w == NULL!!!\r\n");
            return 0;
        }
        else
        {
            printf("ipow type=%d,%d\r\n",min_w->type,min_w->valueint);
        }

        min_watter = min_w->valueint;

        tck_tm_j = cJSON_GetObjectItem(state_item, "tck");
        if(tck_tm_j == NULL)
        {
            printf("tck_tm_j == NULL!!!\r\n");
            return 0;
        }
        else
        {
            printf("tck type=%d,%f\r\n",tck_tm_j->type,tck_tm_j->valuedouble);
        }

        tck_tm = tck_tm_j->valuedouble;

        /**************************设置开关*****************************/
        printf("set sw id:%d, ch:%d, sw:%d, tm:%d, tck_tm:%d\r\n",i, ch,sw, op_tm, tck_tm);

        get_ch[i] = ch;
        if(sw == 0)
        {

            if(charge_mg_set_off(ch)==0)
            {
                get_sta[i] = REPORT_TYPE_SET_SW_OK;//
                SET_EVENT(MCU_EVENT_AUDIO_NUM_1+ch);
                SET_EVENT(MCU_EVENT_AUDIO_CHARGE_END);
                TA6932_DisplayPortNumber(ch+1,ch+1);
                //REPORT_TYPE_CHARG_FINISH;//;
            }
            else
            {
                get_sta[i] = REPORT_TYPE_SET_SW_FAIL;//0;
            }


        }
        else if(sw == 1)
        {

            if(charge_mg_set_on(ch, op_tm, max_watter, min_watter, tck_tm)==0)
            {
                get_sta[i] = REPORT_TYPE_SET_SW_OK;//1
                SET_EVENT(MCU_EVENT_AUDIO_NUM_1+ch);
                SET_EVENT(MCU_EVENT_AUDIO_CHARGE_START);
                //SET_EVENT(MCU_EVENT_AUDIO_NUM_1_CHARGE_START);
                if(charge_Info.wait_tm >0)
                	TA6932_DisplayPortNumber(ch+1,op_tm);
            }
            else
            {
                get_sta[i] = REPORT_TYPE_SET_SW_FAIL;//0;
                SET_EVENT(MCU_EVENT_AUDIO_NUM_1+ch);
                SET_EVENT(MCU_EVENT_AUDIO_CHARGE_AO);
                SET_EVENT(MCU_EVENT_AUDIO_CHARGE_BUG);
            }

        }
        /************************设置功率参数***************************/
        //cut_wire_run_set_sw_parm(ch ,max_watter, min_watter, tck_tm);
    }

    return sw_cnt;
}

void json_parse(char *recv, char *topic)
{
    cJSON *root_json;
#if 1
    if(memcmp(sub_topics[0], topic, strlen(sub_topics[0])) == 0)
    {
        /*服务端下发开关命令*/
        uint8_t get_ch[CHARGE_NUM];
        Report_type get_sta[CHARGE_NUM];
        int num;

        root_json=cJSON_Parse(recv);
        if (!root_json)
        {
            printf("set sw Error before: [%s]\r\n",cJSON_GetErrorPtr());
            return;
        }

        num = json_parse_set_sw(root_json, get_ch, get_sta);
        cJSON_Delete(root_json);
        //return ;
        // mem_init();
        if(num > 0)
        {
            mqtt_pub_sw_report(get_ch,get_sta, num);
        }
    }

    else if(memcmp(sub_topics[1], topic, strlen(sub_topics[1])) == 0)
    {
        /*获取/设置参数命令*/
        root_json=cJSON_Parse(recv);
        if (!root_json)
        {
            printf("get/set parm Error before: [%s]\r\n",cJSON_GetErrorPtr());
            return;
        }

        json_parse_set_parm(root_json);
        cJSON_Delete(root_json);
        // mem_init();
    }

    else if(memcmp(sub_topics[2], topic, strlen(sub_topics[2])) == 0)
    {
        /*5.固件升级*/
        root_json=cJSON_Parse(recv);
        if (!root_json)
        {
            printf("get/set parm Error before: [%s]\r\n",cJSON_GetErrorPtr());
            return;
        }
        //json_parse_set_update(root_json);
        json_parse_set_update2(root_json);
        cJSON_Delete(root_json);
        // mem_init();
    }
#endif
    return;
}

//int json_online_cmd(char *get_str,char* id_str)
int json_online_cmd(uint8_t *get_str,int json_len)
{
    cJSON * root = 0;
    char * out = 0;
    int len;
    if((get_str == NULL) )//|| (id_str== NULL) )
    {
        PRINT_DEBUG("create js string is %s\r\n",out);
        return -1;
    }
    /*
    else if(strlen(id_str) >= sizeof(device_uuid))
    {
    	return -2;
    }
    	*/
    root = cJSON_CreateObject();
    if(!root)
    {
        PRINT_DEBUG("%s err\r\n",__func__);
        return -3;
    }

    cJSON_AddStringToObject(root,"did",client_id);
    out = cJSON_Print(root);
    if(out)
    {
        //PRINT_DEBUG("create js string is %s\r\n",out);
        len = strlen(out);
        if(len > json_len)
        {
            PRINT_DEBUG("len > sizeof(json_buff) \r\n");
            return -4;
        }
        else
        {
            memcpy(get_str, out, len);

        }
        PRINT_DEBUG("myfree start \r\n");
        myfree(out);
    }
    PRINT_DEBUG("cJSON_Delete root \r\n");
    cJSON_Delete(root);
    //mem_init();

    return len;
}
#if 0
int json_sw_change_notify(uint8_t ch, uint8_t *get_str, Report_type type)
{
    cJSON *root, *sta_item, *ch_msg_item;
    double now_energy = 0.0, used_energy = 0.0;
    char *out;
    int len;

    root = cJSON_CreateObject();
    if(root == NULL)
    {
        PRINT_DEBUG("json_charge_finish root err\r\n");
        return -1;
    }

    cJSON_AddStringToObject(root, "did", client_id);
    cJSON_AddNumberToObject(root, "t",RTC_GetCounter());

    sta_item = cJSON_CreateArray();
    if(sta_item == NULL)
    {
        PRINT_DEBUG("json_charge_finish sta_item err\r\n");
        cJSON_Delete(root);
        //mem_init();
        return -2;
    }

    cJSON_AddItemToObject(root, "st", sta_item);
    ch_msg_item = cJSON_CreateObject();
    if(ch_msg_item == NULL)
    {
        PRINT_DEBUG("json_charge_finish ch_msg_item err\r\n");
        cJSON_Delete(root);
        //mem_init();
        return -3;
    }

    cJSON_AddItemToArray(sta_item, ch_msg_item);
    cJSON_AddNumberToObject(ch_msg_item,"cn", ch);

    if(cut_wire_info.dev_sta[ch] == DEV_TURN_ON)
    {
        cJSON_AddNumberToObject(ch_msg_item,"sst", 1);
    }
    else
    {
        cJSON_AddNumberToObject(ch_msg_item,"sst", 0);
    }

    if(charge_Info.valid_tm[ch] == 0)
    {
        cJSON_AddNumberToObject(ch_msg_item,"tl", 0);
    } else {
        cJSON_AddNumberToObject(ch_msg_item,"tl", (charge_Info.valid_tm[ch] / 60 + 1));
    }
    cJSON_AddNumberToObject(ch_msg_item,"type", (int)type);

    now_energy = cut_wire_info.energy_int[ch] + (cut_wire_info.energy_f[ch] * 0.01);
    used_energy = now_energy - charge_Info.start_energy[ch];
    cJSON_AddNumberToObject(ch_msg_item,"en", used_energy);

    out = cJSON_Print(root);
    if(out)
    {
        //PRINT_DEBUG("create js string is %s\r\n",out);
        len = strlen(out);
        memcpy(get_str, out, len);
        PRINT_DEBUG("myfree start \r\n");
        myfree(out);

    }
    cJSON_Delete(root);
    // mem_init();

    return len;
}
#endif
int json_sw_ctrl_report(uint8_t *get_str, uint8_t *set_ch, Report_type *sta, uint8_t sw_num)
{
    cJSON *root, *sta_item, *ch_msg_item;
 //   double now_energy = 0.0;
		double	used_energy = 0.0;
    char *out;
    int len;
    uint8_t i;

    root = cJSON_CreateObject();
    if(root == NULL)
    {
        PRINT_DEBUG("json_charge_finish root err\r\n");
        return -1;
    }

    //cJSON_AddStringToObject(root, "did", client_id);
    cJSON_AddStringToObject(root, "did", client_id);
    cJSON_AddNumberToObject(root, "t",RTC_GetCounter());

    sta_item = cJSON_CreateArray();
    if(sta_item == NULL)
    {
        PRINT_DEBUG("json_charge_finish sta_item err\r\n");
        cJSON_Delete(root);
        //mem_init();
        return 0;
    }

    cJSON_AddItemToObject(root, "st", sta_item);
    for( i = 0; i < sw_num; i++)
    {
        ch_msg_item = cJSON_CreateObject();
        if(ch_msg_item == NULL)
        {
            PRINT_DEBUG("json_charge_finish ch_msg_item err\r\n");
            cJSON_Delete(root);
            //mem_init();
            return -2;
        }

        cJSON_AddItemToArray(sta_item, ch_msg_item);
        cJSON_AddNumberToObject(ch_msg_item,"cn", set_ch[i]);
        // if(cut_wire_info.dev_sta[set_ch[i]] == DEV_TURN_ON)
        if(charge_Info.dc_port_status[set_ch[i]] == PORT_USEING)
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 1);
        }
        else
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 0);
        }

        //if(charge_Info.valid_tm[set_ch[i]] == 0)
        if(charge_Info.charge_tm[set_ch[i]] == 0)
        {
            cJSON_AddNumberToObject(ch_msg_item,"tl", 0);
        }
        else
        {
            cJSON_AddNumberToObject(ch_msg_item,"tl", charge_Info.charge_left_tm[set_ch[i]]);
        }
#if 1
        cJSON_AddNumberToObject(ch_msg_item,"type", sta[i]);
#else
        if(sta[i] == 1)
        {
            cJSON_AddNumberToObject(ch_msg_item,"type", REPORT_TYPE_SET_SW_OK);
        }
        else
        {
            cJSON_AddNumberToObject(ch_msg_item,"type", REPORT_TYPE_SET_SW_FAIL);
        }
#endif

        //now_energy = cut_wire_info.energy_int[set_ch[i]] + (cut_wire_info.energy_f[set_ch[i]] * 0.01);
        //used_energy = now_energy - charge_Info.start_energy[set_ch[i]];
        used_energy =0;
        cJSON_AddNumberToObject(ch_msg_item,"en", used_energy);

		if(charge_mode == CHARGE_MODE_DC )
		{

			cJSON_AddNumberToObject(ch_msg_item,"pow", (uint16_t)(charge_Info.charge_cur_pow[i]*0.1));
		}
		else
		{

			cJSON_AddNumberToObject(ch_msg_item,"pow", (uint16_t)(charge_Info.charge_cur_pow[i]));
		}
	

    }
#if 0
//	cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format);
    myfree
    cJSON_PrintPreallocated(root,pbuffer,51);
#else
    out = cJSON_Print(root);
    if(out)
    {
        //PRINT_DEBUG("create js string is %s\r\n",out);
        len = strlen(out);
        memcpy(get_str, out, len);
        PRINT_DEBUG("myfree start \r\n");
        myfree(out);

    }
#endif
    cJSON_Delete(root);
    //mem_init();

    return len;
}
//static
uint8_t send_tm_point = 0; //分四部分发送, 一次发五个
uint8_t get_cur_upgrade_num(void)
{
    return send_tm_point;
}

int json_updata_cut_wire_msg(uint8_t *get_str)
{
    cJSON *root = NULL, *sta_item = NULL, *ch_msg_item = NULL;
    cJSON_bool json_ret =0;
    //double energy[CUT_WIRE_NUM] = {0};
    //double* energy =(double*)mymalloc(CUT_WIRE_NUM*sizeof(double));
    double* energy =(double*)mymalloc(channel_num*sizeof(double));

    double all_energy;
//    double current;

    char *out;
    //char hw_ver_str[15],cut_hw_ver_str[15];
    uint8_t i;
    int len = 0;
    uint16_t port_status = 0;

    //static uint8_t send_tm_point = 0; //分四部分发送, 一次发五个
    uint8_t end_num = (send_tm_point + 1)* 5;
//    int  ntc_temp;
    float  f_tem;
    // mem_init();
    root = cJSON_CreateObject();
    if(root == NULL)
    {
        // PRINT_DEBUG("json_updata_cut_wire_msg root err\r\n");
        printf("%s err\r\n",__func__);
        goto out;
    }

    cJSON_AddStringToObject(root, "did", client_id);
    cJSON_AddStringToObject(root, "v", VER_PRO_NET);
//   sprintf(hw_ver_str,"%d.%d.%d", dev_info.hw_ver[0],dev_info.hw_ver[1],dev_info.hw_ver[2]);
    //  cJSON_AddStringToObject(root, "hw", hw_ver_str);
    cJSON_AddNumberToObject(root, "t", RTC_GetCounter());
    /********************温度********************/
    // cJSON_AddNumberToObject(root, "temp", charge_Info.temperature);
#if 0
    ntc_temp = get_ntc_temp_adc1_chn9();
    printf("ntc temp %d\r\n",ntc_temp);
    cJSON_AddNumberToObject(root, "temp", ntc_temp);
#else
    f_tem = DS18B20_Get_Temp(NULL,4);
	//if(((int)f_tem) == -100)
	//	f_tem = 100;
    printf("DS18B20_Get_Temp  %f\r\n",f_tem);
    cJSON_AddNumberToObject(root, "temp", (int)f_tem);
#endif

    // cJSON_AddNumberToObject(root, "vol", cut_wire_info.voltage[0]);
    cJSON_AddNumberToObject(root, "vol", 0);
    //  cJSON_AddNumberToObject(root, "rssi", sim800c_msg.rssi);
    /**************电站总度数，建议不用***********/
    all_energy = 0;
    /*
    memset((char*)energy,0,CUT_WIRE_NUM*sizeof(double));
    for(i = 0; i < cut_wire_info.dev_num; i++)
    {
        energy[i] = cut_wire_info.energy_int[i] + (cut_wire_info.energy_f[i] * 0.01);
        all_energy += energy[i];
    }
    */
    cJSON_AddNumberToObject(root, "en", all_energy);

    sta_item = cJSON_CreateArray();
    if(sta_item == NULL)
    {
        PRINT_DEBUG("json_updata_cut_wire_msg sta_item err\r\n");
        cJSON_Delete(root);
        // mem_init();
        len = -1;
        goto out;
    }

    cJSON_AddItemToObject(root, "st", sta_item);
#if 1

    i = send_tm_point * 5;
    for( ; ( i < 10 ) && ( i < end_num)  ; i++)
    {
        ch_msg_item = cJSON_CreateObject();
        if(ch_msg_item == NULL)
        {
            PRINT_DEBUG("json_charge_finish ch_msg_item:%d err\r\n", i);
            cJSON_Delete(root);
            //mem_init();
            len = -2;
            goto out;
        }

        cJSON_AddNumberToObject(ch_msg_item,"cn", i);



        //               charge_Info.charge_tm[ch] = charge_tm;
        //        charge_Info.trickle_tm[ch] = trickle_tm;
        //        charge_Info.start_time[ch] = RTC_GetCounter();//need dw
        if(charge_Info.dc_port_status[i] == PORT_USEING)
        {
            uint32_t cur_sec = RTC_GetCounter();//need dw
            uint16_t charge_left_tm = (charge_Info.charge_tm[i] -  (cur_sec-charge_Info.start_time[i])/60);

            int ret =0;
            if(charge_mode == CHARGE_MODE_DC )
                ret= dc_port_x_status(i+1,&port_status);//chan 1-10
            else
                ret=app_rs485_broker_port_x_status(i+1,&port_status);//chan 1-10

            if(ret ==0 )
                charge_left_tm= charge_Info.charge_left_tm[i];
            if(charge_Info.tck_start_tm[i] != 0)
                charge_left_tm = charge_Info.tck_left_tm[i];
            //TA6932_DisplayPortNumber(i,charge_left_tm);
            //printf("ch=%d charge_left_tm=%d\r\n",i, charge_Info.charge_left_tm[i] );
            cJSON_AddNumberToObject(ch_msg_item,"tl", charge_left_tm);

            
			if(charge_mode == CHARGE_MODE_DC )
			{
				cJSON_AddNumberToObject(ch_msg_item,"cur", (uint16_t)(charge_Info.charge_cur_pow[i]*0.1/220));
            	cJSON_AddNumberToObject(ch_msg_item,"pow", (uint16_t)(charge_Info.charge_cur_pow[i]*0.1));//cut_wire_info.watter[i]);
			}
			else
			{
				cJSON_AddNumberToObject(ch_msg_item,"cur", (uint16_t)(charge_Info.charge_cur_pow[i]/220));
				cJSON_AddNumberToObject(ch_msg_item,"pow", (uint16_t)(charge_Info.charge_cur_pow[i]));
			}
			//if((charge_Info.charge_cur_pow[i]*0.1) >charge_Info.max_watter[i])
            //{
            //	charge_Info.dc_port_status[i] = PORT_USEING;
            //	dc_stop_power(i+1);
            //}
        }
        else
        {
            cJSON_AddNumberToObject(ch_msg_item,"tl", 0);
            cJSON_AddNumberToObject(ch_msg_item,"cur", 0);
            cJSON_AddNumberToObject(ch_msg_item,"pow", 0);
        }
        //sst：插座通断状态，数字，0-断开，（非0-接通）1-待插头，2-充电中，3-涓流中，4-异常
        if(charge_Info.dc_port_status[i] == PORT_FREE)
        {
            // cJSON_AddNumberToObject(ch_msg_item,"sst", 1);
            cJSON_AddNumberToObject(ch_msg_item,"sst", 0);
        }
        else if(charge_Info.dc_port_status[i] == PORT_USEING)
        {
            if( port_status == 0)
                cJSON_AddNumberToObject(ch_msg_item,"sst", 1);
            else
            {
            	if(charge_mode == CHARGE_MODE_DC )
            	{
	                if((charge_Info.charge_cur_pow[i]*0.1) <= charge_Info.min_watter[i])
	                    cJSON_AddNumberToObject(ch_msg_item,"sst", 3);
	                else
	                    cJSON_AddNumberToObject(ch_msg_item,"sst", 2);
            	}
				else
				{
	                if((charge_Info.charge_cur_pow[i]) <= charge_Info.min_watter[i])
	                    cJSON_AddNumberToObject(ch_msg_item,"sst", 3);
	                else
	                    cJSON_AddNumberToObject(ch_msg_item,"sst", 2);

				}
            }
        }

        else if((charge_Info.dc_port_status[i] == PORT_FORBID)|| \
                (charge_Info.dc_port_status[i] == PORT_FAULT))
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 4);
        }


        cJSON_AddNumberToObject(ch_msg_item,"en",0);//energy[i]);

        cJSON_AddItemToArray(sta_item, ch_msg_item);
    }

    send_tm_point++;
    if((send_tm_point * 5) >= channel_num)//cut_wire_info.dev_num)
    {
        send_tm_point = 0;
    }

#else
//FOR TEST
    cut_wire_info.dev_num = CUT_WIRE_NUM;
    i = send_tm_point * 5;
    for( ; ( i < cut_wire_info.dev_num ) && ( i < end_num)  ; i++)
    {
        ch_msg_item = cJSON_CreateObject();
        if(ch_msg_item == NULL)
        {
            PRINT_DEBUG("json_charge_finish ch_msg_item:%d err\r\n", i);
            cJSON_Delete(root);
            //mem_init();
            len = -2;
            goto out;
        }

        cJSON_AddNumberToObject(ch_msg_item,"cn", i);

        current = cut_wire_info.current[i] +( cut_wire_info.current_f[i] * 0.01);
        cJSON_AddNumberToObject(ch_msg_item,"cur", current);
        cJSON_AddNumberToObject(ch_msg_item,"pow", cut_wire_info.watter[i]);

        if(charge_Info.sta[i] == CHARGE_NONE)
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 0);
        }
        else if(charge_Info.sta[i] == CHARGE_MIN)
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 3);
        }
        else if((charge_Info.sta[i] == CHARGE_RUNING)|| (charge_Info.sta[i] == CHARGE_FINISH))
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 2);
        }
        else if((charge_Info.sta[i] == CHARGE_READY)|| (charge_Info.sta[i] == CHARGE_NO_INSERT))
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 1);
        }
        else
        {
            cJSON_AddNumberToObject(ch_msg_item,"sst", 4);
        }

        cJSON_AddNumberToObject(ch_msg_item,"en", energy[i]);
        cJSON_AddNumberToObject(ch_msg_item,"tl", (charge_Info.valid_tm[i] / 60) + 1);
        //memset(cut_hw_ver_str, 0 , sizeof(cut_hw_ver_str));
        //sprintf(cut_hw_ver_str,"%02d%02d%02d", cut_wire_info.sw_ver[i][0],cut_wire_info.sw_ver[i][1],cut_wire_info.sw_ver[i][2]);
        //cJSON_AddStringToObject(ch_msg_item,"sw",cut_hw_ver_str);

        cJSON_AddItemToArray(sta_item, ch_msg_item);
    }

    send_tm_point++;
    if((send_tm_point * 5) >= cut_wire_info.dev_num)
    {
        send_tm_point = 0;
    }
#endif
    PRINT_DEBUG("cJSON_Print START\r\n");
#if 1
    out = mymalloc(600);
    if(out ==NULL)
    {
        PRINT_DEBUG("mymalloc(600) fail\r\n");
        goto out;
    }
#if 0//def MCU_USING_MODULE_BACKTRACE
    {
        uint32_t start_addr = (uint32_t)thread1_stk_3;
        uint32_t max_len = sizeof(thread1_stk_3);

        dump_memeory(start_addr, max_len);
    }
#endif
    memset(out,0,600);
    json_ret = cJSON_PrintPreallocated(root,out,600,1);
    if(json_ret ==1)
    {
        len= strlen(out);
        memcpy(get_str, out, len);
        PRINT_DEBUG("get_str=%s len=%d \r\n",get_str,len);
    }
    else
    {
        PRINT_DEBUG("cJSON_PrintPreallocated fail\r\n");
    }
    myfree(out);
#else
    out = cJSON_Print(root);
    if(out)
    {

        len = strlen(out);
        memcpy(get_str, out, len);
        //len=556
        PRINT_DEBUG("get_str=%s len=%d \r\n",get_str,len);
        myfree(out);
    }
    else
        PRINT_DEBUG("cJSON_Print is NULL\r\n");
#endif
    PRINT_DEBUG("cJSON_Delete root \r\n");
    cJSON_Delete(root);
    // mem_init();
out:
    myfree(energy);
    return len;
}
int json_station_warning_notify(Sta_warning warn, uint8_t *get_str)
{
    cJSON *root;
    char *out;
    int len;

    root = cJSON_CreateObject();
    if(!root)
    {
        PRINT_DEBUG("json_station_warning_notify root err\r\n");
        return -1;
    }

    //cJSON_AddStringToObject(root,"did",client_id);
    cJSON_AddStringToObject(root,"did",client_id);
    cJSON_AddNumberToObject(root, "t", RTC_GetCounter());
    cJSON_AddNumberToObject(root, "warning", (int)warn);
    if(STA_WARN_BOARD_TEMP == warn)
        cJSON_AddNumberToObject(root, "temp", (int)charge_Info.board_temp);
    else
        cJSON_AddNumberToObject(root, "temp", (int)charge_Info.temperature);

    out = cJSON_Print(root);
    if(out)
    {
        //PRINT_DEBUG("create js string is %s\r\n",out);
        len = strlen(out);
        memcpy(get_str, out, len);
        PRINT_DEBUG("myfree start \r\n");
        myfree(out);

    }
    cJSON_Delete(root);
    // mem_init();

    return len;
}

int free_num = 0;
int malloc_num = 0;

void myfree(void *ptr)
{
// 	u32 offset;
    if(ptr==NULL)
        return;//地址为0.
    free_num ++;
    printf("%s:0x%08x\r\n",__func__,ptr);
    free(ptr);//释放内存
}
//分配内存(外部调用)
//size:内存大小(字节)
//返回值:分配到的内存首地址.
void *mymalloc(size_t size)
{
    void * pmalloc = NULL;
    malloc_num ++;
    pmalloc = malloc(size);//释放内存
    printf("%s:%d ,0x%08x\r\n",__func__,size,pmalloc);
    return pmalloc;
}
void print_heap_info(void)
{
    printf("%s:%d,%d\r\n",__func__,malloc_num,free_num);
}
cJSON_Hooks hooks;
void dx_cjson_init(void)
{

    printf("Version: %s\r\n", cJSON_Version());
    hooks.malloc_fn  = mymalloc;
    hooks.free_fn = myfree;
    cJSON_InitHooks(&hooks);
    print_heap_info();

    //main_cjson();

}
#if 0
int main_cjson(void)
{
    int ret = 0;
    uint8_t set_ch[2]= {1,0};
    bool sta[2]= {TRUE,FALSE};
    ///main_time();
    //time_test();
    /* print the version */
    printf("Version: %s\n", cJSON_Version());


    // hooks.malloc_fn  = mymalloc;
    // hooks.free_fn = myfree;
    /// cJSON_InitHooks(&hooks);
    //print_heap_info();
//    int rc =-1;
#if 0
    int json_len=sizeof(json_buff);
    char* pjson_buff=json_buff;

    memset(json_buff,0,json_len);
#else
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);
#endif

    ret = json_online_cmd(pjson_buff,json_len);
    if(ret  > 0)
    {
        printf("ret: %d\r\n", ret);
        printf("cur json_buff is: %s \r\n", pjson_buff);

    }
    else
        printf("ret: %d\r\n", ret);


    print_heap_info();
    memset(pjson_buff,0,json_len);
    ret = json_updata_cut_wire_msg(pjson_buff);
    if(ret  > 0)
    {
        printf("ret: %d\r\n", ret);
        printf("cur json_buff is: %s \r\n", pjson_buff);

    }
    else
        printf("ret: %d\r\n", ret);

    print_heap_info();

    memset(pjson_buff,0,json_len);
    ret = json_station_warning_notify(STA_WARN_SMOKE,pjson_buff);
    if(ret  > 0)
    {
        printf("ret: %d\r\n", ret);
        printf("cur json_buff is: %s \r\n", pjson_buff);

    }
    else
        printf("ret: %d\r\n", ret);

    print_heap_info();


    memset(pjson_buff,0,json_len);

    ret = json_sw_ctrl_report(pjson_buff,set_ch,sta,2);
    if(ret  > 0)
    {
        printf("ret: %d\r\n", ret);
        printf("cur json_buff is: %s \r\n", pjson_buff);

    }
    else
        printf("ret: %d\r\n", ret);

    print_heap_info();

#if 0
    memset(pjson_buff,0,json_len);
    ret = json_sw_change_notify(2,pjson_buff,REPORT_TYPE_CHARG_FINISH);
    if(ret	> 0)
    {
        printf("ret: %d\r\n", ret);
        printf("cur json_buff is: %s \r\n", pjson_buff);

    }
    else
        printf("ret: %d\r\n", ret);
#endif
    print_heap_info();
    {
        char* pTopic= "dc/cs/set/0571TST001";
        char* psetjson=
            "{\"did\":\"12312adce3131\",\"t\":1502202265,\"st\":[{\"cn\":0,\"sst\":1,\"apow\":200,\"ipow\":5,\"tck\":1,\"opt\":120},{\"cn\":1,\"sst\":1,\"apow\":200,\"ipow\":5,\"tck\":1,\"opt\":120}]}";

        json_parse(psetjson,pTopic);
    }
    print_heap_info();
    return 0;
}
#endif
#if 0
//Configure the MQTT protocol version
app4g_AT+QMTCFG="version",0

                uart2 uart_buf_len =30 =
                                        +QMTCFG: "version"
                                        ,3

                                        OK


                                        app4g_AT+QMTCFG="pdpcid",1

                                                uart2 uart_buf_len =29 =
                                                        +QMTCFG: "pdpcid"
                                                        ,1

                                                        OK

                                                        app4g_AT+QMTCFG="will",1

                                                                uart2 uart_buf_len =27 =
                                                                        +QMTCFG: "will"
                                                                        ,0

                                                                        OK

                                                                        app4g_AT+QMTCFG="timeout",1
                                                                                uart2 uart_buf_len =34 =
                                                                                        +QMTCFG: "timeout"
                                                                                        ,5,3,0

                                                                                        OK

                                                                                        app4g_AT+QMTCFG="session",1
                                                                                                uart2 uart_buf_len =30 =
                                                                                                        +QMTCFG: "session"
                                                                                                        ,1

                                                                                                        OK

                                                                                                        app4g_AT+QMTCFG="keepalive",1

                                                                                                                uart2 uart_buf_len =34 =
                                                                                                                        +QMTCFG: "keepalive"
                                                                                                                        ,120

                                                                                                                        OK
                                                                                                                        app4g_AT+QMTCFG="ssl",1

                                                                                                                                uart2 uart_buf_len =26 =
                                                                                                                                        +QMTCFG: "ssl"
                                                                                                                                        ,0

                                                                                                                                        OK
                                                                                                                                        app4g_AT+QMTCFG="recv/mode",1

                                                                                                                                                uart2 uart_buf_len =34 =
                                                                                                                                                        +QMTCFG: "recv/mode"
                                                                                                                                                        ,0,0

                                                                                                                                                        OK
                                                                                                                                                        app4g_AT+QMTCFG="aliauth",1

                                                                                                                                                                uart2 uart_buf_len =31 =
                                                                                                                                                                        +QMTCFG: "aliauth"
                                                                                                                                                                        ,,,

                                                                                                                                                                        OK

#endif

                                                                                                                                                                        void charge_mg_init(void)
{
    memset(&charge_Info, 0, sizeof(charge_Info));
    //charge_Info.ping_tm = 30;
    //charge_Info.updata_tm = 300;
    //charge_Info.warm_temp = 60.0;
    if(charge_mode == CHARGE_MODE_BROKER )
    {
        //app_rs485_broker_init();
       // app_rs485_broker_info_print();

        app_rs485_broker_lookup_all_info();
    }
    else
    {
        dc_port_status();
        dc_get_port_charge_status(1);
        dc_get_total_consumption(1);
        dc_get_maxpower_charge_finish_stopen(1);
    }
    return;
}

void charge_mg_set_parm(uint32_t heartbeat_tm, uint32_t updata_tm, uint32_t wait_tm,float temperature)
{
    charge_Info.warn_temp = temperature;
    charge_Info.ping_tm = heartbeat_tm;
    charge_Info.updata_tm = updata_tm;
    charge_Info.wait_tm = wait_tm;


    return;
}

uint8_t charge_mg_set_on(uint8_t ch, uint32_t charge_tm, uint16_t max_w, uint16_t min_w, uint32_t trickle_tm)
{
#if 1
    int ret =0;
    int i =0;
    DianChuan_Frame *pFrame = &DianChuanRxFrame;


    if( ch >= CHARGE_NUM )
        return 1;
    if(charge_Info.dc_port_status[ch] != PORT_FREE)
    {
        if(charge_mode == CHARGE_MODE_BROKER )
        {
            charge_Info.max_watter[ch] = max_w;
        }
        printf("dc_port_status[%d]=0x%02x\r\n",ch,charge_Info.dc_port_status[ch]);
        return 1;
    }
#if (TIME_VALID_JUDGE == 1)
    {
        myst  *pt;
        myst  src_pt;
        myst  dst_pt;

        time_t seconds = 0;
        time_t src_seconds = 0;
        struct tm dst_tm= {0};

        char buffer[22]= {0};

        dx_get_lte_ntp_time(buffer,22);
        printf("dx_get_lte_ntp_time=%d %s\r\n",ret,buffer);

        pt= &src_pt;
        mytransfor(buffer,pt);
        printf("src year:%d mon:%d day:%d hour:%d min:%d sec:%d tzone:%d\r\n", \
               pt->year,pt->mon,pt->day,pt->hour,pt->min,pt->sec,pt->tzone);

        dst_tm.tm_sec =pt->sec;
        dst_tm.tm_min =pt->min;
        dst_tm.tm_hour =pt->hour;
        dst_tm.tm_mday =pt->day;
        dst_tm.tm_mon =pt->mon;
        dst_tm.tm_year = pt->year+2000-1900;

        src_seconds=mktime(&dst_tm);
        printf("unix cur timestamp:%08x \r\n",src_seconds);


        pt= &dst_pt;
        mytransfor(DEAD_TIME,pt);
        printf("dst year:%d mon:%d day:%d hour:%d min:%d sec:%d tzone:%d\r\n", \
               pt->year,pt->mon,pt->day,pt->hour,pt->min,pt->sec,pt->tzone);

        dst_tm.tm_sec =pt->sec;
        dst_tm.tm_min =pt->min;
        dst_tm.tm_hour =pt->hour;
        dst_tm.tm_mday =pt->day;
        dst_tm.tm_mon =pt->mon;
        dst_tm.tm_year = pt->year+2000-1900;
        seconds=mktime(&dst_tm);
        printf("unix timestamp:%08x \r\n",seconds);
        if(src_seconds > seconds)
        {
            printf("mcu timout,please 缴费\r\n");
            return 5;
        }
    }
#endif
    if(charge_mode == CHARGE_MODE_BROKER )
        ret= app_rs485_broker_start_power(ch+1);
    else
        ret = dc_start_power(ch+1,charge_tm);
    if(ret == 0)
    {

        if(charge_mode == CHARGE_MODE_DC )
        {
            printf("pFrame->sop=0x%02x\r\n",pFrame->sop);
            printf("pFrame->len=0x%02x\r\n",pFrame->len);
            printf("pFrame->cmd=0x%02x\r\n",pFrame->cmd);
            printf("pFrame->session_id:");
            for(i=0; i<6; i++)
                printf("session_id[%d]=0x%02x ",i,pFrame->session_id[i]);
            printf("\r\n");
            printf("pFrame->data:");
            for(i=0; i<(pFrame->len-8); i++)
                printf("data[%d]=0x%02x ",i,pFrame->data[i]);
            printf("\r\n");
            printf("pFrame->sum=0x%02x\r\n",pFrame->sum);


            if((ch+1) == pFrame->data[0])
            {
                if(pFrame->data[1] == 1)
                {
                    charge_Info.dc_port_status[ch] = PORT_USEING;
                    charge_Info.max_watter[ch] = max_w;
                    charge_Info.min_watter[ch] = min_w;
                    charge_Info.charge_tm[ch] = charge_tm;
                    charge_Info.trickle_tm[ch] = trickle_tm;

                    charge_Info.charge_left_tm[ch] = charge_tm;
                    charge_Info.tck_left_tm[ch] = trickle_tm;
                    charge_Info.tck_start_tm[ch] = 0;
                    charge_Info.start_time[ch] = RTC_GetCounter();//need dw
                    charge_Info.port_report_type[ch] = REPORT_TYPE_MAX;
                    return 0;

                }
                else
                {
                    charge_Info.dc_port_status[ch] =  PORT_FAULT;
                    charge_Info.max_watter[ch] = 0;
                    charge_Info.min_watter[ch] = 0;
                    charge_Info.charge_tm[ch] = 0;
                    charge_Info.trickle_tm[ch] = 0;

                    charge_Info.charge_left_tm[ch] = 0;
                    charge_Info.tck_left_tm[ch] = 0;
                    charge_Info.tck_start_tm[ch] = 0;
                    charge_Info.start_time[ch] = 0;//need dw

                    return 1;
                }
            }
            else
            {
                //printf("rec port num error\r\n");
                printf("%s rec port num error=%d %d\r\n",__func__,ch,pFrame->data[0]);
                return 2;
            }

        }
        else if(charge_mode == CHARGE_MODE_BROKER )
        {
            charge_Info.dc_port_status[ch] = PORT_USEING;
            charge_Info.max_watter[ch] = max_w;
            charge_Info.min_watter[ch] = min_w;
            charge_Info.charge_tm[ch] = charge_tm;
            charge_Info.trickle_tm[ch] = trickle_tm;

            charge_Info.charge_left_tm[ch] = charge_tm;
            charge_Info.tck_left_tm[ch] = trickle_tm;
            charge_Info.tck_start_tm[ch] = 0;
            charge_Info.start_time[ch] = RTC_GetCounter();//need dw
            charge_Info.port_report_type[ch] = REPORT_TYPE_MAX;

            return 0;
        }
		else
		{
			return 3;
		}


    }
    else
    {
        charge_Info.dc_port_status[ch] =  PORT_FAULT;
        charge_Info.max_watter[ch] = 0;
        charge_Info.min_watter[ch] = 0;
        charge_Info.charge_tm[ch] = 0;
        charge_Info.trickle_tm[ch] = 0;

        charge_Info.charge_left_tm[ch] = 0;
        charge_Info.tck_left_tm[ch] = 0;
        charge_Info.tck_start_tm[ch] = 0;
        charge_Info.start_time[ch] = 0;//need dw

        return 3;
    }

#endif
}
void reset_dc_port_para(uint8_t index)
{
    charge_Info.dc_port_status[index] = PORT_FREE;
    charge_Info.max_watter[index] = 0;
    charge_Info.min_watter[index] = 0;
    charge_Info.charge_tm[index] = 0;
    charge_Info.trickle_tm[index] = 0;

    charge_Info.charge_cur_pow[index] = 0;
    charge_Info.charge_left_tm[index] = 0;
    charge_Info.tck_left_tm[index] = 0;
    charge_Info.start_time[index] = 0;
    charge_Info.tck_start_tm[index] = 0;
}


uint8_t charge_mg_set_off(uint8_t ch)
{
#if 1
    int ret =0;
    int i =0;
    DianChuan_Frame *pFrame = &DianChuanRxFrame;

    if( ch >= CHARGE_NUM )
        return 1;
	PRINT_DEBUG("ret=%d set %d off\r\n",ret,ch);
    if(charge_mode == CHARGE_MODE_DC )
        ret = dc_stop_power((ch+1));
    else
        ret = app_rs485_broker_stop_power((ch+1));
    

    if(ret == 0)
    {
        if(charge_mode == CHARGE_MODE_DC )
        {
            printf("pFrame->sop=0x%02x\r\n",pFrame->sop);
            printf("pFrame->len=0x%02x\r\n",pFrame->len);
            printf("pFrame->cmd=0x%02x\r\n",pFrame->cmd);
            printf("pFrame->session_id:");
            for(i=0; i<6; i++)
                printf("session_id[%d]=0x%02x ",i,pFrame->session_id[i]);
            printf("\r\n");
            printf("pFrame->data:");
            for(i=0; i<(pFrame->len-8); i++)
                printf("data[%d]=0x%02x ",i,pFrame->data[i]);
            printf("\r\n");
            printf("pFrame->sum=0x%02x\r\n",pFrame->sum);


            if((ch+1) ==pFrame->data[0] )
            {
                printf("left time=%d\r\n",(pFrame->data[1]<<8)|pFrame->data[2]);


                reset_dc_port_para(ch);
                return 0;

            }
            else
            {
                printf("%s rec port num error=%d %d\r\n",__func__,ch,pFrame->data[0]);
                return 1;
            }
        }
        else if(charge_mode == CHARGE_MODE_BROKER )
        {
            reset_dc_port_para(ch);
			return 0;
        }
//        printf("\r\n");

    }
    else
    {
        printf("%s close error=%d\r\n",__func__,ch);
    }
    return 1;
#else
    int ret =0;
    if( ch >= CHARGE_NUM )
        return 1;
    ret = dc_stop_power(ch);
    PRINT_DEBUG("ret=%d set %d off\r\n",ret,ch);
    if(ret == 0)
        //if(cut_wire_user_set_sw(ch, 0) == TRUE)
    {
        charge_Info.sta[ch] = CHARGE_NONE;
        charge_Info.valid_tm[ch] = 0;
        charge_Info.sta_tm[ch] = 0;
        charge_Info.dev_tck_tm[ch] = 0;

        return 0;
    }

    return 1;
#endif
}


