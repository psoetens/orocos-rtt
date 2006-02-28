#ifndef ORO_EXECUTION_PORT_INTERFACE_HPP
#define ORO_EXECUTION_PORT_INTERFACE_HPP

#include <string>
#include "ConnectionTypes.hpp"
#include "ConnectionInterface.hpp"

namespace ORO_Execution
{

    /** 
     * A Port of a task is used to connect it with other tasks
     * using a Connector. The Connector can be a Buffer or a
     * DataObject.
     */
    class PortInterface
    {
    protected:
        std::string portname;
        PortInterface(const std::string& name) : portname(name) {}
    public:
        virtual ~PortInterface() {}

        /**
         * Get the name of this Port.
         */
        const std::string& getName() const { return portname;}

        /**
         * Inspect if this Port is connected with another Port.
         */
        virtual bool connected() const = 0;

        /**
         * Returns the connection in which this Port participates.
         * @retval 0 if not connected to another Port.
         */
        virtual ConnectionInterface::shared_ptr connection() const = 0;

        /**
         * Connect this port to a Connection.
         * @return true if connection was possible, false otherwise.
         */
        virtual bool connectTo( ConnectionInterface::shared_ptr conn ) = 0;

        /**
         * Connect to another Port, which has an existing Connection.
         * This method exists only for convenience only and is equivalent
         * to \a connectTo( \a other->connection() ).
         * If \a other does not yet participate in a Connection, this method fails.
         * @return true upon success, false otherwise.
         */
        virtual bool connectTo( PortInterface* other ) {
            if ( other->connection() == 0 )
                return false;
            return this->connectTo( other->connection() );
        }

        /**
         * No longer participate in a connection.
         */
        virtual void disconnect() = 0;

        /**
         * Create a connection to another port.
         */
        virtual ConnectionInterface::shared_ptr createConnection(PortInterface* other, ConnectionTypes::ConnectionType con_type = ConnectionTypes::lockfree)
        {
            return ConnectionInterface::shared_ptr();
        }

        /**
         * Create accessor DataSources for this Port, for addition to a
         * DataSource interface.
         */
        virtual DataSourceFactoryInterface* createDataSources() {
            return 0;
        }
    };

}

#endif
