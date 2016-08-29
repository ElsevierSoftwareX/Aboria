
#ifndef TRAITS_H_ 
#define TRAITS_H_ 

#include "Vector.h"
#include "CudaInclude.h"
#include "Get.h"
#include <tuple>
#include <vector>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>



namespace Aboria {


struct fbool {
    fbool() {};
    fbool(const bool& arg):data(arg) {};
    uint8_t data;
};

struct default_traits {
    template <typename T>
    struct vector_type {
        typedef std::vector<T> type;
    };

    /*
    template <typename ... TupleTypes >
    struct tuple_type {
        typedef std::tuple<TupleTypes...> type;
    };
    */


    template <typename value_type> 
    struct iter_comp {
        bool operator()(const value_type& t1, const value_type& t2) { return get<0>(t1.get_tuple()) < get<0>(t2.get_tuple()); }
    };

    template< class InputIt, class UnaryFunction >
    static UnaryFunction for_each( InputIt first, InputIt last, UnaryFunction f ) {
        return std::for_each(first,last,f);
    }

    template<typename T1, typename T2>
    static void sort_by_key(T1 start_keys,
            T1 end_keys,
            T2 start_data) {

        typedef zip_iterator<tuple_ns::tuple<T1,T2>> pair_zip_type;
        typedef typename pair_zip_type::reference reference;
        typedef typename pair_zip_type::value_type value_type;

        std::sort(
                pair_zip_type(start_keys,start_data),
                pair_zip_type(end_keys,start_data+std::distance(start_keys,end_keys)),
                iter_comp<value_type>());
    }

    template <typename T> 
    struct lower_bound_impl {
        const T& values_first,values_last;
        lower_bound_impl(const T& values_first, const T& values_last):
            values_first(values_first),values_last(values_last) {}
        typedef typename std::iterator_traits<T>::value_type value_type;
        typedef typename std::iterator_traits<T>::difference_type difference_type;
        difference_type operator()(const value_type & search) { 
            return std::distance(values_first,std::lower_bound(values_first,values_last,search)); 
        }
    };

    template <typename T> 
    struct upper_bound_impl {
        const T& values_first,values_last;
        upper_bound_impl(const T& values_first, const T& values_last):
            values_first(values_first),values_last(values_last) {}
        typedef typename std::iterator_traits<T>::value_type value_type;
        typedef typename std::iterator_traits<T>::difference_type difference_type;
        difference_type operator()(const value_type & search) { 
            return std::distance(values_first,std::upper_bound(values_first,values_last,search)); 
        }
    };

    template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
    static void lower_bound(
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result) {

        std::transform(values_first,values_last,result,lower_bound_impl<ForwardIterator>(first,last));
    }

    template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
    static void upper_bound(
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result) {

        std::transform(values_first,values_last,result,upper_bound_impl<ForwardIterator>(first,last));
    }

    template< class InputIt, class T, class BinaryOperation >
    static T reduce( 
        InputIt first, 
        InputIt last, T init,
        BinaryOperation op ) {

        return std::accumulate(first,last,init,op);
    }

    template <class InputIterator, class OutputIterator, class UnaryOperation>
    static OutputIterator transform (
            InputIterator first, InputIterator last,
            OutputIterator result, UnaryOperation op) {
        
        return std::transform(first,last,result,op);
    }


    template <typename T>
    using counting_iterator = boost::counting_iterator<T>;

    static const boost::lambda::placeholder1_type _1;
    static const boost::lambda::placeholder2_type _2;
    static const boost::lambda::placeholder3_type _3;

    template <class ForwardIterator>
    static void sequence (ForwardIterator first, ForwardIterator last) {
        counting_iterator<unsigned int> count(0);
        std::transform(first,last,count,first,
            [](const typename std::iterator_traits<ForwardIterator>::reference, const unsigned int i) {
                return i;
            });
    }

    template<typename ForwardIterator , typename UnaryOperation >
    static void tabulate (
            ForwardIterator first,
            ForwardIterator last,
            UnaryOperation  unary_op) {	

        counting_iterator<unsigned int> count(0);
        std::transform(first,last,count,first,
            [&unary_op](const typename std::iterator_traits<ForwardIterator>::reference, const unsigned int i) {
                return unary_op(i);
            });

    }

    template<typename InputIterator, typename OutputIterator, typename UnaryFunction, 
        typename T, typename AssociativeOperator>
    static OutputIterator transform_exclusive_scan(
        InputIterator first, InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op, T init, AssociativeOperator binary_op) {

        const size_t n = last-first;
        result[0] = 0;
        for (int i=1; i<n; ++i) {
            result[i] = binary_op(result[i-1],unary_op(first[i]));
        }
    }


    template<typename InputIterator1, typename InputIterator2, 
        typename InputIterator3, typename RandomAccessIterator , typename Predicate >
    static void scatter_if(
            InputIterator1 first, InputIterator1 last,
            InputIterator2 map, InputIterator3 stencil,
            RandomAccessIterator output, Predicate pred) {

        const size_t n = last-first;
        for (int i=0; i<n; ++i) {
            if (pred(stencil[i])) {
                output[map[i]] = first[i];
            }
        }
    }

    template<typename InputIterator1, typename InputIterator2, 
        typename OutputIterator, typename Predicate>
    static OutputIterator copy_if(
            InputIterator1 first, InputIterator1 last, 
            InputIterator2 stencil, OutputIterator result, Predicate pred) {

        const size_t n = last-first;
        for (int i=0; i<n; ++i) {
            if (pred(stencil[i])) {
                *result = first[i];
                ++result;
            }
        }
        return result;
    }
};

template<template<typename,typename> class VECTOR>
struct Traits {};

template <>
struct Traits<std::vector>: public default_traits {};

#ifdef HAVE_THRUST
template <>
struct Traits<thrust::host_vector>: public default_traits {
    template <typename T>
    struct vector_type {
        typedef thrust::host_vector<T> type;
    };

    template <typename ... TupleTypes >
    struct tuple_type {
        typedef thrust::tuple<TupleTypes...> type;
    };

    template <typename T>
    using counting_iterator = thrust::counting_iterator<T>;

    #ifdef __CUDA_ARCH__
    static const thrust::detail::functional::placeholder<0>::type _1;
    static const thrust::detail::functional::placeholder<1>::type _2;
    static const thrust::detail::functional::placeholder<2>::type _3;
    #else
    static const thrust::detail::functional::placeholder<0>::type _1;
    static const thrust::detail::functional::placeholder<1>::type _2;
    static const thrust::detail::functional::placeholder<2>::type _3;
    #endif

    template<typename std_tuple_type>
    struct thrust_tuple_helper {};

    template<typename... Iterators>
    struct thrust_tuple_helper<std::tuple<Iterators...>> {
        typedef thrust::tuple<Iterators...> type;
    };

    template<typename std_tuple_type, std::size_t... I>
    static typename thrust_tuple_helper<std_tuple_type>::type 
    make_thrust_tuple_impl(std_tuple_type& data, index_sequence<I...>) {
        return thrust::make_tuple(std::get<I>(data)...);
    }

    template<typename std_tuple_type, 
        typename Indices = make_index_sequence<std::tuple_size<std_tuple_type>::value>>
    static typename thrust_tuple_helper<std_tuple_type>::type 
    make_thrust_tuple(std_tuple_type& data) {
        return make_thrust_tuple_impl(data, Indices());
    }

    template<typename... Iterators>
    static thrust::zip_iterator<thrust::tuple<Iterators...>> 
    make_thrust_zip_iterator(std::tuple<Iterators...> its) {
        return thrust::make_zip_iterator(make_thrust_tuple(its));
    }

    template<typename... Iterators>
    static thrust::zip_iterator<thrust::tuple<Iterators...>> 
    make_thrust_zip_iterator(thrust::tuple<Iterators...> its) {
        return thrust::make_zip_iterator(its);
    }

    template< class InputIt, class UnaryFunction >
    static UnaryFunction for_each( InputIt first, InputIt last, UnaryFunction f ) {
        return std::for_each(first,last,f);
    }

    template<typename T1, typename T2>
    static void sort_by_key_impl(T1 start_keys,
            T1 end_keys,
            T2 start_data,
            mpl::bool_<true>) {
        thrust::sort_by_key(start_keys,end_keys,make_thrust_zip_iterator(start_data.get_tuple()));
    }

    template<typename T1, typename T2>
    static void sort_by_key_impl(T1 start_keys,
            T1 end_keys,
            T2 start_data,
            mpl::bool_<false>) {
        thrust::sort_by_key(start_keys,end_keys,start_data);
    }

    template<typename T1, typename T2>
    static void sort_by_key(T1 start_keys,
            T1 end_keys,
            T2 start_data) {
        sort_by_key_impl(start_keys,end_keys,start_data,typename is_zip_iterator<T2>::type());
    }

    template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
    static void lower_bound(
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result) {
        thrust::lower_bound(first,last,values_first,values_last,result);
    }

    template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
    static void upper_bound(
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result) {
        thrust::upper_bound(first,last,values_first,values_last,result);
    }

    template< class InputIt, class T, class BinaryOperation >
    static T reduce( 
        InputIt first, 
        InputIt last, T init,
        BinaryOperation op ) {
        return thrust::reduce(first,last,init,op);
    }

    template <class InputIterator, class OutputIterator, class UnaryOperation>
    static OutputIterator transform (
            InputIterator first, InputIterator last,
            OutputIterator result, UnaryOperation op) {
        return thrust::transform(first,last,result,op);
    }

    template <class ForwardIterator>
    static void sequence (ForwardIterator first, ForwardIterator last) {
        thrust::sequence(first,last);
    }

    template<typename ForwardIterator , typename UnaryOperation >
    static void tabulate (
            ForwardIterator first,
            ForwardIterator last,
            UnaryOperation  unary_op) {	
        thrust::tabulate(first,last,unary_op);

    }

    template<typename InputIterator, typename OutputIterator, typename UnaryFunction, 
        typename T, typename AssociativeOperator>
    static OutputIterator transform_exclusive_scan(
        InputIterator first, InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op, T init, AssociativeOperator binary_op) {
        return thrust::transform_exclusive_scan(first,last,result,unary_op,init,binary_op);
    }


    template<typename InputIterator1, typename InputIterator2, 
        typename InputIterator3, typename RandomAccessIterator , typename Predicate >
    static void scatter_if(
            InputIterator1 first, InputIterator1 last,
            InputIterator2 map, InputIterator3 stencil,
            RandomAccessIterator output, Predicate pred) {
        scatter_if(first,last,map,stencil,output,pred);
    }

    template<typename InputIterator1, typename InputIterator2, 
        typename OutputIterator, typename Predicate>
    static OutputIterator copy_if(
            InputIterator1 first, InputIterator1 last, 
            InputIterator2 stencil, OutputIterator result, Predicate pred) {
        return copy_if(first,last,stencil,result,pred);
    }
};

template <>
struct Traits<thrust::device_vector>: public default_traits {
    template <typename T>
    struct vector_type {
        typedef thrust::device_vector<T> type;
    };

    template <typename ... TupleTypes >
    struct tuple_type {
        typedef thrust::tuple<TupleTypes...> type;
    };

    template <typename T>
    using counting_iterator = thrust::counting_iterator<T>;

    #ifdef __CUDA_ARCH__
    static const thrust::detail::functional::placeholder<0>::type _1;
    static const thrust::detail::functional::placeholder<1>::type _2;
    static const thrust::detail::functional::placeholder<2>::type _3;
    #else
    static const thrust::detail::functional::placeholder<0>::type _1;
    static const thrust::detail::functional::placeholder<1>::type _2;
    static const thrust::detail::functional::placeholder<2>::type _3;
    #endif

    template<typename std_tuple_type>
    struct thrust_tuple_helper {};

    template<typename... Iterators>
    struct thrust_tuple_helper<std::tuple<Iterators...>> {
        typedef thrust::tuple<Iterators...> type;
    };

    template<typename std_tuple_type, std::size_t... I>
    static typename thrust_tuple_helper<std_tuple_type>::type 
    make_thrust_tuple_impl(std_tuple_type& data, index_sequence<I...>) {
        return thrust::make_tuple(std::get<I>(data)...);
    }

    template<typename std_tuple_type, 
        typename Indices = make_index_sequence<std::tuple_size<std_tuple_type>::value>>
    static typename thrust_tuple_helper<std_tuple_type>::type 
    make_thrust_tuple(std_tuple_type& data) {
        return make_thrust_tuple_impl(data, Indices());
    }

    template<typename... Iterators>
    static thrust::zip_iterator<thrust::tuple<Iterators...>> 
    make_thrust_zip_iterator(std::tuple<Iterators...> its) {
        return thrust::make_zip_iterator(make_thrust_tuple(its));
    }

    template< class InputIt, class UnaryFunction >
    static UnaryFunction for_each( InputIt first, InputIt last, UnaryFunction f ) {
        return std::for_each(first,last,f);
    }

    template<typename T1, typename T2>
    static void sort_by_key_impl(T1 start_keys,
            T1 end_keys,
            T2 start_data,
            mpl::bool_<true>) {
        thrust::sort_by_key(start_keys,end_keys,make_thrust_zip_iterator(start_data.get_tuple()));
    }

    template<typename T1, typename T2>
    static void sort_by_key_impl(T1 start_keys,
            T1 end_keys,
            T2 start_data,
            mpl::bool_<false>) {
        thrust::sort_by_key(start_keys,end_keys,start_data);
    }

    template<typename T1, typename T2>
    static void sort_by_key(T1 start_keys,
            T1 end_keys,
            T2 start_data) {
        sort_by_key_impl(start_keys,end_keys,start_data,typename is_zip_iterator<T2>::type());
    }

    template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
    static void lower_bound(
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result) {
        thrust::lower_bound(first,last,values_first,values_last,result);
    }

    template<typename ForwardIterator, typename InputIterator, typename OutputIterator>
    static void upper_bound(
            ForwardIterator first,
            ForwardIterator last,
            InputIterator values_first,
            InputIterator values_last,
            OutputIterator result) {
        thrust::upper_bound(first,last,values_first,values_last,result);
    }

    template< class InputIt, class T, class BinaryOperation >
    static T reduce( 
        InputIt first, 
        InputIt last, T init,
        BinaryOperation op ) {
        return thrust::reduce(first,last,init,op);
    }

    template <class InputIterator, class OutputIterator, class UnaryOperation>
    static OutputIterator transform (
            InputIterator first, InputIterator last,
            OutputIterator result, UnaryOperation op) {
        return thrust::transform(first,last,result,op);
    }

    template <class ForwardIterator>
    static void sequence (ForwardIterator first, ForwardIterator last) {
        thrust::sequence(first,last);
    }

    template<typename ForwardIterator , typename UnaryOperation >
    static void tabulate (
            ForwardIterator first,
            ForwardIterator last,
            UnaryOperation  unary_op) {	
        thrust::tabulate(first,last,unary_op);

    }

    template<typename InputIterator, typename OutputIterator, typename UnaryFunction, 
        typename T, typename AssociativeOperator>
    static OutputIterator transform_exclusive_scan(
        InputIterator first, InputIterator last,
        OutputIterator result,
        UnaryFunction unary_op, T init, AssociativeOperator binary_op) {
        return thrust::transform_exclusive_scan(first,last,result,unary_op,init,binary_op);
    }


    template<typename InputIterator1, typename InputIterator2, 
        typename InputIterator3, typename RandomAccessIterator , typename Predicate >
    static void scatter_if(
            InputIterator1 first, InputIterator1 last,
            InputIterator2 map, InputIterator3 stencil,
            RandomAccessIterator output, Predicate pred) {
        scatter_if(first,last,map,stencil,output,pred);
    }

    template<typename InputIterator1, typename InputIterator2, 
        typename OutputIterator, typename Predicate>
    static OutputIterator copy_if(
            InputIterator1 first, InputIterator1 last, 
            InputIterator2 stencil, OutputIterator result, Predicate pred) {
        return copy_if(first,last,stencil,result,pred);
    }
};
#endif

template<typename ARG,unsigned int D, typename TRAITS>
struct TraitsCommon {
    typedef typename ARG::ERROR_FIRST_TEMPLATE_ARGUMENT_TO_PARTICLES_MUST_BE_A_STD_TUPLE_TYPE error; 
    };

template <typename traits, unsigned int D, typename ... TYPES>
struct TraitsCommon<std::tuple<TYPES...>,D,traits>:public traits {

    const static unsigned int dimension = D;
    typedef typename traits::template vector_type<Vector<double,D> >::type vector_double_d;
    typedef typename traits::template vector_type<Vector<int,D> >::type vector_int_d;
    typedef typename traits::template vector_type<Vector<unsigned int,D> >::type vector_unsigned_int_d;
    typedef typename traits::template vector_type<Vector<bool,D> >::type vector_bool_d;
    typedef typename traits::template vector_type<int>::type vector_int;
    typedef typename traits::template vector_type<unsigned int>::type vector_unsigned_int;
    typedef typename traits::template vector_type<Vector<int,2> >::type vector_int2;

    typedef Vector<double,dimension> double_d;
    typedef Vector<int,dimension> int_d;
    typedef Vector<unsigned int,dimension> unsigned_int_d;
    typedef Vector<bool,dimension> bool_d;

    typedef position_d<dimension> position;
    typedef typename position::value_type position_value_type;
    typedef alive::value_type alive_value_type;
    typedef id::value_type id_value_type;
    //typedef random::value_type random_value_type;
    typedef typename traits::template vector_type<position_value_type>::type position_vector_type;
    typedef typename traits::template vector_type<alive_value_type>::type alive_vector_type;
    typedef typename traits::template vector_type<id_value_type>::type id_vector_type;
    //typedef typename traits::template vector_type<random_value_type>::type random_vector_type;

    typedef traits traits_type;
    typedef mpl::vector<position,id,alive,TYPES...> mpl_type_vector;

    /*
    typedef tuple_ns::tuple<
            typename position_vector_type::pointer,
            typename id_vector_type::pointer,
            typename alive_vector_type::pointer,
            typename traits::template vector_type<typename TYPES::value_type>::type::pointer...
            > tuple_of_pointers_type;

    typedef tuple_ns::tuple<
            typename position_vector_type::value_type*,
            typename id_vector_type::value_type*,
            typename alive_vector_type::value_type*,
            typename traits::template vector_type<typename TYPES::value_type>::type::value_type*...
            > tuple_of_raw_pointers_type;
            */

    typedef tuple_ns::tuple<
            typename position_vector_type::iterator,
            typename id_vector_type::iterator,
            typename alive_vector_type::iterator,
            //typename random_vector_type::iterator,
            typename traits::template vector_type<typename TYPES::value_type>::type::iterator...
            > tuple_of_iterators_type;

    typedef tuple_ns::tuple<
            typename position_vector_type::const_iterator,
            typename id_vector_type::const_iterator,
            typename alive_vector_type::const_iterator,
            //typename random_vector_type::const_iterator,
            typename traits::template vector_type<typename TYPES::value_type>::type::const_iterator...
            > tuple_of_const_iterators_type;


    typedef tuple_ns::tuple<
        position_vector_type,
        id_vector_type,
        alive_vector_type,
        //random_vector_type,
        typename traits::template vector_type<typename TYPES::value_type>::type...
            > vectors_data_type;

    typedef typename Aboria::zip_iterator<tuple_of_iterators_type,mpl_type_vector> iterator;
    typedef typename Aboria::zip_iterator<tuple_of_const_iterators_type,mpl_type_vector> const_iterator;

    typedef typename iterator::reference reference;
    typedef typename iterator::value_type value_type;
    typedef typename iterator::pointer pointer;
    typedef typename iterator::raw_pointer raw_pointer;
    typedef typename const_iterator::reference const_reference;
    typedef Aboria::getter_type<vectors_data_type, mpl_type_vector> data_type;

    const static size_t N = tuple_ns::tuple_size<vectors_data_type>::value;

    template<std::size_t... I>
    static iterator begin_impl(data_type& data, index_sequence<I...>) {
        return iterator(get_by_index<I>(data).begin()...);
    }

    template<std::size_t... I>
    static iterator end_impl(data_type& data, index_sequence<I...>) {
        return iterator(get_by_index<I>(data).end()...);
    }

    template<std::size_t... I>
    static reference index_impl(data_type& data, const size_t i, index_sequence<I...>, std::true_type) {
        return reference(get_by_index<I>(data)[i]...);
    }

    template<std::size_t... I>
    static reference index_impl(data_type& data, const size_t i, index_sequence<I...>, std::false_type) {
        return reference(get_by_index<I>(data)[i]...);
    }

    template<std::size_t... I>
    static const_reference index_const_impl(const data_type& data, const size_t i, index_sequence<I...>, std::true_type) {
        return const_reference(get_by_index<I>(data)[i]...);
    }

    template<std::size_t... I>
    static const_reference index_const_impl(const data_type& data, const size_t i, index_sequence<I...>, std::false_type) {
        return const_reference(get_by_index<I>(data)[i]...);
    }

    template<std::size_t... I>
    static void clear_impl(data_type& data, index_sequence<I...>) {
        int dummy[] = { 0, (get_by_index<I>(data.get_tuple()).clear())... };
        static_cast<void>(dummy);
    }

    template<std::size_t... I>
    static void push_back_impl(data_type& data, const value_type& val, index_sequence<I...>) {
        int dummy[] = { 0, (get_by_index<I>(data).push_back(get_by_index<I>(val)),void(),0)... };
        static_cast<void>(dummy); // Avoid warning for unused variable.
    }

    template<std::size_t... I>
    static void pop_back_impl(data_type& data, index_sequence<I...>) {
        int dummy[] = { 0, (get_by_index<I>(data).pop_back(),void(),0)... };
        static_cast<void>(dummy); // Avoid warning for unused variable.
    }

    template<std::size_t... I>
    static void insert_impl(data_type& data, iterator position, const value_type& val, index_sequence<I...>) {
        int dummy[] = { 0, (get_by_index<I>(data).insert(position,get_by_index<I>(val)))... };
        static_cast<void>(dummy);
    }

    template<std::size_t... I>
    static void insert_impl(data_type& data, iterator position, size_t n, const value_type& val, index_sequence<I...>) {
        int dummy[] = { 0, (get_by_index<I>(data).insert(position,n,get_by_index<I>(val)))... };
        static_cast<void>(dummy);
    }

    template<class InputIterator, std::size_t... I>
    static void insert_impl(data_type& data, iterator position, InputIterator first, InputIterator last, index_sequence<I...>) {
        int dummy[] = { 0, (get_by_index<I>(data).insert(position,first,last))... };
        static_cast<void>(dummy);
    }

    template<typename Indices = make_index_sequence<N>>
    static iterator begin(data_type& data) {
        return begin_impl(data, Indices());
    }

    template<typename Indices = make_index_sequence<N>>
    static iterator end(data_type& data) {
        return end_impl(data, Indices());
    }

    template<typename Indices = make_index_sequence<N>>
    static reference index(data_type& data, const size_t i) {
        return index_impl(data, i, Indices(),std::is_reference<decltype(get<id>(data)[0])>());
    }

    template<typename Indices = make_index_sequence<N>>
    static const_reference index(const data_type& data, const size_t i) {
        return index_const_impl(data, i, Indices(),std::is_reference<decltype(get<id>(data)[0])>());
    }

    template<typename Indices = make_index_sequence<N>>
    static void clear(data_type& data) {
        clear_impl(data, Indices());
    }

    template<typename Indices = make_index_sequence<N>>
    static void push_back(data_type& data, const value_type& val) {
        push_back_impl(data, val, Indices());
    }

    template<typename Indices = make_index_sequence<N>>
    static void pop_back(data_type& data) {
        pop_back_impl(data, Indices());
    }

    template<typename Indices = make_index_sequence<N>>
    static void insert(data_type& data, iterator pos, const value_type& val) {
        insert_impl(data, val, Indices());
    }

    template<typename Indices = make_index_sequence<N>>
    static void insert(data_type& data, iterator position, size_t n,  const value_type& val) {
        insert_impl(data, val, n, Indices());
    }

    template<class InputIterator, typename Indices = make_index_sequence<N>>
    static void insert(data_type& data, iterator pos, InputIterator first, InputIterator last) {
        insert_impl(data, pos, first, last, Indices());
    }



    typedef typename position_vector_type::size_type size_type; 
    typedef typename position_vector_type::difference_type difference_type; 
};

#define UNPACK_TRAITS(traits)      \
    static const unsigned int dimension = traits::dimension;                          \
    typedef typename traits::vector_double_d vector_double_d;                           \
    typedef typename vector_double_d::const_iterator vector_double_d_const_iterator;    \
    typedef typename traits::vector_int_d vector_int_d;                                 \
    typedef typename traits::vector_unsigned_int_d vector_unsigned_int_d;               \
    typedef typename traits::vector_bool_d vector_bool_d;                               \
    typedef typename traits::vector_int2 vector_int2;                               \
    typedef Vector<double,dimension> double_d;                                        \
    typedef Vector<int,dimension> int_d;                                              \
    typedef Vector<unsigned int,dimension> unsigned_int_d;                            \
    typedef Vector<bool,dimension> bool_d;                                            \
    typedef Vector<int,2> int2;                                            \
    typedef typename traits::vector_int vector_int;                                     \
    typedef typename traits::vector_unsigned_int vector_unsigned_int;                   \
    typedef typename vector_unsigned_int::iterator vector_unsigned_int_iterator;        \
    typedef typename traits::iterator particles_iterator;                               \
    typedef typename traits::pointer particles_pointer;                               \
    typedef typename traits::raw_pointer particles_raw_pointer;                               \
    typedef typename traits::const_iterator const_particles_iterator;                   \
    typedef typename traits::value_type particles_value_type;                           \
    typedef typename traits::reference particles_reference;                        \
    typedef typename traits::position position;                                         \


}


#endif //TRAITS_H_
