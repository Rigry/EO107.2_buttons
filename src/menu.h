#pragma once

#include "string_buffer.h"
#include "hd44780.h"
#include "select_screen.h"
#include "set_screen.h"
#include "screens.h"
#include "button.h"

template<class Pins, class Flash_data, class Regs, class Flags, /*class UART,*/ size_t qty_lines = 4, size_t size_line = 20>
struct Menu : TickSubscriber {
   String_buffer lcd {};
   HD44780& hd44780 {HD44780::make(Pins{}, lcd.get_buffer())};
   Button_event& up;
   Button_event& down;
   Button_event& enter;
   Flash_data&   flash;
   Regs& modbus_master_regs;
   Flags& flags_03;
   Flags& flags_16;
   // UART& uart_set_03;
   // UART& uart_set_16;
   ADC_& adc;


   Screen* current_screen {&main_screen};
   size_t tick_count{0};

   Buttons_events buttons_events {
        Up_event    {[this](auto c){   up.set_click_callback(c);}}
      , Down_event  {[this](auto c){ down.set_click_callback(c);}}
      , Enter_event {[this](auto c){enter.set_click_callback(c);}}
      , Out_event   {[this](auto c){enter.set_long_push_callback(c);}}
      , Increment_up_event   {[this](auto c){  up.set_increment_callback(c);}}
      , Increment_down_event {[this](auto c){down.set_increment_callback(c);}}
   };

   Menu (
        Pins pins
      , Button_event& up
      , Button_event& down
      , Button_event& enter
      , Flash_data&   flash
      , Regs& modbus_master_regs
      , Flags& flags_03
      , Flags& flags_16
      // , UART& uart_set_03
      // , UART& uart_set_16
      , ADC_& adc
   ) : up{up}, down{down}, enter{enter} 
      , flash{flash}, modbus_master_regs{modbus_master_regs}
      , flags_03{flags_03}, flags_16{flags_16}
      /*, uart_set_03{uart_set_03}, uart_set_16{uart_set_16}*/, adc{adc}
   {
      tick_subscribe();
      current_screen->init();
      while(not hd44780.init_done()){}
   }

   Main_screen<Regs, Flags> main_screen {
          lcd, buttons_events
      //   , Enter_event  { [this](auto c){enter.set_click_callback(c);}}
        , Out_callback { [this]{ change_screen(main_select); }}
        , modbus_master_regs
        , flags_03
        , flags_16
        , adc
        , flash.every_degree
   };

    Select_screen<4> main_select {
          lcd, buttons_events
        , Out_callback          { [this]{change_screen(main_screen);   modbus_master_regs.flags_16.disable = true;}}
        , Line {"Параметры"      ,[this]{change_screen(option_select); modbus_master_regs.power_03.disable = false;
                                                                       modbus_master_regs.work_frequency_03.disable = false;
                                                                       modbus_master_regs.max_current_03.disable    = false;
                                                                       modbus_master_regs.work_current_03.disable   = false; }}
        , Line {"Режим настройки",[this]{change_screen(tune_set);     }}
        , Line {"Режим работы"   ,[this]{change_screen(mode_set);     }}
        , Line {"Конфигурация"   ,[this]{change_screen(config_select);}}
      //   , Line {"Аварии"         ,[this]{change_screen(alarm_select); }}
      //   , Line {"Настройки сети" ,[this]{change_screen(modbus_select); modbus_master_regs.modbus_address_03.disable = false;
      //                                                                  modbus_master_regs.uart_set_03.disable = false;}}
   };

   uint8_t power{0};
   uint16_t work_frequency{0};
   uint16_t max_current{0};
   uint16_t work_current{0};
   Select_screen<7> option_select {
          lcd, buttons_events
        , Out_callback    {       [this]{ modbus_master_regs.power_16.disable = true; 
                                          modbus_master_regs.power_03.disable = true;
                                          modbus_master_regs.work_frequency_16.disable = true;
                                          modbus_master_regs.work_frequency_03.disable = true;
                                          modbus_master_regs.max_current_16.disable    = true;
                                          modbus_master_regs.max_current_03.disable    = true;   
                                          modbus_master_regs.work_current_16.disable   = true;
                                          modbus_master_regs.work_current_03.disable   = true;   change_screen(main_select);    }}
        , Line {"Mощность"       ,[this]{ power = modbus_master_regs.power_03;                   change_screen(duty_cycle_set); }}
        , Line {"Рабочий ток"    ,[this]{ work_current = modbus_master_regs.work_current_03;     change_screen(work_current_set);}}
        , Line {"Макс. ток"      ,[this]{ max_current = modbus_master_regs.max_current_03;       change_screen(max_current_set);}}
        , Line {"Рабочая частота",[this]{ work_frequency = modbus_master_regs.work_frequency_03; change_screen(frequency_set);  }}
        , Line {"Темп.генератора",[this]{ modbus_master_regs.max_temp_03.disable      = false;
                                          modbus_master_regs.recovery_temp_03.disable = false;   change_screen(temp_select);    }}
        , Line {"Ослабление тока",[this]{ change_screen(attenuation_select); }}
        , Line {"Уставка"        ,[this]{ change_screen(set_point); }}
   };

   bool mode_ {flash.m_control};
   Set_screen<bool, mode_to_string> mode_set {
        lcd, buttons_events
      , "Выбор режима"
      , ""
      , mode_
      , Min<bool>{0}, Max<bool>{::mode.size() - 1}
      , Out_callback    { [this]{ change_screen(main_select); }}
      , Enter_callback  { [this]{ 
            flash.m_control = flags_16.manual = mode_;
            modbus_master_regs.flags_16.disable = false;
            modbus_master_regs.frequency_16.disable = flags_16.manual ? false : true;
            modbus_master_regs.frequency_16 = modbus_master_regs.frequency_03;
            change_screen(main_select);
      }}
   };

   bool tune_ {flash.m_search};
   Set_screen<bool, mode_to_string> tune_set {
        lcd, buttons_events
      , "Выбор режима"
      , ""
      , tune_
      , Min<bool>{false}, Max<bool>{true}
      , Out_callback    { [this]{change_screen(main_select);}}
      , Enter_callback  { [this]{ 
            flash.m_search = flags_16.manual_tune = tune_;
            modbus_master_regs.flags_16.disable = false;
            modbus_master_regs.frequency_16.disable = flags_16.manual_tune ? false : true;
            modbus_master_regs.frequency_16 = modbus_master_regs.frequency_03;
            change_screen(main_select);
      }}
   };
   
   Set_screen<uint8_t> duty_cycle_set {
        lcd, buttons_events
      , "Мощность от ном."
      , " %"
      , power
      , Min<uint8_t>{0_percent}, Max<uint8_t>{100_percent}
      , Out_callback    { [this]{ change_screen(option_select);}}
      , Enter_callback  { [this]{ 
            modbus_master_regs.power_16 = power;
            modbus_master_regs.power_16.disable = false;
            change_screen(option_select); }}
   };

   Set_screen<uint16_t> work_current_set {
        lcd, buttons_events
      , "Рабочий ток"
      , " mA"
      , work_current
      , Min<uint16_t>{0_mA}, Max<uint16_t>{10000_mA}
      , Out_callback    { [this]{ change_screen(option_select);}}
      , Enter_callback  { [this]{ 
            modbus_master_regs.work_current_16 = work_current;
            modbus_master_regs.work_current_16.disable = false;
            change_screen(option_select); }}
   };

   Set_screen<uint16_t> max_current_set {
        lcd, buttons_events
      , "Допустимый ток."
      , " mA"
      , max_current
      , Min<uint16_t>{0_mA}, Max<uint16_t>{12000_mA}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
         modbus_master_regs.max_current_16 = max_current;
         modbus_master_regs.max_current_16.disable = false;
            change_screen(option_select); }}
   };

   Set_screen<uint16_t> frequency_set {
        lcd, buttons_events
      , "Рабочая частота"
      , " Гц"
      , work_frequency
      , Min<uint16_t>{18_kHz}, Max<uint16_t>{45_kHz}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
            modbus_master_regs.work_frequency_16 = work_frequency;
            modbus_master_regs.work_frequency_16.disable = false;
            change_screen(option_select); }}
   };

   uint16_t temperatura{0};
   uint16_t recovery{0};
   Select_screen<3> temp_select {
          lcd, buttons_events
        , Out_callback    {      [this]{ modbus_master_regs.max_temp_16.disable      = true;
                                         modbus_master_regs.recovery_temp_16.disable = true; 
                                         modbus_master_regs.max_temp_03.disable      = true;
                                         modbus_master_regs.recovery_temp_03.disable = true; change_screen(main_select);}}
        , Line {"Текущая"       ,[this]{                                                     change_screen(temp_show);  }}
        , Line {"Максимальная"  ,[this]{ temperatura = modbus_master_regs.max_temp_03;       change_screen(temp_set);   }}
        , Line {"Восстановления",[this]{ recovery = modbus_master_regs.recovery_temp_03;     change_screen(temp_recow); }}
   };

   Temp_show_screen temp_show {
        lcd, buttons_events
      , Out_callback  { [this]{ change_screen(temp_select); }}
      , "Текущая темпер."
      , " С"
      , modbus_master_regs.temperatura_03
   };
   
   Set_screen<uint16_t> temp_set {
        lcd, buttons_events
      , "Температ. откл."
      , " С"
      , temperatura
      , Min<uint16_t>{0}, Max<uint16_t>{100}
      , Out_callback    { [this]{ change_screen(temp_select); }}
      , Enter_callback  { [this]{ 
            modbus_master_regs.max_temp_16 = temperatura;
            modbus_master_regs.max_temp_16.disable = false;
            change_screen(temp_select); }}
   };

   Set_screen<uint16_t> temp_recow {
        lcd, buttons_events
      , "Температ. вкл."
      , " С"
      , recovery
      , Min<uint16_t>{0}, Max<uint16_t>{100}
      , Out_callback    { [this]{ change_screen(temp_select); }}
      , Enter_callback  { [this]{ 
            modbus_master_regs.recovery_temp_16 = recovery;
            modbus_master_regs.recovery_temp_16.disable = false;
            change_screen(temp_select); }}
   };

   uint8_t every_degree{flash.every_degree};
   Set_screen<uint8_t> set_point {
        lcd, buttons_events
      , "Уставка"
      , " С"
      , every_degree
      , Min<uint8_t>{2}, Max<uint8_t>{30}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
            flash.every_degree = every_degree;
            change_screen(option_select); }}
   };

   bool attenuation_ {flash.attenuation};
   Set_screen<bool, attenuation_to_string> attenuation_select {
        lcd, buttons_events
      , "Коэфф. ослабления"
      , ""
      , attenuation_
      , Min<bool>{0}, Max<bool>{::attenuation.size() - 1}
      , Out_callback    { [this]{ change_screen(option_select); }}
      , Enter_callback  { [this]{ 
            flash.attenuation = flags_16.attenuation = attenuation_;
            modbus_master_regs.flags_16.disable = false;
            change_screen(option_select);
      }}
   };

   // Select_screen<1> alarm_select {
   //        lcd, buttons_events
   //      , Out_callback    { [this]{ change_screen(main_select);  }}
   //    //   , Line {"Посмотреть",[]{}}
   //      , Line {"Сбросить"  ,[this]{flags_16.no_load = flags_16.overload = false;
   //                                  change_screen(main_select);}}

   // };

   Select_screen<2> config_select {
          lcd, buttons_events
        , Out_callback    { [this]{ modbus_master_regs.range_deviation.disable = true;
                                    modbus_master_regs.qty_changes.disable     = true;
                                    modbus_master_regs.time.disable            = true;
                                    change_screen(main_select);    }}
        , Line {"Девиация" ,[this]{change_screen(deviation_select);}}
        , Line {"Booster"  ,[this]{change_screen(boost_select);  }}
   };

   Select_screen<4> deviation_select {
          lcd, buttons_events
        , Out_callback        { [this]{modbus_master_regs.range_deviation.disable = true;
                                       modbus_master_regs.qty_changes.disable = true;
                                       modbus_master_regs.time.disable = true;
                                       change_screen(config_select);}}
        , Line {"Вкл/Откл"     ,[this]{change_screen(deviation_set);}}
        , Line {"Диапазон"     ,[this]{change_screen(range_deviation_set);}}
        , Line {"Кол-во изм."  ,[this]{change_screen(qty_changes_set);}}
        , Line {"Время изм."   ,[this]{change_screen(time_set);}}
   };

   bool deviation {flash.deviation};
   Set_screen<bool, off_on_to_string> deviation_set {
        lcd, buttons_events
      , "Вкл/Откл"
      , ""
      , deviation
      , Min<bool>{0}, Max<bool>{::off_on.size() - 1}
      , Out_callback    { [this]{ change_screen(deviation_select); }}
      , Enter_callback  { [this]{
         modbus_master_regs.flags_16.disable = false;
         flash.deviation = flags_16.deviation = deviation;
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
         flash.range_deviation = modbus_master_regs.range_deviation = range_deviation;
         modbus_master_regs.range_deviation.disable = false;
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
         flash.qty_changes = modbus_master_regs.qty_changes = qty_changes;
         modbus_master_regs.qty_changes.disable = false;
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
         flash.time = modbus_master_regs.time = time;
         modbus_master_regs.time.disable = false;
            change_screen(deviation_select); }}
   };

   Select_screen<3> boost_select {
          lcd, buttons_events
        , Out_callback        { [this]{modbus_master_regs.work_time.disable  = true;
                                       modbus_master_regs.pause_time.disable = true;
                                       change_screen(config_select);}}
        , Line {"Вкл/Откл"     ,[this]{change_screen(boost_set);}}
        , Line {"Время работы" ,[this]{change_screen(work_time_set);}}
        , Line {"Время паузы"  ,[this]{change_screen(pause_time_set);}}
   };

   bool boost {flash.boost};
   Set_screen<bool, off_on_to_string> boost_set {
        lcd, buttons_events
      , "Вкл/Откл"
      , ""
      , boost
      , Min<bool>{0}, Max<bool>{::off_on.size() - 1}
      , Out_callback    { [this]{ change_screen(boost_select); }}
      , Enter_callback  { [this]{
         modbus_master_regs.flags_16.disable = false;
         flash.boost = flags_16.boost = boost;
            change_screen(boost_select);
      }}
   };

   uint16_t work_time{flash.work_time};
   Set_screen<uint16_t> work_time_set {
        lcd, buttons_events
      , "Время работы"
      , " мс"
      , work_time
      , Min<uint16_t>{100}, Max<uint16_t>{10000}
      , Out_callback    { [this]{ change_screen(boost_select); }}
      , Enter_callback  { [this]{ 
         flash.work_time = modbus_master_regs.work_time = work_time;
         modbus_master_regs.work_time.disable = false;
            change_screen(boost_select); }}
   };

   uint16_t pause_time{flash.pause_time};
   Set_screen<uint16_t> pause_time_set {
        lcd, buttons_events
      , "Время паузы"
      , " мс"
      , pause_time
      , Min<uint16_t>{100}, Max<uint16_t>{10000}
      , Out_callback    { [this]{ change_screen(boost_select); }}
      , Enter_callback  { [this]{ 
         flash.pause_time = modbus_master_regs.pause_time = pause_time;
         modbus_master_regs.pause_time.disable = false;
            change_screen(boost_select); }}
   };

   // uint16_t address{0};
   // uint16_t boudrate_ {0};
   // Select_screen<4> modbus_select {
   //      lcd, buttons_events
   //    , Out_callback        {     [this]{modbus_master_regs.modbus_address_03.disable = true;
   //                                       modbus_master_regs.modbus_address_16.disable = true;
   //                                       modbus_master_regs.uart_set_03.disable       = true;
   //                                       modbus_master_regs.uart_set_16.disable       = true; change_screen(main_select) ;}}
   //    , Line {"Адрес"            ,[this]{address = modbus_master_regs.modbus_address_03;      change_screen(address_set) ;}}
   //    , Line {"Скорость"         ,[this]{boudrate_ = uint16_t(uart_set_03.baudrate);          /*change_screen(boudrate_set);*/}}
   //    , Line {"Проверка на чет." ,[this]{}}
   //    , Line {"Кол-во стоп-бит"  ,[this]{}}
   // };

   // Set_screen<uint16_t> address_set {
   //      lcd, buttons_events
   //    , "Адрес modbus"
   //    , ""
   //    , address
   //    , Min<uint16_t>{1}, Max<uint16_t>{255}
   //    , Out_callback    { [this]{ change_screen(modbus_select);  }}
   //    , Enter_callback  { [this]{ 
   //          modbus_master_regs.modbus_address_16 = address;
   //          modbus_master_regs.modbus_address_16.disable = false;
   //          change_screen(modbus_select);}}
   // };

   
   // Set_screen<uint16_t, boudrate_to_string> boudrate_set {
   //      lcd, buttons_events
   //    , "Скорость в бодах"
   //    , ""
   //    , boudrate_
   //    , Min<uint16_t>{0}, Max<uint16_t>{::boudrate.size() - 1}
   //    , Out_callback    { [this]{ change_screen(modbus_select); }}
   //    , Enter_callback  { [this]{ 
   //          uart_set_16.baudrate = USART::Baudrate(boudrate_);
   //          modbus_master_regs.uart_set_16.disable = false;
   //          change_screen(modbus_select);}}
   // };

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