#include "pico.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "system.h"


static st_systemConfiguration systemConfiguration;

/**
    Initialise system and support data structures.

*/
void system_initialiseSystem(void) {

    systemConfiguration.lcdBacklightValue = LCD_BACKLIGHTING_DEFAULT;
    systemConfiguration.beeper = false;

    // Setup GPIO for LCD backlighting
    gpio_set_function(LCD_BACKLIGHT_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LCD_BACKLIGHT_PWM_PIN);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, system_onPwmWrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 10.f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

}

/**
    Get a pointer to the system configuration.

    @param[out]    st_systemConfiguration pointer to system configuration structure.
*/
st_systemConfiguration *system_getSystemConfiguration(void) {

    return (&systemConfiguration);
}

/**
    Toggle beeper ON or OFF.

*/
void system_toggleBeeper(void) {

    systemConfiguration.beeper ^= true;
}

/**
    Toggle local echo ON or OFF.

*/
void system_toggleLocalEcho(void) {

    systemConfiguration.localEcho ^= true;
}

/**
    Increase backlight value by 1 step size.

*/
void system_increaseBacklightByStep(void) {

    if (systemConfiguration.lcdBacklightValue < LCD_BACKLIGHTING_PWM_MAX) {
        systemConfiguration.lcdBacklightValue += LCD_BACKLIGHTING_STEP_SIZE;
    }
}

/**
    Decrease backlight value by 1 step size.

*/
void system_decreaseBacklightByStep(void) {
    
    if (systemConfiguration.lcdBacklightValue > LCD_BACKLIGHTING_STEP_SIZE) {
        systemConfiguration.lcdBacklightValue -= LCD_BACKLIGHTING_STEP_SIZE;
    }
}

/**
    PWM wrap handler. Needed to ensure PWM duty cycle is updated on edge.
    
*/
void system_onPwmWrap(void) {
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(LCD_BACKLIGHT_PWM_PIN));
  
    uint16_t targetPwmValue = (systemConfiguration.lcdBacklightValue * UINT16_MAX) / LCD_BACKLIGHTING_PWM_MAX;

    if (targetPwmValue == UINT16_MAX) {
        targetPwmValue -= 1;
    }

    pwm_set_gpio_level(LCD_BACKLIGHT_PWM_PIN, targetPwmValue);
}