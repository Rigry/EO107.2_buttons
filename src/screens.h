#pragma once

#include "screen_common.h"
#include "timers.h"
#include "adc.h"
#include "NTC_table.h"
#include <array>
#include <bitset>
#include <cstdlib>

constexpr auto info = std::array {
    "А",
    "П"
};

constexpr std::string_view info_to_string(int i) {
    return info[i];
}

constexpr auto attenuation = std::array {
   "1",
   "10"
};

constexpr std::string_view attenuation_to_string (int i) {
   return attenuation[i];
}

constexpr auto mode = std::array {
    "Автоматический",
    "Пользовательский"  
};

constexpr std::string_view mode_to_string(int i) {
    return mode[i];
}

constexpr auto off_on = std::array {
    "отключить",
    "включить"
};

constexpr std::string_view off_on_to_string(int i) {
    return off_on[i];
}

// constexpr auto boudrate = std::array {
//     "9600",
//     "14400",
//     "19200",
//     "28800",
//     "38400",
//     "57600",
//     "76800",
//     "115200"
// };

// constexpr std::string_view boudrate_to_string(int i) {
//     return boudrate[i];
// }

// для воды

constexpr auto conversion_on_channel {96};
struct ADC_{
   ADC_average& control     = ADC_average::make<mcu::Periph::ADC1>(conversion_on_channel);
   ADC_channel& temperatura = control.add_channel<mcu::PA7>();
}adc;

template <class Regs, class Flags>
struct Main_screen : Screen {
   String_buffer& lcd;
   Buttons_events eventers;
   Callback<> out_callback;
   Regs& modbus_master_regs;
   Flags& flags_03;
   Flags& flags_16;
   ADC_& adc; // для воды
   uint8_t& every_degree{0}; // для воды
   uint16_t temperature{0}; // для воды
   uint16_t last_temp{0};
   Timer blink{500_ms};
   bool blink_{false};

   Main_screen(
        String_buffer& lcd
      , Buttons_events eventers
      , Out_callback out_callback
      , Regs& modbus_master_regs
      , Flags& flags_03
      , Flags& flags_16
      , ADC_& adc
      , uint8_t& every_degree
      
   ) : lcd          {lcd}
     , eventers     {eventers}
     , out_callback {out_callback.value}
     , modbus_master_regs {modbus_master_regs}
     , flags_03   {flags_03}
     , flags_16   {flags_16}
     , adc        {adc}
     , every_degree {every_degree}
   {adc.control.start();}

   void init() override {
      eventers.enter ([this]{ out_callback();});
      eventers.up    ([this]{ modbus_master_regs.frequency_16 += /*(modbus_master_regs.flags_03.manual_tune and modbus_master_regs.flags_03.search) or modbus_master_regs.flags_03.manual ? */ 10;});
      eventers.down  ([this]{ modbus_master_regs.frequency_16 += /*(modbus_master_regs.flags_03.manual_tune and modbus_master_regs.flags_03.search) or modbus_master_regs.flags_03.manual ? */-10;});
      eventers.out   ([this]{  });
      eventers.increment_up   ([this](auto i){   up(i); });
      eventers.increment_down ([this](auto i){ down(i); });
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

      eventers.increment_up  (nullptr);
      eventers.increment_down(nullptr);
   }

   void draw() override {
      lcd.line(0).cursor(2).div_1000(modbus_master_regs.frequency_03) << "кГц";
      lcd.line(0).cursor(13).width(2) << modbus_master_regs.duty_cycle_03 << '%';
      lcd.line(1).cursor(10).width(2) << modbus_master_regs.temperatura_03 << "C ";
      temperature = temp(adc.temperatura);
      // if (abs(temperature - last_temp) >= every_degree) {
      //     flags_16.research = true;
      //     last_temp = temperature;
      // }//для ванн

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

   int n{0};
   uint32_t avg{0};
   std::array<uint16_t, 10> array_temp;
   uint16_t temp(uint32_t adc) {
      array_temp[n++] = adc / conversion_on_channel;
      n = n >= 10 ? 0 : n;
      avg = 0;
      for (auto c : array_temp)
         avg += c;
      avg /= array_temp.size();
      auto p = std::lower_bound(
          std::begin(NTC::u2904<33,5100>),
          std::end(NTC::u2904<33,5100>),
          avg,
          std::greater<uint32_t>());
      return (p - NTC::u2904<33,5100>);
   }

   void down (int increment = 10) 
    {
        modbus_master_regs.frequency_16 -= increment;
    }

    void up (int increment = 10)
    {
        modbus_master_regs.frequency_16 += increment;
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

