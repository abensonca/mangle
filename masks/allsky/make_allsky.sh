#! /bin/sh
# � M E C Swanson 2008
#script to generate allsky polygon files with a given pixelization
# USAGE: make_allsky.sh <r> <scheme>
# EXAMPLE: make_allsky.sh 6 d 

if [ "$MANGLEBINDIR" = "" ] ; then
    MANGLEBINDIR="../../bin"
fi
if [ "$MANGLESCRIPTSDIR" = "" ] ; then
    MANGLESCRIPTSDIR="../../scripts"
fi
if [ "$MANGLEDATADIR" = "" ] ; then
    MANGLEDATADIR="../../masks"
fi

res=$1
scheme=$2
mtol=$3
snap="$4 $5 $6"

#check command line arguments
if [ "$res" = "" ] || [ "$scheme" = "" ] ; then
    echo >&2 "ERROR: enter the desired resolution and pixelization scheme as command line arguments."
    echo >&2 "" 
    echo >&2 "USAGE: make_allsky.sh <r> <scheme>"
    echo >&2 "EXAMPLE: make_allsky.sh 6 d" 
    echo >&2 "or to use non-default values for snap and multiple intersection tolerances:" 
    echo >&2 "USAGE: make_allsky.sh <r> <scheme> mtol_argument snap_tol_arguments"
    echo >&2 "EXAMPLE: make_allsky.sh 6 d -m1e-8 -a.027 -b.027 -t.027"
    exit 1
fi

allsky=$MANGLEDATADIR/allsky/allsky$res$scheme.pol
echo "Generating allsky$res$scheme.pol..."
if [ ! -d "$MANGLEDATADIR/allsky" ] ; then
    echo >&2 "ERROR: $MANGLEDATADIR/allsky not found." 
    echo >&2 "Check that the environment variable MANGLEDATADIR is pointing to" 
    echo >&2 "the appropriate directory (e.g., the mangle 'masks' directory)." 
    exit 1
fi
currentdir=$PWD
cd $MANGLEDATADIR/allsky
$MANGLEBINDIR/pixelize $mtol -P${scheme}0,${res} allsky.pol ja 
$MANGLEBINDIR/snap $snap $mtol ja allsky$res$scheme.pol 
rm ja
cd $currentdir

