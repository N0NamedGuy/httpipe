httpipe
=======

Pipes stuff to HTTP

Usage
-----

If you want to pipe a file to HTTP you would just run:

    httpipe < your_file.html

Then to get your content, via curl for instance, you would run on a client (5000 is the default port where httpipe works):

    curl -O http://your_host_goes_here:5000
