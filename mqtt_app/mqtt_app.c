//#include "includes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <stdio.h>
#include <time.h>

#include "project_config.h"
#include "cjson_middleware.h"

#include "mqtt_app.h"
#include "transport.h"



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#if 0
typedef struct {
    bool vaild;
    uint8_t recv_data[NET_DATA_LEN];
    uint16_t read_pos;
    uint16_t recv_len;
} Net_recv_buf;

typedef struct {
    bool trig;
    uint8_t toggle_cnt;
} Led_trig_st;

Led_trig_st led_trig_st = {FALSE, 0};
#define MQTT_LED_TRIGG()        {led_trig_st.trig = TRUE; led_trig_st.toggle_cnt = 4;}

#endif

uint8_t module4g_init =0;

/* Private macro -------------------------------------------------------------*/
/*
  0:开关控制
  1:获取/设置参数
  2:升级
  */
char sub_topics[3][50];

/*0:定时上报状态
  1:插座警报--废弃
  2:充电站警报
  3:上线通知
  4:充电完成报告*/
char pub_topics[5][50];

char client_id[16];

uint8_t ping_tm_out_cnt = 0;
uint32_t ping_req_tm = 0;
uint8_t mqtt_core_buff[NET_DATA_LEN];//mqtt net data
uint8_t mqtt_user_buff[JSON_DATA_LEN];//json
Mqtt_sta mqtt_sta = MQTT_DISCON;
//Net_recv_buf net_recv_buf;

//Dev_info dev_info;

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/




void mqtt_set(char *id)
{
    // strcpy(client_id, id);
    memcpy(client_id, id,16);
    sprintf(sub_topics[0], TOPIC_SUB_SER_TO_LINK_SET_SW, client_id);
    sprintf(sub_topics[1], TOPIC_SUB_SER_TO_LINK_SET_PAR, client_id);
    sprintf(sub_topics[2], TOPIC_SUB_SER_TO_LINK_SET_UPDATE, client_id);

    sprintf(pub_topics[0], TOPIC_PUB_LINK_TO_SER_TYPE_REPORT, client_id);
   // sprintf(pub_topics[1], TOPIC_PUB_LINK_TO_SER_TYPE_ADP, client_id);
    sprintf(pub_topics[2], TOPIC_PUB_LINK_TO_SER_TYPE_STA, client_id);
    sprintf(pub_topics[3], TOPIC_PUB_LINK_TO_SER_ONLINE, client_id);
    sprintf(pub_topics[4], TOPIC_PUB_LINK_TO_SER_REPORT, client_id);

    return;
}

int mqtt_connack_pro(uint8_t *buf, int buflen)
{
    unsigned char sessionPresent, connack_rc;

    if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0)
    {
        printf("Unable to connect, return code %d\r\n", connack_rc);
        return -1;
    }
    return 0;
}



int mqtt_publish(char *pTopic, uint8_t qos, uint8_t *data, uint16_t data_len)
{
    MQTTString topicString = MQTTString_initializer;
    static unsigned short packet_id = 0;
    unsigned char *buf = mqtt_core_buff;
    int buflen = sizeof(mqtt_core_buff);
    int mqtt_tcp_socket =get_mqtt_transport_sock();
    int len;
    int rc = -1;
    if(mqtt_sta != MQTT_OK)
        return -1;

    packet_id++;
    topicString.cstring = pTopic;
    qos = 1;
    len = MQTTSerialize_publish(buf, buflen, 0, qos, 0, packet_id, topicString, (unsigned char*)data, data_len);

    printf("pTopic:%s len=%d\r\n",pTopic,len);
    //net_msg_queue_add((uint8_t *)buf, len);
    rc = transport_sendPacketBuffer(mqtt_tcp_socket, buf, len);


    return rc;
}

int mqtt_link_pub_online(void)
{
    //char online_cmd[200];
    int rc =-1;
#if 0
    int json_len=sizeof(json_buff);
    char* pjson_buff=json_buff;

    memset(json_buff,0,json_len);
#else
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(mqtt_user_buff,0,json_len);
#endif
    if(mqtt_sta != MQTT_OK)
        return -1;

    printf("mqtt_link_pub_online\r\n");
    json_len = json_online_cmd(pjson_buff,json_len);
    if(json_len  > 0)
    {
        printf("cmd_len: %d\r\n", json_len);
        printf("cur json_buff is: %s \r\n", pjson_buff);

    }
    else
        printf("ret: %d\r\n", json_len);

    if( json_len > 0)
        rc=mqtt_publish(pub_topics[3], 2, (uint8_t *)pjson_buff, json_len);


    return rc ;
}

void mqtt_ping_pro(void)
{
    static uint32_t bef_ping_tm;
    uint32_t sys_ms_cnt;
    uint8_t buff[10];
    int len;

    if(mqtt_sta != MQTT_OK)
        return;
    sys_ms_cnt = get_curtime();
    if((ping_tm_out_cnt != 0) && ((sys_ms_cnt - ping_req_tm) > 5000))
    {
        if((sys_ms_cnt - bef_ping_tm) < PING_CONFIRM_TM )
            return;
    } else {
        if((sys_ms_cnt - bef_ping_tm) < (charge_Info.ping_tm * 1000))
            return;
    }

    bef_ping_tm = sys_ms_cnt;
    len = MQTTSerialize_pingreq(buff, sizeof(buff));
    ping_tm_out_cnt++;
    if(ping_tm_out_cnt >= PING_TM_OUT_MAX)
    {
        mqtt_sta = MQTT_DISCON;
        //sim800c_msg_release();
        ping_tm_out_cnt = 0;
        //net_msg_init();
        printf("ping time out\r\n");
        return;
    }

    //printf("ping\r\n");
    //net_msg_queue_add((uint8_t *)buff, len);

    return;
}






int mqtt_updata_pro(void)
{
//    static uint32_t bef_sec;
//    uint32_t now_sec;
    int len;

    int rc =-1;
    if(mqtt_sta != MQTT_OK)
        return -10;

#if 0
    int json_len=sizeof(json_buff);
    char* pjson_buff=json_buff;

    memset(json_buff,0,json_len);
#else
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;
    memset(pjson_buff,0,json_len);
#endif
    //now_sec = RTC_GetCounter();
    //if((now_sec - bef_sec) >= charge_Info.updata_tm)
    //{
    // bef_sec = now_sec;
    printf("mqtt_updata_pro\r\n");
    print_heap_info();
    len = json_updata_cut_wire_msg( pjson_buff );
    print_heap_info();
    printf("json_updata_cut_wire_msg=%d\r\n",len);
    if(len > 0)
    {
        rc = mqtt_publish(pub_topics[0], 2, (uint8_t *)pjson_buff, len);
        printf("mqtt_publish %d\r\n",rc);
        if(rc != 0)
            return rc;
        else
            return 0;
    }
    else
    {
        printf("mqtt_updata_pro fail\r\n");
        return -13;
    }
    //}

}
#if 0
void mqtt_pub_full_charge_ok(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch, mqtt_user_buff, REPORT_TYPE_LESS_MIN_W);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}

void mqtt_pub_(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch,mqtt_user_buff, REPORT_TYPE_LESS_MIN_W);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}
void mqtt_pub_sw_finish(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;
    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch, mqtt_user_buff, REPORT_TYPE_CHARG_FINISH);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}

#endif
void mqtt_pub_station_warning(Sta_warning warn)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;
    memset(pjson_buff,0,json_len);

    len = json_station_warning_notify(warn, mqtt_user_buff);
    if(len > 0)
    {
        mqtt_publish(pub_topics[2], 2, mqtt_user_buff, len);
    }
}


//void mqtt_pub_sw_report(uint8_t * set_ch,uint8_t * sta,uint8_t sw_num)

void mqtt_pub_sw_report(uint8_t * set_ch,Report_type * sta,uint8_t sw_num)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_ctrl_report(mqtt_user_buff, set_ch, sta, sw_num);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}
#if 0
void mqtt_pub_max_watter(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch, mqtt_user_buff, REPORT_TYPE_MORE_MAX_W);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}

void mqtt_pub_no_insert(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch, mqtt_user_buff, REPORT_TYPE_NO_INSERT);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}

void mqtt_pub_pull_out(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch,mqtt_user_buff, REPORT_TYPE_PULL_OUT);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }
}

void mqtt_pub_min_zero_pull_out(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch,mqtt_user_buff, REPORT_TYPE_MIN_ZERO_PULL_OUT);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }

    mqtt_pub_pull_out(ch);
}

void mqtt_pub_run_zero_pull_out(uint8_t ch)
{
    int len;
    int json_len=sizeof(mqtt_user_buff);
    uint8_t* pjson_buff=mqtt_user_buff;

    memset(pjson_buff,0,json_len);

    len = json_sw_change_notify(ch, mqtt_user_buff, REPORT_TYPE_RUN_ZERO_PULL_OUT);
    if(len > 0)
    {
        mqtt_publish(pub_topics[4], 2, mqtt_user_buff, len);
    }

    mqtt_pub_pull_out(ch);
}

#endif

void mqtt_recv_publish_pro(char *topic, uint8_t *data, uint16_t len)
{
    json_parse((char *)data, topic);
}

void mqtt_publish_packet_pro(uint8_t *buf, int buflen)
{
    unsigned char dup;
    int qos;
    unsigned char retained;
    unsigned short msgid;
    int payloadlen_in;
    unsigned char* payload_in;
    MQTTString receivedTopic;

    MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                            &payload_in, &payloadlen_in, buf, buflen)    ;

    printf("publish:");
    // print_uart_poll_send_data((uint8_t *)receivedTopic.lenstring.data, receivedTopic.lenstring.len);
    printf(",dup:%d,qos:%d,retained:%d, msgid:%d, datalen:%d, data:\r\n",dup, qos, retained, msgid, payloadlen_in);
    // print_uart_poll_send_data(payload_in, payloadlen_in);
    printf("\r\n");

#if 0
    {
        uint16_t i;
        for(i = 0; i < payloadlen_in; i++)
        {
            PRINT_DEBUG("%02X ",payload_in[i]);
        }
        PRINT_DEBUG("\n");
    }
#endif

    if(qos == 2)
    {
        /*send publish rec packet*/
        buf[0] = 0x50;
        buf[1] = 0x02;
        buf[2] = (msgid >> 8) & 0xff;
        buf[3] = msgid  & 0xff;

        //net_msg_queue_add(buf, 4);
        printf("qos == 2 send PUBREC\r\n");
    }
    else if(qos == 1)
    {
        int len;

        len = MQTTSerialize_puback(buf, buflen,msgid);
        //net_msg_queue_add(buf, len);
        printf("qos == 1 send PUBACK\r\n");
    }

    mqtt_recv_publish_pro(receivedTopic.lenstring.data, payload_in, payloadlen_in);
}

void mqtt_msg_proc(void)
{
    enum msgTypes recv_type;
    int buflen = sizeof(mqtt_core_buff);
    unsigned char *buf = mqtt_core_buff;

//    if( net_recv_buf.vaild == FALSE)
    //    return;

//    recv_type =(enum msgTypes)MQTTPacket_read(buf, buflen, socket_get_data);
    if( recv_type == PUBLISH)
    {
        mqtt_publish_packet_pro(buf, buflen);
    } else if( recv_type == CONNECT)
    {
        printf("recv CONNECT\r\n");
    } else if( recv_type == CONNACK)
    {
        printf("recv CONNACK\r\n");
        if((mqtt_sta == MQTT_CON) && (mqtt_connack_pro(buf, buflen) == 0))
        {
            mqtt_sta = MQTT_CON_ACK;
            printf("mqtt_connack_pro success\r\n");
        }
    } else if( recv_type == PUBACK)
    {
        printf("recv PUBACK\r\n");
    } else if( recv_type == PUBREC)
    {
        unsigned short msg_id;
        int len;

        msg_id = (buf[2]  << 8) |buf[3] ;

        printf("recv PUBREC, msg_id:%d\r\n", msg_id);
        len = MQTTSerialize_pubrel(buf, buflen, 0, msg_id);
        //net_msg_queue_add(buf, len);
    }
    else if( recv_type == PUBREL)
    {
        unsigned short packetid;
        int len;

        printf("recv PUBREL\r\n");

        packetid = (buf[2] << 8) |buf[3];
        len =  MQTTSerialize_pubcomp(buf,sizeof(buf), packetid);
        // net_msg_queue_add(buf, len);
#if 0
        if(transport_sendPacketBuffer((char *)buf, len) == len)
            printf("send PUBCOM sucess\r\n");
        else
            printf("send PUBCOM fail\r\n");
#endif

    } else if( recv_type == PUBCOMP)
    {
        printf("recv PUBCOMP\r\n");
    } else if( recv_type == SUBSCRIBE)
    {
        printf("recv SUBSCRIBE\r\n");

    } else if( recv_type == SUBACK)
    {
        printf("recv SUBACK\r\n");
       // if((mqtt_sta == MQTT_SUB) && (mqtt_suback_pro(buf, buflen)== 0))
        {
            mqtt_sta = MQTT_OK;
            printf("sub ack success\r\n");
            mqtt_link_pub_online();
        }
    } else if( recv_type == UNSUBSCRIBE)
    {
        printf("recv UNSUBSCRIBE\r\n");
    } else if( recv_type == UNSUBACK)
    {
        printf("recv UNSUBACK\r\n");
    } else if( recv_type == PINGREQ)
    {
        printf("recv PINGREQ\r\n");
    } else if( recv_type == PINGRESP)
    {
        ping_req_tm = get_curtime();;
        ping_tm_out_cnt = 0;
        //printf("recv PINGRESP\r\n");
    } else if( recv_type == DISCONNECT)
    {
        printf("recv DISCONNECT\r\n");
    } else
    {
        printf("recv other\r\n");
    }

}

void mqtt_led_dir(void)
{
    if(mqtt_sta == MQTT_OK)
    {
        //LED_NET_DIR_CON();
    } else {
        //LED_NET_DIR_DIS_CON();
    }
}


/*
void mqtt_process(void)
{
    //mqtt_connect_pro();
    mqtt_subscibe_pro();
    mqtt_ping_pro();
    mqtt_msg_proc();
    mqtt_updata_pro();
}

*/


