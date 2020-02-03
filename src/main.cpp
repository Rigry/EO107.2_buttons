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
#include "communication.h"

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

   // auto us_on = Button<Enter>();

   struct Regs {
      Register<1, Modbus_function::write_16, 4> frequency;
      Register<1, Modbus_function::write_16, 5> work_frequency;

      Register<1, Modbus_function::read_03, 14> temperatura;
   } modbus_master_regs;

   decltype(auto) modbus_master = make_modbus_master <
          mcu::Periph::USART1
        , TX_master
        , RX_master
        , RTS_master
    > (100_ms, flash.uart_set, modbus_master_regs);

   // volatile decltype(auto) modbus = Modbus_slave<In_regs, Out_regs, coils_qty>
   //               ::make<mcu::Periph::USART1, TX, RX, RTS>
   //                     (flash.modbus_address, flash.uart_set);

   // auto& work_flags = modbus.outRegs.flags;

   // управление по модбас
   // modbus.force_single_coil_05[0] = [&](bool on) {
   //    if (on)
   //       work_flags.on = true;
   //    if (not on)
   //       work_flags.on = false;
   // };

   // modbus.force_single_coil_05[1] = [&](bool on) {
   //    if (on)
   //       flash.search = true;
   //    if (not on)
   //       flash.search = false;
   // };

   // #define ADR(reg) GET_ADR(In_regs, reg)
   // modbus.outRegs.device_code       = 9;
   // modbus.outRegs.factory_number    = flash.factory_number;
   // modbus.outRegs.modbus_address    = flash.modbus_address;
   // modbus.outRegs.uart_set          = flash.uart_set;    
   // modbus.arInRegsMax[ADR(uart_set)]= 0b11111111;
   // modbus.inRegsMin.modbus_address  = 1;
   // modbus.inRegsMax.modbus_address  = 255;
   
   // volatile decltype(auto) pwm = PWM::make<mcu::Periph::TIM3, PWM_pin>(490);
   volatile decltype(auto) encoder = Encoder::make<mcu::Periph::TIM8, mcu::PC6, mcu::PC7, true>();

   using Flash  = decltype(flash);
   // using Master = Modbus_master<Regs>;
   // using Modbus = Modbus_slave<In_regs, Out_regs, coils_qty>;
   // using Generator = Generator<Flash>;

   // Generator generator {adc, pwm, led_green, led_red, work_flags, flash};

   Communication<Regs> communication{modbus_master_regs};


   auto up    = Button<Right>();
   auto down  = Button<Left>();
   auto enter = Button<Enter>();
   enter.set_down_callback([&]{flash.factory_number = 10;});
   constexpr auto hd44780_pins = HD44780_pins<RS, RW, E, DB4, DB5, DB6, DB7>{};
   [[maybe_unused]] auto menu = Menu (
      hd44780_pins, encoder, up, down, enter
      , flash
      , modbus_master_regs
   );
   
   while(1){
      modbus_master();
      communication();
      __WFI();
   }
}
