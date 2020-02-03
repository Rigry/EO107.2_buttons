#pragma once

#include "string_buffer.h"
#include "hd44780.h"
#include "select_screen.h"
#include "set_screen.h"
#include "screens.h"
#include "button.h"
#include "encoder_rotary.h"

template<class Pins, class Flash_data, class Regs, size_t qty_lines = 4, size_t size_line = 20>
struct Menu : TickSubscriber {
   String_buffer lcd {};
   HD44780& hd44780 {HD44780::make(Pins{}, lcd.get_buffer())};
   Encoder& encoder;
   Button_event& up;
   Button_event& down;
   Button_event& enter;
   Flash_data&   flash;
   Regs& modbus_master_regs;


   Screen* current_screen {&main_screen};
   size_t tick_count{0};

   Buttons_events buttons_events {
        Up_event    {[this](auto c){encoder.set_plus_callback(c);}}
      , Down_event  {[this](auto c){encoder.set_minus_callback(c);}}
      , Enter_event {[this](auto c){enter.set_click_callback(c);}}
      , Out_event   {[this](auto c){enter.set_long_push_callback(c);}}
      , Increment_up_event   {[this](auto c){  up.set_increment_callback(c);}}
      , Increment_down_event {[this](auto c){down.set_increment_callback(c);}}
   };

   Menu (
        Pins pins
      , Encoder& encoder
      , Button_event& up
      , Button_event& down
      , Button_event& enter
      , Flash_data&   flash
      , Regs& modbus_master_regs
   ) : encoder{encoder}, up{up}, down{down}, enter{enter}
      , flash{flash}, modbus_master_regs{modbus_master_regs}
   {
      tick_subscribe();
      current_screen->init();
      while(not hd44780.init_done()){}
   }

   Main_screen main_screen {
          lcd, buttons_events
      //   , Enter_event  { [this](auto c){enter.set_click_callback(c);}}
        , Out_callback { [this]{ change_screen(main_select); }}
        , modbus_master_regs.frequency
        , modbus_master_regs.work_frequency
        , modbus_master_regs.temperatura
   };

    Select_screen<5> main_select {
          lcd, buttons_events
        , Out_callback          { [this]{change_screen(main_screen);  }}
        , Line {"Параметры"      ,[this]{change_screen(option_select);}}
        , Line {"Настройка"      ,[this]{ if (flash.search)
                                             change_screen(select_tune_end);
                                          else
                                             change_screen(select_tune_begin);  }}
        , Line {"Режим работы"   ,[this]{change_screen(mode_set);     }}
        , Line {"Конфигурация"   ,[this]{change_screen(config_select);}}
        , Line {"Аварии"         ,[this]{change_screen(alarm_select); }}
   };

   Select_screen<4> option_select {
          lcd, buttons_events
        , Out_callback    {       [this]{ change_screen(main_select);    }}
        , Line {"Mощность"       ,[this]{ change_screen(duty_cycle_set); }}
        , Line {"Макс. ток"      ,[this]{ change_screen(max_current_set);}}
        , Line {"Рабочая частота",[this]{ change_screen(frequency_set);  }}
        , Line {"Температура"    ,[this]{ change_screen(temp_select);    }}
   };

   uint8_t mode_ {flash.m_control};
   Set_screen<uint8_t, mode_to_string> mode_set {
        lcd, buttons_events
      , "Выбор режима"
      , ""
      , mode_
      , Min<uint8_t>{0}, Max<uint8_t>{::mode.size() - 1}
      , Out_callback    { [this]{ change_screen(main_select); }}
      , Enter_callback  { [this]{ 
         flash.m_control = mode_;
            change_screen(main_select);
      }}
   };

   Select_screen<2> select_tune_begin {
          lcd, buttons_events
        , Out_callback    {        [this]{ change_screen(main_select);    }}
        , Line {"Режим настройки" ,[this]{ change_screen(tune_set);       }}
        , Line {"Начать"          ,[this]{ flash.search = true;
                                           change_screen(main_screen);    }}
   };

   Select_screen<2> select_tune_end {
          lcd, buttons_events
        , Out_callback    {        [this]{ change_screen(main_select);    }}
        , Line {"Режим настройки" ,[this]{ change_screen(tune_set);       }}
        , Line {"Закончить"       ,[this]{ flash.search = false;
                                           change_screen(main_screen);    }}
   };
   
   uint8_t tune_ {flash.m_search};
   Set_screen<uint8_t, tune_to_string> tune_set {
        lcd, buttons_events
      , "Настройка"
      , ""
      , tune_
      , Min<uint8_t>{0}, Max<uint8_t>{::tune.size() - 1}
      , Out_callback    { [this]{ if (flash.search)
                                    change_screen(select_tune_end);
                                  else
                                    change_screen(select_tune_begin); }}
      , Enter_callback  { [this]{ 
         flash.m_search = tune_;
            if (flash.search)
               change_screen(select_tune_end);
            else
               change_screen(select_tune_begin);
      }}
   };
   
   uint8_t power{flash.power};
   Set_screen<uint8_t> duty_cycle_set {
        lcd, buttons_events
      , "Мощность от ном."
      , " %"
      , power
      , Min<uint8_t>{0_percent}, Max<uint8_t>{100_percent}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
         flash.power = power;
            change_screen(option_select); }}
   };

   uint16_t max_current{flash.max_current};
   Set_screen<uint16_t> max_current_set {
        lcd, buttons_events
      , "Допустимый ток."
      , " mA"
      , max_current
      , Min<uint16_t>{0_mA}, Max<uint16_t>{12000_mA}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
         flash.max_current = max_current;
            change_screen(option_select); }}
   };

   uint16_t work_frequency{flash.work_frequency};
   Set_screen<uint16_t> frequency_set {
        lcd, buttons_events
      , "Рабочая частота"
      , " Гц"
      , work_frequency
      , Min<uint16_t>{18_kHz}, Max<uint16_t>{45_kHz}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
         flash.work_frequency = work_frequency;
            change_screen(option_select); }}
   };

   Select_screen<3> temp_select {
          lcd, buttons_events
        , Out_callback    {      [this]{ change_screen(main_select);     }}
        , Line {"Текущая"       ,[this]{ change_screen(temp_show);  }}
        , Line {"Максимальная"  ,[this]{ change_screen(temp_set);;  }}
        , Line {"Восстановления",[this]{ change_screen(temp_recow);;  }}
   };

   Temp_show_screen temp_show {
        lcd, buttons_events
      , Out_callback  { [this]{ change_screen(temp_select); }}
      , "Текущая темпер."
      , " С"
      , modbus_master_regs.temperatura
   };
   
   uint8_t temperatura{flash.temperatura};
   Set_screen<uint8_t> temp_set {
        lcd, buttons_events
      , "Температ. откл."
      , " С"
      , temperatura
      , Min<uint8_t>{0}, Max<uint8_t>{100}
      , Out_callback    { [this]{ change_screen(temp_select); }}
      , Enter_callback  { [this]{ 
         flash.temperatura = temperatura;
            change_screen(temp_select); }}
   };

   uint8_t recovery{flash.recovery};
   Set_screen<uint8_t> temp_recow {
        lcd, buttons_events
      , "Температ. вкл."
      , " С"
      , recovery
      , Min<uint8_t>{0}, Max<uint8_t>{100}
      , Out_callback    { [this]{ change_screen(temp_select); }}
      , Enter_callback  { [this]{ 
         flash.recovery = recovery;
            change_screen(temp_select); }}
   };

   Select_screen<2> alarm_select {
          lcd, buttons_events
        , Out_callback    { [this]{ change_screen(main_select);  }}
        , Line {"Посмотреть",[]{}}
        , Line {"Сбросить"  ,[this]{/*generator.flags.no_load = generator.flags.overload = false;
                                    change_screen(main_select); */}}

   };

   Select_screen<2> config_select {
          lcd, buttons_events
        , Out_callback    { [this]{change_screen(main_select);     }}
        , Line {"Девиация" ,[this]{change_screen(deviation_select);}}
        , Line {"Booster" ,[]{}}
   };

   Select_screen<4> deviation_select {
          lcd, buttons_events
        , Out_callback        { [this]{change_screen(config_select);}}
        , Line {"Вкл/Откл"     ,[this]{change_screen(deviation_set);}}
        , Line {"Диапазон"     ,[this]{change_screen(range_deviation_set);}}
        , Line {"Кол-во изм."  ,[this]{change_screen(qty_changes_set);}}
        , Line {"Время изм."   ,[this]{change_screen(time_set);}}
   };

   uint8_t deviation {flash.deviation};
   Set_screen<uint8_t, deviation_to_string> deviation_set {
        lcd, buttons_events
      , "Вкл/Откл"
      , ""
      , deviation
      , Min<uint8_t>{0}, Max<uint8_t>{::deviation.size() - 1}
      , Out_callback    { [this]{ change_screen(deviation_select); }}
      , Enter_callback  { [this]{ 
         flash.deviation = deviation;
            change_screen(deviation_select);
      }}
   };

   uint16_t range_deviation{flash.range_deviation};
   Set_screen<uint16_t> range_deviation_set {
        lcd, buttons_events
      , "Размер диапазона"
      , " Гц"
      , range_deviation
      , Min<uint16_t>{0}, Max<uint16_t>{4000}
      , Out_callback    { [this]{ change_screen(deviation_select); }}
      , Enter_callback  { [this]{ 
         flash.range_deviation = range_deviation;
            change_screen(deviation_select); }}
   };

   uint8_t qty_changes{flash.qty_changes};
   Set_screen<uint8_t> qty_changes_set {
        lcd, buttons_events
      , "Кол-во изменений"
      , " "
      , qty_changes
      , Min<uint8_t>{2}, Max<uint8_t>{100}
      , Out_callback    { [this]{ change_screen(deviation_select); }}
      , Enter_callback  { [this]{ 
         flash.qty_changes = qty_changes;
            change_screen(deviation_select); }}
   };

   uint16_t time{flash.time};
   Set_screen<uint16_t> time_set {
        lcd, buttons_events
      , "Время изменений"
      , " мс"
      , time
      , Min<uint16_t>{100}, Max<uint16_t>{2000}
      , Out_callback    { [this]{ change_screen(deviation_select); }}
      , Enter_callback  { [this]{ 
         flash.time = time;
            change_screen(deviation_select); }}
   };

   void notify() override {
      every_qty_cnt_call(tick_count, 50, [this]{
         current_screen->draw();
      });
   }

   void change_screen(Screen& new_screen) {
      current_screen->deinit();
      current_screen = &new_screen;
      current_screen->init();
   }

};