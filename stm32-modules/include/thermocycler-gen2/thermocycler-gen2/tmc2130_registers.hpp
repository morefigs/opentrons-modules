/**
 * @file tmc2130_registers.hpp
 * @brief Contains register mapping information for the TMC2130 motor
 * driver IC.
 *
 */
#pragma once

#include <concepts>
#include <cstdint>

namespace tmc2130 {

// Register mapping

enum class Registers : uint8_t {
    GCONF = 0x00,
    GSTAT = 0x01,
    IOIN = 0x04,
    IHOLD_IRUN = 0x10,
    TPOWERDOWN = 0x11,
    TSTEP = 0x12,
    TPWMTHRS = 0x13,
    TCOOLTHRS = 0x14,
    THIGH = 0x15,
    XDIRECT = 0x2D,
    VDCMIN = 0x33,
    CHOPCONF = 0x6C,
    COOLCONF = 0x6D,
    DCCTRL = 0x6E,
    DRVSTATUS = 0x6F,
    PWMCONF = 0x70,
    ENCM_CTRL = 0x72,
    MSLUT_0 = 0x60,
    MSLUT_1 = 0x61,
    MSLUT_2 = 0x62,
    MSLUT_3 = 0x63,
    MSLUT_4 = 0x64,
    MSLUT_5 = 0x65,
    MSLUT_6 = 0x66,
    MSLUT_7 = 0x67,
    MSLUTSEL = 0x68,
    MSLUTSTART = 0x69,
    MSCNT = 0x6A,
    MSCURACT = 0x6B,
    PWM_SCALE = 0x71,
    LOST_STEPS = 0x73,
};

/** Template concept to constrain what structures encapsulate registers.*/
template <typename Reg>
concept TMC2130Register = requires(Reg& r, uint64_t value) {
    // Struct has a valid register address
    std::same_as<decltype(Reg::address), Registers&>;
    // Struct has an integer with the total number of bits in a register.
    // This is used to mask the 64-bit value before writing to the IC.
    std::integral<decltype(Reg::value_mask)>;
};

template <typename Reg>
concept WritableRegister = requires() {
    {Reg::writable};
};

template <typename Reg>
concept ReadableRegister = requires() {
    {Reg::readable};
};

struct __attribute__((packed, __may_alias__)) GConfig {
    static constexpr Registers address = Registers::GCONF;
    static constexpr bool readable = true;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = (1 << 17) - 1;

    uint32_t i_scale_analog : 1 = 0;
    uint32_t internal_rsense : 1 = 0;
    uint32_t en_pwm_mode : 1 = 0;
    uint32_t enc_commutation : 1 = 0;  // MUST be 0
    uint32_t shaft : 1 = 0;
    uint32_t diag0_error : 1 = 0;
    uint32_t diag0_otpw : 1 = 0;
    uint32_t diag0_stall : 1 = 0;
    uint32_t diag1_stall : 1 = 0;
    uint32_t diag1_index : 1 = 0;
    uint32_t diag1_onstate : 1 = 0;
    uint32_t diag1_steps_skipped : 1 = 0;
    uint32_t diag0_int_pushpull : 1 = 0;
    uint32_t diag1_pushpull : 1 = 0;
    uint32_t small_hysteresis : 1 = 0;
    uint32_t stop_enable : 1 = 0;
    uint32_t direct_mode : 1 = 0;
    uint32_t test_mode : 1 = 0;  // MUST be 0
};

struct __attribute__((packed, __may_alias__)) GStatus {
    static constexpr Registers address = Registers::GSTAT;
    static constexpr bool readable = true;
    static constexpr uint32_t value_mask = (1 << 3) - 1;

    uint32_t undervoltage_error : 1 = 0;
    uint32_t driver_error : 1 = 0;
    uint32_t reset : 1 = 0;
};

/**
 * This register sets the control current for holding and running.
 */
struct __attribute__((packed, __may_alias__)) CurrentControl {
    static constexpr Registers address = Registers::IHOLD_IRUN;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = (1 << 20) - 1;

    // Arbitrary scale from 0-31
    uint32_t hold_current : 5 = 0;
    uint32_t bit_padding_1 : 3 = 0;
    // Arbitrary scale from 0-31
    uint32_t run_current : 5 = 0;
    uint32_t bit_padding_2 : 3 = 0;
    // Motor powers down after (hold_current_delay * (2^18)) clock cycles
    uint32_t hold_current_delay : 4 = 0;
};

/**
 * This is the time to delay between ending a movement and moving to power-down
 * current. Scale goes up to "about 4 seconds"
 */
struct __attribute__((packed, __may_alias__)) PowerDownDelay {
    static constexpr Registers address = Registers::TPOWERDOWN;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = (1 << 8) - 1;

    static constexpr double max_time = 4.0F;
    static constexpr uint32_t max_val = 0xFF;

    [[nodiscard]] static auto reg_to_seconds(uint8_t reg) -> double {
        return (static_cast<double>(reg) / static_cast<double>(max_val)) *
               max_time;
    }
    static auto seconds_to_reg(double seconds) -> uint8_t {
        if (seconds > max_time) {
            return max_val;
        }
        return static_cast<uint32_t>((seconds / max_time) *
                                     static_cast<double>(max_val));
    }

    uint32_t time : 8 = 0;
};

/**
 * This is the threshold velocity for switching on smart energy coolStep
 * and stallGuard.
 */
struct __attribute__((packed, __may_alias__)) TCoolThreshold {
    static constexpr Registers address = Registers::TCOOLTHRS;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = (1 << 20) - 1;

    uint32_t threshold : 20 = 0;
};

/**
 * This is the velocity threshold at which the controller will automatically
 * move into a different chopper mode w/ fullstepping to maximize torque,
 * applied whenever TSTEP < THIGH
 */
struct __attribute__((packed, __may_alias__)) THigh {
    static constexpr Registers address = Registers::THIGH;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = (1 << 20) - 1;

    uint32_t threshold : 20 = 0;
};

/**
 * This is the measured time per step of the motor.
 */
struct __attribute__((packed, __may_alias__)) TStep {
    static constexpr Registers address = Registers::TSTEP;
    static constexpr bool readable = true;
    static constexpr uint32_t value_mask = (1 << 20) - 1;

    uint32_t value : 20 = 0;
};

/**
 * The CHOPCONFIG register contains a number of configuration options for the
 * Chopper control.
 */
struct __attribute__((packed, __may_alias__)) ChopConfig {
    static constexpr Registers address = Registers::CHOPCONF;
    static constexpr bool readable = true;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = 0x7FFFFFFF;

    /** 0 = Driver disable
     *  1 = "use only with TBL >= 2"
     *  2...15 sets duration of slow decay phase:
     * Nclk = 12 + 32 * TOFF
     */
    uint32_t toff : 4 = 0;
    /**
     * CHM = 0: sets hysteresis start value added to HEND
     *  - Add 1,2,...,8 to hysteresis low value HEND
     *
     * CHM = 1: sets fast decay time, TFD :
     *  - Nclk = 32*HSTRT
     */
    uint32_t hstrt : 3 = 0;
    /**
     * CHM = 0: Hysteresis is -3, -2, -1, ... 12. This is the hysteresis
     * value used for the hysteresis chopper
     *
     * CHM = 1: Sine wave offset. 1/512 of the val gets added to abs of each
     * sine wave entry
     */
    uint32_t hend : 4 = 0;
    /**
     * CHM = 1: MSB of fast decay time setting TFD
     */
    uint32_t fd3 : 1 = 0;
    /**
     * CHM = 1: Fast decay mode. Set to 1 to disable current comparator
     * usage for termination of fast decay cycle.
     */
    uint32_t disfdcc : 1 = 0;
    /**
     * 0 = chopper OFF time fixed as set by TOFF (aka hstrt and fd3)
     *
     * 1 = Random mode, TOFF is modulated by [-12,3] clocks
     */
    uint32_t rndtf : 1 = 0;
    /**
     * Chopper mode. 0 = standard mode, 1 = constant off-time with fast decay.
     */
    uint32_t chm : 1 = 0;
    /**
     * Blank Time Select. Sets comparator blank time to 16,24,36,54
     */
    uint32_t tbl : 2 = 2;
    /**
     * 0 = low sensitivity, high sense resistor voltage
     *
     * 1 = high sensitivity, low sense resistor voltage
     */
    uint32_t vsense : 1 = 0;
    /**
     * High velocity fullstep selection: Enables switching to fullstep when
     * VHIGH is exceeded. Only switches at 45º position.
     */
    uint32_t vhighfs : 1 = 0;
    /**
     * High Velocity Chopper Mode: Enables switching to chm=1 and fd=0 when
     * VHIGH is exceeded. If set, the TOFF setting automatically becomes
     * doubled during high velocity operation.
     */
    uint32_t vhighchm : 1 = 0;
    /**
     * SYNC PWM synchronization clock: Allows synchroniation of the chopper
     * for both phases of a two phase motor to avoid occurrence of a beat.
     * Automatically switched off above VHIGH
     *
     * 0 = disabled
     *
     * 1...15 = synchronized with fsync = fclk/(sync*64)
     */
    uint32_t sync : 4 = 0;
    /**
     * Microstep resolution:
     *
     * 0 = native 256 microstep setting
     *
     * 0b1...0b1000 = 128,64,32,16,8,4,2,FULLSTEP
     *
     * Reduced microstep resolution for STEP/DIR operation. Resolution gives
     * the number of microstep entries per sine quarter wave.
     */
    uint32_t mres : 4 = 0;
    /**
     * Interpolation to 256 microsteps: If set, the actual MRES becomes
     * extrapolated to 256 usteps for smoothest motor operation
     */
    uint32_t intpol : 1 = 0;
    /**
     * Enable double edge step pulses: If set, enable step impulse at each
     * step edge to reduce step frequency requirement
     */
    uint32_t dedge : 1 = 0;
    /**
     * Short to GND protectino disable:
     *
     * 0 = short to gnd protection on
     *
     * 1 = short to gnd protection disabled
     */
    uint32_t diss2g : 1 = 0;
};

/**
 * COOLCONF contains information to configure the Coolstep and Smartguard (SG)
 * features in the TMC2130
 */
struct __attribute__((packed, __may_alias__)) CoolConfig {
    static constexpr Registers address = Registers::COOLCONF;
    static constexpr bool writable = true;
    static constexpr uint32_t value_mask = (1 << 25) - 1;

    /**
     * Minimum SG value for smart current control & smart current enable.
     *
     * If SG result falls below SEMIN*32, motor current increases to
     * reduce motor load angle.
     *
     * 0 = smart current control coolStep OFF
     *
     * 1..15 = set threshold value
     */
    uint32_t semin : 4 = 0;
    uint32_t padding_1 : 1 = 0;
    /**
     * Current up step width: Current increment steps per measured SG value:
     *
     * 1,2,4,8
     */
    uint32_t seup : 2 = 0;
    uint32_t padding_2 : 1 = 0;
    /**
     * If the SG result is >= (SEMIN+SEMAX+1)*32, motor current decreases
     * to save energy.
     */
    uint32_t semax : 4 = 0;
    uint32_t padding_3 : 1 = 0;
    /**
     * Current down step speed:
     *
     * 0: for each 32 SG values, decrease by one
     *
     * 1: for each 8 SG values, decrease by one
     *
     * 2: for each 2 SG values, decrease by one
     *
     * 3: for each SG value, decrease by one
     */
    uint32_t sedn : 2 = 0;
    /**
     * Minimum current for smart current control:
     *
     * 0 = 1/2 of current setting in IRUN
     *
     * 1 = 1/4 of current setting in IRUN
     */
    uint32_t seimin : 1 = 0;
    /**
     * SG Threshold Value: This signed value controlls SG level for stall
     * output and sets optimum measurement range for readout. A lower val
     * gives a higher sensitivity. Zero is starting value working with most
     * motors.
     *
     * -64 to +63: Higher value makes SG less sensitive and requires more
     * torque to indicate a stall
     */
    int32_t sgt : 7 = 0;
    uint32_t padding_4 : 1 = 0;
    /**
     * SG filter enable:
     *
     * 0 = standard mode, high time res for SG
     *
     * 1 = filtered mode, SG signal updated for each 4 full steps
     */
    uint32_t sfilt : 1 = 0;
};

/**
 * Holds error & stallguard information. Read only.
 */
struct __attribute__((packed, __may_alias__)) DriveStatus {
    static constexpr Registers address = Registers::DRVSTATUS;
    static constexpr bool readable = true;
    static constexpr uint32_t value_mask = 0xFFFFFFFF;

    // Stallguard2 Result, represents mechanical load.
    // 0 is max load, 0x3FF is the max load.
    uint32_t sg_result : 10 = 0;
    // Reserved padding
    uint32_t padding_0 : 5 = 0;
    // Fullstep-active indicator
    uint32_t fsactive : 1 = 0;
    // Actual motor current/ smart energy current.
    // For monitoring function of automatic current scaling.
    uint32_t cs_actual : 5 = 0;
    // Rserved padding
    uint32_t padding_1 : 3 = 0;
    // Motor stall detected (sg_result = 0), or DcStep stall
    // in DcStep mode.
    uint32_t stallguard : 1 = 0;
    // If 1, overtemp detected and driver is shut down.
    uint32_t overtemp_flag : 1 = 0;
    // If 1, pre-warning threshold for overtemp is exceeded.
    uint32_t overtemp_prewarning_flag : 1 = 0;
    // Short to ground in phase A
    uint32_t s2ga : 1 = 0;
    // Short to ground in phase B
    uint32_t s2gb : 1 = 0;
    // Open load in phase A
    uint32_t ola : 1 = 0;
    // Open load in phase B
    uint32_t olb : 1 = 0;
    // Standstill indicator. Occurs 2^20 clocks after last step.
    uint32_t stst : 1 = 0;
};

// Encapsulates all of the registers that should be configured by software
struct TMC2130RegisterMap {
    GConfig gconfig = {};
    CurrentControl ihold_irun = {};
    PowerDownDelay tpowerdown = {};
    TCoolThreshold tcoolthrs = {};
    THigh thigh = {};
    ChopConfig chopconf = {};
    CoolConfig coolconf = {};
};

// Registers are all 32 bits
using RegisterSerializedType = uint32_t;
// Type definition to allow type aliasing for pointer dereferencing
using RegisterSerializedTypeA = __attribute__((__may_alias__)) uint32_t;

}  // namespace tmc2130
