//
// Created by Dongge Liu on 4/10/20.
//

#ifndef AFLNET_GLIB_HELPER_H
    #define AFLNET_GLIB_HELPER_H
    #if __APPLE__
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/gmodule.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib-unix.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib-object.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib/gtypes.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib/grand.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib/gnode.h"
        #include "/usr/local/Cellar/glib/2.66.0/include/glib-2.0/glib/gprintf.h"
//        #include "/Volumes/T7_500G/MacHome/Documents/AFLNet/glib/glib.h"
//        #include "/Volumes/T7_500G/MacHome/Documents/AFLNet/glib/gmodule.h"
//        #include "/Volumes/T7_500G/MacHome/Documents/AFLNet/glib/glib-unix.h"
//        #include "/Volumes/T7_500G/MacHome/Documents/AFLNet/glib/glib-object.h"
//        #include "/Volumes/T7_500G/MacHome/Documents/AFLNet/glib/glib/gprintf.h"
    #elif __linux__
        #include <glib.h>
        #include <glib/gprintf.h>
    #else
        #error "Unknow compiler of glib"
    #endif
#endif //AFLNET_GLIB_HELPER_H
