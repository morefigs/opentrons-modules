#include "catch2/catch.hpp"
#include "systemwide.h"
#include "test/task_builder.hpp"
#include "thermocycler-refresh/messages.hpp"
#include "thermocycler-refresh/motor_task.hpp"
#include "thermocycler-refresh/motor_utils.hpp"

SCENARIO("motor task message passing") {
    GIVEN("a motor task") {
        auto tasks = TaskBuilder::build();
        // auto &motor_task = tasks->get_motor_task();
        auto &motor_policy = tasks->get_motor_policy();
        auto &motor_queue = tasks->get_motor_queue();

        THEN("the TMC2130 is not initialized") {
            REQUIRE(!motor_policy.has_been_written());
        }

        WHEN("sending ActuateSolenoid message to turn solenoid on") {
            auto message =
                messages::ActuateSolenoidMessage{.id = 123, .engage = true};
            motor_queue.backing_deque.push_back(message);
            tasks->run_motor_task();
            THEN(
                "the motor task should tun on the solenoid and respond to the "
                "message") {
                REQUIRE(motor_queue.backing_deque.empty());
                REQUIRE(!tasks->get_host_comms_queue().backing_deque.empty());
            }
            THEN("the TMC2130 has been initialized by the task") {
                REQUIRE(motor_policy.has_been_written());
            }
            THEN("the solenoid should be actuated") {
                REQUIRE(motor_policy.solenoid_engaged());
            }
            AND_THEN("sending ActuateSolenoid message to turn solenoid off") {
                message.id = 456;
                message.engage = false;
                motor_queue.backing_deque.push_back(message);
                tasks->run_motor_task();
                THEN("the solenoid message is received") {
                    REQUIRE(motor_queue.backing_deque.empty());
                }
                THEN("the solenoid is disabled") {
                    REQUIRE(!motor_policy.solenoid_engaged());
                }
            }
        }
        WHEN("sending a LidStepperDebugMessage") {
            static constexpr double ANGLE = 10.0F;
            auto message =
                messages::LidStepperDebugMessage{.id = 123, .angle = ANGLE};
            motor_queue.backing_deque.push_back(message);
            tasks->run_motor_task();
            THEN("the message is received but no response is sent yet") {
                REQUIRE(motor_policy.get_vref() > 0);
                REQUIRE(motor_policy.get_angle() ==
                        motor_util::LidStepper::angle_to_microsteps(ANGLE));
                REQUIRE(motor_queue.backing_deque.empty());
                REQUIRE(tasks->get_host_comms_queue().backing_deque.empty());
            }
            AND_WHEN("sending another one") {
                message.id = 999;
                motor_queue.backing_deque.push_back(message);
                tasks->run_motor_task();
                THEN("the second message is ACKed with an error") {
                    REQUIRE(
                        !tasks->get_host_comms_queue().backing_deque.empty());
                    auto ack =
                        tasks->get_host_comms_queue().backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::AcknowledgePrevious>(
                            ack));
                    auto ack_msg = std::get<messages::AcknowledgePrevious>(ack);
                    REQUIRE(ack_msg.responding_to_id == 999);
                    REQUIRE(ack_msg.with_error ==
                            errors::ErrorCode::LID_MOTOR_BUSY);
                }
            }
            AND_WHEN("receiving a LidStepperComplete message") {
                auto done_msg = messages::LidStepperComplete();
                motor_queue.backing_deque.push_back(done_msg);
                tasks->run_motor_task();
                THEN("the ACK is sent to the host comms task") {
                    REQUIRE(motor_policy.get_vref() == 0);
                    REQUIRE(motor_queue.backing_deque.empty());
                    REQUIRE(
                        !tasks->get_host_comms_queue().backing_deque.empty());
                    auto ack =
                        tasks->get_host_comms_queue().backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::AcknowledgePrevious>(
                            ack));
                    auto ack_msg = std::get<messages::AcknowledgePrevious>(ack);
                    REQUIRE(ack_msg.responding_to_id == 123);
                }
            }
        }
        GIVEN("a lid motor fault") {
            motor_policy.trigger_lid_fault();
            WHEN("sending a LidStepperDebugMessage") {
                static constexpr double ANGLE = 10.0F;
                auto message =
                    messages::LidStepperDebugMessage{.id = 123, .angle = ANGLE};
                motor_queue.backing_deque.push_back(message);
                tasks->run_motor_task();
                THEN("the message is ACKed with an error") {
                    REQUIRE(
                        !tasks->get_host_comms_queue().backing_deque.empty());
                    auto ack =
                        tasks->get_host_comms_queue().backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::AcknowledgePrevious>(
                            ack));
                    auto ack_msg = std::get<messages::AcknowledgePrevious>(ack);
                    REQUIRE(ack_msg.responding_to_id == 123);
                    REQUIRE(ack_msg.with_error ==
                            errors::ErrorCode::LID_MOTOR_FAULT);
                }
                THEN("no motion is started") {
                    REQUIRE(motor_policy.get_vref() == 0);
                }
            }
        }
        WHEN("sending a SealStepperDebugMessage") {
            static constexpr uint32_t STEPS = 10;
            auto message =
                messages::SealStepperDebugMessage{.id = 123, .steps = STEPS};
            motor_queue.backing_deque.push_back(message);
            tasks->run_motor_task();
            THEN("the message is received but no response is sent yet") {
                REQUIRE(motor_policy.seal_moving());
                // True for positive
                REQUIRE(motor_policy.get_tmc2130_direction());
                REQUIRE(motor_policy.get_tmc2130_enabled());
                REQUIRE(motor_queue.backing_deque.empty());
                REQUIRE(tasks->get_host_comms_queue().backing_deque.empty());
            }
            AND_WHEN("sending another one") {
                message.id = 999;
                motor_queue.backing_deque.push_back(message);
                tasks->run_motor_task();
                THEN("the second message is ACKed with an error") {
                    REQUIRE(
                        !tasks->get_host_comms_queue().backing_deque.empty());
                    auto ack =
                        tasks->get_host_comms_queue().backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::AcknowledgePrevious>(
                            ack));
                    auto ack_msg = std::get<messages::AcknowledgePrevious>(ack);
                    REQUIRE(ack_msg.responding_to_id == 999);
                    REQUIRE(ack_msg.with_error ==
                            errors::ErrorCode::SEAL_MOTOR_BUSY);
                }
            }
            WHEN("sending a SealStepperComplete message with a stall") {
                using namespace messages;
                auto stall_msg = SealStepperComplete{
                    .reason = SealStepperComplete::CompletionReason::STALL};
                motor_queue.backing_deque.push_back(stall_msg);
                tasks->run_motor_task();
                THEN("the original message is ACK'd with an error") {
                    REQUIRE(
                        !tasks->get_host_comms_queue().backing_deque.empty());
                    auto ack =
                        tasks->get_host_comms_queue().backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::AcknowledgePrevious>(
                            ack));
                    auto ack_msg = std::get<messages::AcknowledgePrevious>(ack);
                    REQUIRE(ack_msg.responding_to_id == 123);
                    REQUIRE(ack_msg.with_error ==
                            errors::ErrorCode::SEAL_MOTOR_STALL);
                }
                THEN("the seal motor was stopped") {
                    REQUIRE(!motor_policy.seal_moving());
                }
            }
            WHEN("sending a SealStepperComplete message with an error") {
                using namespace messages;
                auto stall_msg = SealStepperComplete{
                    .reason = SealStepperComplete::CompletionReason::ERROR};
                motor_queue.backing_deque.push_back(stall_msg);
                tasks->run_motor_task();
                THEN("the original message is ACK'd with an error") {
                    REQUIRE(
                        !tasks->get_host_comms_queue().backing_deque.empty());
                    auto ack =
                        tasks->get_host_comms_queue().backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::AcknowledgePrevious>(
                            ack));
                    auto ack_msg = std::get<messages::AcknowledgePrevious>(ack);
                    REQUIRE(ack_msg.responding_to_id == 123);
                    REQUIRE(ack_msg.with_error ==
                            errors::ErrorCode::SEAL_MOTOR_FAULT);
                }
                THEN("the seal motor was stopped") {
                    REQUIRE(!motor_policy.seal_moving());
                }
            }
            AND_WHEN("incrementing the tick for up to a second") {
                uint32_t i = 0;
                for (; i < motor_policy.MotorTickFrequency; ++i) {
                    motor_policy.tick();
                    if (!motor_policy.seal_moving()) {
                        break;
                    }
                }
                THEN("the seal movement ends after at least 1 second") {
                    REQUIRE(!motor_policy.seal_moving());
                    REQUIRE(i >= 10);
                }
                THEN("the seal motor has moved 10 steps") {
                    REQUIRE(motor_policy.get_tmc2130_steps() == 10);
                }
                THEN("a SealStepperComplete message is received") {
                    REQUIRE(!motor_queue.backing_deque.empty());
                    auto msg = motor_queue.backing_deque.front();
                    REQUIRE(
                        std::holds_alternative<messages::SealStepperComplete>(
                            msg));
                }
                WHEN("running the task") {
                    tasks->run_motor_task();
                    THEN("an ack is received by the host task") {
                        REQUIRE(motor_queue.backing_deque.empty());
                        REQUIRE(!tasks->get_host_comms_queue()
                                     .backing_deque.empty());
                        auto ack =
                            tasks->get_host_comms_queue().backing_deque.front();
                        REQUIRE(std::holds_alternative<
                                messages::AcknowledgePrevious>(ack));
                        auto ack_msg =
                            std::get<messages::AcknowledgePrevious>(ack);
                        REQUIRE(ack_msg.responding_to_id == 123);
                        REQUIRE(ack_msg.with_error ==
                                errors::ErrorCode::NO_ERROR);
                    }
                }
            }
        }
        WHEN(
            "sending a GetSealDriveStatus command with a stallguard result of "
            "0xF") {
            motor_policy.write_register(tmc2130::Registers::DRVSTATUS, 0xF);
            auto message = messages::GetSealDriveStatusMessage{.id = 123};
            motor_queue.backing_deque.push_back(message);
            tasks->run_motor_task();
            THEN("the message is recieved and ACKd with correct data") {
                REQUIRE(!motor_queue.has_message());
                REQUIRE(tasks->get_host_comms_queue().has_message());
                auto msg = tasks->get_host_comms_queue().backing_deque.front();
                REQUIRE(std::holds_alternative<
                        messages::GetSealDriveStatusResponse>(msg));
                auto response =
                    std::get<messages::GetSealDriveStatusResponse>(msg);
                REQUIRE(response.responding_to_id == message.id);
                REQUIRE(response.status.sg_result == 0xF);
                REQUIRE(response.status.stallguard == 0);
            }
        }
        WHEN("sending a SetSealParameter message with hold current = 0x2F") {
            auto message = messages::SetSealParameterMessage{
                .id = 123,
                .param = motor_util::SealStepper::Parameter::HoldCurrent,
                .value = 1000};
            motor_queue.backing_deque.push_back(message);
            tasks->run_motor_task();
            THEN("the message is recieved and ACKd with correct id") {
                REQUIRE(!motor_queue.has_message());
                REQUIRE(tasks->get_host_comms_queue().has_message());
                auto msg = tasks->get_host_comms_queue().backing_deque.front();
                REQUIRE(
                    std::holds_alternative<messages::AcknowledgePrevious>(msg));
                auto response = std::get<messages::AcknowledgePrevious>(msg);
                REQUIRE(response.responding_to_id == message.id);
                REQUIRE(response.with_error == errors::ErrorCode::NO_ERROR);
            }
            THEN("the current is set to a clamped value of 0x1F (5 bits)") {
                auto reg =
                    motor_policy.read_register(tmc2130::Registers::IHOLD_IRUN);
                uint32_t mask = 0x1F;
                REQUIRE(reg.has_value());
                REQUIRE((reg.value() & mask) == mask);
            }
        }
    }
}