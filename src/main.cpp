#define STM32F405xx
#define F_OSC   8000000UL
#define F_CPU   48000000UL

#include "init_clock.h"
#include "periph_rcc.h"
#include "periph_flash.h"
#include "pin.h"
#include "literals.h"
#include "button.h"
#include "menu.h"
#include "flash.h"
#include "modbus_master.h"

using TX_master  = mcu::PA9;
using RX_master  = mcu::PA10;
using RTS_master = mcu::PA12;
using LED_red = mcu::PA15;
using LED_green = mcu::PC10;
using Enter = mcu::PA8;
using Left = mcu::PB13;
using Right = mcu::PB14;
using E   = mcu::PC14;       
using RW  = mcu::PC15;       
using RS  = mcu::PC13;      
using DB4 = mcu::PC2;       
using DB5 = mcu::PC3;
using DB6 = mcu::PC0;    
using DB7 = mcu::PC1;

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
         .baudrate       = USART::Baudrate::BR9600,
         .res            = 0
      };
      uint8_t  modbus_address  = 1;
      uint16_t model_number    = 0;
      uint16_t work_frequency  = 18_kHz;
      uint16_t max_current     = 0;
      uint16_t a_current       = 0;
      uint16_t m_current       = 0;
      uint16_t m_resonance     = 18_kHz;
      uint16_t a_resonance     = 18_kHz;
      uint16_t range_deviation = 200;
      uint16_t  time            = 200_ms;
      uint8_t  qty_changes     = 2;
      uint8_t  power           = 100_percent;
      uint8_t  temperatura     = 65;
      uint8_t  recovery        = 45;
      bool     m_search        = false;
      bool     m_control       = false;
      bool     search          = false;
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
      uint16_t           :8; //Bits 11:2 res: Reserved, must be kept cleared
      bool is_alarm() { return overheat or no_load or overload; }
   };

   struct {
      // регистры на запись
      // Register<1, Modbus_function::write_16, 0> uart_set;
      // Register<1, Modbus_function::write_16, 1> modbus_address;
      // Register<1, Modbus_function::write_16, 4>  frequency_16;
      // Register<1, Modbus_function::write_16, 5>  work_frequency_16;
      // Register<1, Modbus_function::write_16, 6>  power_16;
      // Register<1, Modbus_function::write_16, 7>  max_current_16;
      Register<1, Modbus_function::write_16, 8>  max_temp_16;
      Register<1, Modbus_function::write_16, 9>  recovery_temp_16;
      Register<1, Modbus_function::write_16, 10, Flags> flags_16;
      // регистры на чтение
      Register<1, Modbus_function::read_03, 4>  power_03;
      Register<1, Modbus_function::read_03, 5>  duty_cycle_03;
      Register<1, Modbus_function::read_03, 6>  work_frequency_03;
      Register<1, Modbus_function::read_03, 7>  frequency_03;
      // Register<1, Modbus_function::read_03, 8>  m_resonance_03;
      // Register<1, Modbus_function::read_03, 9>  a_resonance_03;
      Register<1, Modbus_function::read_03, 10> current_03;
      Register<1, Modbus_function::read_03, 11> max_current_03;
      Register<1, Modbus_function::read_03, 12> a_current_03;
      Register<1, Modbus_function::read_03, 13> m_current_03;
      Register<1, Modbus_function::read_03, 14> temperatura_03;
      Register<1, Modbus_function::read_03, 15> max_temp_03;
      Register<1, Modbus_function::read_03, 16> recovery_temp_03;
      Register<1, Modbus_function::read_03, 17, Flags> flags_03;
   } modbus_master_regs;

   modbus_master_regs.max_temp_16       = flash.temperatura;
   modbus_master_regs.recovery_temp_16  = flash.recovery;
   // modbus_master_regs.work_frequency_16 = flash.work_frequency;
   // modbus_master_regs.max_current_16    = flash.max_current;

   modbus_master_regs.flags_16.disable = true;

   decltype(auto) modbus_master = make_modbus_master <
          mcu::Periph::USART1
        , TX_master
        , RX_master
        , RTS_master
    > (100_ms, flash.uart_set, modbus_master_regs);

   volatile decltype(auto) encoder = Encoder::make<mcu::Periph::TIM8, mcu::PC6, mcu::PC7, true>();

   // using Flash  = decltype(flash);

   auto up    = Button<Right>();
   auto down  = Button<Left>();
   auto enter = Button<Enter>();
   constexpr auto hd44780_pins = HD44780_pins<RS, RW, E, DB4, DB5, DB6, DB7>{};
   [[maybe_unused]] auto menu = Menu (
      hd44780_pins, encoder, up, down, enter
      , flash
      , modbus_master_regs
   );
   
   while(1){
      modbus_master();
      __WFI();
   }
}
