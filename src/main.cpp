#define STM32F405xx
#define F_OSC   8000000UL
#define F_CPU   168000000UL

#include "init_clock.h"
#include "periph_rcc.h"
#include "periph_flash.h"
#include "pin.h"
#include "literals.h"
#include "button.h"
#include "menu.h"
#include "flash.h"
#include "modbus_master.h"

using TX_master  = mcu::PA2;
using RX_master  = mcu::PA3;
using RTS_master = mcu::PA15;
using LED_red = mcu::PA4;
using LED_green = mcu::PA5;
using Enter = mcu::PB15;
using Left = mcu::PC9;
using Right = mcu::PB13;
using Start = mcu::PA12;
using Set   = mcu::PB7;
using E   = mcu::PC14;       
using RW  = mcu::PC15;       
using RS  = mcu::PC13;      
using DB4 = mcu::PC0;       
using DB5 = mcu::PC1;
using DB6 = mcu::PC2;    
using DB7 = mcu::PC3;

extern "C" void init_clock () { init_clock<F_OSC, F_CPU>(); }

int main()
{
   struct Flash_data {
      uint16_t factory_number = 0;
      UART::Settings uart_set = {
         .parity_enable  = false,
         .parity         = USART::Parity::even,
         .data_bits      = USART::DataBits::_8,
         .stop_bits      = USART::StopBits::_1,
         .baudrate       = USART::Baudrate::BR115200,
         .res            = 0
      };
      uint8_t  modbus_address  = 1;
      uint16_t model_number    = 0;
      uint16_t a_current       = 0;
      uint16_t m_current       = 0;
      uint16_t m_resonance     = 20_kHz;
      uint16_t a_resonance     = 20_kHz;
      uint16_t range_deviation = 200;
      uint16_t time            = 200_ms;
      uint8_t  qty_changes     = 2;
      uint8_t  every_degree    = 5;
      bool     m_control       = false;
      bool     deviation       = false;
   } flash;
   
   [[maybe_unused]] auto _ = Flash_updater<
        mcu::FLASH::Sector::_9
      , mcu::FLASH::Sector::_8
   >::make (&flash);
   
   decltype(auto) led_green = Pin::make<LED_green, mcu::PinMode::Output>();
   decltype(auto) led_red   = Pin::make<LED_red, mcu::PinMode::Output>();

   struct Flags {
      bool on            :1; 
      bool search        :1;
      bool manual        :1;
      bool manual_tune   :1;
      bool overheat      :1;
      bool no_load       :1;
      bool overload      :1;
      bool connect       :1;
      bool research      :1;
      bool end_research  :1;
      uint16_t           :6; //Bits 11:2 res: Reserved, must be kept cleared
      bool is_alarm() { return overheat or no_load or overload; }
   }flags_03, flags_16;

   using UART = UART::Settings;
   UART uart_set_03, uart_set_16;

   struct {
      // регистры управления
      Register<1, Modbus_function::force_coil_05, 0>  on;
      Register<1, Modbus_function::force_coil_05, 1>  search;
      // регистры на запись
      Register<1, Modbus_function::write_16, 0, UART> uart_set_16;
      Register<1, Modbus_function::write_16, 1>  modbus_address_16;
      Register<1, Modbus_function::write_16, 4>  frequency_16;
      Register<1, Modbus_function::write_16, 5>  work_frequency_16;
      Register<1, Modbus_function::write_16, 6>  power_16;
      Register<1, Modbus_function::write_16, 7>  max_current_16;
      Register<1, Modbus_function::write_16, 8>  max_temp_16;
      Register<1, Modbus_function::write_16, 9>  recovery_temp_16;
      Register<1, Modbus_function::write_16, 10, Flags> flags_16;
      // регистры на чтение
      Register<1, Modbus_function::read_03, 2, UART> uart_set_03;
      Register<1, Modbus_function::read_03, 3>  modbus_address_03;
      Register<1, Modbus_function::read_03, 4>  power_03;
      Register<1, Modbus_function::read_03, 5>  duty_cycle_03;
      Register<1, Modbus_function::read_03, 6>  work_frequency_03;
      Register<1, Modbus_function::read_03, 7>  frequency_03;
      // Register<1, Modbus_function::read_03, 8>  m_resonance_03;
      // Register<1, Modbus_function::read_03, 9>  a_resonance_03;
      Register<1, Modbus_function::read_03, 10> current_03;
      Register<1, Modbus_function::read_03, 11> max_current_03;
      // Register<1, Modbus_function::read_03, 12> a_current_03;
      // Register<1, Modbus_function::read_03, 13> m_current_03;
      Register<1, Modbus_function::read_03, 14> temperatura_03;
      Register<1, Modbus_function::read_03, 15> max_temp_03;
      Register<1, Modbus_function::read_03, 16> recovery_temp_03;
      Register<1, Modbus_function::read_03, 17, Flags> flags_03;
   } modbus_master_regs;

   // запросы отправляются только при необходимости изменить значение
   modbus_master_regs.uart_set_16.disable       = true;
   modbus_master_regs.modbus_address_16.disable = true;
   modbus_master_regs.frequency_16.disable      = true;
   modbus_master_regs.work_frequency_16.disable = true;
   modbus_master_regs.power_16.disable          = true;
   modbus_master_regs.max_current_16.disable    = true;
   modbus_master_regs.max_temp_16.disable       = true;
   modbus_master_regs.recovery_temp_16.disable  = true;
   modbus_master_regs.flags_16.disable          = true;
   // запросы отправляются только при необходимости прочитать значение
   modbus_master_regs.uart_set_03.disable       = true;
   modbus_master_regs.modbus_address_03.disable = true;
   modbus_master_regs.power_03.disable          = true;
   modbus_master_regs.work_frequency_03.disable = true;
   modbus_master_regs.max_current_03.disable    = true;
   modbus_master_regs.power_03.disable          = true;
   modbus_master_regs.max_temp_03.disable       = true;
   modbus_master_regs.recovery_temp_03.disable  = true;
   // modbus_master_regs.temperatura_03.disable    = true;
   // modbus_master_regs.current_03.disable        = true;
   // modbus_master_regs.duty_cycle_03.disable     = true;

   volatile decltype(auto) modbus_master = make_modbus_master <
          mcu::Periph::USART2
        , TX_master
        , RX_master
        , RTS_master
    > (100_ms, flash.uart_set, modbus_master_regs);

   // volatile decltype(auto) encoder = Encoder::make<mcu::Periph::TIM8, mcu::PC6, mcu::PC7, true>();

   auto up    = Button<Right>();
   auto down  = Button<Left>();
   auto enter = Button<Enter>();
   
   constexpr auto hd44780_pins = HD44780_pins<RS, RW, E, DB4, DB5, DB6, DB7>{};
   [[maybe_unused]] auto menu = Menu (
      hd44780_pins, up, down, enter 
      , flash
      , modbus_master_regs
      , flags_03
      , flags_16
      , uart_set_03
      , uart_set_16
      , adc
   );

   auto start = Button<Start>();
   start.set_down_callback([&]{ modbus_master_regs.on ^= 1; if(flags_03.manual) modbus_master_regs.frequency_16 = modbus_master_regs.frequency_03;});
   auto set   = Button<Set>();
   set.set_down_callback([&]{modbus_master_regs.search ^= 1; if (flags_03.manual_tune) modbus_master_regs.frequency_16 = modbus_master_regs.frequency_03;});

   Timer blink{500_ms};


   
   while(1){
      flags_03 = modbus_master_regs.flags_03;
      modbus_master_regs.flags_16 = flags_16;
      if ((flags_03.manual_tune and flags_03.search) or (flags_03.manual and not flags_03.search))
         modbus_master_regs.frequency_16.disable = false;
      else 
         modbus_master_regs.frequency_16.disable = true;

      if (flags_16.research) {
         modbus_master_regs.flags_16.disable = false;
      }
      
      if (flags_03.end_research) {
         flags_16.research = false;
      }

      // uart_set_03 = modbus_master_regs.uart_set_03;
      // uart_set_16.res = 0;
      // uart_set_16.parity = USART::Parity::even;
      // modbus_master_regs.uart_set_16 = uart_set_16;
      // if (not flags_03.search and modbus_master_regs.search) modbus_master_regs.search = false; // если закончен поиск, запрет команды на
      modbus_master();
      if (flags_03.on and flags_03.search and not flags_03.is_alarm())
         led_green ^= blink.event();
      else if (flags_03.on and not flags_03.search and not flags_03.is_alarm()) {
         led_green = true;
         modbus_master_regs.search = false;
      } else
         led_green = false;

      flags_03.is_alarm() ? led_red ^= blink.event() : led_red = false;
      __WFI();
   }
}
