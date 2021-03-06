/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Aboria.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef CHEBYSHEV_TEST_H_
#define CHEBYSHEV_TEST_H_

#include <cxxtest/TestSuite.h>

#include <random>
#include <time.h>
#include <chrono>
typedef std::chrono::system_clock Clock;
#include "Level1.h"
#include "Chebyshev.h"

using namespace Aboria;

class ChebyshevTest : public CxxTest::TestSuite {
public:
#ifdef HAVE_EIGEN
    template <unsigned int D>
    void helper_Rn_calculation(void) {
        const double tol = 1e-10;
        // randomly generate a bunch of positions over a range 
        std::uniform_real_distribution<double> U(-10,100);
        generator_type generator(time(NULL));
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        const size_t N = 50;
        std::vector<double_d> positions(N);
        for (int i=0; i<N; i++) {
            for (int d=0; d<D; ++d) {
                positions[i][d] = U(generator);
            }
        }
        detail::Chebyshev_Rn<D> Rn;
        for (int n=1; n<10; ++n) {
            Rn.calculate_Sn(std::begin(positions),N,n);
            const int_d start = int_d(0);
            const int_d end = int_d(n-1);
            auto range = iterator_range<lattice_iterator<D>>(
                lattice_iterator<D>(start,end,start)
                ,++lattice_iterator<D>(start,end,end)
                );
            const double_d scale = double_d(1.0)/(Rn.box.bmax-Rn.box.bmin);
            for (int i=0; i<positions.size(); ++i) {
                const double_d &x =  (2*positions[i]-Rn.box.bmin-Rn.box.bmax)*scale;
                for (const int_d& m: range) {
                    TS_ASSERT_DELTA(Rn(m,i),detail::chebyshev_Rn_slow(x,m,n),tol);
                }
            }
        }
    }

    template <unsigned int D>
    void helper_chebyshev_interpolation(void) {
        const double tol = 1e-10;
        // randomly generate a bunch of positions over a range 
        const double pos_min = 0;
        const double pos_max = 1;
        std::uniform_real_distribution<double> U(pos_min,pos_max);
        generator_type generator(time(NULL));
        auto gen = std::bind(U, generator);
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        const size_t N = 1000;

        ABORIA_VARIABLE(source,double,"source");
        ABORIA_VARIABLE(target_algorithm,double,"target algorithm");
        ABORIA_VARIABLE(target_manual,double,"target manual");
        ABORIA_VARIABLE(target_operator,double,"target operator");
        typedef Particles<std::tuple<source,target_algorithm,target_manual,target_operator>,D> ParticlesType;
        typedef typename ParticlesType::position position;
        ParticlesType particles(N);

        for (int i=0; i<N; i++) {
            for (int d=0; d<D; ++d) {
                get<position>(particles)[i][d] = gen();
                get<source>(particles)[i] = gen();
            }
        }

        // generate a source vector using a smooth cosine
        auto source_fn = [&](const double_d &p) {
            //return (p-double_d(0)).norm();
            double ret=1.0;
            const double scale = 2.0*detail::PI/(pos_max-pos_min); 
            for (int i=0; i<D; i++) {
                ret *= cos((p[i]-pos_min)*scale);
            }
            return ret;
        };
        std::transform(std::begin(get<position>(particles)), std::end(get<position>(particles)), 
                       std::begin(get<source>(particles)), source_fn);

        const double c = 0.1;
        auto kernel = [&c](const double_d &dx, const double_d &pa, const double_d &pb) {
            return std::sqrt(dx.squaredNorm() + c); 
        };


        // perform the operation manually
        std::fill(std::begin(get<target_manual>(particles)), std::end(get<target_manual>(particles)),
                    0.0);

        auto t0 = Clock::now();
        for (int i=0; i<N; i++) {
            const double_d pi = get<position>(particles)[i];
            for (int j=0; j<N; j++) {
                const double_d pj = get<position>(particles)[j];
                get<target_manual>(particles)[i] += kernel(pi-pj,pi,pj)*get<source>(particles)[j];
            }
        }
        auto t1 = Clock::now();
        std::chrono::duration<double> time_manual = t1 - t0;

        const double scale = std::accumulate(
            std::begin(get<target_manual>(particles)), std::end(get<target_manual>(particles)),
            0.0,
            [](const double t1, const double t2) { return t1 + t2*t2; }
        );

        const unsigned int maxn = std::pow(N/2,1.0/D);
        for (unsigned int n = 1; n < maxn; ++n) {
            // perform the operation using chebyshev interpolation algorithm
            t0 = Clock::now();
            chebyshev_interpolation<D>(
             std::begin(get<source>(particles)), std::end(get<source>(particles)),
             std::begin(get<target_algorithm>(particles)), std::end(get<target_algorithm>(particles)),
             std::begin(get<position>(particles)), std::begin(get<position>(particles)),
             kernel,n);
            t1 = Clock::now();
            std::chrono::duration<double> time_alg = t1 - t0;

            const double L2_alg = std::inner_product(
             std::begin(get<target_algorithm>(particles)), std::end(get<target_algorithm>(particles)),
             std::begin(get<target_manual>(particles)), 
                        0.0,
                        [](const double t1, const double t2) { return t1 + t2; },
                        [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                       );


            std::cout << "dimension = "<<D<<". n = "<<n<<". L2_alg error = "<<L2_alg<<". L2_alg relative error is "<<std::sqrt(L2_alg/scale)<<". time_alg/time_manual = "<<time_alg/time_manual<<std::endl;


            // perform the operation using chebyshev interpolation operator 
            t0 = Clock::now();
            auto C = create_chebyshev_operator(particles,particles,n,kernel);
            t1 = Clock::now();
            std::chrono::duration<double> time_op_setup = t1 - t0;
            typedef Eigen::Matrix<double,Eigen::Dynamic,1> vector_type;
            typedef Eigen::Map<vector_type> map_type;
            map_type source_vect(get<source>(particles).data(),N);
            map_type target_vect(get<target_operator>(particles).data(),N);
            t0 = Clock::now();
            target_vect = C*source_vect;
            t1 = Clock::now();
            std::chrono::duration<double> time_op_mult = t1 - t0;

            const double L2_op= std::inner_product(
             std::begin(get<target_operator>(particles)), std::end(get<target_operator>(particles)),
             std::begin(get<target_manual>(particles)), 
                        0.0,
                        [](const double t1, const double t2) { return t1 + t2; },
                        [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                       );

            std::cout << "dimension = "<<D<<". n = "<<n<<". L2_op error = "<<L2_op<<". L2_op relative error is "<<std::sqrt(L2_op/scale)<<". time_op/time_manual = "<<(time_op_setup+time_op_mult)/time_manual<<std::endl;
            /*
            std::cout << "time_op_setup = "<<time_op_setup/(time_op_setup+time_op_mult)
                      << "time_op_mult = "<<time_op_mult/(time_op_setup+time_op_mult)
                      << std::endl;
             */

            //TODO: is there a better test than this, maybe shouldn't randomly do it?
            if (D==2 && n >=10) TS_ASSERT_LESS_THAN(std::sqrt(L2_alg/scale),0.001);
            if (D==2 && n >=10) TS_ASSERT_LESS_THAN(std::sqrt(L2_op/scale),0.001);
        }
    }
#endif 

        


    void test_chebyshev_polynomial_calculation(void) {
#ifdef HAVE_EIGEN
        const double tol = 1e-10;
        // evaluate polynomial of order k at i-th root
        // of polynomial of order n
        // should by cos(k*(2*i-1)/(2*n)*PI
        std::cout << "testing polynomial calculation..." << std::endl;
        const int n = 4;
        for (int i=0; i<n; ++i) {
            const double x = cos((2.0*i+1.0)/(2.0*n)*detail::PI);
            for (int k=0; k<n; ++k) {
                    TS_ASSERT_DELTA(detail::chebyshev_polynomial(x,k),
                                    cos(k*(2.0*i+1.0)/(2.0*n)*detail::PI),tol);
            }
        }
#endif
    }


    void test_chebyshev_interpolation(void) {
#ifdef HAVE_EIGEN
        std::cout << "testing 2D..." << std::endl;
        helper_chebyshev_interpolation<2>();
        std::cout << "testing 3D..." << std::endl;
        helper_chebyshev_interpolation<3>();
        std::cout << "testing 4D..." << std::endl;
        helper_chebyshev_interpolation<4>();
#endif
    }


    void test_Rn_calculation(void) {
#ifdef HAVE_EIGEN
        std::cout << "testing 1D..." << std::endl;
        helper_Rn_calculation<1>();
        std::cout << "testing 2D..." << std::endl;
        helper_Rn_calculation<2>();
        std::cout << "testing 3D..." << std::endl;
        helper_Rn_calculation<3>();
        std::cout << "testing 4D..." << std::endl;
        helper_Rn_calculation<4>();
#endif
    }
};


#endif
