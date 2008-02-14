#!/bin/sh

# this script updates all code from CVS in the standard developer's layout.
# <hans@at.or.at>

# Usage: just run it and it should find things if you have your stuff layed
# out in the standard dev layout, or used checkout-developer-layout.sh to
# checkout your pd source tree

# Be aware that SourceForge's anonymous CVS server is generally 24 hours
# behind the authenticated CVS.

cvs_root_dir=`echo $0 | sed 's|\(.*\)/.*$|\1|'`/..

cd $cvs_root_dir
echo "Running svn update:"
svn update
echo "Running cvs update:"
for section in Gem GemLibs; do
	 echo "$section"
	 cd $section
    cvs up -Pd
	 cd ..
done
