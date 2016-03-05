kde-thumbnailer-chm
===================

A KDE thumbnail generator for the CHM file format.
It depends on [CHMlib](http://www.jedrea.com/chmlib/).
Since ? release it works with KDE Frameworks 5, for KDE 4 just keep using 0.2 stable release.

http://kde-apps.org/content/show.php/KDE+CHM+Thumbnailer?content=153410

Installation from source (KF5)
------------------------

    mkdir build
    cd build
    cmake -DKDE_INSTALL_USE_QT_SYS_PATHS=ON -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` ..
    make
    sudo make install
    kbuildsycoca5

Installation from source (KDE 4)
------------------------

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix` ..
    make
    sudo make install
    kbuildsycoca4