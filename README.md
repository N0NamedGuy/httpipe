httpipe
=======

Pipes stuff to HTTP

Usage
-----

If you want to pipe a file to HTTP you would just run:

    httpipe < your_file.html

Then to get your content, via curl for instance, you would run on a client (5000 is the default port where httpipe works):

    curl -O http://your_host_goes_here:5000

Other fun commands you may do:

    cal | httpipe
    httpipe <<< Hello World
    ls | httpipe

If you run **httpipe** by itself, you may input manually the data you want to serve. To end piping data, place an EOF (by making Ctrl-D).
