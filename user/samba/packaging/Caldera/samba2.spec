Summary: Samba SMB client and server
Name: samba
Version: 2.0.4b
Release: 19990519
Copyright: GNU GPL version 2
Group: Networking
Source: ftp://samba.org/pub/samba/samba-2.0.4b.tar.gz
Patch: makefile-path.patch
Patch1: smbw.patch
Packager: John H Terpstra [Samba-Team] <jht@samba.org>
BuildRoot: /var/tmp/samba

%description
Samba provides an SMB server which can be used to provide
network services to SMB (sometimes called "Lan Manager")
clients, including various versions of MS Windows, OS/2,
and other Linux machines. Samba also provides some SMB
clients, which complement the built-in SMB filesystem
in Linux. Samba uses NetBIOS over TCP/IP (NetBT) protocols
and does NOT need NetBEUI (Microsoft Raw NetBIOS frame)
protocol.

Samba-2 features an almost working NT Domain Control
capability and includes the new SWAT (Samba Web Administration
Tool) that allows samba's smb.conf file to be remotely managed
using your favourite web browser. For the time being this is
being enabled on TCP port 901 via inetd.

Please refer to the WHATSNEW.txt document for fixup information.
This binary release includes encrypted password support.
Please read the smb.conf file and ENCRYPTION.txt in the
docs directory for implementation details.

%changelog
* Mon Nov 16 1998 John H Terpstra <jht@samba.org>
 - Ported to Cadera OpenLinux

%prep
%setup
%patch -p1
%patch1 -p1

%build
cd source
./configure --prefix=/usr --libdir=/etc --with-lockdir=/var/lock/samba --with-privatedir=/etc --with-swatdir=/usr/share/swat --with-pam
make all

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/codepages/src
mkdir -p $RPM_BUILD_ROOT/etc/{logrotate.d,pam.d}
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/{init.d,rc0.d,rc1.d,rc2.d,rc3.d,rc5.d,rc6.d}
mkdir -p $RPM_BUILD_ROOT/home/samba
mkdir -p $RPM_BUILD_ROOT/usr/{bin,sbin}
mkdir -p $RPM_BUILD_ROOT/usr/share/swat/{images,help,include}
mkdir -p $RPM_BUILD_ROOT/usr/man/{man1,man5,man7,man8}
mkdir -p $RPM_BUILD_ROOT/var/lock/samba
mkdir -p $RPM_BUILD_ROOT/var/log/samba
mkdir -p $RPM_BUILD_ROOT/var/spool/samba

# Install standard binary files
for i in nmblookup smbclient smbpasswd smbstatus testparm testprns \
      make_smbcodepage make_printerdef rpcclient
do
install -m755 -s source/bin/$i $RPM_BUILD_ROOT/usr/bin
done
for i in addtosmbpass mksmbpasswd.sh smbtar 
do
install -m755 source/script/$i $RPM_BUILD_ROOT/usr/bin
done

# Install secure binary files
for i in smbd nmbd swat
do
install -m755 -s source/bin/$i $RPM_BUILD_ROOT/usr/sbin
done

# Install level 1 man pages
for i in smbclient.1 smbrun.1 smbstatus.1 smbtar.1 testparm.1 testprns.1 make_smbcodepage.1 nmblookup.1
do
install -m644 docs/manpages/$i $RPM_BUILD_ROOT/usr/man/man1
done

# Install codepage source files
for i in 437 737 850 852 861 866 932 936 949 950
do
install -m644 source/codepages/codepage_def.$i $RPM_BUILD_ROOT/etc/codepages/src
done

# Install SWAT helper files
for i in swat/help/*.html docs/htmldocs/*.html
do
install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/help
done
for i in swat/images/*.gif
do
install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/images
done
for i in swat/include/*.html
do
install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/include
done

# Install the miscellany
install -m644 swat/README $RPM_BUILD_ROOT/usr/share/swat
install -m644 docs/manpages/smb.conf.5 $RPM_BUILD_ROOT/usr/man/man5
install -m644 docs/manpages/lmhosts.5 $RPM_BUILD_ROOT/usr/man/man5
install -m644 docs/manpages/smbpasswd.5 $RPM_BUILD_ROOT/usr/man/man5
install -m644 docs/manpages/samba.7 $RPM_BUILD_ROOT/usr/man/man7
install -m644 docs/manpages/smbd.8 $RPM_BUILD_ROOT/usr/man/man8
install -m644 docs/manpages/nmbd.8 $RPM_BUILD_ROOT/usr/man/man8
install -m644 docs/manpages/swat.8 $RPM_BUILD_ROOT/usr/man/man8
install -m644 docs/manpages/smbpasswd.8 $RPM_BUILD_ROOT/usr/man/man8
install -m644 packaging/RedHat/smb.conf $RPM_BUILD_ROOT/etc/smb.conf
install -m644 packaging/RedHat/smbusers $RPM_BUILD_ROOT/etc/smbusers
install -m755 packaging/RedHat/smbprint $RPM_BUILD_ROOT/usr/bin
install -m755 packaging/RedHat/findsmb $RPM_BUILD_ROOT/usr/bin
install -m755 packaging/RedHat/smbadduser $RPM_BUILD_ROOT/usr/bin
install -m755 packaging/RedHat/smb.init $RPM_BUILD_ROOT/etc/rc.d/init.d/smb
install -m755 packaging/RedHat/smb.init $RPM_BUILD_ROOT/usr/sbin/samba
install -m644 packaging/RedHat/samba.pamd $RPM_BUILD_ROOT/etc/pam.d/samba
install -m644 packaging/RedHat/samba.log $RPM_BUILD_ROOT/etc/logrotate.d/samba
echo 127.0.0.1 localhost > $RPM_BUILD_ROOT/etc/lmhosts

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add smb

# Build codepage load files
for i in 437 737 850 852 861 866 932 936 949 950
do
/usr/bin/make_smbcodepage c $i /etc/codepages/src/codepage_def.$i /etc/codepages/codepage.$i
done

# Add swat entry to /etc/services if not already there
if !( grep ^[:space:]*swat /etc/services > /dev/null ) then
	echo 'swat		901/tcp				# Add swat service used via inetd' >> /etc/services
fi

# Add swat entry to /etc/inetd.conf if needed
if !( grep ^[:space:]*swat /etc/inetd.conf > /dev/null ) then
	echo 'swat	stream	tcp	nowait.400	root	/usr/sbin/swat swat' >> /etc/inetd.conf
killall -1 inetd || :
fi

%preun
if [ $1 = 0 ] ; then
    /sbin/chkconfig --del smb

    for n in /etc/codepages/*; do
	if [ $n != /etc/codepages/src ]; then
	    rm -rf $n
	fi
    done
    # We want to remove the browse.dat and wins.dat files so they can not interfer with a new version of samba!
    if [ -e /var/lock/samba/browse.dat ]; then
	    rm -f /var/lock/samba/browse.dat
    fi
    if [ -e /var/lock/samba/wins.dat ]; then
	    rm -f /var/lock/samba/wins.dat
    fi
fi

%postun
# Only delete remnants of samba if this is the final deletion.
if [ $1 != 0 ] ; then
    exit 0

    if [ -x /etc/pam.d/samba ]; then
      rm -f /etc/pam.d/samba
    fi
    if [ -e /var/log/samba ]; then
      rm -rf /var/log/samba
    fi
    if [ -e /var/lock/samba ]; then
      rm -rf /var/lock/samba
    fi

    # Remove swat entries from /etc/inetd.conf and /etc/services
    cd /etc
    tmpfile=/etc/tmp.$$
    sed -e '/^[:space:]*swat.*$/d' /etc/inetd.conf > $tmpfile
    mv $tmpfile inetd.conf
    sed -e '/^[:space:]*swat.*$/d' /etc/services > $tmpfile
    mv $tmpfile services
fi

%files
%doc README COPYING Manifest Read-Manifest-Now
%doc WHATSNEW.txt Roadmap
%doc docs
%doc swat/README
%doc examples
%attr(-,root,root) /usr/sbin/smbd
%attr(-,root,root) /usr/sbin/nmbd
%attr(-,root,root) /usr/sbin/swat
%attr(0750,root,root) /usr/sbin/samba
%attr(-,root,root) /usr/bin/addtosmbpass
%attr(-,root,root) /usr/bin/mksmbpasswd.sh
%attr(-,root,root) /usr/bin/smbclient
%attr(-,root,root) /usr/bin/rpcclient
%attr(-,root,root) /usr/bin/testparm
%attr(-,root,root) /usr/bin/testprns
%attr(-,root,root) /usr/bin/findsmb
%attr(-,root,root) /usr/bin/smbstatus
%attr(-,root,root) /usr/bin/nmblookup
%attr(-,root,root) /usr/bin/make_smbcodepage
%attr(-,root,root) /usr/bin/make_printerdef
%attr(-,root,root) /usr/bin/smbpasswd
%attr(-,root,root) /usr/bin/smbtar
%attr(-,root,root) /usr/bin/smbprint
%attr(-,root,root) /usr/bin/smbadduser
%attr(-,root,root) /usr/share/swat/help/welcome.html
%attr(-,root,root) /usr/share/swat/help/DOMAIN_MEMBER.html
%attr(-,root,root) /usr/share/swat/help/lmhosts.5.html
%attr(-,root,root) /usr/share/swat/help/make_smbcodepage.1.html
%attr(-,root,root) /usr/share/swat/help/nmbd.8.html
%attr(-,root,root) /usr/share/swat/help/nmblookup.1.html
%attr(-,root,root) /usr/share/swat/help/samba.7.html
%attr(-,root,root) /usr/share/swat/help/smb.conf.5.html
%attr(-,root,root) /usr/share/swat/help/smbclient.1.html
%attr(-,root,root) /usr/share/swat/help/smbd.8.html
%attr(-,root,root) /usr/share/swat/help/smbpasswd.5.html
%attr(-,root,root) /usr/share/swat/help/smbpasswd.8.html
%attr(-,root,root) /usr/share/swat/help/smbrun.1.html
%attr(-,root,root) /usr/share/swat/help/smbstatus.1.html
%attr(-,root,root) /usr/share/swat/help/smbtar.1.html
%attr(-,root,root) /usr/share/swat/help/swat.8.html
%attr(-,root,root) /usr/share/swat/help/testparm.1.html
%attr(-,root,root) /usr/share/swat/help/testprns.1.html
%attr(-,root,root) /usr/share/swat/images/globals.gif
%attr(-,root,root) /usr/share/swat/images/home.gif
%attr(-,root,root) /usr/share/swat/images/passwd.gif
%attr(-,root,root) /usr/share/swat/images/printers.gif
%attr(-,root,root) /usr/share/swat/images/shares.gif
%attr(-,root,root) /usr/share/swat/images/samba.gif
%attr(-,root,root) /usr/share/swat/images/status.gif
%attr(-,root,root) /usr/share/swat/images/viewconfig.gif
%attr(-,root,root) /usr/share/swat/include/header.html
%attr(-,root,root) /usr/share/swat/include/footer.html
%attr(-,root,root) %config(noreplace) /etc/lmhosts
%attr(-,root,root) %config(noreplace) /etc/smb.conf
%attr(-,root,root) %config(noreplace) /etc/smbusers
%attr(-,root,root) /etc/rc.d/init.d/smb
%attr(-,root,root) /etc/logrotate.d/samba
%attr(-,root,root) /etc/pam.d/samba
%attr(-,root,root) /etc/codepages/src/codepage_def.437
%attr(-,root,root) /etc/codepages/src/codepage_def.737
%attr(-,root,root) /etc/codepages/src/codepage_def.850
%attr(-,root,root) /etc/codepages/src/codepage_def.852
%attr(-,root,root) /etc/codepages/src/codepage_def.861
%attr(-,root,root) /etc/codepages/src/codepage_def.866
%attr(-,root,root) /etc/codepages/src/codepage_def.932
%attr(-,root,root) /etc/codepages/src/codepage_def.936
%attr(-,root,root) /etc/codepages/src/codepage_def.949
%attr(-,root,root) /etc/codepages/src/codepage_def.950
%attr(-,root,root) /usr/man/man1/smbstatus.1
%attr(-,root,root) /usr/man/man1/smbclient.1
%attr(-,root,root) /usr/man/man1/make_smbcodepage.1
%attr(-,root,root) /usr/man/man1/smbrun.1
%attr(-,root,root) /usr/man/man1/smbtar.1
%attr(-,root,root) /usr/man/man1/testparm.1
%attr(-,root,root) /usr/man/man1/testprns.1
%attr(-,root,root) /usr/man/man1/nmblookup.1
%attr(-,root,root) /usr/man/man5/smb.conf.5
%attr(-,root,root) /usr/man/man5/lmhosts.5
%attr(-,root,root) /usr/man/man5/smbpasswd.5
%attr(-,root,root) /usr/man/man7/samba.7
%attr(-,root,root) /usr/man/man8/smbd.8
%attr(-,root,root) /usr/man/man8/nmbd.8
%attr(-,root,root) /usr/man/man8/smbpasswd.8
%attr(-,root,root) /usr/man/man8/swat.8
%attr(-,root,nobody) %dir /home/samba
%attr(-,root,root) %dir /etc/codepages
%attr(-,root,root) %dir /etc/codepages/src
%attr(-,root,root) %dir /var/lock/samba
%attr(-,root,root) %dir /var/log/samba
%attr(1777,root,root) %dir /var/spool/samba
