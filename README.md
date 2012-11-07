GDAM
====

Glider Database Alternative with Mongo

About
----

This repository includes code and a makefile to build your own Webb Glider Database Alternative processor.  The code in this repository merges flight and science data from Teledyne Webb Glider in dbd,ebd,sbd,tbd,mbd,and nbd formats then exports them to a Mongo database.  

Extension
----
To change this code for your own desired database engine, users need to write a class that implements the methods detailed in the interface IGliderDataExporter.hpp and modify the main method in GDAM.cpp.  You will probably also need to add the libraries you used to the Makefile LIBS argument.

Usage
----
To run GDAM on our servers, we have done the following:

1. Setup a cron job to run 20 minutes after each surfacing
2. In the cron job, it rsyncs files from the glider server to the processing server
3. After the rsync is complete, GDAM is run inside the cron job with the following command:

<path-to-GDAM>/GDAM <glider-name>:<path-to-glider-files> <mongo-connection-string>
