%define name npcupsdrv
%define version 2.3.0
%define release 11

Name:		%{name}
Summary:	NP CUPS printer drivers.
Version:	%{version}
Release:	%{release}
Copyright:	GPL
Group:		Hardware/Printing
Source:		http://www.star-m.jp/service/s_print/bin/%{name}-2.3.0.tar.gz
URL:		http://www.star-m.jp/service/s_print/starcupsdrv_linux_yyyymmdd.htm
Vendor:		Star Micronics Co., Ltd.
Packager:	Albert Kennis <albert@star-m.jp>
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
The StarCUPSDrv package contains CUPS based printer drivers
for the following models:

NP211

NP326


These drivers allow for printing from all applications that use the
standard printing path (CUPS).

After installing this package, go to http://localhost:631 (aka http://127.0.0.1:631)
or use your favorite CUPS print manager to add a new queue for your printer.

%prep
%setup

%build
make RPMBUILD=true

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT rpmbuild

%clean
rm -rf $RPM_BUILD_ROOT

%pre
LDCONFIGREQ=0
if [ ! -e /usr/lib/libcups.so ]
then
    for libcups in $(ls /usr/lib/libcups.so*)
    do
        if [ -x $libcups ]
        then
            LDCONFIGREQ=1
            ln -s $libcups /usr/lib/libcups.so
            break
        fi
    done
fi
if [ ! -x /usr/lib/libcups.so ]
then
    echo "required library libcups.so not available"
    exit 1
fi
if [ ! -e /usr/lib/libcupsimage.so ]
then
    for libcupsimage in $(ls /usr/lib/libcupsimage.so*)
    do
        if [ -x $libcupsimage ]
        then
            LDCONFIGREQ=1
            ln -s $libcupsimage /usr/lib/libcupsimage.so
            break
        fi
    done
fi
if [ ! -x /usr/lib/libcupsimage.so ]
then
    echo "required library libcupsimage.so not available"
    exit 1
fi
if [ "$LDCONFIGREQ" = "1" ]
then
    ldconfig
fi
exit 0

%post
if [ -x /etc/software/init.d/cups ]
then
    /etc/software/init.d/cups stop
    /etc/software/init.d/cups start
elif [ -x /etc/rc.d/init.d/cups ]
then
    /etc/rc.d/init.d/cups stop
    /etc/rc.d/init.d/cups start
elif [ -x /etc/init.d/cups ]
then
    /etc/init.d/cups stop
    /etc/init.d/cups start
elif [ -x /sbin/init.d/cups ]
then
    /sbin/init.d/cups stop
    /sbin/init.d/cups start
elif [ -x /etc/software/init.d/cupsys ]
then
    /etc/software/init.d/cupsys stop
    /etc/software/init.d/cupsys start
elif [ -x /etc/rc.d/init.d/cupsys ]
then
    /etc/rc.d/init.d/cupsys stop
    /etc/rc.d/init.d/cupsys start
elif [ -x /etc/init.d/cupsys ]
then
    /etc/init.d/cupsys stop
    /etc/init.d/cupsys start
elif [ -x /sbin/init.d/cupsys ]
then
    /sbin/init.d/cupsys stop
    /sbin/init.d/cupsys start
else
    killall -HUP cupsd
fi
exit 0

%preun
for ppd in $(ls '/etc/cups/ppd')
do
    if grep "NP211\|NP326"  "/etc/cups/ppd/$ppd"; then exit 127; fi
done
exit 0

%files
%attr(755, root, root) /usr/lib/cups/filter/rastertonp
%attr(755, root, root) %dir /usr/share/cups/model/np/
%attr(644, root, root) /usr/share/cups/model/star/np211.ppd.gz
%attr(644, root, root) /usr/share/cups/model/star/np326.ppd.gz

%changelog
* Dri Feb 13 2005 Dwayne Harris <dharris@starmicronics.com>
- Version 2.3.0 initial release

