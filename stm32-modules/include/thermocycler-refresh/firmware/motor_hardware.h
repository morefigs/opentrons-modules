#ifndef __MOTOR_HARDWARE_H
#define __MOTOR_HARDWARE_H
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdbool.h>
#include <stdint.h>

// ----------------------------------------------------------------------------
// Public definitions

// Frequency of the motor interrupt callbacks is 500kHz
#define MOTOR_INTERRUPT_FREQ (500000)

// Enumeration of motor error types
typedef enum MotorError {
    MOTOR_ERROR, /**< The motor raised an error flag.*/
    MOTOR_STALL  /**< The motor raised a stall flag.*/
} MotorError_t;

// ----------------------------------------------------------------------------
// Type definitions

// Void return and no parameters
typedef void (*motor_step_callback_t)(void);
// Void return and no parameters
typedef void (*motor_error_callback_t)(MotorError_t);

// This structure is used to define callbacks out of motor interrupts
typedef struct {
    motor_step_callback_t lid_stepper_complete;
    motor_step_callback_t seal_stepper_tick;
    motor_error_callback_t seal_stepper_error;
} motor_hardware_callbacks;

// ----------------------------------------------------------------------------
// Function definitions

/**
 * @brief Initialize the motor hardware
 *
 * @param callbacks Structure containing callbacks for stepper motor
 * interrupts
 */
void motor_hardware_setup(const motor_hardware_callbacks* callbacks);

/**
 * @brief Start a lid stepper movement
 * @param[in] steps Number of steps to move the stepper
 */
void motor_hardware_lid_stepper_start(int32_t steps);
/**
 * @brief Stop a lid stepper movement
 *
 */
void motor_hardware_lid_stepper_stop();
/**
 * @brief Callback whenever a lid stepper callback is invoked
 *
 */
void motor_hardware_lid_increment();
/**
 * @brief Set the output of the lid stepper DAC
 *
 * @param dacval Value to set DAC register to
 */
void motor_hardware_lid_stepper_set_dac(uint8_t dacval);
/**
 * @brief Check if a fault is present for the lid stepper
 *
 * @return True if a fault is signalled, false otherwise
 */
bool motor_hardware_lid_stepper_check_fault(void);
/**
 * @brief Reset the lid stepper driver
 *
 * @return True if a fault is detected \e after reset, false otherwise
 */
bool motor_hardware_lid_stepper_reset(void);

/**
 * @brief Sets the enable pin on the TMC2130
 * @param[in] enable True to enable, false to disable the TMC
 * @return True if the enable pin was set, false if it couldn't be set
 */
bool motor_hardware_set_seal_enable(bool enable);
/**
 * @brief Set the direction pin of the seal stepper
 * @param direction Direction to set. True = forwards,
 * false = backwards
 * @return true on success, false on error
 */
bool motor_hardware_set_seal_direction(bool direction);
/**
 * @brief Begin a seal motor movement
 * @return true if the movement could be started,
 * false otherwise
 */
bool motor_hardware_start_seal_movement(void);
/**
 * @brief Stop a seal motor movement
 * @return true if the movement could be stopped,
 * false otherwise
 */
bool motor_hardware_stop_seal_movement(void);
/**
 * @brief Callback for the seal motor timer interrupt.
 */
void motor_hardware_seal_interrupt(void);
/**
 * @brief Pulse the seal motor step pin
 */
void motor_hardware_seal_step_pulse(void);

/**
 * @brief Engage the lid lock solenoid
 *
 */
void motor_hardware_solenoid_engage();
/**
 * @brief Disengage the lid lock solenoid
 *
 */
void motor_hardware_solenoid_release();

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
#endif  // __MOTOR_HARDWARE_H