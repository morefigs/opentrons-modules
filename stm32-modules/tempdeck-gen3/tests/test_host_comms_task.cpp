#include "catch2/catch.hpp"
#include "test/test_tasks.hpp"

SCENARIO("usb message parsing") {
    GIVEN("a host_comms_task") {
        auto *tasks = new tasks::TestTasks();
        std::string tx_buf(128, 'c');
        WHEN("calling run_once() with nothing in the queue") {
            THEN("the task should call recv(), which should throw") {
                REQUIRE_THROWS(
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end()));
            }
        }
        WHEN("calling run_once() with an empty gcode message") {
            auto message_text = std::string("\n");
            auto message_obj =
                messages::HostCommsMessage(messages::IncomingMessageFromHost(
                    &*message_text.begin(), &*message_text.end()));
            tasks->_comms_queue.backing_deque.push_back(message_obj);
            THEN("the task should call recv() and get the message") {
                REQUIRE_NOTHROW(
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end()));
                REQUIRE(tasks->_comms_queue.backing_deque.empty());
            }
            THEN("the task writes nothing to the transmit buffer") {
                auto written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                REQUIRE(written == tx_buf.begin());
                REQUIRE_THAT(tx_buf,
                             Catch::Matchers::Equals(std::string(128, 'c')));
            }
        }
        WHEN(
            "calling run_once() with an insufficient tx buffer when it wants "
            "to write data") {
            auto message_text = std::string("aslkdhasd\n");
            auto message_obj =
                messages::HostCommsMessage(messages::IncomingMessageFromHost(
                    &*message_text.begin(), &*message_text.end()));
            tasks->_comms_queue.backing_deque.push_back(message_obj);
            std::string small_buf(64, 'c');
            auto ends_at =
                errors::write_into(small_buf.begin(), small_buf.end(),
                                   errors::ErrorCode::USB_TX_OVERRUN);
            small_buf.resize(ends_at - small_buf.begin() - 5);
            auto written =
                tasks->_comms_task.run_once(small_buf.begin(), small_buf.end());
            REQUIRE_THAT(small_buf,
                         Catch::Matchers::Equals("ERR001:tx buffer ove"));
            REQUIRE(written ==
                    small_buf.begin() + strlen("ERR001:tx buffer ove"));
        }
        WHEN("calling run_once() with a malformed gcode message") {
            auto message_text = std::string("aosjhdakljshd\n");
            auto message_obj =
                messages::HostCommsMessage(messages::IncomingMessageFromHost(
                    &*message_text.begin(), &*message_text.end()));
            tasks->_comms_queue.backing_deque.push_back(message_obj);
            THEN("the task writes an error to the transmit buffer") {
                auto written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(
                                         "ERR003:unhandled gcode\n"));
                REQUIRE(written ==
                        tx_buf.begin() + strlen("ERR003:unhandled gcode\n"));
            }
        }
    }
}

SCENARIO("host comms commands to system task") {
    auto *tasks = tasks::BuildTasks();
    std::string tx_buf(128, 'c');
    WHEN("sending gcode M115") {
        auto message_text = std::string("M115\n");
        auto message_obj =
            messages::HostCommsMessage(messages::IncomingMessageFromHost(
                &*message_text.begin(), &*message_text.end()));
        REQUIRE(tasks->_comms_queue.try_send(message_obj));
        auto written =
            tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
        THEN("the task does not immediately ack") {
            REQUIRE(written == tx_buf.begin());
        }
        THEN("a message is sent to the system task") {
            REQUIRE(tasks->_system_queue.has_message());
            auto sys_msg = tasks->_system_queue.backing_deque.front();
            REQUIRE(std::holds_alternative<messages::GetSystemInfoMessage>(
                sys_msg));
            auto id = std::get<messages::GetSystemInfoMessage>(sys_msg).id;
            AND_WHEN("sending a response") {
                auto resp = messages::GetSystemInfoResponse{
                    .responding_to_id = id,
                    .serial_number = {'a', 'b', 'c'},
                    .fw_version = "def",
                    .hw_version = "ghi"};
                REQUIRE(tasks->_comms_queue.try_send(resp));
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("an ack is sent") {
                    auto expected = "M115 FW:def HW:ghi SerialNo:abc OK\n";
                    REQUIRE(written != tx_buf.begin());
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
        }
    }
    WHEN("sending SetSerialNumber gcode") {
        auto message_text = std::string("M996 Serial1234\n");
        auto message_obj =
            messages::HostCommsMessage(messages::IncomingMessageFromHost(
                &*message_text.begin(), &*message_text.end()));
        REQUIRE(tasks->_comms_queue.try_send(message_obj));
        auto written =
            tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
        THEN("the task does not immediately ack") {
            REQUIRE(written == tx_buf.begin());
        }
        THEN("a message is sent to the system task") {
            REQUIRE(tasks->_system_queue.has_message());
            auto sys_msg = tasks->_system_queue.backing_deque.front();
            REQUIRE(std::holds_alternative<messages::SetSerialNumberMessage>(
                sys_msg));
            auto id = std::get<messages::SetSerialNumberMessage>(sys_msg).id;
            auto *ser = &std::get<messages::SetSerialNumberMessage>(sys_msg)
                             .serial_number[0];
            REQUIRE_THAT(std::string(ser),
                         Catch::Matchers::StartsWith("Serial1234"));
            AND_WHEN("sending a response") {
                auto resp =
                    messages::AcknowledgePrevious{.responding_to_id = id};
                REQUIRE(tasks->_comms_queue.try_send(resp));
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("an ack is sent") {
                    auto expected = "M996 OK\n";
                    REQUIRE(written != tx_buf.begin());
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
        }
    }
    WHEN("sending dfu command") {
        auto message_text = std::string("dfu\n");
        auto message_obj =
            messages::HostCommsMessage(messages::IncomingMessageFromHost(
                &*message_text.begin(), &*message_text.end()));
        REQUIRE(tasks->_comms_queue.try_send(message_obj));
        auto written =
            tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
        THEN("the task does not immediately ack") {
            REQUIRE(written == tx_buf.begin());
        }
        THEN("a message is sent to the system task") {
            REQUIRE(tasks->_system_queue.has_message());
            auto sys_msg = tasks->_system_queue.backing_deque.front();
            REQUIRE(std::holds_alternative<messages::EnterBootloaderMessage>(
                sys_msg));
        }
    }
}

SCENARIO("host comms commands to thermal task") {
    auto *tasks = tasks::BuildTasks();
    std::string tx_buf(128, 'c');
    WHEN("sending gcode M105.D") {
        auto message_text = std::string("M105.D\n");
        auto message_obj =
            messages::HostCommsMessage(messages::IncomingMessageFromHost(
                &*message_text.begin(), &*message_text.end()));
        REQUIRE(tasks->_comms_queue.try_send(message_obj));
        auto written =
            tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
        THEN("the task does not immediately ack") {
            REQUIRE(written == tx_buf.begin());
        }
        THEN("a message is sent to the thermal task") {
            REQUIRE(tasks->_thermal_queue.has_message());
            auto thermal_msg = tasks->_thermal_queue.backing_deque.front();
            REQUIRE(std::holds_alternative<messages::GetTempDebugMessage>(
                thermal_msg));
            auto id = std::get<messages::GetTempDebugMessage>(thermal_msg).id;
            AND_WHEN("sending response with wrong id") {
                auto response =
                    messages::GetTempDebugResponse{.responding_to_id = id + 1,
                                                   .plate_temp = 1.0,
                                                   .heatsink_temp = 2.0,
                                                   .plate_adc = 123,
                                                   .heatsink_adc = 456};
                tasks->_comms_queue.backing_deque.push_back(response);
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("an error is printed") {
                    auto expected = errorstring(
                        errors::ErrorCode::BAD_MESSAGE_ACKNOWLEDGEMENT);
                    REQUIRE(written == (tx_buf.begin() + strlen(expected)));
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
            AND_WHEN("sending a good response") {
                auto response =
                    messages::GetTempDebugResponse{.responding_to_id = id,
                                                   .plate_temp = 1.0,
                                                   .heatsink_temp = 2.0,
                                                   .plate_adc = 123,
                                                   .heatsink_adc = 456};
                tasks->_comms_queue.backing_deque.push_back(response);
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("the data is printed") {
                    auto expected =
                        "M105.D PT:1.00 HST:2.00 PA:123 HSA:456 OK\n";
                    REQUIRE(written == (tx_buf.begin() + strlen(expected)));
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
        }
    }
    WHEN("sending gcode M104.D") {
        auto message_text = std::string("M104.D S1\n");
        auto message_obj =
            messages::HostCommsMessage(messages::IncomingMessageFromHost(
                &*message_text.begin(), &*message_text.end()));
        REQUIRE(tasks->_comms_queue.try_send(message_obj));
        auto written =
            tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
        THEN("the task does not immediately ack") {
            REQUIRE(written == tx_buf.begin());
        }
        THEN("a message is sent to the thermal task") {
            REQUIRE(tasks->_thermal_queue.has_message());
            auto thermal_msg = tasks->_thermal_queue.backing_deque.front();
            REQUIRE(std::holds_alternative<messages::SetPeltierDebugMessage>(
                thermal_msg));
            auto id =
                std::get<messages::SetPeltierDebugMessage>(thermal_msg).id;
            AND_WHEN("sending response with wrong id") {
                auto response =
                    messages::AcknowledgePrevious{.responding_to_id = id + 1};
                tasks->_comms_queue.backing_deque.push_back(response);
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("an error is printed") {
                    auto expected = errorstring(
                        errors::ErrorCode::BAD_MESSAGE_ACKNOWLEDGEMENT);
                    REQUIRE(written == (tx_buf.begin() + strlen(expected)));
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
            AND_WHEN("sending an error response") {
                auto response = messages::AcknowledgePrevious{
                    .responding_to_id = id,
                    .with_error = errors::ErrorCode::THERMAL_PELTIER_ERROR};
                tasks->_comms_queue.backing_deque.push_back(response);
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("an ack is printed") {
                    auto expected =
                        errorstring(errors::ErrorCode::THERMAL_PELTIER_ERROR);
                    REQUIRE(written == (tx_buf.begin() + strlen(expected)));
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
            AND_WHEN("sending a good response") {
                auto response =
                    messages::AcknowledgePrevious{.responding_to_id = id};
                tasks->_comms_queue.backing_deque.push_back(response);
                written =
                    tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
                THEN("an ack is printed") {
                    auto expected = "M104.D OK\n";
                    REQUIRE(written == (tx_buf.begin() + strlen(expected)));
                    REQUIRE_THAT(tx_buf, Catch::Matchers::StartsWith(expected));
                }
            }
        }
    }
}

SCENARIO("host comms usb disconnect") {
    auto *tasks = tasks::BuildTasks();
    std::string tx_buf(128, 'c');
    WHEN("sending a DisconnectUSB message") {
        auto msg = messages::ForceUSBDisconnect{
            .id = 123,
            .return_address = tasks::TestTasks::Queues::SystemAddress};
        tasks->_comms_queue.backing_deque.push_back(msg);
        tasks->_comms_task.run_once(tx_buf.begin(), tx_buf.end());
        THEN("the host comms task disables USB") {
            REQUIRE(!tasks->_comms_task.may_connect());
        }
        THEN("an ack is sent to system task") {
            REQUIRE(tasks->_system_queue.has_message());
            auto sys_msg = tasks->_system_queue.backing_deque.front();
            REQUIRE(
                std::holds_alternative<messages::AcknowledgePrevious>(sys_msg));
            auto ack = std::get<messages::AcknowledgePrevious>(sys_msg);
            REQUIRE(ack.responding_to_id == msg.id);
        }
    }
}