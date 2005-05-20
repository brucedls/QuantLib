/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003 Ferdinando Ametrano
 Copyright (C) 2000-2005 StatPro Italia srl
 Copyright (C) 2005 Klaus Spanderen

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/reference/license.html>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file multipathgenerator.hpp
    \brief Generates a multi path from a random-array generator
*/

#ifndef quantlib_multi_path_generator_hpp
#define quantlib_multi_path_generator_hpp

#include <ql/stochasticprocess.hpp>
#include <ql/Processes/stochasticprocessarray.hpp>
#include <ql/MonteCarlo/multipath.hpp>
#include <ql/MonteCarlo/sample.hpp>
#include <ql/Math/pseudosqrt.hpp>

namespace QuantLib {

    namespace Old {

        //! Generates a multipath from a random number generator.
        /*! RSG is a sample generator which returns a random sequence.
            It must have the minimal interface:
            \code
            RSG {
                Sample<Array> next();
            };
            \endcode

            \ingroup mcarlo

            \test the generated paths are checked against cached results
        */
        template <class GSG>
        class MultiPathGenerator {
          public:
            typedef Sample<MultiPath> sample_type;
            MultiPathGenerator(
                           const boost::shared_ptr<GenericStochasticProcess>&,
                           const TimeGrid&,
                           GSG generator,
                           bool brownianBridge = false);
            MultiPathGenerator(
                  const std::vector<boost::shared_ptr<StochasticProcess1D> >&,
                  const Matrix& correlation,
                  const TimeGrid& timeGrid,
                  GSG generator,
                  bool brownianBridge = false);
            const sample_type& next() const;
            const sample_type& antithetic() const;
          private:
            const sample_type& next(bool antithetic) const;
            bool brownianBridge_;
            boost::shared_ptr<GenericStochasticProcess> process_;
            GSG generator_;
            mutable sample_type next_;
        };

    }

    namespace New {

        //! Generates a multipath from a random number generator.
        /*! RSG is a sample generator which returns a random sequence.
            It must have the minimal interface:
            \code
            RSG {
                Sample<Array> next();
            };
            \endcode

            \ingroup mcarlo

            \test the generated paths are checked against cached results
        */
        template <class GSG>
        class MultiPathGenerator {
          public:
            typedef Sample<MultiPath> sample_type;
            MultiPathGenerator(
                           const boost::shared_ptr<GenericStochasticProcess>&,
                           const TimeGrid&,
                           GSG generator,
                           bool brownianBridge = false);
            const sample_type& next() const;
            const sample_type& antithetic() const;
          private:
            const sample_type& next(bool antithetic) const;
            bool brownianBridge_;
            boost::shared_ptr<GenericStochasticProcess> process_;
            GSG generator_;
            mutable sample_type next_;
        };

    }

    #ifndef QL_DISABLE_DEPRECATED
    using Old::MultiPathGenerator;
    #else
    using New::MultiPathGenerator;
    #endif

    namespace Old {

        // template definitions

        template <class GSG>
        MultiPathGenerator<GSG>::MultiPathGenerator(
                   const boost::shared_ptr<GenericStochasticProcess>& process,
                   const TimeGrid& times,
                   GSG generator,
                   bool brownianBridge)
        : brownianBridge_(brownianBridge), process_(process),
          generator_(generator),
          next_(MultiPath(process->size(), times), 1.0) {

            QL_REQUIRE(generator_.dimension() ==
                       process->size()*(times.size()-1),
                       "dimension (" << generator_.dimension()
                       << ") is not equal to ("
                       << process->size() << " * " << times.size()-1
                       << ") the number of assets "
                       << "times the number of time steps");
            QL_REQUIRE(times.size() > 1,
                       "no times given");
        }

        template <class GSG>
        MultiPathGenerator<GSG>::MultiPathGenerator(
                   const std::vector<boost::shared_ptr<StochasticProcess1D> >&
                                                               diffusionProcs,
                   const Matrix& correlation,
                   const TimeGrid& times,
                   GSG generator,
                   bool brownianBridge)
        : brownianBridge_(brownianBridge),
          generator_(generator),
          next_(MultiPath(correlation.rows(), times), 1.0) {

            QL_REQUIRE(generator_.dimension() ==
                       diffusionProcs.size()*(times.size()-1),
                       "dimension (" << generator_.dimension()
                       << ") is not equal to ("
                       << diffusionProcs.size() << " * " << times.size()-1
                       << ") the number of assets "
                       << "times the number of time steps");
            QL_REQUIRE(times.size() > 1, "no times given");

            process_ = boost::shared_ptr<GenericStochasticProcess>(
                     new StochasticProcessArray(diffusionProcs, correlation));
        }

        template <class GSG>
        inline const typename MultiPathGenerator<GSG>::sample_type&
        MultiPathGenerator<GSG>::next() const {
            return next(false);
        }

        template <class GSG>
        inline const typename MultiPathGenerator<GSG>::sample_type&
        MultiPathGenerator<GSG>::antithetic() const {
            return next(true);
        }

        template <class GSG>
        const typename MultiPathGenerator<GSG>::sample_type&
        MultiPathGenerator<GSG>::next(bool antithetic) const {

            if (brownianBridge_) {

                QL_FAIL("Brownian bridge not supported");

            } else {

                typedef typename GSG::sample_type sequence_type;
                const sequence_type& sequence_ =
                    antithetic ? generator_.lastSequence()
                               : generator_.nextSequence();

                Size n = process_->size();
                Array asset = process_->initialValues();
                Array temp(n);
                next_.weight = sequence_.weight;

                TimeGrid timeGrid = next_.value[0].timeGrid();
                Time t, dt;
                for (Size i = 0; i < next_.value[0].size(); i++) {
                    Size offset = i*n;
                    t = timeGrid[i];
                    dt = timeGrid.dt(i);
                    std::copy(sequence_.value.begin()+offset,
                              sequence_.value.begin()+offset+n,
                              temp.begin());

                    Array drift = process_->drift(t, asset);
                    Array diffusion = antithetic ?
                        process_->stdDeviation(t, asset, dt)*(-temp) :
                        process_->stdDeviation(t, asset, dt)*temp;
                    Array change(n);
                    for (Size j=0; j<n; j++) {
                        // not fully satisfactory---we should use expectation
                        next_.value[j].drift()[i] = dt * drift[j];
                        // this is ok
                        next_.value[j].diffusion()[i] = diffusion[j];
                        change[j] = next_.value[j][i];
                    }
                    asset = process_->apply(asset, change);
                }
                return next_;
            }
        }

    }


    namespace New {

        template <class GSG>
        MultiPathGenerator<GSG>::MultiPathGenerator(
                   const boost::shared_ptr<GenericStochasticProcess>& process,
                   const TimeGrid& times,
                   GSG generator,
                   bool brownianBridge)
        : brownianBridge_(brownianBridge), process_(process),
          generator_(generator),
          next_(MultiPath(process->size(), times), 1.0) {

            QL_REQUIRE(generator_.dimension() ==
                       process->size()*(times.size()-1),
                       "dimension (" << generator_.dimension()
                       << ") is not equal to ("
                       << process->size() << " * " << times.size()-1
                       << ") the number of assets "
                       << "times the number of time steps");
            QL_REQUIRE(times.size() > 1,
                       "no times given");
        }

        template <class GSG>
        inline const typename MultiPathGenerator<GSG>::sample_type&
        MultiPathGenerator<GSG>::next() const {
            return next(false);
        }

        template <class GSG>
        inline const typename MultiPathGenerator<GSG>::sample_type&
        MultiPathGenerator<GSG>::antithetic() const {
            return next(true);
        }


        template <class GSG>
        const typename MultiPathGenerator<GSG>::sample_type&
        MultiPathGenerator<GSG>::next(bool antithetic) const {

            if (brownianBridge_) {

                QL_FAIL("Brownian bridge not supported");

            } else {

                typedef typename GSG::sample_type sequence_type;
                const sequence_type& sequence_ =
                    antithetic ? generator_.lastSequence()
                               : generator_.nextSequence();

                Size n = process_->size();
                MultiPath& path = next_.value;

                Array asset = process_->initialValues();
                for (Size j=0; j<n; j++)
                    path[j].value(0) = asset[j];

                Array temp(n);
                next_.weight = sequence_.weight;

                TimeGrid timeGrid = path[0].timeGrid();
                Time t, dt;
                for (Size i = 1; i < path.pathSize(); i++) {
                    Size offset = (i-1)*n;
                    t = timeGrid[i-1];
                    dt = timeGrid.dt(i-1);
                    if (antithetic)
                        std::transform(sequence_.value.begin()+offset,
                                       sequence_.value.begin()+offset+n,
                                       temp.begin(),
                                       std::negate<Real>());
                    else
                        std::copy(sequence_.value.begin()+offset,
                                  sequence_.value.begin()+offset+n,
                                  temp.begin());

                    asset = process_->evolve(t, asset, dt, temp);
                    for (Size j=0; j<n; j++)
                        path[j].value(i) = asset[j];
                }
                return next_;
            }
        }

    }

}


#endif
