
/*
 * Copyright (C) 2000-2001 QuantLib Group
 *
 * This file is part of QuantLib.
 * QuantLib is a C++ open source library for financial quantitative
 * analysts and developers --- http://quantlib.org/
 *
 * QuantLib is free software and you are allowed to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of it under the conditions stated
 * in the QuantLib License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You should have received a copy of the license along with this file;
 * if not, please email quantlib-users@lists.sourceforge.net
 * The license is also available at http://quantlib.org/LICENSE.TXT
 *
 * The members of the QuantLib Group are listed in the Authors.txt file, also
 * available at http://quantlib.org/group.html
*/

/*! \file singleassetoption.hpp
    \brief common code for option evaluation

    \fullpath
    Include/ql/Pricers/%singleassetoption.hpp
*/

// $Id$
// $Log$
// Revision 1.6  2001/08/28 14:47:46  nando
// unsigned int instead of int
//
// Revision 1.5  2001/08/21 16:42:12  nando
// european option optimization
//
// Revision 1.4  2001/08/13 15:06:17  nando
// added dividendRho method
//
// Revision 1.3  2001/08/09 14:59:47  sigmud
// header modification
//
// Revision 1.2  2001/08/07 11:25:54  sigmud
// copyright header maintenance
//
// Revision 1.1  2001/08/07 07:49:45  nando
// forgot to add the singleassetoption.* files
//
// Revision 1.15  2001/07/26 13:56:23  nando
// straddle barrier option handled

#ifndef BSM_option_pricer_h
#define BSM_option_pricer_h

#include "ql/options.hpp"
#include "ql/handle.hpp"
#include "ql/solver1d.hpp"

#define QL_MIN_VOLATILITY 0.0001
#define QL_MAX_VOLATILITY 4.0

namespace QuantLib {

    //! Pricing models for options
    namespace Pricers {


        double ExercisePayoff(Option::Type type, double price, double strike);

        //! Black-Scholes-Merton option
        class SingleAssetOption : public Option {
          public:
            SingleAssetOption(Type type, double underlying, double strike,
                Rate dividendYield, Rate riskFreeRate, Time residualTime,
                double volatility);
            virtual ~SingleAssetOption() {}    // just in case
            // modifiers
            virtual void setVolatility(double newVolatility) ;
            virtual void setRiskFreeRate(Rate newRate) ;
            virtual void setDividendYield(Rate newDividendYield) ;
            // accessors
            virtual double value() const = 0;
            virtual double delta() const = 0;
            virtual double gamma() const = 0;
            virtual double theta() const = 0;
            virtual double vega() const;
            virtual double rho() const;
            virtual double dividendRho() const;
            double impliedVolatility(double targetValue,
                double accuracy = 1e-4, unsigned int maxEvaluations = 100,
                double minVol = QL_MIN_VOLATILITY,
                double maxVol = QL_MAX_VOLATILITY) const ;
            virtual Handle<SingleAssetOption> clone() const = 0;
          protected:
            // results declared as mutable to preserve the logical
            Type type_;
            double underlying_;
            double strike_;
            Rate dividendYield_;
            Time residualTime_;
            mutable bool hasBeenCalculated_;
            mutable double rho_, dividendRho_, vega_;
            mutable bool rhoComputed_, dividendRhoComputed_, vegaComputed_;
            double volatility_;
            Rate riskFreeRate_;
            const static double dVolMultiplier_;
            const static double dRMultiplier_;
          private:
            class VolatilityFunction;
            friend class VolatilityFunction;
        };

        class SingleAssetOption::VolatilityFunction : public ObjectiveFunction {
          public:
            VolatilityFunction(const Handle<SingleAssetOption>& tempBSM, double targetPrice);
            double operator()(double x) const;
          private:
            mutable Handle<SingleAssetOption> bsm;
            double targetPrice_;
        };

        inline SingleAssetOption::VolatilityFunction::VolatilityFunction(
                const Handle<SingleAssetOption>& tempBSM, double targetPrice) {
            bsm = tempBSM;
            targetPrice_ = targetPrice;
        }

        inline double SingleAssetOption::VolatilityFunction::operator()(double x) const {
            bsm -> setVolatility(x);
            return (bsm -> value() - targetPrice_);
        }

    }

}


#endif
