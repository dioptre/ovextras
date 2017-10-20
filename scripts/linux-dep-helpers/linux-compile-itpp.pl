#!/usr/bin/perl

# Intended to be run from linux-install_dependencies.pl
# 
# Variables are wrt that parent scope
#

# Installation of packages not available in the apt database or PPA
# Eigen installation
	
if (!$no_install && $distribution eq 'Fedora') {
#if (1) {
  
    my $old_dir = Cwd::getcwd();
 
    my $itpp_build_dir = $dependencies_dir . "/itpp-build";
    my $itppe_src_dir = $dependencies_arch_dir . "/itpp-external-3.0.0";
    my $itpp_src_dir = $dependencies_arch_dir . "/itpp-4.0.7";

    if (! -e $itpp_build_dir) {
      mkdir($itpp_build_dir) or die("Failed to create directory [$itpp_build_dir]");
    }

    # fetch the packages
    chdir "$dependencies_arch_dir";
    if (! -e "itpp-external-3.0.0.tar.bz2") {
      system('wget "http://openvibe.inria.fr/dependencies/linux-x86/itpp-external-3.0.0.tar.bz2"');
      ($CHILD_ERROR != 0) and die ("Could not download the itpp external sources [$CHILD_ERROR]");
    }
    if (! -e $itppe_src_dir) {
      system('tar -xjf "itpp-external-3.0.0.tar.bz2"');
      ($CHILD_ERROR != 0) and die ("Could not extract the itpp external archive");
    }
    
    if (! -e "itpp-4.0.7.tar.bz2") {
      system('wget "http://openvibe.inria.fr/dependencies/linux-x86/itpp-4.0.7.tar.bz2"');
      ($CHILD_ERROR != 0) and die ("Could not download the itpp sources [$CHILD_ERROR]");
    }
    if (! -e $itpp_src_dir) {
      system('tar -xjf "itpp-4.0.7.tar.bz2"');
      ($CHILD_ERROR != 0) and die ("Could not extract the itpp archive");
    }

    # compile
  
    # external
    print "Compiling itpp external ...\n";
    chdir $itppe_src_dir;

    system('sed -i "s/_EXT_ETIME/_INT_ETIME/g" patches/lapack-3.1.1-autotools.patch');
    system('sed -i "s/_EXT_ETIME/_INT_ETIME/g" src/lapack-lite-3.1.1/SRC/Makefile.in');
    system('sed -i "s/_EXT_ETIME/_INT_ETIME/g" src/lapack-lite-3.1.1/SRC/Makefile.am');

    system("./configure --prefix=$dependencies_dir/ >$itpp_build_dir/itppe-configure.log 2>&1");
    system("make >$itpp_build_dir/itpp-external-build.log 2>&1");
    ($CHILD_ERROR != 0) and die("Failed to run make for itpp-external [$CHILD_ERROR]");
    system('make install');
    ($CHILD_ERROR != 0) and die("Failed to run make install for itpp-external [$CHILD_ERROR]");

    # main pkg
    print "Compiling itpp ...\n";
    chdir $itpp_src_dir;

    system("./configure --prefix=$dependencies_dir/ CPPFLAGS=\"-I$dependencies_dir/include\" LDFLAGS=\"-L$dependencies_dir/lib -L$dependencies_dir/lib64\" >$itpp_build_dir/itpp-configure.log 2>&1");
    system("make >$itpp_build_dir/itpp-build.log 2>&1");
    ($CHILD_ERROR != 0) and die("Failed to run make for itpp [$CHILD_ERROR]");
    system("make install");
    ($CHILD_ERROR != 0) and die("Failed to run make install for itpp [$CHILD_ERROR]");

    chdir $old_dir
};

