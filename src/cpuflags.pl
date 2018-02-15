#!/usr/bin/env perl
#
# CPUflags provides database for cpuflags (with and without 
# optimisation) for different operating systems and processors. 
# By autodetecting the current CPU, optimal cflags for the 
# target system are returned.
# 
#   supported platforms : Linux, Solaris, AIX, IRIX, 
#                         Darwin, Tru64, HP-UX, Win32
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (version 2 of the License).
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

use Getopt::Long;

###############################################################
###############################################################
##
## default settings
##
###############################################################
###############################################################

$system      = "";
$cpu         = $ENV{"PROCESSOR"};
$bits        = 64;
$show_cpu    = "0";
$show_cflags = "1";
$show_opt    = "0";
$compiler    = "cc";
$compversion = "";
$version     = "0.7.2";
$verbosity   = 0;

$cflags      = "";
$oflags      = "-O";

#
# terminal colour options
# (N: normal, F: bold, R: red, G: green, B: blue)
#
$tput = "tput";
%tc   = ( N => "", F => "", R => "", G => "", B => "" );

###############################################################
###############################################################
##
## misc. functions
##
###############################################################
###############################################################

###############################################################
#
# print error message
# 
sub error 
{
    print( stderr "$tc{F}$tc{R}cpuflags$tc{N}: $_[0]" );
}

###############################################################
#
# show usage
#

sub usage
{
    print "cpuflags v$version by Ronald Kriemann\n";
    print "usage: cpuflags [ options ] \n";
    print "  where options include :\n";
    print "    --cpu <name>        : show flags for given cpu-identifier (default autodetect)\n";
    print "    --comp <name>       : set compiler to use (default: cc)\n";
    print "    --showcpu           : print CPU identifier (default off)\n";
    print "    --[no]cflags        : (don't) print compiler flags (default on)\n";
    print "    --[no]opt           : (don't) print optimisation flags (default off)\n";
    print "    -32|-64             : set bitsize (default: max. available)\n";
    print "    -h, --help          : print this message\n";
    print "    -v, --verbose [int] : verbosity level (default 0)\n";
    print "    -V, --version       : print version number\n";
    print "\n";
    print "  Supported systems and compilers:\n";
    print "    Linux   : gcc-[234].x, icc-[5678], pgi, PathScale 2.x, Sun Studio 11\n";
    print "    Solaris : gcc-[234].x, SunCC-5.x, KaiCC\n";
    print "    AIX     : gcc, xlC\n";
    print "    IRIX    : gcc, mipscc\n";
    print "    Darwin  : gcc-[234].x\n";
    print "    Tru64   : Compaq-CC-6.x\n";
    print "    HP-UX   : HP Ansi C 3.x \n";
    print "    Win32   : MS Visual C/C++ 14.x\n";
    print "\n";
    print "  Supported CPUs:\n";
    print "    Linux   : i386, i486, i586, pentium, pentium-mmx, pentium-pro,\n";
    print "              p2, p3, p3-sse, p3-xeon, centrino, p4, p4-(w|p|n|f),\n";
    print "              k6, k6-2, k6-3, athlon, athlon-tbird, athlon-xp,\n";
    print "              opteron, via-c3a, ppc750, ppc7450\n";
    print "    Solaris : ultra1, ultra2, ultra3, ultra3cu, opteron\n";
    print "    AIX     : power4\n";
    print "    IRIX    : mips4\n";
    print "    Darwin  : ppc750, ppc7400, ppc7450, ppc970\n";
    print "    Tru64   : ev5, ev56, ev6, ev67\n";
    print "    HP-UX   : PA7xxx, PA8xxx\n";
    print "    Win32   : native\n";
    print "\n";

    exit 0;
}

###############################################################
#
# parse command-line
#

sub parse_cmdline
{
    my  $help, $show_version, $bit32, $bit64;
    my  %options = ( 'cpu=s'       => \$cpu,
                     'showcpu'     => \$show_cpu,
                     'cflags!'     => \$show_cflags,
                     'opt!'        => \$show_oflags,
                     'comp=s'      => \$compiler,
                     'h|help'      => \$help,
                     'v|verbose:i' => \$verbosity,
                     'V|version'   => \$show_version,
                     '32'          => \$bit32,
                     '64'          => \$bit64 );
    

    Getopt::Long::Configure( "no_ignore_case", "pass_through", "no_auto_abbrev" );
    GetOptions( %options ) || usage;

    usage if $help;
    
    if ( $show_version )
    {
        print "cpuflags $tc{F}v$version$tc{N} by Ronald Kriemann\n";
        exit 0;
    }

    $bits = 32 if ( $bit32 );
    $bits = 64 if ( $bit64 );
}

###############################################################
#
# init colour output
#

sub init_colours
{
    $nc = `$tput colors 2>/dev/null`;
    
    if ( $nc >= 8 )
    {
        $tc{N} = `$tput sgr0`;
        $tc{F} = `$tput bold`;
        $tc{R} = `$tput setaf 1`; $tc{R} .= $tc{F};
        $tc{G} = `$tput setaf 2`; $tc{G} .= $tc{F};
        $tc{B} = `$tput setaf 4`; $tc{B} .= $tc{F};
    }
}

###############################################################
###############################################################
##
## setup compiler-flags
##
###############################################################
###############################################################

###############################################################
#
# setup basic CFLAGS
# 
sub get_cflags 
{
    my $system   = $_[0];
    my $cpu      = $_[1];
    my $compiler = $_[2];
    my $cflags   = "undef";

    if ( $system =~ /Linux|CYGWIN/ )
    {
        if ( $compiler =~ /gcc-2/ )
        { 
            for ( $cpu )
            {
                if    ( /i386/ )                 { $cflags = "-march=i386"; }
                elsif ( /i486/ )                 { $cflags = "-march=i486"; }
                elsif ( /i586|pentium($|-mmx)/ ) { $cflags = "-march=i586"; }
                elsif ( /via-c3/ )               { $cflags = "-march=i586"; }
                elsif ( /k6/ )                   { $cflags = "-march=k6"; }
                elsif ( /pentiumpro|p2|p3|p4/ )  { $cflags = "-march=i686"; }
                elsif ( /athlon|opteron/ )       { $cflags = "-march=i686"; }
                elsif ( /ppc750/ )               { $cflags = "-mcpu=750 -mpowerpc-gfxopt  -fsigned-char"; }
                elsif ( /ppc7400/ )              { $cflags = "-mcpu=7400 -mpowerpc-gfxopt -fsigned-char" }
                elsif ( /ppc7450/ )              { $cflags = "-mcpu=7450 -mpowerpc-gfxopt -fsigned-char" }
                elsif ( /itanium2/ )             { $cflags = " "; }
            }
        }
        elsif ( $compiler =~ /gcc-3.[0123]/ )
        {
            for ( $cpu )
            {
                if    ( /i386/ )          { $cflags = "-march=i386"; }
                elsif ( /i486/ )          { $cflags = "-march=i486"; }
                elsif ( /i586|pentium$/ ) { $cflags = "-march=i586"; }
                elsif ( /k6$/ )           { $cflags = "-march=k6"; }
                elsif ( /k6-2/ )          { $cflags = "-march=k6-2"; }
                elsif ( /k6-3/ )          { $cflags = "-march=k6-3"; }
                elsif ( /pentium-mmx/ )   { $cflags = "-march=pentium-mmx"; }
                elsif ( /pentiumpro/ )    { $cflags = "-march=i686"; }
                elsif ( /p2/ )            { $cflags = "-march=pentium2"; }
                elsif ( /p3-(sse|xeon)/ ) { $cflags = "-march=pentium3 -mfpmath=sse";  }
                elsif ( /p3/ )            { $cflags = "-march=pentium3";  }
                elsif ( /pentium-m/ )     { $cflags = "-march=pentium3 -mfpmath=sse"; }
                elsif ( /core|core2/ )    { $cflags = "-march=pentium3 -mfpmath=sse"; }
                elsif ( /p4/ )            { $cflags = "-march=pentium4 -mfpmath=sse"; }
                elsif ( /athlon-tbird/ )  { $cflags = "-march=athlon-tbird"; }
                elsif ( /athlon-xp/ )     { $cflags = "-march=athlon-xp -msse -mfpmath=sse,387"; }
                elsif ( /athlon/ )        { $cflags = "-march=athlon"; }
                elsif ( /opteron/ )       { $cflags = "-march=opteron -m$bits -mfpmath=sse,387"; }
                elsif ( /via-c3a/ )       { $cflags = "-march=c3 -msse -mfpmath=sse"; }
                elsif ( /ppc750/ )        { $cflags = "-mcpu=750 -mpowerpc-gfxopt -fsigned-char"; }
                elsif ( /ppc7400/ )       { $cflags = "-mcpu=7400 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /ppc7450/ )       { $cflags = "-mcpu=7450 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /itanium2/ )      { $cflags = " "; }
            }
        }
        elsif ( $compiler =~ /gcc-(3.4|4.?)/ )
        {
            for ( $cpu )
            {
                if    ( /i386/ )          { $cflags = "-march=i386"; }
                elsif ( /i486/ )          { $cflags = "-march=i486"; }
                elsif ( /i586|pentium$/ ) { $cflags = "-march=i586"; }
                elsif ( /k6$/ )           { $cflags = "-march=k6"; }
                elsif ( /k6-2/ )          { $cflags = "-march=k6-2"; }
                elsif ( /k6-3/ )          { $cflags = "-march=k6-3"; }
                elsif ( /pentium-mmx/ )   { $cflags = "-march=pentium-mmx"; }
                elsif ( /pentiumpro/ )    { $cflags = "-march=i686"; }
                elsif ( /p2/ )            { $cflags = "-march=pentium2"; }
                elsif ( /p3-(sse|xeon)/ ) { $cflags = "-march=pentium3 -mfpmath=sse";  }
                elsif ( /p3/ )            { $cflags = "-march=pentium3";  }
                elsif ( /pentium-m/ )     { $cflags = "-march=pentium-m -msse2 -mfpmath=sse"; }
                elsif ( /core2/ )         { $cflags = "-march=core2 -msse3 -mfpmath=sse"; }
                elsif ( /core/ )          { $cflags = "-march=core -msse3 -mfpmath=sse"; }
                elsif ( /p4$/ )           { $cflags = "-march=pentium4 -msse2 -mfpmath=sse"; }
                elsif ( /p4-willamette/ ) { $cflags = "-march=pentium4 -msse2 -mfpmath=sse"; }
                elsif ( /p4-northwood/ )  { $cflags = "-march=pentium4 -msse2 -mfpmath=sse"; }
                elsif ( /p4-prescott/ )   { $cflags = "-march=prescott -msse3 -mfpmath=sse"; }
                elsif ( /p4-nocona/ )     { $cflags = "-march=nocona -msse3 -mfpmath=sse"; }
                elsif ( /p4-foster/ )     { $cflags = "-march=nocona -msse3 -mfpmath=sse"; }
                elsif ( /athlon-tbird/ )  { $cflags = "-march=athlon-tbird"; }
                elsif ( /athlon-xp/ )     { $cflags = "-march=athlon-xp -msse -mfpmath=sse,387"; }
                elsif ( /athlon/ )        { $cflags = "-march=athlon"; }
                elsif ( /opteron/ )       { $cflags = "-march=opteron -m$bits -mfpmath=sse,387"; }
                elsif ( /via-c3a/ )       { $cflags = "-march=c3 -msse -mfpmath=sse"; }
                elsif ( /ppc750/ )        { $cflags = "-mcpu=750 -mpowerpc-gfxopt -fsigned-char"; }
                elsif ( /ppc7400/ )       { $cflags = "-mcpu=7400 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /ppc7450/ )       { $cflags = "-mcpu=7450 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /itanium2/ )      { $cflags = " "; }
            }
        }
        elsif ( $compiler =~ /icc-[567]/ )
        {
            for ( $cpu )
            {
                if    ( /pentium$/ )      { $cflags = "-tpp5"; }
                elsif ( /pentium-mmx/ )   { $cflags = "-tpp5 -xM"; }
                elsif ( /k6/ )            { $cflags = "-tpp5 -xM"; }
                elsif ( /pentiumpro/ )    { $cflags = "-tpp6"; }
                elsif ( /p2/ )            { $cflags = "-tpp6 -xM"; }
                elsif ( /p3-(sse|xeon)/ ) { $cflags = "-tpp6 -march=pentiumiii -xK";  } 
                elsif ( /p3/ )            { $cflags = "-tpp6 -march=pentiumii -xM"; }
                elsif ( /banias|dothan|yonah/ ) { $cflags = "-tpp7 -xB"; }
                elsif ( /p4/ )            { $cflags = "-tpp7 -xW"; }
                elsif ( /athlon-xp/ )     { $cflags = "-tpp6 -xK"; }
                elsif ( /athlon/ )        { $cflags = "-tpp6 -xM"; }
                elsif ( /opteron/ )       { $cflags = "-tpp7 -xW"; }
                elsif ( /via-c3a/ )       { $cflags = "-tpp5 -xK"; }
                elsif ( /itanium2/ )      { $cflags = "-tpp2"; }
            }
        }
        elsif ( $compiler =~ /icc-[89]/ )
        {
            for ( $cpu )
            {
                if    ( /pentium|k6/ )    { $cflags = "-tpp5"; }
                elsif ( /pentiumpro/ )    { $cflags = "-tpp6"; }
                elsif ( /p2/ )            { $cflags = "-tpp6"; }
                elsif ( /p3-(sse|xeon)/ ) { $cflags = "-tpp6 -march=pentiumiii -xK";  } 
                elsif ( /p3/ )            { $cflags = "-tpp6 -march=pentiumii"; }
                elsif ( /pentium-m/ )     { $cflags = "-tpp7 -xB"; }
                elsif ( /core/ )          { $cflags = "-tpp7 -xP"; }
                elsif ( /core2/ )         { $cflags = "-tpp7 -xP"; }
                elsif ( /p4$/ )           { $cflags = "-tpp7 -xW"; }
                elsif ( /p4-willamette/ ) { $cflags = "-tpp7 -xW"; }
                elsif ( /p4-northwood/ )  { $cflags = "-tpp7 -xN"; }
                elsif ( /p4-prescott/ )   { $cflags = "-tpp7 -xP"; }
                elsif ( /p4-nocona/ )     { $cflags = "-tpp7 -xP"; }
                elsif ( /p4-foster/ )     { $cflags = "-tpp7 -xP"; }
                elsif ( /athlon-xp/ )     { $cflags = "-tpp6 -xK"; }
                elsif ( /athlon/ )        { $cflags = "-tpp6 -march=pentiumii"; }
                elsif ( /opteron/ )       { $cflags = "-tpp7 -xW"; }
                elsif ( /via-c3a/ )       { $cflags = "-tpp5 -xK"; }
                elsif ( /itanium2/ )      { $cflags = "-tpp2"; }
            }
        }
        elsif ( $compiler =~ /pgi/ )
        {
            for ( $cpu )
            {
                if    ( /i386|i486|i586/ )   { $cflags = "-tp px"; }
                elsif ( /pentium($|-mmx)/ )  { $cflags = "-tp p5"; }
                elsif ( /pentiumpro|p2|p3/ ) { $cflags = "-tp p6"; }
                elsif ( /pentium-m/ )        { $cflags = "-tp p6"; }
                elsif ( /core|core2/ )       { $cflags = "-tp p7"; }
                elsif ( /p4/ )               { $cflags = "-tp p7"; }
                elsif ( /athlon/ )           { $cflags = "-tp p6"; }
                elsif ( /opteron/ )          { $cflags = "-tp k8-$bits"; }
                elsif ( /via-c3a/ )          { $cflags = "-tp p5"; }
            }
        }
        elsif ( $compiler =~ /path-2/ )
        {
            for ( $cpu )
            {
                if    ( /i386|i486|i586/ )   { $cflags = "-march=anyx86"; }
                elsif ( /pentium($|-mmx)/ )  { $cflags = "-march=anyx86"; }
                elsif ( /pentiumpro|p2|p3/ ) { $cflags = "-march=anyx86"; }
                elsif ( /athlon/ )           { $cflags = "-march=anyx86"; }
                elsif ( /via-c3a/ )          { $cflags = "-march=anyx86"; }
                elsif ( /pentium-m/ )        { $cflags = "-march=anyx86"; }
                elsif ( /core|core2/ )       { $cflags = "-march=pentium4"; }
                elsif ( /p4/ )               { $cflags = "-march=pentium4"; }
                elsif ( /opteron/ )          { $cflags = "-march=opteron -m$bits"; }
            }
        }
        elsif ( $compiler =~ /suncc-5.9/ )
        {
            if ( $bits == 32 )
            {
                for ( $cpu )
                {
                    if    ( /i386|i486|i586/ )   { $cflags = "-xtarget=generic -xarch=386"; }
                    elsif ( /pentium($|-mmx)/ )  { $cflags = "-xtarget=pentium -xarch=386"; }
                    elsif ( /pentiumpro|p2/ )    { $cflags = "-xtarget=pentium_pro -xarch=386"; }
                    elsif ( /p3/ )               { $cflags = "-xtarget=pentium3 -xarch=sse"; }
                    elsif ( /athlon/ )           { $cflags = "-xtarget=pentium3 -xarch=ssea"; }
                    elsif ( /via-c3a/ )          { $cflags = "-xtarget=pentium -xarch=sse"; }
                    elsif ( /pentium-m/ )        { $cflags = "-xtarget=pentium3 -xarch=sse2"; }
                    elsif ( /core|core2/ )       { $cflags = "-xtarget=pentium4 -xarch=sse2"; }
                    elsif ( /p4/ )               { $cflags = "-xtarget=pentium4 -xarch=sse2"; }
                    elsif ( /opteron/ )          { $cflags = "-xtarget=opteron -xarch=sse2a"; }
                }
            }
            elsif ( $bits == 64 )
            {
                for ( $cpu )
                {
                    if    ( /i386|i486|i586/ )   { $cflags = "-xtarget=generic -xarch=386"; }
                    elsif ( /pentium($|-mmx)/ )  { $cflags = "-xtarget=pentium -xarch=386"; }
                    elsif ( /pentiumpro|p2/ )    { $cflags = "-xtarget=pentium_pro -xarch=386"; }
                    elsif ( /p3/ )               { $cflags = "-xtarget=pentium3 -xarch=sse"; }
                    elsif ( /athlon/ )           { $cflags = "-xtarget=pentium3 -xarch=ssea"; }
                    elsif ( /via-c3a/ )          { $cflags = "-xtarget=pentium -xarch=sse"; }
                    elsif ( /pentium-m/ )        { $cflags = "-xtarget=pentium3 -xarch=sse2"; }
                    elsif ( /core|core2/ )       { $cflags = "-xtarget=pentium4 -xarch=sse2"; }
                    elsif ( /p4/ )               { $cflags = "-xtarget=pentium4 -xarch=sse2"; }
                    elsif ( /opteron/ )          { $cflags = "-xtarget=opteron -xarch=amd64a"; }
                }
            }
        }
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}Linux/Cygwin$tc{N}\n" );
        }
    }
    elsif ( $system eq "Solaris" )
    {
        if ( $compiler =~ /gcc-2/ )
        {
            if ( $cpu =~ /ultra[123]/ ) { $cflags = "-mcpu=ultrasparc"; }
        }
        elsif ( $compiler =~ /gcc-(3|4)/ )
        {
            for ( $cpu )
            {
                if    ( /ultra[12]/ ) { $cflags = "-mcpu=ultrasparc -m$bits"; }
                elsif ( /ultra3/    ) { $cflags = "-mcpu=ultrasparc3 -m$bits"; }
                elsif ( /opteron/   ) { $cflags = "-march=opteron -m$bits -mfpmath=sse,387"; }
            }
        }
        elsif ( $compiler =~ /suncc-5.[0123]|kcc/ )
        {
            if ( $bits == 32 )
            {
                for ( $cpu )
                {
                    if    ( /ultra1/ )  { $cflags = "-xtarget=ultra -xarch=v8"; }
                    elsif ( /ultra2/ )  { $cflags = "-xtarget=ultra2 -xarch=v8plusa"; }
                    elsif ( /ultra3/ )  { $cflags = "-xtarget=ultra3 -xarch=v8plusb"; }
                }
            }
            elsif ( $bits == 64 )
            {
                for ( $cpu )
                {
                    if    ( /ultra1/ ) { $cflags = "-xtarget=ultra -xarch=v9"; }
                    elsif ( /ultra2/ ) { $cflags = "-xtarget=ultra2 -xarch=v9a"; }
                    elsif ( /ultra3/ ) { $cflags = "-xtarget=ultra3 -xarch=v9b"; }
                }
            }
        }
        elsif ( $compiler =~ /suncc-5.[45678]/ )
        {
            if ( $bits == 32 )
            {
                for ( $cpu )
                {
                    if    ( /ultra1/ )  { $cflags = "-xtarget=ultra -xarch=v8"; }
                    elsif ( /ultra2/ )  { $cflags = "-xtarget=ultra2 -xarch=v8plusa"; }
                    elsif ( /ultra3/ )  { $cflags = "-xtarget=ultra3cu -xarch=v8plusb"; }
                    elsif ( /opteron/ ) { $cflags = "-xtarget=opteron -xarch=sse2a"; }
                }
            }
            elsif ( $bits == 64 )
            {
                for ( $cpu )
                {
                    if    ( /ultra1/ )  { $cflags = "-xtarget=ultra -xarch=v9"; }
                    elsif ( /ultra2/ )  { $cflags = "-xtarget=ultra2 -xarch=v9a"; }
                    elsif ( /ultra3/ )  { $cflags = "-xtarget=ultra3cu -xarch=v9b"; }
                    elsif ( /opteron/ ) { $cflags = "-xtarget=opteron -xarch=amd64a -xmodel=medium"; }
                }
            }
        }
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}Solaris$tc{N}\n" );
        }
    }
    elsif ( $system eq "AIX" )
    {
        if    ( $compiler =~ /gcc-[23]/ )
        { 
            if ( $bits == 32 ) { $cflags = "-mcpu=power2"; }
            else               { $cflags = "-mcpu=powerpc64 -maix64"; }
        }
        elsif ( $compiler =~ /xlc/i )
        {
            $cflags  = "-q64" if ( $bits == 64 );

            for ( $cpu )
            {
                if ( /power4/ ) { $cflags .= " -qarch=pwr4 -qtune=pwr4"; }
            }
        }
        else
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}AIX$tc{N}\n" );
        }
    }
    elsif ( $system eq "IRIX" )
    {
        if    ( $compiler =~ /gcc-2/ )  { $cflags = "-march=mips4"; }
        elsif ( $compiler =~ /gcc-3/ )  { $cflags = "-march=mips4 -mabi=$bits"; }
        elsif ( $compiler =~ /mipscc/ ) { $cflags = "-mips4 -$bits"; }
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}IRIX$tc{N}\n" );
        }
    }
    elsif ( $system eq "Darwin" )
    {
        if ( $compiler =~ /gcc/ )
        { 
            for ( $cpu )
            {
                if    ( /ppc750/ )   { $cflags = "-mcpu=750 -mpowerpc-gfxopt -fsigned-char"; }
                elsif ( /ppc7400/ )  { $cflags = "-mcpu=7400 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /ppc7450/ )  { $cflags = "-mcpu=7450 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /ppc970/ )   { $cflags = "-mcpu=970 -mpowerpc-gfxopt -maltivec -mabi=altivec -fsigned-char" }
                elsif ( /ppc/ )      { $cflags = " " }
            }
        }
        elsif ( $compiler =~ /xlc/ )
        {
            for ( $cpu )
            {
                if    ( /ppc750/ )     { $cflags = "-qarch=auto -qcache=auto"; }
                elsif ( /ppc74[05]0/ ) { $cflags = "-qarch=ppcv -qtune=ppcv -qcache=auto -qaltivec" }
                elsif ( /ppc970/ )     { $cflags = "-qarch=g5 -qtune=g5 -qcache=auto -qaltivec" }
                elsif ( /ppc/ )        { $cflags = "-qarch=auto -qcache=auto" }
            }
        }
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}Darwin$tc{N}\n" );
        }
    }
    elsif ( $system eq "Tru64" )
    {
        if ( $compiler =~ /gcc/ )
        {
        }
        elsif ( $compiler =~ /compaqcc/ )
        {
            for ( $cpu ) 
            {
                if    ( /ev56/ ) { $cflags = "-arch ev56"; }
                elsif ( /ev5/ )  { $cflags = "-arch ev5"; }
                elsif ( /ev67/ ) { $cflags = "-arch ev67"; }
                elsif ( /ev6/ )  { $cflags = "-arch ev6"; }
            }
        }
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}Tru64$tc{N}\n" );
        }
    }
    elsif ( $system eq "HP-UX" )
    {
        if ( $compiler =~ /acc/ )
        {
            for ( $cpu )
            {
                if    ( /PA8[25]/  ) { $cflags = "+tm K8000 +DA2.0"; }
                elsif ( /PA8[678]/ ) { $cflags = "+tm K8000 +DA2.0W"; }
                elsif ( /PA7/ )      { $cflags = "+tm K7200"; }
            }

            if    ( $bits == 32 ) { $cflags .= " +DD32"; }
            elsif ( $bits == 64 ) { $cflags .= " +DD64"; }
        }
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}HP-UX$tc{N}\n" );
        }
    }
    elsif ( $system eq "Win32" )
    {
        if ( $compiler =~ /msvc/ )
	{
            for ( $cpu )
            {
                if    ( /i386/ )          { $cflags = ""; }
                elsif ( /i486/ )          { $cflags = ""; }
                elsif ( /i586|pentium$/ ) { $cflags = ""; }
                elsif ( /k6$/ )           { $cflags = ""; }
                elsif ( /k6-2/ )          { $cflags = ""; }
                elsif ( /k6-3/ )          { $cflags = ""; }
                elsif ( /pentium-mmx/ )   { $cflags = ""; }
                elsif ( /pentiumpro/ )    { $cflags = ""; }
                elsif ( /p2/ )            { $cflags = ""; }
                elsif ( /p3$/ )           { $cflags = "";  }
                elsif ( /p3-(sse|xeon)/ ) { $cflags = "/arch:SSE";  }
                elsif ( /pentium-m/ )     { $cflags = "/arch:SSE2"; }
                elsif ( /core|core2/ )    { $cflags = "/arch:SSE2"; }
                elsif ( /p4/ )            { $cflags = "/arch:SSE2"; }
                elsif ( /athlon$/ )       { $cflags = ""; }
                elsif ( /athlon-tbird/ )  { $cflags = ""; }
                elsif ( /athlon-xp/ )     { $cflags = "/arch:SSE"; }
                elsif ( /opteron/ )       { $cflags = "/arch:SSE2 /favor:AMD64"; }
                elsif ( /via-c3a/ )       { $cflags = "/arch:SSE"; }
            }
	}
        else 
        {
            error( "compiler \"$tc{F}$compiler$tc{N}\" not supported on $tc{F}Win32$tc{N}\n" );
        }
    }

    if ( $cflags eq "undef" )
    {
        error( "CPU \"$tc{F}$cpu$tc{N}\" not supported by compiler \"$tc{F}$compiler$tc{N}\" on $tc{F}$system$tc{N}\n" );
        
        $cflags = ""; 
    }

    return $cflags;
}

###############################################################
#
# setup optimisation CFLAGS
# 
sub get_optflags 
{
    my $system   = $_[0];
    my $cpu      = $_[1];
    my $compiler = $_[2];
    my $oflags   = "undef";

    if ( $system =~ /Linux|CYGWIN/ )
    {
        for ( $compiler )
        {
            if ( /gcc-[23]/ )
            { 
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops";
                
                if    ( $cpu =~ /i586|pentium($|-mmx)/ ) { $oflags .= " -malign-double"; }
                elsif ( $cpu =~ /p2|k6|via-c3/ )         { $oflags .= " -malign-double"; }
                elsif ( $cpu =~ /pentium-m|core|core2/ ) 
                {
                    $oflags .= " -malign-double -fprefetch-loop-arrays"; 
                }
                elsif ( $cpu =~ /p[34]|athlon|opteron/ ) 
                {
                    $oflags .= " -malign-double -fprefetch-loop-arrays"; 
                }
            }
            elsif ( /gcc-[4]/ )
            { 
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops";
                
                if    ( $cpu =~ /i586|pentium($|-mmx)/ )    { $oflags .= " -malign-double"; }
                elsif ( $cpu =~ /p2|k6|via-c3/ )            { $oflags .= " -malign-double"; }
                elsif ( $cpu =~ /pentium-m|core|core2/ ) 
                {
                    $oflags .= " -malign-double -fprefetch-loop-arrays -ftree-vectorize"; 
                }
                elsif ( $cpu =~ /p[34]|athlon/ ) 
                {
                    $oflags .= " -malign-double -fprefetch-loop-arrays -ftree-vectorize"; 
                }
                elsif ( $cpu =~ /opteron/ ) 
                {
                    if    ( $bits == 32 ) { $oflags .= " -malign-double -fprefetch-loop-arrays -ftree-vectorize"; }
                    elsif ( $bits == 64 ) { $oflags .= " -fprefetch-loop-arrays -ftree-vectorize"; }
                }
            }
            elsif ( /icc/ )
            {
                $oflags = "-O3 -Ob2 -unroll -ip";
                
                if ( $cpu =~ /itanium2/ ) { $oflags .= " -IFP_fma"; }
            }
            elsif ( /pgi/ )
            {
                $oflags = "-O3 -fast -Minline";
                
                if ( $cpu =~ /p[234]|athlon|opteron|via-c3|pentium-m|core|core2/ ) 
                { 
                    $oflags .= " -Mvect"; 
                }
            }
            elsif ( /path-2/ )
            {
                $oflags = "-O3 -OPT:Ofast -OPT:fast_math -fno-math-errno -finline-functions -funroll-loops";
            }
            elsif ( $compiler =~ /suncc-5.9/ )
            {
                $oflags = "-fast -xO5";
                
                for ( $cpu )
                {
                    if    ( /p3/ )         { $oflags .= " -xvector=simd"; }
                    elsif ( /pentium-m/ )  { $oflags .= " -xvector=simd"; }
                    elsif ( /p4/ )         { $oflags .= " -xvector=simd"; }
                    elsif ( /core|core2/ ) { $oflags .= " -xvector=simd"; }
                    elsif ( /athlon/ )     { $oflags .= " -xvector=simd"; }
                    elsif ( /via-c3a/ )    { $oflags .= " -xvector=simd"; }
                    elsif ( /opteron/ )    { $oflags .= " -xprefetch -xprefetch_level=3 -xvector=simd"; }
                }
            }
        }
    }
    elsif ( $system eq "Solaris" )
    {
        for ( $compiler )
        {
            if ( /gcc-[234]/ ) 
            { 
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops"; 
            }
            elsif ( /suncc-5.[45678]/ )
            {
                $oflags = "-fast -xO5";

                if    ( $cpu =~ /ultra3/  ) { $oflags .= " -xprefetch -xprefetch_level=3"; }
                elsif ( $cpu =~ /opteron/ ) { $oflags .= " -xprefetch -xprefetch_level=3 -xvector=simd"; }
            }
            elsif ( /suncc-5.[0123]/ )
            {
                $oflags = "-fast -xO5";

                if ( $cpu =~ /ultra3/ ) { $oflags .= " -xprefetch"; }
            }
            elsif ( /kcc/ )
            {
                $oflags = "-fast +K3 -xO5 --abstract_float";

                if ( $cpu =~ /ultra3/ ) { $oflags .= " -xprefetch"; }
            }
        }
    }
    elsif ( $system eq "AIX" )
    {
        for ( $compiler )
        {
            if ( /gcc-[23]/ )
            { 
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops"; 
            }
            elsif ( /xlc/i ) 
            {
                $oflags = "-O2 -qinline -qrtti"; 
            }
        }
    }
    elsif ( $system eq "IRIX" )
    {
        for ( $compiler )
        {
            if ( /gcc-[23]/ )
            {
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops"; 
            }
            elsif ( /mipscc/ ) 
            { 
                $oflags = "-O3"; 
            }
        }
    }
    elsif ( $system =~ /Darwin/ )
    {
        for ( $compiler )
        {
            if ( /gcc-[23]/ )
            {
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops"; 
            }
            elsif ( /gcc-[4]/ )
            { 
                $oflags = "-O3 -fomit-frame-pointer -ffast-math -funroll-loops";
                
                if    ( $cpu =~ /ppc7400/ )  { $oflags .= " -ftree-vectorize" }
                elsif ( $cpu =~ /ppc7450/ )  { $oflags .= " -ftree-vectorize" }
                elsif ( $cpu =~ /ppc970/ )   { $oflags .= " -ftree-vectorize" }
            }
            elsif ( /xlc/ )
            {
                $oflags = "-O3 -qunroll -qinline";
            }
        }
    }       
    elsif ( $system eq "Tru64" )
    {
        if ( $compiler =~ /gcc/ )
        {
        }
        elsif ( $compiler =~ /compaqcc/ )
        {
            $oflags = "-fast -O4 -inline speed -unroll 0";
        }
    }
    elsif ( $system eq "HP-UX" )
    {
        if ( $compiler =~ /acc/ )
        {
            $oflags = "-fast +Odataprefetch";
        }
    }
    elsif ( $system eq "Win32" )
    {
        if ( $compiler =~ /msvc/ )
	{
	    $oflags = "/Ox /fp:fast";
	}
    }

    if ( $oflags eq "undef" )
    {
        error( "CPU \"$tc{F}$cpu$tc{N}\" not supported by compiler \"$tc{F}$compiler$tc{N}\" on $tc{F}$system$tc{N}\n" );
        $oflags = "";
    }

    return $oflags;
}

###############################################################
###############################################################
##
## determine processor
##
###############################################################
###############################################################

###############################################################
#
# lookup x86 processor based on vendor, family and model
#

sub lookup_x86
{
    my @cpudata = $_[0];

    my $vendor = $cpudata{vendor};
    my $family = $cpudata{family};
    my $model  = $cpudata{model};

    #
    # switch between different outputs
    #

    if ( $vendor eq "AuthenticAMD" )
    {
        if ( $family eq "4" ) 
        {
            # AMD 486
            return "i486";
        }
        elsif ( $family eq "5" ) 
        {
            # K5 and K6
            if    ( $model eq "0"  ) { return "i586"; }
            elsif ( $model eq "1"  ) { return "i586"; }
            elsif ( $model eq "2"  ) { return "i586"; }
            elsif ( $model eq "3"  ) { return "i586"; }
            elsif ( $model eq "6"  ) { return "k6"; }
            elsif ( $model eq "7"  ) { return "k6"; }
            elsif ( $model eq "8"  ) { return "k6-2"; }
            elsif ( $model eq "9"  ) { return "k6-3"; }
            elsif ( $model eq "13" ) { return "k6-3"; }
            else  { error( "cpuflags : unknown AMD K5/K6 mode \"$tc{F}$model$tc{N}\"\n" ); 
                    return "k5"; }
        }
        elsif ( $family eq "6" ) 
        {
            # Athlon
            if    ( $model eq "1"  ) { return "athlon"; }
            elsif ( $model eq "2"  ) { return "athlon"; }
            elsif ( $model eq "3"  ) { return "athlon"; }  # Duron
            elsif ( $model eq "4"  ) { return "athlon-tbird"; }
            elsif ( $model eq "6"  ) { return "athlon-xp"; }
            elsif ( $model eq "7"  ) { return "athlon-xp"; } # Duron-XP
            elsif ( $model eq "10" ) { return "athlon-xp"; }
            else  { error( "cpuflags : unknown AMD Athlon model \"$tc{F}$model$tc{N}\"\n" ); 
                    return "athlon-tbird"; }
        }
        elsif ( $family eq "15" ) 
        {
            # AMD Opteron
            if    ( $model eq "5"  ) { return "opteron"; }
            elsif ( $model eq "33" ) { return "opteron"; }
            elsif ( $model eq "37" ) { return "opteron"; }
            else  { error( "cpuflags : unknown AMD Opteron model \"$tc{F}$model$tc{N}\"\n" ); 
                    return "opteron"; }
        }
    }
    elsif ( $vendor eq "GenuineIntel" )
    {
        if    ( $family eq "3" )  { return "i386"; }
        elsif ( $family eq "4" )  { return "i486"; }
        elsif ( $family eq "5" ) 
        {
            # Pentium
            if    ( $model eq "0" ) { return "pentium"; }
            elsif ( $model eq "1" ) { return "pentium"; }
            elsif ( $model eq "2" ) { return "pentium"; }
            elsif ( $model eq "3" ) { return "pentium"; }
            elsif ( $model eq "4" ) { return "pentium-mmx"; }
            elsif ( $model eq "7" ) { return "pentium"; }
            elsif ( $model eq "8" ) { return "pentium-mmx"; }
            else  { error( "cpuflags : unknown Intel Pentium model \"$tc{F}$model$tc{N}\"\n" ); 
                    return "pentium"; }
        }
        elsif ( $family eq "6" ) 
        {
            # PentiumPro, P2, P3
            if    ( $model eq "0"  ) { return "pentiumpro"; }
            elsif ( $model eq "1"  ) { return "pentiumpro"; }
            elsif ( $model eq "3"  ) { return "p2"; }
            elsif ( $model eq "5"  ) { return "p2"; }
            elsif ( $model eq "6"  ) { return "p2"; }
            elsif ( $model eq "7"  ) { return "p3"; }
            elsif ( $model eq "8"  ) { return "p3-sse"; }
            elsif ( $model eq "9"  ) { return "pentium-m"; }   # Banias
            elsif ( $model eq "10" ) { return "p3-xeon"; }
            elsif ( $model eq "11" ) { return "p3-sse"; }
            elsif ( $model eq "13" ) { return "pentium-m"; }   # Dothan
            elsif ( $model eq "14" ) { return "core"; }        # Yonah
            elsif ( $model eq "15" ) { return "core2"; }
            else  { error( "cpuflags : unknown Intel Pentium3 model \"$tc{F}$model$tc{N}\"\n" );
                    return "pentiumpro"; }
        }
        elsif ( $family eq "15" ) 
        {
            # Pentium 4
            if    ( $model eq "0"  ) { return "p4"; }
            elsif ( $model eq "1"  ) { return "p4-willamette"; }  # Willamette
            elsif ( $model eq "2"  ) { return "p4-northwood";  }  # Northwood
            elsif ( $model eq "3"  ) { return "p4-prescott";   }  # Prescott
            elsif ( $model eq "4"  ) { return "p4-nocona";     }  # Nocona
            elsif ( $model eq "5"  ) { return "p4-foster";     }  # Foster
            else  { error( "cpuflags : unknown Intel Pentium4 model \"$tc{F}$model$tc{N}\"\n" );  return "p4"; }
        }
        elsif ( $family eq "Itanium 2" ) 
        {
            return "itanium2";
        }
    }
    elsif ( $vendor eq "CentaurHauls" )
    {
        if ( $family eq "6" )
        {
            # VIA C3
            if ( $model eq "9" ) { return "via-c3a"; }
            else { error( "cpuflags : unknown Via C3 model \"$tc{F}$model$tc{N}\"\n" ); 
                   return "via-c3"; }
        }
    }
    else
    {
        error( "cpuflags : unknown cpu-vendor \"$tc{F}$vendor$tc{N}\"\n" );
        return "i386";
    }

    return $cpu;
}
###############################################################
#
# functions for Linux
#

sub parse_linux
{
    my @cpuinfo = `cat /proc/cpuinfo 2>&1`;
    my @cpudata = ();

    foreach $line (@cpuinfo)
    {
        my $entry, $data;
       
        $entry = $line;
        $entry =~ s/[ \t]*\:.*//g;
        $entry =~ s/\n//g;

        $data  = $line;
        $data  =~ s/.*\: *//g;
        $data  =~ s/\n//g;

        print "entry = ($entry), data = ($data)\n" if ( $verbosity >= 2 );

        if    ( $entry =~ /vendor/  ) { $cpudata{vendor} = $data; }
        elsif ( $entry eq "cpu"     ) { $cpudata{type}   = $data; }
        elsif ( $entry =~ /family/  ) { $cpudata{family} = $data; }
        elsif ( $entry eq "model"   ) { $cpudata{model}  = $data; }
    }

    if ( $verbosity >= 1 )
    {
	print "Linux\n";
        print "  Vendor  = $cpudata{vendor}\n";
        print "  CPUtype = $cpudata{type}\n";
        print "  Family  = $cpudata{family}\n";
        print "  Model   = $cpudata{model}\n";
    }

    #
    # switch between different outputs
    #

    if ( $cpudata{vendor} ne "" )
    {
	return lookup_x86( @cpudata );
    }
    elsif ( $cpudata{type} ne "" )
    {
        if ( $cpudata{type} eq "745/755" ) { return "ppc750"; }
        else { error( "cpuflags : unknown cpu \"$tc{F}$cpudata{type}$tc{N}\"\n" ); }
    }
    else
    {
        error( "cpuflags : unsupported structure in $tc{F}/proc/cpuinfo$tc{N}\n" );
    }

    return "";
}

###############################################################
#
# functions for Solaris
#

sub parse_solaris
{
    my $isalist = `isalist`;
    my $optisa  = `optisa $isalist`;
    $optisa     =~ s/\n//g;

    print "CPU = $optisa\n" if ( $verbosity >= 1 );

    if    ( $optisa eq "sparcv9"         ) { return "ultra1"; }
    elsif ( $optisa eq "sparcv9+vis"     ) { return "ultra2"; }
    elsif ( $optisa eq "sparcv9+vis2"    ) { return "ultra3"; }
    elsif ( $optisa eq "i386"            ) { return "i386"; }
    elsif ( $optisa eq "i486"            ) { return "i486"; }
    elsif ( $optisa eq "pentium"         ) { return "pentium"; }
    elsif ( $optisa eq "pentium_pro"     ) { return "pentiumpro"; }
    elsif ( $optisa eq "pentium_pro+mmx" ) { return "p2"; }
    elsif ( $optisa eq "amd64"           ) { return "opteron"; }
    else  { error( "unsupported processor \"$tc{F}$optisa$tc{N}\"" ); }

    return "";
}

###############################################################
#
# functions for AIX
#

sub parse_aix
{
    my @cfg = `/usr/sbin/lscfg | awk 'BEGIN { FS = "[ \t]+" } { print $2 }'`;

    foreach $line (@cfg)
    {
        if ( $line =~ /.*proc.*/ )
        {
            my @entry = split( /[ \t]+/, $line );
            my $proc = $entry[1];
            my $info = `/usr/sbin/lscfg -p -l $proc | grep -i name`;

            if ( $info =~ /.*POWER4.*/ ) { return "power4" }
            else { error( "unsupported processor \"$tc{F}$info$tc{N}\"\n" ); }
            return "";
        }
    }

    return "";
}

###############################################################
#
# functions for IRIX
#

sub parse_irix
{
    my @cfg = `hinv`;

    foreach $line (@cfg)
    {
        if ( $line =~ /.*CPU.*/ )
        {
            my @entry = split( /[ \t]+/, $line );
            my $proc = $entry[2];

            if ( $proc =~ /.*R10000.*/ ) { return "r10000" }
            else { error( "unsupported processor \"$tc{F}$proc$tc{N}\"\n" ); }
            return "";
        }
    }

    return "";
}

###############################################################
#
# functions for Darwin (aka MacOSX)
#

sub parse_darwin
{
    #
    # determine CPU type
    #

    my $cputype    = "";
    my $cpusubtype = "";
    my @cfg        = `sysctl -a`;

    foreach $line (@cfg)
    {
        if ( $line =~ /cputype/i )
        {
            $cputype = $line;
            $cputype =~ s/.*\: *//g;
            $cputype =~ s/\n//g;
        }
        elsif ( $line =~ /cpusubtype/i )
        {
            $cpusubtype = $line;
            $cpusubtype =~ s/.*\: *//g;
            $cpusubtype =~ s/\n//g;
        }
    }

    print "CPU type/subtype = $cputype / $cpusubtype\n" if ( $verbosity >= 1 );

    #
    # set processor name
    #

    if ( $cputype == 7 )
    {
        if    ( $cpusubtype == 4   ) { return "core"; }
        else
        {
            error( "unsupported processor type \"$tc{F}$cputype/$cpusubtype$tc{N}\"\n" ); 
        }
    }
    elsif ( $cputype == 18 )
    {
        if    ( $cpusubtype == 0   ) { return "ppc"; }
        elsif ( $cpusubtype == 1   ) { return "ppc601"; }
        elsif ( $cpusubtype == 2   ) { return "ppc602"; }
        elsif ( $cpusubtype == 3   ) { return "ppc603"; }
        elsif ( $cpusubtype == 4   ) { return "ppc603"; }
        elsif ( $cpusubtype == 5   ) { return "ppc603"; }
        elsif ( $cpusubtype == 6   ) { return "ppc604"; }
        elsif ( $cpusubtype == 7   ) { return "ppc604"; }
        elsif ( $cpusubtype == 8   ) { return "ppc620"; }
        elsif ( $cpusubtype == 9   ) { return "ppc750"; }
        elsif ( $cpusubtype == 10  ) { return "ppc7400"; }
        elsif ( $cpusubtype == 11  ) { return "ppc7450"; }
        elsif ( $cpusubtype == 100 ) { return "ppc970"; }
        else
        {
            error( "unsupported processor type \"$tc{F}$cputype/$cpusubtype$tc{N}\"\n" ); 
        }
    }
    else
    {
        error( "unsupported processor type \"$tc{F}$cputype$tc{N}\"\n" ); 
    }

    return "";
}

###############################################################
#
# functions for Tru64
#

sub parse_tru64
{
    my @cfg = `/usr/sbin/pinfo -v`;

    foreach $line (@cfg)
    {
        if ( $line =~ /.*alpha.*/ )
        {
            my @entry = split( /[ \t]+/, $line );
            my $proc = $entry[3];

            print "processor = $proc\n" if ( $verbosity >= 2 );

            if    ( $proc =~ /EV5\.6/ ) { return "ev56" }
            elsif ( $proc =~ /EV6\.7/ ) { return "ev67" }
            else { error( "unsupported processor \"$tc{F}$proc$tc{N}\"\n" ); }
            return "";
        }
    }

    return "";
}

###############################################################
#
# functions for HP-UX
#

sub parse_hpux
{
    my $type = `getconf CPU_CHIP_TYPE`;

    print "CPU chip type = $type\n" if ( $verbosity >= 2 );

    # remove lower 5 bits to reveal actual cpu type
    $type >>= 5;

    if    ( $type == 0xb)  { return "PA7200"; }
    elsif ( $type == 0xd)  { return "PA7100LC"; }
    elsif ( $type == 0xe)  { return "PA8000"; }
    elsif ( $type == 0xf)  { return "PA7300LC"; }
    elsif ( $type == 0x10) { return "PA8200"; }
    elsif ( $type == 0x11) { return "PA8500"; }
    elsif ( $type == 0x12) { return "PA8600"; }
    elsif ( $type == 0x13) { return "PA8700"; }
    elsif ( $type == 0x14) { return "PA8800"; }
    elsif ( $type == 0x15) { return "PA8750"; }
    else                   { return "PA"; }

    return "";
}

###############################################################
#
# functions for Win32
#

sub parse_win32
{
    require Win32::TieRegistry;

    $Win32::TieRegistry::Registry->Delimiter( "/" );

    # evaluate regsitry to determine exact processor type
    my $proc_info;
    my $reg_key = "LMachine/HARDWARE/DESCRIPTION/System/CentralProcessor/0"; 
    my $errmsg  = "";
  
    $proc_info = $Win32::TieRegistry::Registry->Open( $reg_key, { Access => KEY_READ } )
	    or $errmsg = "could not access registry because: $tc{F}$^E$tc{N}\n";

    if ( $errmsg ne "" )
    {
        print $errmsg;
	return "i386";
    }

    #
    # now distinguish between different processors
    #

    my @cpudata = ();
    my $identifier = $proc_info->GetValue( "Identifier" );

    $cpudata{vendor} = $proc_info->GetValue( "VendorIdentifier" );

    if ( $identifier =~ /Family (\d+)/i ) { $cpudata{family} = $1; }
    if ( $identifier =~ /Model (\d+)/i )  { $cpudata{model}  = $1; }

    if ( $verbosity >= 1 )
    {
	print "Win32\n";
	print "  Vendor = $cpudata{vendor}\n";
	print "  Family = $cpudata{family}\n";
	print "  Model  = $cpudata{model}\n";
    }

    return lookup_x86( @cpudata );
}

###############################################################
###############################################################
##
## determine compiler version
##
###############################################################
###############################################################

sub compiler_version
{
    my @options  = ( "-V", "-v", "--version", "-qversion" );
    my $compiler = $_[0];

    foreach $opt (@options)
    {
        my @output = `$compiler $opt 2>&1`;

        foreach $line (@output)
        {
            print "output: $line" if ( $verbosity >= 2 );

            if    ( $line =~ /^gcc.* (\d+)\.(\d+)/i )                           { return "gcc-$1.$2"; }
            elsif ( $line =~ /intel.*compiler.*version.* (\d+)\.(\d+)/i )       { return "icc-$1.$2"; }
            elsif ( $line =~ /Version (\d+)\.(\d+).*l_cc/ )                     { return "icc-$1.$2"; }
            elsif ( $line =~ /pg(cc|CC|f77|f90) (\d+)\.(\d+)/i )                { return "pgi-$2.$3"; }
            elsif ( $line =~ /PathScale.* Version (\d+)\.(\d+)/i )              { return "path-$1.$2"; }
            elsif ( $line =~ /forte developer .* c\+* (\d+)\.(\d+)/i )          { return "suncc-$1.$2"; }
            elsif ( $line =~ /sun workshop .* c\+* (\d+)\.(\d+)/i )             { return "suncc-$1.$2"; }
            elsif ( $line =~ /sun c\+* (\d+)\.(\d+)/i )                         { return "suncc-$1.$2"; }
            elsif ( $line =~ /hp.*ansi.*a\.(\d+)/i )                            { return "acc-$1"; }
            elsif ( $line =~ /mipspro compilers.* (\d+)\.(\d+)/i )              { return "mipscc-$1"; }
            elsif ( $line =~ /IBM XL .* Version (\d+)\.(\d+)/i )                { return "xlc-$1"; }
            elsif ( $line =~ /IBM XL .* Edition V(\d+)\.(\d+)/i )               { return "xlc-$1"; }
            elsif ( $line =~ /C for AIX version (\d+)\.(\d+)/i )                { return "xlc-$1"; }
            elsif ( $line =~ /Compaq C V(\d+)\.(\d+)/i )                        { return "compaqcc-$1"; }
            elsif ( $line =~ /Microsoft .* C\/C\+\+ .* Version (\d+)\.(\d+)/i ) { return "msvc-$1"; }
        }
    }

    return "";
}

###############################################################
###############################################################
##
## main program
##
###############################################################
###############################################################

init_colours();

parse_cmdline();

#
# determine operating system
#

if ( $^O eq "MSWin32" ) 
{
    $system = "Win32";
}
else
{
    $system = `uname -s`;
    $system =~ s/\n//g;
}

# adjust name
if ( $system eq "SunOS"  ) { $system = "Solaris"; }
if ( $system eq "IRIX64" ) { $system = "IRIX"; }
if ( $system eq "OSF1"   ) { $system = "Tru64"; }

#
# determine compiler version
#

$compversion = compiler_version( $compiler ) if ( $compversion eq "" );

if ( $compversion eq "" )
{
    error( "compiler \"$tc{F}$compiler$tc{N}\" not available or unsupported by cpuflags\n" );
    exit( 1 );
}

print "compiler = $compversion\n" if ( $verbosity >= 1 );

#
# determine processor
#

if ( $cpu eq "" )
{
    if    ( $system =~ /Linux|CYGWIN/ ) { $cpu = parse_linux();   }
    elsif ( $system eq "Solaris" )      { $cpu = parse_solaris(); }
    elsif ( $system eq "AIX" )          { $cpu = parse_aix(); }
    elsif ( $system eq "IRIX" )         { $cpu = parse_irix(); }
    elsif ( $system eq "Darwin" )       { $cpu = parse_darwin(); }
    elsif ( $system eq "Tru64" )        { $cpu = parse_tru64(); }
    elsif ( $system eq "HP-UX" )        { $cpu = parse_hpux(); }
    elsif ( $system eq "Win32" )        { $cpu = parse_win32(); }
    else  { error( "unknown system \"$tc{F}$system$tc{N}\"\n" ); exit( 1 ); }
}

#
# setup cflags
#

if ( $show_cflags == 1 ) { $cflags = get_cflags( $system, $cpu, $compversion ); }
if ( $show_oflags == 1 ) { $oflags = get_optflags( $system, $cpu, $compversion ); }

#
# output everything
#

if ( $show_cpu == 1 )
{
    print "$cpu\n";
}
else
{
    # standard flags
    $oflags = $std_oflags if ( $oflags eq "" );

    # first print optimisation flags to omit warnings of sun-compiler
    print "$oflags " if ( $show_oflags == 1 );
    print "$cflags " if ( $show_cflags == 1 );
    print "\n"       if ( ( $show_cflags == 1 and $cflags ne "" ) or 
                          ( $show_oflags == 1 and $oflags ne "" ) );
}

