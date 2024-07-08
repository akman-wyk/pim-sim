//
// Created by wyk on 11/1/23.
//

#include "energy_counter.h"

namespace pimsim {

double EnergyCounter::running_time_ = 0.0;
bool EnergyCounter::set_running_time_ = false;

EnergyCounter::EnergyCounter(const EnergyCounter& another)
    : static_power_(another.static_power_), dynamic_energy_(another.dynamic_energy_), activity_time_(another.activity_time_) {}

void EnergyCounter::clear() {
    static_power_ = 0.0;
    dynamic_energy_ = 0.0;
    activity_time_ = 0.0;
    dynamic_time_tag_ = std::map<int, sc_core::sc_time>();
}

void EnergyCounter::setStaticPowerMW(double power) {
    static_power_ = power;
}

void EnergyCounter::addDynamicEnergyPJ(double latency, double power) {
    activity_time_ += latency;
    dynamic_energy_ += latency * power;
}

void EnergyCounter::addDynamicEnergyPJ(int tag, const sc_core::sc_time& time, double latency, double power) {
    if (dynamic_time_tag_.find(tag) == dynamic_time_tag_.end()) {
        dynamic_time_tag_.emplace(tag, time);
        addDynamicEnergyPJ(latency, power);
    } else if (dynamic_time_tag_[tag] != time) {
        addDynamicEnergyPJ(latency, power);
    }
}

void EnergyCounter::addPipelineDynamicEnergyPJ(int unit_latency_cycle, int pipeline_length, double period,
                                               double power) {
    if (pipeline_length <= 0) {
        return;
    }

    int total_cycle = (unit_latency_cycle == 0) ? pipeline_length : (unit_latency_cycle - 1 + pipeline_length);
    double total_latency = total_cycle * period;
    addDynamicEnergyPJ(total_latency, power);
}

void EnergyCounter::setRunningTimeNS(double time) {
    running_time_ = time;
    set_running_time_ = true;
}

void EnergyCounter::setRunningTimeNS(const sc_core::sc_time& time) {
    setRunningTimeNS(time.to_seconds() * 1e9);
}

double EnergyCounter::getRunningTimeNS() {
    if (!set_running_time_) {
        throw std::runtime_error("No running time has been set yet");
    }
    return running_time_;
}

double EnergyCounter::getStaticEnergyPJ() const {
    return static_power_ * getRunningTimeNS();  // mW * ns = pJ
}

double EnergyCounter::getDynamicEnergyPJ() const {
    return dynamic_energy_;
}

double EnergyCounter::getActivityTime() const {
    return activity_time_;
}

double EnergyCounter::getTotalEnergyPJ() const {
    return getStaticEnergyPJ() + getDynamicEnergyPJ();
}

double EnergyCounter::getAveragePowerMW() const {
    return getTotalEnergyPJ() / getRunningTimeNS();  // pJ / ns = mW
}

EnergyCounter& EnergyCounter::operator+=(const EnergyCounter& another) {
    activity_time_ += another.activity_time_;
    dynamic_energy_ += another.dynamic_energy_;
    static_power_ += another.static_power_;
    return *this;
}

}  // namespace pim