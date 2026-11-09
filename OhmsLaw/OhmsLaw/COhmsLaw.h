// COhmsLaw.h — public interface for the Ohm's law / power solver.
// Provide ANY two of {Voltage, Current, Resistance, Power}; solve() computes the rest.
#pragma once

#include <iosfwd>     // forward-declares std::ostream (lighter than <iostream>)
#include <optional>

namespace elec {

    class COhmsLaw {
    public:
        // Setters are chainable. Setting a value after solve() starts a new problem.
        COhmsLaw& setVoltage(double volts);
        COhmsLaw& setCurrent(double amps);
        COhmsLaw& setResistance(double ohms);   // must be > 0
        COhmsLaw& setPower(double watts);

        void clear();

        // Throws std::invalid_argument unless exactly two values are set;
        // throws std::domain_error for physically impossible inputs (e.g. divide by zero).
        void solve();

        // Throw std::logic_error if the value is still unknown.
        double voltage() const;
        double current() const;
        double resistance() const;
        double power() const;

        // Cheap queries are fine to define inline in the header.
        bool isSolved() const noexcept { return solved_; }

        friend std::ostream& operator<<(std::ostream& os, const COhmsLaw& o);

    private:
        std::optional<double> V_, I_, R_, P_;
        bool solved_ = false;

        COhmsLaw& set(std::optional<double>& slot, double value, const char* name,
            bool mustBePositive = false);
        static void requireNonZero(double v, const char* name);
        static double get(const std::optional<double>& v, const char* name);
    };

} // namespace elec