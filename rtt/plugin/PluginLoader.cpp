/**
 * PluginLoader.cpp
 *
 *  Created on: May 19, 2010
 *      Author: kaltan
 */

#include "PluginLoader.hpp"

using namespace RTT;
using namespace plugin;
using namespace std;

void PluginLoader::loadPlugins(string const& path_list) {
    // Accepted abbreviated syntax:
    // loadLibrary("liborocos-helloworld-gnulinux.so")
    // loadLibrary("liborocos-helloworld-gnulinux")
    // loadLibrary("liborocos-helloworld")
    // loadLibrary("orocos-helloworld")
    // With a path:
    // loadLibrary("/usr/lib/liborocos-helloworld-gnulinux.so")
    // loadLibrary("/usr/lib/liborocos-helloworld-gnulinux")
    // loadLibrary("/usr/lib/liborocos-helloworld")

    // Discover what 'name' is.
    bool name_is_path = false;
    bool name_is_so = false;
    if ( name.find("/") != std::string::npos )
        name_is_path = true;

    std::string so_name(name);
    if ( so_name.rfind(SO_EXT) == std::string::npos)
        so_name += SO_EXT;
    else
        name_is_so = true;

    // Extract the libname such that we avoid double loading (crashes in case of two # versions).
    libname = name;
    if ( name_is_path )
        libname = libname.substr( so_name.rfind("/")+1 );
    if ( libname.find("lib") == 0 ) {
        libname = libname.substr(3);
    }
    // finally:
    libname = libname.substr(0, libname.find(SO_EXT) );

    // check if the library is already loaded
    // NOTE if this library has been loaded, you can unload and reload it to apply changes (may be you have updated the dynamic library)
    // anyway it is safe to do this only if thereisn't any istance whom type was loaded from this library

    std::vector<LoadedLib>::iterator lib = loadedLibs.begin();
    while (lib != loadedLibs.end()) {
        // there is already a library with the same name
        if ( lib->name == libname) {
            log(Warning) <<"Library "<< libname <<".so already loaded. " ;

            bool can_unload = true;
            CompList::iterator cit;
            for( std::vector<std::string>::iterator ctype = lib->components_type.begin();  ctype != lib->components_type.end() && can_unload; ++ctype) {
                for ( cit = comps.begin(); cit != comps.end(); ++cit) {
                    if( (*ctype) == cit->second.type ) {
                        // the type of an allocated component was loaded from this library. it might be unsafe to reload the library
                        log(Warning) << "Can NOT reload because of the instance " << cit->second.type  <<"::"<<cit->second.instance->getName()  <<endlog();
                        can_unload = false;
                    }
                }
            }
            if( can_unload ) {
                log(Warning) << "Try to RELOAD"<<endlog();
                dlclose(lib->handle);
                // remove the library info from the vector
                std::vector<LoadedLib>::iterator lib_un = lib;
                loadedLibs.erase(lib_un);
                lib = loadedLibs.end();
            }
            else   return true;
        }
        else lib++;
    }


    std::vector<string> errors;
    // try form "liborocos-helloworld-gnulinux.so"
    handle = dlopen ( so_name.c_str(), RTLD_NOW | RTLD_GLOBAL );
    // if no path is given:
    if (!handle && !name_is_path ) {

        // with target provided:
        errors.push_back(string( dlerror() ));
        //cout << so_name.substr(0,3) <<endl;
        // try "orocos-helloworld-gnulinux.so"
        if ( so_name.substr(0,3) != "lib") {
            so_name = "lib" + so_name;
            handle = dlopen ( so_name.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (!handle)
                errors.push_back(string( dlerror() ));
        }
        // try "liborocos-helloworld-gnulinux.so" in compPath
        if (!handle) {
            handle = dlopen ( string(compPath.get() + "/" + so_name).c_str(), RTLD_NOW | RTLD_GLOBAL);
        }

        if ( !handle && !name_is_so ) {
            // no so given, try to append target:
            so_name = name + "-" + target.get() + SO_EXT;
            errors.push_back(string( dlerror() ));
            //cout << so_name.substr(0,3) <<endl;
            // try "orocos-helloworld"
            if ( so_name.substr(0,3) != "lib")
                so_name = "lib" + so_name;
            handle = dlopen ( so_name.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (!handle)
                errors.push_back(string( dlerror() ));
        }
        // try "liborocos-helloworld" in compPath
        if (!handle) {
            handle = dlopen ( string(compPath.get() + "/" + so_name).c_str(), RTLD_NOW | RTLD_GLOBAL);
        }

    }
    // if a path is given:
    if (!handle && name_is_path) {
        // just append target.so or .so"
        // with target provided:
        errors.push_back(string( dlerror() ));
        // try "/path/liborocos-helloworld-gnulinux.so" in compPath
        if (!handle) {
            handle = dlopen ( string(compPath.get() + "/" + so_name).c_str(), RTLD_NOW | RTLD_GLOBAL);
        }

        if ( !handle && !name_is_so ) {
            // no so given, try to append target:
            so_name = name + "-" + target.get() + SO_EXT;
            errors.push_back(string( dlerror() ));
            //cout << so_name.substr(0,3) <<endl;
            // try "/path/liborocos-helloworld"
            handle = dlopen ( so_name.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (!handle)
                errors.push_back(string( dlerror() ));
        }

        // try "/path/liborocos-helloworld" in compPath
        if (!handle) {
            handle = dlopen ( string(compPath.get() + "/" + so_name).c_str(), RTLD_NOW | RTLD_GLOBAL);
        }
    }

    if (!handle) {
        errors.push_back(string( dlerror() ));
        log(Error) << "Could not load library '"<< name <<"':"<<endlog();
        for(vector<string>::iterator i=errors.begin(); i!= errors.end(); ++i)
            log(Error) << *i << endlog();
        return false;
    }

    //------------- if you get here, the library has been loaded -------------
    log(Debug)<<"Succesfully loaded "<<libname<<endlog();
    LoadedLib loading_lib(libname,handle);
    dlerror();    /* Clear any existing error */

    bool (*loadPlugin)(RTT::TaskContext*) = 0;
    std::string(*pluginName)(void) = 0;
    loadPlugin = (bool(*)(RTT::TaskContext*))(dlsym(handle, "loadRTTPlugin") );
    if ((error = dlerror()) == NULL) {
        string plugname;
        pluginName = (std::string(*)(void))(dlsym(handle, "getRTTPluginName") );
        if ((error = dlerror()) == NULL) {
            plugname = (*pluginName)();
        } else {
            plugname  = libname;
        }

        // ok; try to load it.
        bool success = false;
        try {
            success = (*loadPlugin)(this);
        } catch(...) {
            log(Error) << "Unexpected exception in loadRTTPlugin !"<<endlog();
        }

        if ( success )
            log(Info) << "Loaded RTT Plugin '"<< plugname <<"'"<<endlog();
        else {
            log(Error) << "Failed to load RTT Plugin '" <<plugname<<"': plugin refused to load into this TaskContext." <<endlog();
            return false;
        }
        return true;
    } else {
        log(Debug) << error << endlog();
    }

}

bool PluginLoader::loadService(string const& servicename, TaskContext* tc) {

}
