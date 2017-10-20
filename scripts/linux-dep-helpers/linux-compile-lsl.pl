#!/usr/bin/perl

# Intended to be run from linux-install_dependencies.pl
# 
# Variables are wrt that parent scope
#

# Installation of packages not available in the apt database or PPA
# Eigen installation
	
if (!$no_install) {
  
    my $old_dir = Cwd::getcwd();
 
    my $lsl_src_dir = $dependencies_arch_dir . "/LSL";

    # fetch the packages
    chdir "$dependencies_arch_dir";
    if (! -e "liblsl-1.04.ov1-src.tar.bz2") {
      system('wget "http://openvibe.inria.fr/dependencies/linux-x86/liblsl-1.04.ov1-src.tar.bz2"');
      ($CHILD_ERROR != 0) and die ("Could not download the lsl sources [$CHILD_ERROR]");
    }
    if (! -e $lsl_src_dir) {
      system('tar xjf "liblsl-1.04.ov1-src.tar.bz2"');
      ($CHILD_ERROR != 0) and die ("Could not extract the lsl archive");
    }
    
    # compile
  
    # external
    print "Compiling lsl ...\n";

    chdir($lsl_src_dir);
    mkdir("liblsl/build");
    system("cd liblsl/build && cmake .. >cmake-liblsl.log 2>&1");
    system("cd liblsl/build && make >make-liblsl.log 2>&1");

    mkdir("$dependencies_dir/lib");
    mkdir("$dependencies_dir/include");

    system("cp liblsl/build/src/liblsl.so $dependencies_dir/lib");
    system("cp -R liblsl/include/* $dependencies_dir/include");


    chdir $old_dir
};

