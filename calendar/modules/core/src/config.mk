#
#           CONFIDENTIAL AND PROPRIETARY SOURCE CODE OF
#              NETSCAPE COMMUNICATIONS CORPORATION
# Copyright � 1996, 1997 Netscape Communications Corporation.  All Rights
# Reserved.  Use of this Source Code is subject to the terms of the
# applicable license agreement from Netscape Communications Corporation.
# The copyright notice(s) in this Source Code does not indicate actual or
# intended publication of this Source Code.
#

#
#  Override TARGETS variable so that only static libraries
#  are specifed as dependencies within rules.mk.
#

CFLAGS         +=-D_IMPL_NS_CALENDAR -DNSPR20 -I$(GDEPTH)/include

LD_LIBS += \
	raptorbase \
	raptorhtmlpars \
	$(NATIVE_JULIAN_DLL) \
	xpcom$(MOZ_BITS) \
    $(NATIVE_LIBNLS_LIBS) \
	$(NATIVE_RAPTOR_WIDGET) \
	calcapi10 \
	util10  \
	xpcom$(MOZ_BITS) \
	$(XP_REG_LIB)

LIBRARY_NAME      = calcore
LIBRARY_VERSION   = 10

EXTRA_LIBS += $(NSPR_LIBS)
