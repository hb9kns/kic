# KIC layout editor, ver.2.4b

KIC is a small layout editor capable of working with GDSII files
of medium complexity. It is quite dated now, and probably should
no longer be used for real work.

An excellent alternative is `XIC` which is available from the
website of [Whiteley Research Inc.]( http://www.wrcad.com )
Recently, it has been put into the open-source domain; sources can
be found on <https://github.com/wrcad/xictools> .

A short shell script `kictree` has been added, which allows to search
recursively in a KIC file for references to other files. A short help
is displayed if the script is run without arguments.

*(2018-Jan, Y.Bonetti)*

---

*following is the original KIC README file as downloaded September 2018 in*
<http://wrcad.com/ftp/pub/kic-2.4c.tar.gz>

	README for KIC-2.4c 11/8/2014
	
	Questions, comments, requests, and hate mail should be addressed to
	Steve Whiteley, stevew@srware.com
	
	KIC is distributed by Whiteley Research Inc., www.wrcad.com.
	
	Disclaimer:  Steve Whiteley and Whiteley Research Inc. don't guarantee
	or imply that this software is suitable for any purpose.  This is free
	public domain software, use at your own risk.
	
	######################################################################
	#                                                                    #
	#  KIC LAYOUT EDITOR                                                 #
	#                                                                    #
	######################################################################
	
	Release 2.4c  11/8/14
	
	>  Another round of fixes for compiler warnings, etc., and updates
	   for compatibility with newer Linux distributions.
	
	Release 2.4b  1/11/09
	
	>  Fixed benign compiler warnings from gcc-4.2.1.
	
	>  Added patch so that coordinates entered as text are read as floating
	   values rather than integers, following the Debian patch by
	   Yargo C. Bonetti.
	
	>  Added the msw_package directory and the mkwinpkg script in util,
	   used for building Windows package files.  This is mostly for my
	   convenience.
	
	Release 2.4a  7/24/05 (re-re-release)
	
	> Fixed layer counting in converter.  The top layer would not be
	  added to output.
	
	Release 2.4a  7/22/05 (re-release)
	
	> The Windows binary package should now be relocatable, the previous
	  release would not work if not installed in the default location,
	  unless the environment variable KIC_LIB_DIR was set to the
	  installation directory.
	
	> GDSII layer/datatype numbers can now extend from 0-255.  Previously
	  the numbers were limited to 63, an old GDSII spec.
	
	Release 2.4a  7/1/05
	
	> Fixed a bug in the Donut command - creating a donut with inner
	  radius zero would cause garbage values when written to a file,
	  making the file invisible since the scaling sould be way off.  It
	  is now impossible to create donuts with inner radius zero (use the
	  Flash command to create disk objects).
	
	> Fixed bug - if the PATH has trailing white space (which it does by
	  default) then cells would be searched for spurriously in the root
	  directory.
	
	> Misc. changes to avoid compiler warning messages.
	
	Release 2.4   8/11/02
	
	> Replaced the build control system with GNU configure.  This avoids
	  using the (mostly) obsolete operating system specific configuration
	  files in the conf subdirectory (which no longer exists).  The program
	  should build on just about any Unix/Linux system, plus Mingw and
	  Cygwin under Windows (Mingw is a native build, Cygwin requires X).
	
	> Fixed a bug:  The program would hang when prompting for input on
	  sparc (big endian) machines.
	
	> The CIF parser can now handle nested comments.
	
	> The "help" program is now named "kichelp" since "help" is too
	> generic.
	
	Release 2.3b  5/4/02
	
	> The maximum number of layers was increased from 35 to 70.
	
	> Native Windows release only:  The Font command in the Attri menu can
	  be used to change the font size.  The Updat command will save the
	  new font size in the technology file, using the FontName keyword. 
	  Note that under Windows, the FontName is in the form "WxH", for
	  example "8x15", whereas under UNIX the FontName is the name of an
	  X font.
	
	Release 2.3a  8/18/00
	
	> Native Windows release only:  The installer program places a key in
	  the registry 
	    (HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Kic)
	  that locates the install.log file, which is installed with the startup
	  files.  This is now used to locate the startup files, though the
	  KIC_LIB_DIR variable, if set, has precedence.  This means that the
	  program works without setting KIC_LIB_DIR if installed in a non-
	  standard location, or if started from a different drive.  But...
	  if you move these files later by hand to a different location, they
	  won't be found automatically, and uninstall won't work.
	
	> Native Windows release only:  Various fatal errors now pop up a
	  message box explaining the error.
	
	> Native Windows release only:  256 color mode is now supported.
	  Note that when changing colors in 256 color mode, you have to redraw
	  the display to see the new colors (except that the layer table is
	  updated automatically), as in higher color modes.  Windows does
	  not support dynamic hardware colormap changes reliably, so the
	  instant global color change seen in X-windows is absent.  Also,
	  "blinking" layers is not supported in Windows.
	
	> Native Windows release only:  Fixed a scaling bug in hardcopy
	  generation.
	
	Release 2.3  8/15/00
	
	> This release was ported to Windows as a native application, using the
	  Mingw compiler (www.mingw.org).  The port is as direct as possible,
	  using a single window, so operation should be very close to that on
	  unix, or the Cygwin release on NT (which requires an X server).
	  Since this required fairly substantial modifications to the program
	  structure, a new release minor number was given.
	
	> Printing notes:  In Windows, you can print to a file in either PCL
	  or PostScript format.  It might work to set the "PRN" value to "prn",
	  which should instead stream the output to the default printer.
	
	> In the native Windows port, you must use at least 16-bit graphics.
	  Support for 8-bits (256 colors) may be added later.  The Font button
	  presently does nothing, and the Cursr button toggles the full screen
	  cursor.  Otherwise, this version provides all of the features of
	  the unix releases (or at least that is the intention).
	
	> Added a techfile for Mosis scalable cmos: kic_tech.scm.  This will
	  require customization to specific processes.  The default kic_tech
	  file is a copy of this file.  The Conductus kic_tech.con file has
	  been removed since the process is no longer available.
	
	> The native Windows port accepts drag/drop files, i.e., you can drag
	  kic cell files from the file manager and drop them into the kic window.
	
	> When a menu name changes, such as TBRL -> TB -> RL, the accelerators
	  for the menu are recomputed, and the menu redisplayed.  In particular,
	  when RL is displayed, the Rdraw entry becomes RDraw, so that there is
	  no longer a conflict.  Previously, pressing "r" would redraw the screen,
	  so the "RL" was inaccessible from key presses.  Now, "rd" redisplays
	  the screen when "RL" is present.
	
	> Kic now uses a simplified transformation for placed cells, consisting
	  of a maximum of one each of MirrorY, Rotation, Translation.  Previously,
	  if the LLREF option was used, a second translation was added, which
	  confused the kictostr program.  This should no longer be a problem.
	
	> Fixed a problem in strtokic where it would silently omit the layer
	  names from kic files if it couldn't parse the kic_tech file.
	
	> The default file extension for GDSII files is now ".gds".  It was
	  ".str" in previous releases.
	
	> The "cd" shell command in now recognized internally, and changes the
	  current working directory, i.e., the directory where kic writes
	  files.  Previously, it changed the directory of the subshell that
	  executes the command and dies, so did nothing.  Thus, to change
	  to /usr/mydir, type "!cd /usr/mydir".  If no directory path is given,
	  the user's home directory is understood (in Windows, the value of
	  the HOME environment variable, or the root of the current drive if
	  HOME is undefined).
	
	Release 2.2f  1/27/00
	
	> Fixed a whole collection of compile-time fatal errors when compiling
	  under glibc-2.x (Linux).  In this version of the C library, several
	  of the common string functions are defined as macros, and declarations
	  then generate an error.  These have been fixed.
	
	> Fixed the Xor command, which would as likely as not crash the program.
	
	Release 2.2e  7/5/99
	
	> Fixed a bug that would crash the program when an open failed on a file
	  under some circumstances.
	> Fixed a bug that would sometimes crash after a deletion.
	> Added "solaris" config file.
	> Added "mkdirpth" script to fix install problems.
	
	Release 2.2d  11/29/98
	
	> Polygon entry no longer crashes program.
	> Works with TrueColor graphics when PsuedoColor visual is also available
	  but not the default (i.e., it wouldn't start with certain PC X-servers).
	
	Release 2.2c 11/4/98
	
	> Fixed bug in cif in/out.
	> Fixed other minor bugs.
	
	DOS is no longer supported, since I no longer run DOS.
	
	Release 2.2b 2/24/97
	
	> Port to DEC Alpha OSF.
	> Defines in the Stream conversion routines to change case conversion
	  policy of symbol names.  Conversion policy has been changed to no
	  case conversion.
	
	Release 2.2a 10/1/94
	
	> bugs fixed.
	
	######################################################################
	#                                                                    #
	#  OBTAINING KIC                                                     #
	#                                                                    #
	######################################################################
	
	The kic program is available in two files:
	  kic-x.x.tar.gz            Source code (no binaries)
	  kic-x.x-setup.exe         Binary for Windows NT/98/95/2000
	
	sites:
	www.wrcad.com
	www.srware.com
	ftp.srware.com
	
	DOS is no longer directly supported, since I no longer run DOS.  I can
	supply the DOS direct-write graphics library if anyone wants to build
	a DOS-only version (contact stevew@srware.com).
	
	There is a fairly complete unix emulation package (Cygwin) for Windows
	available from www.sourceware.cygnus.com.  If you download the
	development tools, kic can be built in this environment, though you
	will also need the X libraries and an X server.  See the links on the
	Cygwin web page for sources.
	
	Downloading the Cygwin user tools is a good idea, if you are used to
	unix commands.  In particular, the bash shell is very convenient
	for scripting and general command-line interaction.
	
	The Mingw compiler, used to build the Windows native version,
	is available from www.mingw.org.
	
	######################################################################
	** This is free software, use at your own risk. **
	** NO GUARANTEE OF MERCHANTABILITY OR FITNESS FOR ANY PURPOSE **
	######################################################################
	######################################################################
	#                                                                    #
	#  KIC LAYOUT EDITOR                                                 #
	#                                                                    #
	######################################################################
	
	This is an updated version of the venerable KIC layout editor.  It
	currently runs under various versions of UNIX, Cygnus NT/9x with X
	support, and native Windows.  Under UNIX, the X windowing system is
	required.
	
	Some of the features of this version of KIC are:
	
	1.      An on-line help system provides help for all KIC functions.
	
	2.      The CIF and Calma Stream conversion routines are now built into
	        KIC, under the "cnvrt" menu item in the basic menu.  The free
	        standing conversion routines are also provided.
	
	3.      Most commands that previously operated on selected items now
	        allow the user to choose an item if none have been previously
	        selected.
	
	4.      Ghosting and rubber-banding have been added to several commands.
	
	
	There is no manual for kic, only the on-line help system.  With the
	help button selected, clicking on other buttons will provide descriptions
	and cross references.  New users should spend some time reading the
	help on the various commands.
	
	Quick Introduction
	
	-  Kic works best with a three (or more) button pointing device.
	   * Button 1 (left):     Button pressing, selections, etc.
	   * Button 2 (middle):   Pan operation
	   * Button 3 (right):    Zoom operation
	   * Button 4 (wherever): No operation, but updates coordinate display
	   If buttons are missing, kic can still be used, but less efficiently.
	
	-  By default, kic is in split-screen mode, with a coarse view and
	   a detail view.  The Zoom command is used to change the window scale,
	   and to switch to/from single-window mode.
	
	-  The Help button, when active, supplies information about the
	   functions of the various buttons.
	
	-  The ESC key terminates the current operation or command.
	
	-  The uppercase part of the button labels represents the keyboard
	   accelerator for the button.
	
	######################################################################
	#                                                                    #
	#  INSTALLATION                                                      #
	#                                                                    #
	######################################################################
	
	###########
	#          #
	# UNIX      #
	#          #
	###########
	
	This applies to the source code distributions (.tar.gz files).
	
	kic-2.4 and later --
	
	The portability of the build system is semi-dependent on use of the
	gcc compiler.  It will probably work with your non-gcc compiler, but
	be prepared to tweek the CFLAGS in the Makefiles if there is trouble.
	
	1)  Unpack the distribution file.  You will need the gzip program
	    and it is nice to have GNU tar.  Both are available from
	    prep.ai.mit.edu.
	
	    With GNU tar:
	        tar xzf kic-2.4.tar.gz
	    note: GNU tar is sometimes installed as "gtar".
	
	    Without GNU tar:
	        gunzip kic-2.4.tar.gz
	        tar xf kic-2.4.tar
	
	2)  Go to the kic-2.4 directory and run the configuration script.
	    (See #4 for a modification that might be of interest)
	
	        cd kic-2.4
	        ./configure
	
	3)  Build the programs.
	
	        make
	
	4)   Install the programs.  You probably have to be root to do this.
	
	        make install
	
	     The programs are installed by default under /usr/local.  Binary
	     executables go in /usr/local/bin, startup and help files go under
	     /usr/local/lib/kic.
	
	     You can install the programs in another location.  The easiest way
	     to do this is to use the --prefix argument to the configure script.
	     As in step #2, type instead
	
	        ./configure --prefix=/some/path
	
	     Where "/some/path" is the path to be used instead of "/usr/local".
	
	The configure/build/install should work on just about any Unix/Linux
	system, plus Mingw and Cygwin under Windows.  If any trouble, contact
	Steve Whiteley at stevew@srware.com.
	
	This completes the build/install procedure for kic-2.4 and later.
	
	To build under Mingw:
	
	  Same as above, but...
	
	  I use the Cygwin "helper" tools such as make and bash.  The "Mingw"
	  bash window uses Cygwin bash that sets a path to the Mingw compiler
	  executables ahead of the Cygwin tools.  The make suppled by Mingw is
	  moved to a different name.  The make/configure will probably fail
	  using the Mingw-specific make and bash.  You may be able to build
	  a native version under Cygwin using the -mingw option, but I haven't
	  tried this.
	
	
	Earlier releases --
	
	Use the FTP program to obtain the full source code distribution in
	file kic-xxx.tar.gz.  Uncompress and un-tar into a source tree.
	(See #1 above).
	
	Examine the files in the 'conf' subdirectory and select/create one
	to work with your system.
	
	From the top directory (the one with this file) run the configure script.
	It will ask a question or two, then create and configure the makefiles.
	You can enter the name of the configuration file on the command line,
	otherwise you will be prompted for the name by the configure script.
	
	When configure exits, cd to src/bin and run the 'make' program:
	csh:   make |& tee errs
	bash:  make 2>&1 |tee errs
	
	If your make program is not named 'make' use your name on the command
	line in place of 'make' as above.  The command will compile all the
	source code and create the executables, saving messages in the 'errs'
	file.
	
	The compilation will take a few minutes.  If there is a problem that
	you can't fix, mail the resulting errs file to stevew@srware.com,
	with a brief explanation.
	
	When finished, you can 'make install'.  Usually, you have to be root
	to do this.  This copies the binaries into KIC_EXEC_DIR (which must
	exist), and the startup files into KIC_LIB_DIR.  The parent of
	KIC_LIB_DIR must exist, and any existing KIC_LIB_DIR will be removed
	before the new one is installed.
	
	
	###########
	#          #
	# WINDOWS   #
	#          #
	###########
	
	The binary distribution for Windows comes in a self-extracting
	archive.  Simply execute the file, and answer a few questions.
	
	The program will be installed (by default) under c:\usr\local,
	with the executables in c:\usr\local\bin and the startup files
	in c:\usr\local\lib\kic.  A shortcut is installed in the Start
	menu.  An uninstall executable is also provided (UnGins.exe),
	which is placed in the Windows directory.
	
	Unless you have a good reason (such as an existing installation
	that you don't want to clobber) it is recommended that the
	default location be used, but feel free to change the drive
	letter.  If a different location is chosen, the KIC_LIB_DIR
	environment variable must be set to the kic library directory
	path, for kic to find the startup files.
	
	Kic should be started from a DOS box, or (much) better yet a
	bash box from Cygwin (www.sourceware.cygnus.com).  If you start
	by clicking on the kic icon, you should use the !cd command to
	change to some known directory, or your saved files may end up
	in odd places.  Kic must be started in the save drive as the
	setup files, unless a KIC_LIB_DIR environment variable is
	defined with a full path to the setup files directory
	including the drive letter.
	
	In all directory paths, the '/' and '\' characters can be used
	as directory separators interchangeably.  Paths can include the
	drive letter prefix.
	
	
	######################################################################
	#                                                                    #
	#  NOTES                                                             #
	#                                                                    #
	######################################################################
	
	On UNIX versions, the hcopy command at 300 dpi resolution fails when
	output is directed to the printer on many systems.  This is because
	the resulting file is too large.  You can get around this by dumping
	to a file, then using "lpr -s file", (the -s says use a symbolic link,
	see the man page).
	
	The GDSII (Stream) format uses symbol names that are upper case only.
	KIC cell names are converted to upper case during conversion.
	!!!There is currently no checking for case-insensitive name clashes!!!
	You should make sure there are no clashes before converting to GDSII.
	Be particularly careful of cells created with the logo command.
	In back-conversion, GDSII names are converted to lower case KIC cells.
	
	Previous versions of KIC used technology files named .KIC.xx in UNIX,
	and dotkic.xx in DOS.  These names are still supported for now.  The
	files are now named kic_tech.xx, under DOS or UNIX.
	
	Things to remember:
	
	1.      The ESC key gets you out of any command.
	
	2.      When performing a conversion from CIF or Stream, BE SURE TO MOVE
	        TO A NEW DIRECTORY if kic versions of the cells exist in the current
	        directory.  Otherwise, the original files will be clobbered.
	        A message reminds the user of this fact.
	
	3.      Under UNIX/X, the option "-d display" is supported, where "display"
	        is the X display name of the terminal to be used.
	        The default is found in the "DISPLAY" environment variable.
	
	4.      The "-t tech" option supplies an extension to the assumed name of
	        the tech file.  For example, if "-t trw" is given, the layer
	        attributes startup file read and written would be kic_tech.trw
	        in both cases.
	
	5.      The startup files in the startup directory can be located if they
	        are moved by setting the environment variable "KIC_LIB_DIR".  For
	        example, if the startup directory is moved to /foo/bar/kic/startup",
	        then "setenv KIC_LIB_DIR /foo/bar/kic/startup" lets kic know where
	        these files are (you could also change global.h and recompile).
	        This is similarly true for DOS.
	
	
	
	######################################################################
	#                                                                    #
	#  UTILITIES                                                         #
	#                                                                    #
	######################################################################
	
	The KIC package also contains stand-alone utilities listed below.  These
	are mostly obsolete, as the functionality is built into KIC.
	
	
	strtokic
	
	    Stand-alone stream to KIC conversion program.
	
	    Usage: strtokic [options] [streamfile]
	
	    options (case insensitive):
	
	      -P          Convert manhattan polygons to boxes,
	                  four-sided manhattan polygons are always converted.
	
	      -E          Print errors in file "strtokic.err",
	                  default is to screen (stderr).
	
	      -Csname     sname = Root structure name to convert.
	                  Default, convert all structures in file.
	
	      -Rfilename  filename = Name of root cell (default "Root").
	                  The root cell contains global library information, and
	                  can usually be ignored.
	
	      -Xfilename  Use filename as layer table reference ("ltab") file,
	                  default is to use layers from dotkic file
	                  (StreamData lines).
	
	      -Text       Use layers from dotkic.ext (.KIC.ext in UNIX).
	
	      -N          Use stream layer numbers for layer names. Will
	                  reference layers as "L NNDD" where N is the layer
	                  number and D is the data type, both as two character
	                  fields. Supersedes -X, -T.
	
	      -Lmicprl    Microns per lambda (default 1.0).
	
	    Will prompt for stream file name to convert if not supplied,
	    hit return for help.
	
	    Search path for dotkic file: . , KIC_LIB_DIR
	
	    If the data type supplied from the dotkic or ltab files is -1,
	    the datatype comparison is supressed, and all datatypes associated
	    with a given layer will be mapped to a single KIC layer.
	    Otherwise, the datatype, and of course the layer number, must
	    match those supplied.  Valid stream layer and data type numbers
	    are 0-63.  In DOS versions, the file "dos__str.als" is created or
	    added to, when it is necessary to change a cell name for DOS
	    compatibility.
	
	
	
	
	kictostr
	
	    Stand-alone KIC to stream conversion program.
	
	    Usage: kictostr [options] [root_kic_cell]
	
	    options (case insensitive):
	
	      -C          Convert only cells found in current directory.
	
	      -Zname      Stream library name (default "KICTOSTREAM").
	
	      -Ostrname   Stream file name to create.  (default is root kic cell
	                  name with .str extension)
	
	      -S          Convert symbolic layers only.
	
	      -D          Convert detail layers only.
	
	      -Xfilename  Use filename as layer table reference ("ltab") file,
	                  default is to use layers from dotkic file
	                  (StreamData lines).
	
	      -Text       Use layers in dotkic.ext (.KIC.ext in UNIX).
	
	      -N          Parse layer names for stream layers, KIC layers
	                  must be named "NNDD" (num datatype). Supersedes
	                  -X, -T.
	
	      -Lmicprl    Microns per lambda (default 1.0).
	
	      -Mupermic   Database units per micron (default 100.0).
	
	    Will prompt for kic cell name to convert if not supplied,
	    hit return for help.
	
	    If the datatype obtained from the dotkic or ltab file is outside
	    of 0-63, the written datatype is 0.
	
	    The valid stream layer numbers and data types are 0-63.
	
	    In DOS versions, if the file "dos__str.als" is present, the
	    entries are used to map DOS cell names into stream cell names.
	    This file is created by the stream to KIC converters.
	
	
	ciftokic
	
	    Stand-alone CIF to KIC converter
	
	    Usage: ciftokic [options] [cif_file]
	
	    options:
	
	      -Lmicprl    Microns per lambda (default 1.0).
	
	      -prefix     (cif dialect) where prefix =
	
	         k           Generated from KIC
	
	         a Stanford: A Stanford symbol name follows a DS command as
	                     in (PadIn);
	
	         b NCA:      An NCA symbol name follows a DS command as in
	                     (PadIn);
	
	         h IGS:      A KIC or IGS symbol name follows a DS command as
	                     in 9 PadIn; 
	
	         i Icarus:   An Icarus symbol name follows a DS command as in
	                     (9 PadIn);
	
	         q Squid:    A Squid symbol name follows a DS command as in
	                     9 /usr/joe/PadIn;
	
	         s Sif:      A Sif symbol name follows a DS command as in
	                     (Name: PadIn);
	
	         n none of the above\n");
	
	    Will prompt for cif input file name if not specified, hit
	    return for help.
	
	
	
	kictocif
	
	    Stand-alone KIC to CIF converter
	
	    Usage: kictocif [options] [root_kic_cell]
	
	    options (case insensitive):
	
	      -Ocifname   CIF file name to create.
	
	      -S          Convert symbolic layers only.
	
	      -D          Convert detail layers only.
	
	      -Pc         c = Program prefix (a,b NCA/Stanford, i Icarus, s SIF).
	
	      -Text       Use layers in dotkic.ext (.KIC.ext in UNIX).
	
	      -Lmicprl    Microns per lambda (default 1.0).
	
	    Will prompt for root kic cell if not specified, hit return for help.
	
	
	
	strmtext
	
	    Stream to text file converter
	
	    Usage:  strmtotext [-id] [-n2345678] [ streamfile [textfile] ]
	
	    options:
	
	      -id         Prints the version of the program and copyright
	                  information.
	
	      -n          Indicates a non-standard Stream file is to be read,
	                  that is, one structure beginning with BGBSTR and
	                  ending with ENDSTR.
	
	      1-8         Indicate the number of Stream records per line in
	                  the output text.  (Default is one per line).
	
	      streamfile  A Calma Stream file (3.0) input to this program.
	                  (Standard input default).
	
	      textfile    The name of the file to receive the program's output.
	                  (Standard output default).
	
	
	scale
	
	    KIC cell scaling program
	
	    Usage:  scale [-a numerator] [-b denominator] [-t ext] [root_kic_file]
	
	    options:
	
	      -a numer    Numerator of scale factor, positive integer >= 1
	                  (default 1).
	
	      -b denom    Denominator of scale factor, positive integer >= 1
	                  (default 1).
	
	      -t ext      Use dotkic.ext for layer definitions.
	
	    Will prompt for root kic file if not given.  The program multiplies
	    all coordinates in the root cell and subcells by numer/denom, and
	    writes over the original cells. If you want to preserve the original
	    cells, they have to be copied to another name or directory. BE
	    CAREFUL, EXPERIMENTING WITH THIS PROGRAM CAN CLOBBER YOUR FILES.
	
	    
