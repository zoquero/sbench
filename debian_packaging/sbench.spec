#
# Quick guide for building RPM on Ubuntu:
#
# $ apt-get install rpm
# $ mkdir -p ~/rpmbuild/SOURCES ~/rpmbuild/SPECS
# $ cp ....tar.gz ~/rpmbuild/SOURCES 
# $ rpmbuild -ba hellorpm.spec 
#

%define sbench_version 1.0

Summary:   Simple benchmarks of CPU, memory, disk and network
Name:      sbench
Version:   %{sbench_version}
Release:   1%{?dist}
Source0:   %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
# BuildRoot: %{_tmppath}/%{name}-%{version}-build
License:   GPLv3 (GNU General Public License v3)
Group:     Applications/System
URL:       https://github.com/zoquero/sbench/
Packager:  Angel Galindo Munoz <zoquero@gmail.com>

#########################################################################################
# Ubuntu
#########################################################################################
%if 0%{?ubuntu_version}

Description: Simple benchmarks of CPU, memory, disk and network.
More info can be found here: https://github.com/zoquero/sbench
BuildRequires: libcurl4-openssl-dev
BuildRequires: liboping-dev
Requires:      libcurl3
Requires:      libcurl3-gnutls
Requires:      liboping0

%endif


#########################################################################################
# SuSE, openSUSE
#########################################################################################
%if 0%{?suse_version}

BuildRequires: libcurl-devel
# octo's ping library is not available on SLES
# BuildRequires: liboping-devel 


##
## ... I can't test it on SLES 11 SP4 32 b because can't I install
## sle-sdk-release packages following https://www.novell.com/support/kb/doc.php?id=7015337
##

%ifarch i386 i486 i586 i686 athlon
Requires:      libcurl4-32bit
# octo's ping library is not available on SLES
# Requires:      liboping0
%endif

%ifarch x86_64
Requires:      libcurl4
# octo's ping library is not available on SLES
# Requires:      liboping0
%endif

%endif

# Requires(post): info
# Requires(preun): info

%description 

Simple benchmarks of CPU, memory, disk and network.
More info can be found here: https://github.com/zoquero/sbench

%prep
%setup -q

%build
%configure
%if 0%{?ubuntu_version}
make %{?_smp_mflags} PING_ENABLE=y
%else
make %{?_smp_mflags}
%endif

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
make install DESTDIR=$RPM_BUILD_ROOT/usr/bin MANDIR=$RPM_BUILD_ROOT/usr/share/man/man1

# %make_install
# %find_lang %{name}
# rm -f %{buildroot}/%{_infodir}/dir

%clean
rm -rf $RPM_BUILD_ROOT

%post
# /sbin/install-info %{_infodir}/%{name}.info %{_infodir}/dir || :

%preun
# if [ $1 = 0] ; then
# /sbin/install-info --delete %{_infodir}/%{name}.info %{_infodir}/dir || :
# fi

%files
%defattr(0644,root,root,0644)
%attr(0755, root, root) /usr/bin/%{name}
### Can't make it work on Ubuntu and SLES at the same time. It just works on SLES:
### %attr(0644, root, root) /usr/share/man/man1/sbench.1.gz

# %files -f %{name}.lang
# %doc AUTHORS ChangeLog COPYING NEWS README THANKS TODO
# %{_mandir}/man1/hellorpm.1.gz
# %{_infodir}/%{name}.info.gz
# %{_bindir}/hellorpm

%changelog
* Sun Nov 13 2016 Angel <zoquero@gmail.com> 1.0
- Initial version of the package
# ORG-LIST-END-MARKER
