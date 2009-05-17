say english    "English"
say francais   "Fran�ais"
say deutsch    "Deutsch"
say catala     "Catal�"
say espanol    "Espa�ol"
say portugues  "Portugu�s"
say brasileiro "Portugu�s Do Brasil"
say italiano   "Italiano"
say euskara    "Euskara"
say bokmal     "Norsk Bokm�l"
say dansk      "Dansk"
say nederlands "Nederlands"
say polski     "Polski"
say turkce     "T�rk�e"

# those were made using:
# iconv -f utf-8 -t ucs-2 | od -tx2 -An | sed 's/ /\\u/g'

say russkij "\u0420\u0443\u0441\u0441\u043a\u0438\u0439"
say chinese "\u4e2d\u6587"
say nihongo "\u65e5\u672c\u8a9e"

set ::langoptions {
	english francais deutsch catala espanol portugues brasileiro
	italiano euskara bokmal dansk nederlands turkce polski russkij chinese nihongo
	
}
