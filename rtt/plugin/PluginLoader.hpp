#ifndef ORO_PLUGINLOADER_HPP_
#define ORO_PLUGINLOADER_HPP_

#include <string>
#include "TaskContext.hpp"

namespace RTT {
    namespace plugin {
        /**
         * Loads plugins found on the filesystem and keeps track of found plutins, typekits and services.
         */
        class PluginLoader
        {
        public:
            /**
             * Load any typekit found in the types subdirectory of each path in path_list.
             * @param path_list A colon or semi-colon seperated list of paths
             * to look for typekits.
             */
            static void loadTypekits(std::string const& path_list);

            /**
             * Loads a plugin found in path_list in the current process.
             * @param name The name of the plugin to load.
             * @param path_list A colon or semi-colon seperated list of paths
             * to look for plugins.
             */
            static void loadPlugin(std::string const& name, std::string const& path_list);

            /**
             * Loads an earlier discovered service into a TaskContext.
             * @param servicename The name of the service or plugin containing
             * the service
             * @param tc The TaskContext to load into.
             * @return false if the plugin refused to load into the TaskContext.
             */
            static bool loadService(std::string const& servicename, TaskContext* tc);
        };
    }
}


#endif /* ORO_PLUGINLOADER_HPP_ */
