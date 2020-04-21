#pragma once

#include "screen_common.h"
#include "timers.h"
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

constexpr auto boudrate = std::array {
    "9600",
    "14400",
    "19200",
    "28800",
    "38400",
    "57600",
    "76800",
    "115200"
};

constexpr std::string_view boudrate_to_string(int i) {
    return boudrate[i];
}

template <class Regs, class Flags>
struct Main_screen : Screen {
   String_buffer& lcd;
   Buttons_events eventers;
   Callback<> out_callback;
   Regs& modbus_master_regs;
   Flags& flags_03;
   Flags& flags_16;
   Timer blink{500_ms};
   bool blink_{false};

   Main_screen(
        String_buffer& lcd
      , Buttons_events eventers
      , Out_callback out_callback
      , Regs& modbus_master_regs
      , Flags& flags_03
      , Flags& flags_16
      
   ) : lcd          {lcd}
     , eventers     {eventers}
     , out_callback {out_callback.value}
     , modbus_master_regs {modbus_master_regs}
     , flags_03 {flags_03}
     , flags_16 {flags_16}
   {}

   void init() override {
      eventers.enter ([this]{ });
      eventers.up    ([this]{ modbus_master_regs.frequency_16 += /*(modbus_master_regs.flags_03.manual_tune and modbus_master_regs.flags_03.search) or modbus_master_regs.flags_03.manual ? */ 10;});
      eventers.down  ([this]{ modbus_master_regs.frequency_16 += /*(modbus_master_regs.flags_03.manual_tune and modbus_master_regs.flags_03.search) or modbus_master_regs.flags_03.manual ? */-10;});
      eventers.out   ([this]{ out_callback(); });
      lcd.clear();
      lcd.line(0) << "F=";
      lcd.line(0).cursor(11) << "P=";
      lcd.line(1) << "I=";
      lcd.line(1).cursor(10).width(2) << modbus_master_regs.temperatura_03 << "C";
      lcd.line(1).cursor(14) << ::info[flags_03.manual_tune];
      lcd.line(1).cursor(15) << ::info[flags_03.manual];
   }

   void deinit() override {
      eventers.enter (nullptr);
      eventers.up    (nullptr);
      eventers.down  (nullptr);
      eventers.out   (nullptr);
   }

   void draw() override {
      lcd.line(0).cursor(2).div_1000(modbus_master_regs.frequency_03) << "кГц";
      lcd.line(0).cursor(13).width(2) << modbus_master_regs.duty_cycle_03 << '%';
      lcd.line(1).cursor(10).width(2) << modbus_master_regs.temperatura_03 << "C ";
      if (modbus_master_regs.search) {
          blink_ ^= blink.event();
          blink_ ? lcd.line(1).cursor(14) << ::info[flags_03.manual_tune] : lcd.line(1).cursor(14) << ' ';
      } else
        lcd.line(1).cursor(14) << ::info[flags_03.manual_tune];
      lcd.line(1).cursor(15) << ::info[flags_03.manual];
      
      if (not flags_03.is_alarm()) {
         lcd.line(1) << "I="; lcd.line(1).cursor(2).div_1000(modbus_master_regs.current_03) << "А ";
         return;
      } else if (flags_03.overheat) {
         lcd.line(1) << "OVERHAET";
         return;
      } else if (flags_03.no_load) {
         lcd.line(1) << "NO LOAD ";
         return;
      } else if (flags_03.overload) {
         lcd.line(1) << "OVERLOAD";
         return;
      }
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

