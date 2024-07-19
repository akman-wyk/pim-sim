//
// Created by wyk on 11/2/23.
//

#pragma once
#include <memory>

#include "clock.h"
#include "systemc.h"

namespace pimsim {

// Register with synchronization reset(next clock)
template <class T>
class Register : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(Register);

    Register(const sc_core::sc_module_name& name, Clock* clk)
        : sc_core::sc_module(name), clk_(clk), reset_enable_(false) {
        SC_METHOD(processInput);
        sensitive << input_;

        SC_METHOD(updateOutput);
        sensitive << trigger_;
    }

    void processInput() {
        // when input is different from value, update output at next positive edge
        if (!(value_ == input_.read())) {
            clk_->notifyNextPosEdge(&trigger_);
        }
    }

    void updateOutput() {
        if (clk_->posEdge()) {
            value_ = input_.read();
            if (reset_enable_ && reset_ptr_ != nullptr && reset_ptr_->read()) {
                value_ = reset_value_;
            }
            output_.write(value_);
        }
    }

    void setReset(const T& reset_value, const sc_core::sc_signal<bool>& reset_signal) {
        reset_enable_ = true;
        reset_value_ = reset_value;
        reset_ptr_ = std::make_shared<sc_core::sc_in<bool>>();
        reset_ptr_->bind(reset_signal);
    }

public:
    sc_core::sc_in<T> input_;
    sc_core::sc_out<T> output_;

private:
    Clock* clk_;
    T value_;
    sc_core::sc_event trigger_;

    std::shared_ptr<sc_core::sc_in<bool>> reset_ptr_;
    bool reset_enable_;
    T reset_value_;
};

// Register with enable wire and synchronization reset(next clock)
template <class T>
class RegEnable : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(RegEnable);

    RegEnable(const sc_core::sc_module_name& name, Clock* clk)
        : sc_core::sc_module(name), clk_(clk), reset_enable_(false) {
        SC_METHOD(processInput)
        sensitive << input_;

        SC_METHOD(processEnable);
        sensitive << enable_;

        SC_METHOD(updateOutput)
        sensitive << trigger_;
    }

    void processInput() {
        // when input is different from value, update output at next positive edge
        if (!(value_ == input_.read())) {
            clk_->notifyNextPosEdge(&trigger_);
        }
    }

    void processEnable() {
        // when enable is true, update output at next positive edge
        if (enable_.read()) {
            clk_->notifyNextPosEdge(&trigger_);
        }
    }

    void updateOutput() {
        if (enable_.read() && clk_->posEdge()) {
            value_ = input_.read();
            if (reset_enable_ && reset_ptr_ != nullptr && reset_ptr_->read()) {
                value_ = reset_value_;
            }
            output_.write(value_);
        }
    }

    void setReset(const T& reset_value, const sc_core::sc_signal<bool>& reset_signal) {
        reset_enable_ = true;
        reset_value_ = reset_value;
        reset_ptr_ = std::make_shared<sc_core::sc_in<bool>>();
        reset_ptr_->bind(reset_signal);
    }

public:
    sc_core::sc_in<T> input_;
    sc_core::sc_in<bool> enable_;
    sc_core::sc_out<T> output_;

private:
    Clock* clk_;
    T value_;
    sc_core::sc_event trigger_;

    std::shared_ptr<sc_core::sc_in<bool>> reset_ptr_;
    bool reset_enable_;
    T reset_value_;
};

}  // namespace pimsim
