httpipe
=======

Pipes stuff to HTTP. That simple.

Compiling
---------

Just cd to the **src** directory and issue
    
    make

To install **httpipe** on your system, you may (as root) issue

    make install

If you would like **httpipe** compiled in a package, there's a PKGBUILD for Arch Linux in the AUR at https://aur.archlinux.org/packages/httpipe-git/

Usage
-----

If you want to pipe a file to HTTP you would just run:

    httpipe < your_file.html

Then to get your content, via curl for instance, you would run on a client (5000 is the default port where httpipe works):

    curl -O http://your_host_goes_here:5000

Other fun commands you may do:

    cal | httpipe                 # outputs a calendar
    httpipe <<< Hello World       # outputs the string Hello world
    httpipe -f video.avi          # streams a video via HTTP

If you run **httpipe** by itself, you may input manually the data you want to serve. To end piping data, place an EOF (by making Ctrl-D).
