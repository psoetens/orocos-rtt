#ifndef ORO_FUSEDFUNCTORDATASOURCE_HPP_
#define ORO_FUSEDFUNCTORDATASOURCE_HPP_

#include "DataSource.hpp"
#include "../base/MethodBase.hpp"
#include "CreateSequence.hpp"
#include <boost/bind.hpp>
#include <boost/type_traits.hpp>
#include <boost/fusion/include/invoke.hpp>
#include <boost/fusion/include/invoke_procedure.hpp>

namespace RTT
{
    namespace internal
    {
        namespace bf = boost::fusion;

        /**
         * A DataSource that calls a method which gets its arguments from other
         * data sources. The result type of this data source is the result type
         * of the called function.
         */
        template<typename Signature>
        struct FusedMCallDataSource
        : public DataSource<
                  typename boost::function_traits<Signature>::result_type>
          {
              typedef typename boost::function_traits<Signature>::result_type
                      result_type;
              typedef result_type value_t;
              typedef create_sequence<
                      typename boost::function_types::parameter_types<Signature>::type> SequenceFactory;
              typedef typename SequenceFactory::type DataSourceSequence;
              DataSourceSequence args;
              typename base::MethodBase<Signature>::shared_ptr ff;
          public:
              typedef boost::intrusive_ptr<FusedMCallDataSource<Signature> >
                      shared_ptr;

              FusedMCallDataSource(typename base::MethodBase<Signature>::shared_ptr g,
                                     const DataSourceSequence& s = DataSourceSequence() ) :
                  ff(g), args(s)
              {
              }

              void setArguments(const DataSourceSequence& a1)
              {
                  args = a1;
              }

              value_t value() const
              {
                  return ff->ret();
              }

              value_t get() const
              {
                  // put the member's object as first since SequenceFactory does not know about the MethodBase type.
                  bf::invoke(&base::MethodBase<Signature>::call, bf::cons<base::MethodBase<Signature>*, typename SequenceFactory::data_type>(ff.get(), SequenceFactory::data(args)));
                  // TODO: XXX do updated() on all datasources that need it ?
                  return ff->ret();
              }

              virtual FusedMCallDataSource<Signature>* clone() const
              {
                  return new FusedMCallDataSource<Signature> (ff, args);
              }
              virtual FusedMCallDataSource<Signature>* copy(
                                                          std::map<
                                                                  const base::DataSourceBase*,
                                                                  base::DataSourceBase*>& alreadyCloned) const
              {
                  return new FusedMCallDataSource<Signature> (ff, SequenceFactory::copy(args, alreadyCloned));
              }
          };

        /**
         * A DataSource that sends a method which gets its arguments from other
         * data sources. The result type of this data source is a SendHandle.
         */
        template<typename Signature>
        struct FusedMSendDataSource
        : public DataSource<SendHandle<Signature> >
          {
              typedef SendHandle<Signature> result_type;
              typedef result_type value_t;
              typedef create_sequence<
                      typename boost::function_types::parameter_types<Signature>::type> SequenceFactory;
              typedef typename SequenceFactory::type DataSourceSequence;
              DataSourceSequence args;
              typename base::MethodBase<Signature>::shared_ptr ff;
              mutable SendHandle<Signature> sh; // mutable because of get() const
          public:
              typedef boost::intrusive_ptr<FusedMSendDataSource<Signature> >
                      shared_ptr;

              FusedMSendDataSource(typename base::MethodBase<Signature>::shared_ptr g,
                                     const DataSourceSequence& s = DataSourceSequence() ) :
                  ff(g), args(s)
              {
              }

              void setArguments(const DataSourceSequence& a1)
              {
                  args = a1;
              }

              value_t value() const
              {
                  return sh;
              }

              value_t get() const
              {
                  // put the member's object as first since SequenceFactory does not know about the MethodBase type.
                  sh = bf::invoke(&base::MethodBase<Signature>::send, bf::cons<base::MethodBase<Signature>*, typename SequenceFactory::data_type>(ff.get(), SequenceFactory::data(args)));
                  return sh;
              }

              virtual FusedMSendDataSource<Signature>* clone() const
              {
                  return new FusedMSendDataSource<Signature> (ff, args);
              }
              virtual FusedMSendDataSource<Signature>* copy(
                                                          std::map<
                                                                  const base::DataSourceBase*,
                                                                  base::DataSourceBase*>& alreadyCloned) const
              {
                  return new FusedMSendDataSource<Signature> (ff, SequenceFactory::copy(args, alreadyCloned));
              }
          };

        /**
         * A DataSource that sends a method which gets its arguments from other
         * data sources. The result type of this data source is a SendStatus.
         * @param Signature is the signature of the collect function, not of the
         * original send function.
         */
        template<typename Signature>
        struct FusedMCollectDataSource
        : public DataSource<SendStatus>
          {
              typedef SendStatus result_type;
              typedef result_type value_t;
              // push the SendHandle pointer in front.
              typedef typename CollectType<Signature>::type CollectSignature;
              typedef typename boost::function_types::parameter_types<CollectSignature>::type arg_types;
              typedef typename mpl::push_front<arg_types,typename boost::add_reference<typename SendHandle<Signature>::CBase>::type >::type handle_and_arg_types;
              typedef create_sequence< handle_and_arg_types// @todo XXX use &SendHandle to avoid a copy ???
                      > SequenceFactory;
              typedef typename SequenceFactory::type DataSourceSequence;
              DataSourceSequence args;
              bool isblocking;
              mutable SendStatus ss; // because of get() const
          public:
              typedef boost::intrusive_ptr<FusedMCollectDataSource<Signature> >
                      shared_ptr;

              FusedMCollectDataSource(
                                     const DataSourceSequence& s = DataSourceSequence(), bool blocking = true ) :
                  args(s), isblocking(blocking), ss(SendFailure)
              {
              }

              void setArguments(const DataSourceSequence& a1)
              {
                  args = a1;
              }

              value_t value() const
              {
                  return ss;
              }

              value_t get() const
              {
                  // put the member's object as first since SequenceFactory does not know about the MethodBase type.
                  if (isblocking)
                      return  ss = bf::invoke(&SendHandle<Signature>::CBase::collect, SequenceFactory::data(args));
                  else
                      return  ss = bf::invoke(&SendHandle<Signature>::CBase::collectIfDone, SequenceFactory::data(args));
              }

              virtual FusedMCollectDataSource<Signature>* clone() const
              {
                  return new FusedMCollectDataSource<Signature> ( args, isblocking);
              }
              virtual FusedMCollectDataSource<Signature>* copy(
                                                          std::map<
                                                                  const base::DataSourceBase*,
                                                                  base::DataSourceBase*>& alreadyCloned) const
              {
                  return new FusedMCollectDataSource<Signature> ( SequenceFactory::copy(args, alreadyCloned), isblocking);
              }
          };
    }
}

#endif /* ORO_FUSEDFUNCTORDATASOURCE_HPP_ */