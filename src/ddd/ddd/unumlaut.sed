# $Id: unumlaut.sed,v 1.1.1.1 2006/04/11 21:46:37 cchavva Exp $
# Convert 8-bit umlauts to 7-bit replacements (ae, oe, ue, ss)

s/�/e/g
s/�/ae/g
s/�/oe/g
s/�/ue/g
s/�/ss/g
s/�[a-z]/Ae/g
s/�[a-z]/Oe/g
s/�[a-z]/Ue/g
s/�/AE/g
s/�/OE/g
s/�/UE/g
