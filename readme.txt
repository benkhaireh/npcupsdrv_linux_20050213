npcupsdrv-2.3.0

** Package Description **

This is the CUPS printer driver package, containing:

1. Compiled printer drivers for the following NP models:
   - NP211
   - NP326

2. Printer driver option setting help files.

3. Source code for all components participant in this software set.

4. A makefile for building and installing this software on your system.

5. This readme file.

6. The GNU General Public License or GPL, under which this software set is freely licensed to you. Adhere to it please!

** Requirements **

This software requires that the following is present on your computer:

1. CUPS server & architecure (see www.cups.org)
2. CUPS development headers (if you choose to re-compile the drivers)
   These files are included in the cups-devel-1.1.19-13.i386.rpm package, which can be obtained at:
   http://rpmfind.net/linux/rpm2html/search.php?query=cups-devel&submit=Search+...&system=&arch=

** Compile & Install Instructions **

To begin using this software, please do the following:

1. Obtain the distribution archive npcupsdrv_linux_20050213.tar.gz.
   (You probably already have this)

2. Extract this archive creating the npcupsdrv-2.3.0 directory automatically.
   (You probably already did this)

3. Open your favorite shell (bash, etc.) and navigate to the npcupsdrv-2.3.0 directory.

4. Decide if you are going to install the pre-compiled drivers contained in this package, or if you are going to re-compile them.  The precompiled drivers should work, as-is, on most Linux i386 distributions.  If the precompiled drivers do not work, recompiling them should do the trick.

   If you are going to use the pre-compiled drivers, skip ahead to step 5.
   If you are going to re-compile the drivers, do this:

   Type 'make' at the prompt to begin building the package. A successful build should result in output similar to this:

   mkdir bin
   # compiling rastertonp filter
   gcc -Wl,-rpath,/usr/lib -Wall -fPIC -O2  -o bin/rastertonp src/rastertonp.c -lcupsimage -lcups
   # gzip ppd file
   gzip -c ppd/np211.ppd >> bin/np211.ppd.gz
   # gzip ppd file
   gzip -c ppd/np326.ppd >> bin/np326.ppd.gz
   # create setup shell script
   cp src/setup.sh bin/setup
   chmod +x bin/setup
   # packaging
   mkdir install
   cp bin/rastertonp install
   cp bin/*.ppd.gz install
   cp bin/setup install

5. Execute the 'su' command to obtain super-user level permissions. You must enter the root user's password for this to succeed. Be careful!

6. Type 'make install' to install this package onto your computer. A successfull install should result in output similar to this:

   # installing
   cd install; exec ./setup
   Star Micronics
   npcupsdrv-2.3.0 installer
   ---------------------------------------

   Models included:
   NP211
   NP326

   Searching for ServerRoot, ServerBin, and DataDir tags in /etc/cups/cupsd.conf

   ServerBin tag is present as an absolute path

   DataDir tag is present as an absolute path

   ServerRoot = /etc/cups
   ServerBin  = /usr/lib/cups
   DataDir    = /usr/share/cups

   Copying rastertonp filter to //usr/lib/cups/filter

   Copying model ppd files to //usr/share/cups/model/np

   Restarting CUPS
   Stopping CUPS printing system:                                  [  OK  ]
   Starting CUPS printing system:                                  [  OK  ]

   Install Complete
   Add printer queue using OS tool, http://localhost:631, or http://127.0.0.1:631

7. Goto http://localhost:631 or use your favorite CUPS admin tool.

8. Add a new printer queue for your model.

9. Print happily and buy more Star printers:)

** Driver Option Settings **

There are 3 ways that driver option settings can be configured:

1. Global configuration

   Using your favorite CUPS admin tool, configure all option settings for the printer queue you installed, and then save these as the default.  Each tool will present this functionality in a different way, but all should contain support for it.

   Global configuration settings remain in effect indefinitely.  After configuring your prefered settings, documents can be printed according to these settings WITHOUT the need for further configuration.  In otherwords, you can simply 'lpr' your document, and it will be printed according to the global settings - no need for the "Print" dialog box to be shown to the user.

2. Command line configuration

   When using CUPS, documents are submitted for printing via the 'lpr' command.  This command will typically have the form: 'lpr -P queue -o PageSize=choice -o Option=choice document.ps'.  In this command, "-o" begins and option / choice setting.  You can specify as many option / choice settings as required to effect the printing mode you need.

   Each of the printer drivers in this set contain support for differnt options and choices.  Within the 'docs' folder of this package, you will find files (such as tsp700-options.txt) that described the set of supported options and choices for that printer.  Please use these documents when formulating your print command.

   Here is an example command effecting the best print quality and fixed length pages (a setting combination you might use for printing tickets):

   lpr -P NP211 -o PrintQuality=2Best -o PageType=1Fixed lift-ticket.ps

   Specifying option / choice settings on the command line has 2 main virtues:

   i. Option and choice settings are under programmatic control.  Users do not have to set global defaults.  Or, if they do, those global defaults are overridden - thus preventing faulty configuration usage.

   ii. Option and choice settings can be varied on each print job without the need for user interaction or the display of the print dialog - automation:)

3. Print dialog configuration

   Users can interact with the Print Dialog window to configure print options just prior to the printing of documents.  This method is what most of us are familiar with - hit the print button - configure the settings - then print.  This scenario is not ideal for POS, ticketing, or kiosk environments, and so the above 2 alternative methods are prefered:(

