/***************************************************************************
  tag: Peter Soetens  Mon Jan 19 14:11:25 CET 2004  ParsedValue.hpp

                        ParsedValue.hpp -  description
                           -------------------
    begin                : Mon January 19 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef PARSEDVALUE_HPP
#define PARSEDVALUE_HPP

#include "DataSource.hpp"
#include <corelib/Property.hpp>
#include "AssignVariableCommand.hpp"

namespace ORO_Execution
{
  using ORO_CoreLib::Property;
  using ORO_CoreLib::PropertyBase;

  struct bad_assignment
  {
  };

  /**
   * We keep defined variables and constants as ParsedValue's,
   * together with some information about their types etc.  This is
   * the abstract base class, the implementations are in
   * ValueParser.cpp
   */
  class ParsedValueBase
  {
  public:
    virtual ~ParsedValueBase();
    // construct a new ParsedValueBase, containing a
    // VariableDataSource of the same type as this one..
    virtual DataSourceBase* toDataSource() const = 0;

    /**
     * Returns a clone of this ParsedValueBase.
     */
    virtual ParsedValueBase* clone() const = 0;

    /**
     * Returns a copy of this ParsedValueBase.  Uses the given
     * replacements to copy held DataSources.
     */
    virtual ParsedValueBase* copy( std::map<const DataSourceBase*, DataSourceBase*>& replacements ) const = 0;

    /**
     * return a command assigning rhs to the DataSource you're
     * holding..  The default implementation returns 0, indicating
     * this DataSource cannot be assigned to..  Throw a
     * bad_assignment if rhs is of the wrong type..
     *
     * If initialization is true, then this is the initialization of
     * the value.  E.g. ConstantValue needs to know this because it
     * does allow one assignment at initialization time, but no
     * further assignments..
     */
    virtual CommandInterface* assignCommand( DataSourceBase* rhs,
                                             bool initialization ) const;

      /**
       * Create an assignment command of the ParsedValueBase
       * to the rhs DataSource, with an index.
       * example is the left hand side [] operator. Returns zero if
       * not applicable. It can not be used for initialisation.
       */
    virtual CommandInterface* assignIndexCommand( DataSourceBase* index,
                                             DataSourceBase* rhs ) const;
  };

  template<typename T>
  class ParsedValue
    : public ParsedValueBase
  {
  public:
    virtual DataSource<T>* toDataSource() const = 0;
  };

  /**
   * An alias is a name that is assigned to a certain compound
   * expression, I guess you could compare it to a 0-ary function,
   * only less powerful..   Maybe this can be extended to allow for
   * functions with arguments..
   * This class is the most basic ParsedValue implementation, does
   * not allow any assignment, just stores a DataSource<T>, and
   * returns it..  This also makes it useful as the return type of
   * temporary expressions like literal values, and rhs
   * expressions..
   */
  template<typename T>
  class ParsedAliasValue
    : public ParsedValue<T>
  {
    typename DataSource<T>::shared_ptr data;
  public:
    ParsedAliasValue( DataSource<T>* d )
      : data( d )
      {
      }
    DataSource<T>* toDataSource() const
      {
        return data.get();
      }
    ParsedAliasValue<T>* clone() const
      {
        return new ParsedAliasValue<T>( data.get() );
      }
    ParsedAliasValue<T>* copy( std::map<const DataSourceBase*, DataSourceBase*>& replacements ) const
      {
        return new ParsedAliasValue<T>( data->copy( replacements ) );
      }
  };

  /**
   * This class represents a variable held in ValueParser..  It is
   * the only ParsedValue that does something useful in its
   * assignCommand() method..
   */
  template<typename T>
  class ParsedVariableValue
    : public ParsedValue<T>
  {
  public:
    typename VariableDataSource<T>::shared_ptr data;
    ParsedVariableValue()
      : data( new VariableDataSource<T>( T() ) )
      {
      }
    ParsedVariableValue( typename VariableDataSource<T>::shared_ptr d )
      : data( d )
      {
      }
    VariableDataSource<T>* toDataSource() const
      {
        return data.get();
      }
    CommandInterface* assignCommand( DataSourceBase* rhs, bool ) const
      {
        DataSourceBase::shared_ptr r( rhs );
        DataSource<T>* t = dynamic_cast<DataSource<T>*>( r.get() );
        if ( ! t )
          throw bad_assignment();
        return new AssignVariableCommand<T>( data.get(), t );
      };
    ParsedVariableValue<T>* clone() const
      {
        return new ParsedVariableValue<T>( data );
      }
    ParsedVariableValue<T>* copy( std::map<const DataSourceBase*, DataSourceBase*>& replacements ) const
      {
        return new ParsedVariableValue<T>( data->copy( replacements ) );
      }
  };

  template<typename T, typename Index, typename SetType, typename Pred>
  class ParsedIndexValue
    : public ParsedValue<T>
  {
    Pred p;
  protected:
    typename VariableDataSource<T>::shared_ptr data;
  public:
    ParsedIndexValue(Pred _p)
        :p(_p), data( new VariableDataSource<T>( T() ) )
      {
      }
    ParsedIndexValue( Pred _p, typename VariableDataSource<T>::shared_ptr d )
      : p( _p ), data( d )
      {
      }
    VariableDataSource<T>* toDataSource() const
      {
        return data.get();
      }

    CommandInterface* assignCommand( DataSourceBase* rhs, bool ) const
      {
        DataSourceBase::shared_ptr r( rhs );
        DataSource<T>* t = dynamic_cast<DataSource<T>*>( r.get() );
        if ( ! t )
          throw bad_assignment();
        return new AssignVariableCommand<T>( data.get(), t );
      }

    CommandInterface* assignIndexCommand( DataSourceBase* index, DataSourceBase* rhs ) const
      {
        DataSourceBase::shared_ptr r( rhs );
        DataSourceBase::shared_ptr i( index );
        DataSource<SetType>* t = dynamic_cast<DataSource<SetType>*>( r.get() );
        if ( ! t )
          throw bad_assignment();
        DataSource<Index>* ind = dynamic_cast<DataSource<Index>*>( i.get() );
        if ( ! ind )
          throw bad_assignment();
        return new AssignIndexCommand<T, Index, SetType,Pred>(data.get(), ind ,t, p );
      }

    ParsedIndexValue<T, Index, SetType, Pred>* clone() const
      {
        return new ParsedIndexValue<T, Index, SetType, Pred>( p, data );
      }
    ParsedIndexValue<T, Index, SetType, Pred>* copy( std::map<const DataSourceBase*, DataSourceBase*>& replacements ) const
      {
        return new ParsedIndexValue<T, Index, SetType, Pred>( p, data->copy( replacements ) );
      }
  };

  /**
   * This represents a constant value, does not allow assignment,
   * only initialization..
   */
  template<typename T>
  class ParsedConstantValue
    : public ParsedVariableValue<T>
  {
  public:
    ParsedConstantValue()
      : ParsedVariableValue<T>()
      {
      }
    ParsedConstantValue( typename VariableDataSource<T>::shared_ptr d )
      : ParsedVariableValue<T>( d )
      {
      }
    CommandInterface* assignCommand( DataSourceBase* rhs, bool init ) const
      {
        if ( init )
          return ParsedVariableValue<T>::assignCommand( rhs, init );
        else return 0;
      }
    ParsedConstantValue<T>* clone() const
      {
        return new ParsedConstantValue<T>( this->data );
      }
    ParsedConstantValue<T>* copy( std::map<const DataSourceBase*, DataSourceBase*>& replacements ) const
      {
        return new ParsedConstantValue<T>( this->data->copy( replacements ) );
      }
  };
}

#endif
