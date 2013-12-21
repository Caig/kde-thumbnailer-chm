kde-thumbnailer-chm
===================

A KDE thumbnail generator for the CHM file format. It uses [CHMlib](http://www.jedrea.com/chmlib/) to extract the covers.

http://kde-apps.org/content/show.php/KDE+CHM+Thumbnailer?content=153410

Installation from source
------------------------

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` ..
    make
    sudo make install
    kbuildsycoca4
