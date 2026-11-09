// COhmsLaw.cpp — implementation of elec::COhmsLaw.
#include "COhmsLaw.h"

#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>

namespace elec {

    COhmsLaw& COhmsLaw::setVoltage(double volts) { return set(V_, volts, "Voltage"); }
    COhmsLaw& COhmsLaw::setCurrent(double amps) { return set(I_, amps, "Current"); }
    COhmsLaw& COhmsLaw::setResistance(double ohms) { return set(R_, ohms, "Resistance", true); }
    COhmsLaw& COhmsLaw::setPower(double watts) { return set(P_, watts, "Power"); }

    void COhmsLaw::clear() { V_.reset(); I_.reset(); R_.reset(); P_.reset(); solved_ = false; }

    void COhmsLaw::solve() {
        const int known = V_.has_value() + I_.has_value() + R_.has_value() + P_.has_value();
        if (known != 2)
            throw std::invalid_argument("Exactly two quantities must be set (got " +
                std::to_string(known) + ").");

        if (V_ && I_) {                       // R = V/I,       P = V*I
            requireNonZero(*I_, "Current");
            R_ = *V_ / *I_;
            P_ = *V_ * *I_;
        }
        else if (V_ && R_) {                // I = V/R,       P = V^2/R
            I_ = *V_ / *R_;
            P_ = (*V_ * *V_) / *R_;
        }
        else if (V_ && P_) {                // I = P/V,       R = V^2/P
            requireNonZero(*V_, "Voltage");
            requireNonZero(*P_, "Power");
            I_ = *P_ / *V_;
            R_ = (*V_ * *V_) / *P_;
        }
        else if (I_ && R_) {                // V = I*R,       P = I^2*R
            V_ = *I_ * *R_;
            P_ = (*I_ * *I_) * *R_;
        }
        else if (I_ && P_) {                // V = P/I,       R = P/I^2
            requireNonZero(*I_, "Current");
            V_ = *P_ / *I_;
            R_ = *P_ / (*I_ * *I_);
        }
        else if (R_ && P_) {                // V = sqrt(P*R), I = sqrt(P/R)
            if (*P_ < 0) throw std::domain_error("Power must be >= 0 when solving from R and P.");
            V_ = std::sqrt(*P_ * *R_);
            I_ = std::sqrt(*P_ / *R_);
        }
        solved_ = true;
    }

    double COhmsLaw::voltage()    const { return get(V_, "Voltage"); }
    double COhmsLaw::current()    const { return get(I_, "Current"); }
    double COhmsLaw::resistance() const { return get(R_, "Resistance"); }
    double COhmsLaw::power()      const { return get(P_, "Power"); }

    std::ostream& operator<<(std::ostream& os, const COhmsLaw& o) {
        auto show = [&](const char* name, const std::optional<double>& v, const char* unit) {
            os << std::setw(11) << std::left << name << ": ";
            if (v) os << *v << ' ' << unit; else os << "(unknown)";
            os << '\n';
            };
        show("Voltage", o.V_, "V");
        show("Current", o.I_, "A");
        show("Resistance", o.R_, "Ohm");
        show("Power", o.P_, "W");
        return os;
    }

    COhmsLaw& COhmsLaw::set(std::optional<double>& slot, double value, const char* name,
        bool mustBePositive) {
        if (!std::isfinite(value))
            throw std::invalid_argument(std::string(name) + " must be a finite number.");
        if (mustBePositive && value <= 0.0)
            throw std::invalid_argument(std::string(name) + " must be > 0.");
        if (solved_) clear();
        slot = value;
        return *this;
    }

    void COhmsLaw::requireNonZero(double v, const char* name) {
        if (v == 0.0)
            throw std::domain_error(std::string(name) + " cannot be zero for this calculation.");
    }

    double COhmsLaw::get(const std::optional<double>& v, const char* name) {
        if (!v) throw std::logic_error(std::string(name) + " is unknown; call solve() first.");
        return *v;
    }

} // namespace elec