#pragma once


#define EMBLED_NUM_LEDS 2    /* Maximum number of automated leds */
#define EMBLED_MAX_STATES 10 /* Maximum number of states allowed for the load activation profile */
#define EMBLED_CYCLE_TIME_MS 100 /* Base time for the LED control timer */


#define EMBLED_INFINITE 0xFFFF /* Define for infinite number of repetitions */
#define EMBLED_CALC_TICKS(x)         \
    ((x) < EMBLED_CYCLE_TIME_MS) ? 1 \
                              : ((x) / EMBLED_CYCLE_TIME_MS) /* Calculate the number of ticks for the given time in ms */

typedef enum /* Possible LED control modes. */
{
    EMBLED_MODE_OFF = 0,                /**< Load off */
    EMBLED_MODE_ON,                     /**< Load on */
    EMBLED_MODE_PULSE_FAST_ONCE,        /**< Load blinks once only, with short on time */
    EMBLED_MODE_PULSE_SLOW_ONCE,        /**< Load blinks once only, with long on time */
    EMBLED_MODE_PULSE_FAST_DOUBLE_ONCE, /**< Load blinks twice fast and then turns off */
    EMBLED_MODE_PULSE_SINGLE,           /**< Load blinks once, continuously and fast */
    EMBLED_MODE_PULSE_DOUBLE,           /**< Load blinks two times, continuously and fast */
    EMBLED_MODE_PULSE_TRIPLE,           /**< Load blinks three times, continuously and fast */
    EMBLED_MODE_BLINK_SLOW,             /**< Load blinks in 1hz frequency */
    EMBLED_MODE_BLINK_MEDIUM,           /**< Load blinks in 2hz frequency */
    EMBLED_MODE_BLINK_FAST,             /**< Load blinks in 5hz frequency */
    EMBLED_START_DYNAMIC,
    EMBLED_MODE_DYNAMIC_1 = EMBLED_START_DYNAMIC,
    EMBLED_MODE_DYNAMIC_2,              /**< Dynamic mode to be defined by the application */
    EMBLED_MODE_DYNAMIC_3,              /**< Dynamic mode to be defined by the application */
    EMBLED_MODE_DYNAMIC_4,              /**< Dynamic mode to be defined by the application */
    EMBLED_MODE_DYNAMIC_5,              /**< Dynamic mode to be defined by the application */
    EMBLED_MAX_MODES                    /**< Maximum number of modes */
} embled_modes_t;

typedef enum /* Logical levels when active. */
{
    EMBLED_ACTIVE_LOW = 0,
    EMBLED_ACTIVE_HIGH,
} embled_levels_t;

typedef enum /* LED control modes, digital or via PWM */
{
    EMBLED_DRIVER_MODE_DIGITAL = 0,
    EMBLED_DRIVER_MODE_PWM,
} embled_driver_mode_t;

typedef struct
{
    void (*write_gpio)(uint16_t pin, bool level);
    bool (*read_gpio)(uint16_t pin);
    void (*start_pwm)(uint16_t pin);
    void (*stop_pwm)(uint16_t pin);
} embled_callbacks_t;

/**
 * @brief  	Defines the GPIO and mode for the led and activates the control mode
 *
 * @param[in] 	GPIO pin
 * @param[in] 	LED driver mode.  This parameter can be one of the following enum values:
 *           	@arg EMBLED_DRIVER_MODE_DIGITAL: LED control via digital output
 *          	@arg EMBLED_DRIVER_MODE_PWM: LED control via PWM output
 * @param[in] 	LED operation mode. See @ref embled_modes_t
 * @param[in] 	Logical activation level. This parameter can be one of the following enum values:
 *            	@arg EMBLED_ACTIVE_LOW: LED activates at GND
 *            	@arg EMBLED_ACTIVE_HIGH: LED activates at VCC
 * @param[in] 	true: returns to the previous mode | false: does not return to the previous mode
 * @return 	Operation status. false: error | true: success
 *
 * Example:
 * @code
 * // Define minimal callbacks (replace bodies with HAL/GPIO implementations)
 * static void cb_write(uint16_t pin, bool level) { (void)pin; (void)level; // no-op }
 * static bool cb_read(uint16_t pin) { (void)pin; return false; }
 *
 * embled_callbacks_t cbks = {
 *     .write_gpio = cb_write,
 *     .read_gpio = cb_read,
 *     .start_pwm = NULL,
 *     .stop_pwm = NULL,
 * };
 *
 * embled_init(&cbks);
 *
 * // Turn LED on pin 5 ON (digital, active-high), not returning to previous mode
 * bool ok = embled_set_mode(5,
 *                           EMBLED_DRIVER_MODE_DIGITAL,
 *                           EMBLED_MODE_ON,
 *                           EMBLED_ACTIVE_HIGH,
 *                           false);
 * (void)ok;
 * @endcode
 */
bool embled_set_mode(uint16_t pin, embled_driver_mode_t driver_mode, embled_modes_t new_mode, embled_levels_t level,
                     bool return_last_mode);

/**
 * @brief  	Creates a new mode by configuring a new dynamic profile
 *
 * @param[in] 	Number of states
 * @param[in] 	Number of repetitions
 * @param[in] 	Pointer to array containing the duration, in milliseconds, of each state
 * @return 	Index of the created mode for led activation. If -1, invalid value
 *
 * Example:
 * @code
 * // Define a custom blink pattern: on/off/on/off durations (ms)
 * uint16_t pattern_ms[] = { 200, 200, 800, 400 };
 * // num_states = 4, repetitions = EMBLED_INFINITE
 * int8_t mode_idx = embled_new_profile(4,
 *                                      EMBLED_INFINITE,
 *                                      pattern_ms);
 * if (mode_idx >= 0) {
 *     // Activate the newly created dynamic mode on pin 5
 *     bool ok = embled_set_mode(5,
 *                               EMBLED_DRIVER_MODE_DIGITAL,
 *                               (embled_modes_t)mode_idx,
 *                               EMBLED_ACTIVE_HIGH,
 *                               false);
 * }
 * @endcode
 */
int8_t embled_new_profile(uint8_t num_states, uint16_t num_repetitions, uint16_t* duration_ms);

/**
 * @brief  	LED control service processing task. Must be call by application timer every 100ms
 *
 * @param	 	Null
 * @return 	Null
 */
void embled_task(void* argument);

/**
 * @brief  	Initializes the LED led control service
 *
 * @param[in]	Pointer to the callback structure @led_callbacks_t
 * @return 	Null
 */
void embled_init(embled_callbacks_t* cbks);