#include "config.h"
/*****************************************************************************************************************/
void setup()
{
  // gpio_hold_dis(GPIO_NUM_32);                             //解锁电源引脚
  // gpio_deep_sleep_hold_dis();                             //禁止深度睡眠中引脚锁定
  // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0); //禁用EXT0唤醒源
  delay(1000);
  f_hardware_init(); //硬件初始化
  f_software_init(); //软件初始化
  f_if_wakeup_mode();
  wakeup_init_time(); //开机更新

  xTaskCreatePinnedToCore(xieyi_Task, "xieyi_Task", 3000, NULL, 2, &xieyi_task, tskNO_AFFINITY); //创建DS1302任务
  xTaskCreatePinnedToCore(ds1302_task, "ds1302_task", 1000, NULL, 2, &ds_task, tskNO_AFFINITY);  //创建DS1302任务
  xTaskCreatePinnedToCore(codeForTask1, "task1", 3000, NULL, 2, &task1, tskNO_AFFINITY);

  // if (rollback)
  // {
  //   /*************如果rollback置1, 会恢复出厂设置,数据全清***********/
  //   Serial.println("clean EEPROM");
  //   EEPROM.write(1, 0);
  //   EEPROM.commit();
  //   Serial.println("OK");
  //   ESP.deepSleep(300000000);
  // }

  if (oledState == OLED_ON)
  {
    showWelcome();
    Serial.printf("ok\n");
    //modem.begin();
    //modem.sleepEnable(0);
    screen_loopEnabled = true;
    postMsgId = 0; //清记录条数
  }
}

/*****************************************************************************************************************/
void loop()
{
  //电源管理
  f_power_management();

  if (oledState == OLED_ON)
  {
    getTempAndHumilocation(); //获取温湿度和经纬度
    //Serial.printf("temp_alarm:%d,tempUA:%.2f,tempLA:%.2f\n", temp_alarm, tempU, tempL);
    if (temp_alarm)           //检测温度阈值，更新安全状态
    {
      if ((tp > tempU) || (tp < tempL)){
        F_alarm_status=0;   
      }
      else{
        F_alarm_status=1;
      }
    //Serial.printf("F_alarm_status====%d\n",F_alarm_status);
    digitalWrite(LED,F_alarm_status);
    }
    else {
      //qualifiedState = true;
      F_alarm_status=1;
    }
    
    screen_loop();
    key_loop();
    screen_show();                                                //OLED最终显示
    if (now_unixtime - P_sleep_last_time >= period && !f_MQTT_ON) //记录间隔到了吗？
    {
      send_Msg_var_GSM_while_OLED_on();
      screen_loopEnabled=true;
    }
  }
  //客户端扫描

  if (f_MQTT_ON)
  {
    Serial.printf("f_MQTT_ON=%d\n", f_MQTT_ON);
    client.loop();
  }
  else
    oled_on_off_switch();
}










/**************************************************************
                        //任务
***************************************************************/
/*----------------------按键检测任务---------------------------*/
void codeForTask1(void *parameter)
{
  while (1) //这是核1 的loop
  {
    vTaskDelay(100);
    button.tick(); //扫描按键
    button2.tick();
    //Serial.printf("key_stete=%d\n", keyState);
  }
  vTaskDelete(NULL);
}

/*----------------------ds1302任务----------------------------*/
void ds1302_task(void *parameter)
{
  uint8_t sec = 0;
  while (1)
  {
    ds_rtc.getDateTime(&now1); //读取时间参数到NOW
    now_unixtime = unixtime();
    if (now1.second == sec + 1)
    {
      sys_sec++;
      // Serial.printf("sec:%d\n",sys_sec);
    }
    sec = now1.second;
    vTaskDelay(100);
  }
  vTaskDelete(NULL);
}

/*-----------------------通讯协议任务----------------------------*/

void xieyi_Task(void *parameter)
{
  while (1) //这是核1 的loop
  {
    xieyi_scan();
    vTaskDelay(200);
  }
  vTaskDelete(NULL);
}
