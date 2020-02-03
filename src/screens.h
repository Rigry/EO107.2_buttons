#pragma once

#include "screen_common.h"
#include <array>
#include <bitset>

constexpr auto info = std::array {
    "А",
    "П"  
};

constexpr std::string_view info_to_string(int i) {
    return info[i];
}

constexpr auto mode = std::array {
    "Автоматический",
    "Пользовательский"  
};

constexpr std::string_view mode_to_string(int i) {
    return mode[i];
}

constexpr auto tune = std::array {
    "Автоматическая",
    "Пользовательская"  
};

constexpr std::string_view tune_to_string(int i) {
    return tune[i];
}

constexpr auto tune_control = std::array {
    "Начать",
    "Закончить"  
};

constexpr std::string_view tune_control_to_string(int i) {
    return tune_control[i];
}

constexpr auto search = std::array {
    "да",
    "нет"  
};


constexpr std::string_view search_to_string(int i) {
    return search[i];
}

constexpr auto deviation = std::array {
    "отключить",
    "включить"
};

constexpr std::string_view deviation_to_string(int i) {
    return deviation[i];
}

// template <class function>
struct Main_screen : Screen {
   String_buffer& lcd;
   Buttons_events eventers;
   Callback<> out_callback;
   uint16_t& frequency;
   uint16_t& work_frequency;
   uint16_t& temperatura;
   // uint16_t& current;
   // uint16_t& cur;
   // Flags& flags;

   Main_screen(
        String_buffer& lcd
      , Buttons_events eventers
      , Out_callback out_callback
      , uint16_t& frequency
      , uint16_t& work_frequency
      , uint16_t& temperatura
      // , uint16_t& current
      // , uint16_t& cur
      // , Flags& flags
      
   ) : lcd          {lcd}
     , eventers     {eventers}
     , out_callback {out_callback.value}
     , frequency    {frequency}
     , work_frequency {work_frequency}
     , temperatura  {temperatura}
   //   , current      {current}
   //   , cur          {cur}
   //   , flags        {flags}
   {}

   void init() override {
      // eventers.enter ([this]{ flags.on ^= 1;});
      // eventers.up    ([this]{ pwm.frequency += (flags.manual_tune and flags.search) or flags.manual ?  10 : 0;});
      // eventers.down  ([this]{ pwm.frequency += (flags.manual_tune and flags.search) or flags.manual ? -10 : 0;});
      // eventers.out   ([this]{ out_callback(); });
      lcd.clear();
      lcd.line(0) << "F=";
      lcd.line(0).cursor(11) << "P=";
      lcd.line(1) << "I=";
      lcd.line(1).cursor(10).width(2) << temperatura << "C";
      // lcd.line(1).cursor(9) << cur;
      // lcd.line(1).cursor(14) << ::info[this->flags.manual_tune];
      // lcd.line(1).cursor(15) << ::info[this->flags.manual];
   }

   void deinit() override {
      eventers.enter (nullptr);
      eventers.up    (nullptr);
      eventers.down  (nullptr);
      eventers.out   (nullptr);
   }

   void draw() override {
      // lcd.line(0).cursor(2).div_1000(pwm.frequency) << "кГц";
      // lcd.line(0).cursor(13).width(2) << (pwm.duty_cycle / 5) << '%';
      lcd.line(1).cursor(10).width(2) << temperatura << "C ";
      // lcd.line(1).cursor(8).div_1000(cur);
      // lcd.line(1).cursor(14) << ::info[this->flags.manual_tune];
      // lcd.line(1).cursor(15) << ::info[this->flags.manual];
      
      // if (not flags.is_alarm()) {
      //    // lcd.line(1) << "I="; lcd.line(1).cursor(2).div_1000(current) << "А ";
      //    return;
      // } else if (flags.overheat) {
      //    lcd.line(1) << "OVERHAET";
      //    return;
      // } else if (flags.no_load) {
      //    lcd.line(1) << "NO LOAD ";
      //    return;
      // } else if (flags.overload) {
      //    lcd.line(1) << "OVERLOAD";
      //    return;
      // }
   }

};

struct Temp_show_screen : Screen {
   String_buffer& lcd;
   Buttons_events eventers;
   Callback<> out_callback;
   const std::string_view name;
   const std::string_view unit;
   const uint16_t& temperatura;

   Temp_show_screen (
         String_buffer& lcd
       , Buttons_events eventers
       , Out_callback out_callback
       , std::string_view name
       , std::string_view unit
       , uint16_t& temperatura
   ) : lcd          {lcd}
     , eventers     {eventers}
     , out_callback {out_callback.value}
     , name {name}
     , unit {unit}
     , temperatura  {temperatura}
   {}

   void init() override {
      eventers.up    ([this]{ });
      eventers.down  ([this]{ });
      eventers.out   ([this]{ out_callback(); });
      lcd.line(0) << name << next_line;
      redraw();
   }
   void deinit() override {
      eventers.up    (nullptr);
      eventers.down  (nullptr);
      eventers.out   (nullptr);
   }

   void draw() override {}

   

private:

   void redraw() {
      lcd.line(1) << temperatura << unit;
      lcd << clear_after;
   }
};

